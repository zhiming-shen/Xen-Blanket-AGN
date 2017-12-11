#include <linux/init.h>
#include <linux/smp.h>
#include <linux/preempt.h>
#include <linux/hardirq.h>
#include <linux/percpu.h>
#include <linux/delay.h>
#include <linux/start_kernel.h>
#include <linux/sched.h>
#include <linux/kprobes.h>
#include <linux/bootmem.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/page-flags.h>
#include <linux/highmem.h>
#include <linux/console.h>
#include <linux/pci.h>
#include <linux/gfp.h>
#include <linux/memblock.h>

#include <xen/xen.h>
#include <xen/interface/xen.h>
#include <xen/interface/version.h>
#include <xen/interface/physdev.h>
#include <xen/interface/vcpu.h>
#include <xen/interface/memory.h>
#include <xen/features.h>
#include <xen/page.h>
#include <xen/hvm.h>
#include "xenblanket.h"

int xenblanket_platform;
EXPORT_SYMBOL_GPL(xenblanket_platform);

volatile struct shared_info * volatile HYPERVISOR_shared_info_hvm;
EXPORT_SYMBOL_GPL(HYPERVISOR_shared_info_hvm);

int xen_have_vector_callback_hvm;
EXPORT_SYMBOL_GPL(xen_have_vector_callback_hvm);

int xenblanket_platform_initialized;
EXPORT_SYMBOL_GPL(xenblanket_platform_initialized);
