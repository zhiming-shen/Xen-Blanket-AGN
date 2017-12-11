/********************************************
 * platform-pci.c
 *
 * Xen platform PCI device driver
 * Copyright (c) 2005, Intel Corporation.
 * Copyright (c) 2007, XenSource Inc.
 * Copyright (c) 2010, Citrix
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA 02111-1307 USA.
 *
 */


#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/pci.h>

#include <asm/xen/page.h>
#include <linux/delay.h>
#include <xen/platform_pci.h>
#include "features.h"
#include "grant_table.h"
#include "xenbus/xenbus.h"
#include "events.h"
#include <xen/interface/memory.h>
#include <xen/hvm.h>
#include <xen/xen-ops.h>
#include <asm/setup.h>
#include <asm/xen/hypercall.h>
#include "xenblanket.h"
#include "xenblanket_hypercall.h"

#define DRV_NAME    "xen-platform-pci"

MODULE_AUTHOR("ssmith@xensource.com and stefano.stabellini@eu.citrix.com");
MODULE_DESCRIPTION("Xen platform PCI device");
MODULE_LICENSE("GPL");

int xen_setup_shutdown_event_hvm(void);
void xen_unplug_emulated_devices_hvm(void);

static unsigned long platform_mmio;
static unsigned long platform_mmio_alloc;
static unsigned long platform_mmiolen;
static uint64_t callback_via;

static unsigned long alloc_xen_mmio_hvm(unsigned long len);

static bool __init xen_blanket_platform(void)
{
        long rc = 0;
        uint32_t base;
        struct blanket_get_cpuid cpuid;
        char signature[13] = {0};
        printk("XENBLANKET-DOM0: detecting nested hvm environment.\n");
        for (base = 0x40000000; base < 0x40010000; base += 0x100) {
                rc = HYPERVISOR_blanket_get_cpuid(base, &cpuid);
                if(rc)
                        break;
                *(uint32_t *)(signature + 0) = cpuid.ebx;
                *(uint32_t *)(signature + 4) = cpuid.ecx;
                *(uint32_t *)(signature + 8) = cpuid.edx;
                signature[12] = 0;
                if (!strcmp("XenVMMXenVMM", signature) && ((cpuid.eax - base) >= 2)) {
                        printk("XENBLANKET-DOM0: nested hvm environment detected.\n");
                        return true;
                }
        }
        printk("XENBLANKET-DOM0: fail to detect nested hvm environment, cpuid %s.\n", signature);
        return false;
}

static void xenblanket_get_shared_info(void)
{
        struct xen_add_to_physmap xatp;
	static struct shared_info *shared_info_page = 0;

        shared_info_page = page_address(alloc_pages(GFP_ATOMIC, 0));
	printk("XENBLAKENT-DOM0: debug xenblanket_get_shared_info: %p %lx %lx.\n",
	    shared_info_page, __pa(shared_info_page) >> PAGE_SHIFT,
	    pfn_to_mfn(__pa(shared_info_page) >> PAGE_SHIFT)); 
	memset(shared_info_page, 0, PAGE_SIZE);
        xatp.domid = DOMID_SELF;
        xatp.idx = 0;
        xatp.space = XENMAPSPACE_shared_info;
        xatp.gpfn = pfn_to_mfn(__pa(shared_info_page) >> PAGE_SHIFT);
        if (HYPERVISOR_blanket_memory_op(XENMEM_add_to_physmap, &xatp))
                BUG();

        HYPERVISOR_shared_info_hvm = shared_info_page;

	printk("XENBLANKET-DOM0: debug shared info page wc - %lx %lx %lx.\n",
	    HYPERVISOR_shared_info_hvm->wc.version,
	    HYPERVISOR_shared_info_hvm->wc.sec,
	    HYPERVISOR_shared_info_hvm->wc.nsec);

	printk("XENBLANKET-DOM0: shared info page - %p.\n", HYPERVISOR_shared_info_hvm);
}

static void xenblanket_init(void)
{
	if(xen_blanket_platform())
		xenblanket_platform = 1;
	else
		xenblanket_platform = 0;
	if(xenblanket_platform)
	{
		printk("XENBLANKET-DOM0: initialize xenblanket hypercalls.\n");
		HYPERVISOR_blanket_init_nested_hypercall();		
		xenblanket_get_shared_info();	
	}
}

static unsigned long alloc_xen_mmio_hvm(unsigned long len)
{
	unsigned long addr;

	addr = platform_mmio + platform_mmio_alloc;
	platform_mmio_alloc += len;
	BUG_ON(platform_mmio_alloc > platform_mmiolen);

	return addr;
}

static uint64_t get_callback_via(struct pci_dev *pdev)
{
	u8 pin;
	int irq;

	irq = pdev->irq;
	if (irq < 16)
		return irq; /* ISA IRQ */

	pin = pdev->pin;

	/* We don't know the GSI. Specify the PCI INTx line instead. */
	return ((uint64_t)0x01 << 56) | /* PCI INTx identifier */
		((uint64_t)pci_domain_nr(pdev->bus) << 32) |
		((uint64_t)pdev->bus->number << 16) |
		((uint64_t)(pdev->devfn & 0xff) << 8) |
		((uint64_t)(pin - 1) & 3);
}

static irqreturn_t do_hvm_evtchn_intr(int irq, void *dev_id)
{
	xen_hvm_evtchn_do_upcall_hvm();
	return IRQ_HANDLED;
}

static int xen_allocate_irq(struct pci_dev *pdev)
{
	printk("XENBLANKET: DEBUG in xen_allocate_irq()");
	return request_irq(pdev->irq, do_hvm_evtchn_intr,
			IRQF_DISABLED | IRQF_NOBALANCING | IRQF_TRIGGER_RISING,
			"xen-platform-pci", pdev);
}

static int platform_pci_resume(struct pci_dev *pdev)
{
	int err;
	if (xen_have_vector_callback_hvm)
		return 0;
	err = xen_set_callback_via_hvm(callback_via);
	if (err) {
		dev_err(&pdev->dev, "platform_pci_resume failure!\n");
		return err;
	}
	return 0;
}

static int __devinit platform_pci_init(struct pci_dev *pdev,
				       const struct pci_device_id *ent)
{
	int i, ret;
	long ioaddr;
	long mmio_addr, mmio_len;
	unsigned int max_nr_gframes;

        printk("XENBLANKET-DOM0: platform_pci_init().\n");

	i = pci_enable_device(pdev);
	if (i)
		return i;

	ioaddr = pci_resource_start(pdev, 0);

	mmio_addr = pci_resource_start(pdev, 1);
	mmio_len = pci_resource_len(pdev, 1);

	if (mmio_addr == 0 || ioaddr == 0) {
		dev_err(&pdev->dev, "no resources found\n");
		ret = -ENOENT;
		goto pci_out;
	}

	ret = pci_request_region(pdev, 1, DRV_NAME);
	if (ret < 0)
		goto pci_out;

	ret = pci_request_region(pdev, 0, DRV_NAME);
	if (ret < 0)
		goto mem_out;

	platform_mmio = mmio_addr;
	platform_mmiolen = mmio_len;

        xenblanket_init();
        if(xenblanket_platform)
                xen_unplug_emulated_devices_hvm();
        if (!xen_platform_pci_unplug)
                return -ENODEV;

	xen_setup_features_hvm();

	//L2 Xen cannot recognize callback vector
        //if (xen_feature_hvm(XENFEAT_hvm_callback_vector))
        //        xen_have_vector_callback_hvm = 1;	

	xen_init_IRQ_hvm();

	if (!xen_have_vector_callback_hvm) {
		ret = xen_allocate_irq(pdev);
		printk("XENBLANKET: DEBUG returned from xen_allocate_irq");
		if (ret) {
			dev_warn(&pdev->dev, "request_irq failed err=%d\n", ret);
			goto out;
		}
		callback_via = get_callback_via(pdev);
		ret = xen_set_callback_via_hvm(callback_via);
		if (ret) {
			dev_warn(&pdev->dev, "Unable to set the evtchn callback "
					 "err=%d\n", ret);
			goto out;
		}
	}

	max_nr_gframes = gnttab_max_grant_frames_hvm();
	xen_hvm_resume_frames_hvm = alloc_xen_mmio_hvm(PAGE_SIZE * max_nr_gframes);
	ret = gnttab_init_hvm();
	if (ret)
		goto out;

	xenbus_init_hvm();

	//xenbus_probe_backend_init_hvm();

	xenbus_probe_frontend_init_hvm();
	printk("XENBLANKET: DEBUG returned from xenbus_probe_frontend_init_hvm()\n");	

	xen_setup_shutdown_event_hvm();
	printk("XENBLANKET: DEBUG returned from xen_setup_shutdown_event_hvm() \n");

	xenbus_probe_hvm(NULL);
	printk("XENBLANEKT: DEBUG returned from xenbus_probe_hvm()\n");

	xenblanket_platform_initialized = 1;

	xlblk_init_hvm();
	return 0;

out:
	pci_release_region(pdev, 0);
mem_out:
	pci_release_region(pdev, 1);
pci_out:
	pci_disable_device(pdev);
	return ret;
}

static struct pci_device_id platform_pci_tbl[] __devinitdata = {
	{PCI_VENDOR_ID_XEN, PCI_DEVICE_ID_XEN_PLATFORM,
		PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{0,}
};

MODULE_DEVICE_TABLE(pci, platform_pci_tbl);

static struct pci_driver platform_driver = {
	.name =           DRV_NAME,
	.probe =          platform_pci_init,
	.id_table =       platform_pci_tbl,
#ifdef CONFIG_PM
	.resume_early =   platform_pci_resume,
#endif
};

static int __init platform_pci_module_init_hvm(void)
{
	return pci_register_driver(&platform_driver);
}

module_init(platform_pci_module_init_hvm);
