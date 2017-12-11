#include <asm/processor.h>
#include <public/blanket.h>
#include <xen/config.h>
#include <xen/init.h>
#include <xen/lib.h>
#include <xen/errno.h>
#include <xen/version.h>
#include <xen/sched.h>
#include <xen/paging.h>
#include <xen/nmi.h>
#include <xen/guest_access.h>
#include <asm/current.h>
#include <public/sched.h>
#include <public/memory.h>
#include <public/nmi.h>
#include <public/version.h>
#include <xen/hypercall.h>
#include <xen/grant_table.h>
#ifdef CONFIG_X86
#include <asm/shared.h>
#endif
#include <compat/memory.h>


static char *hypercall_stubs_raw;
static char *hypercall_stubs;

#include <xen/blanket_hypercall.h>

long do_blanket_get_cpuid(uint32_t base, XEN_GUEST_HANDLE(void) arg)
{
   blanket_get_cpuid_t cpuid;
   cpuid(base, &(cpuid.eax), &(cpuid.ebx), &(cpuid.ecx), &(cpuid.edx));
   if(copy_to_guest(arg, &cpuid, 1))
       return -EFAULT;
   return 0; 
}

static inline uint32_t xen_cpuid_base(void)
{
    uint32_t base, eax, ebx, ecx, edx;
    char signature[13];
    for (base = 0x40000000; base < 0x40010000; base += 0x100) {
        cpuid(base, &eax, &ebx, &ecx, &edx);
        *(uint32_t *)(signature + 0) = ebx;
        *(uint32_t *)(signature + 4) = ecx;
        *(uint32_t *)(signature + 8) = edx;
        signature[12] = 0;

        if (!strcmp("XenVMMXenVMM", signature) && ((eax - base) >= 2))
            return base;
    }

    return 0;    
}

long do_blanket_init_nested_hypercall(void)
{
    uint32_t eax, ebx, ecx, edx, pages, msr, base, i;
    unsigned long mfn;
    int major, minor;

    if(current->domain->domain_id != 0)
        return 0xffffffffffffffda;

    base = xen_cpuid_base();
    cpuid(base + 1, &eax, &ebx, &ecx, &edx);

    major = eax >> 16;
    minor = eax & 0xffff;
    printk(KERN_INFO "XENBLANKET: Xen version %d.%d.\n", major, minor);
 
    cpuid(base + 2, &pages, &msr, &ecx, &edx);

    hypercall_stubs_raw = xmalloc_bytes((pages * PAGE_SIZE) + PAGE_SIZE);
    hypercall_stubs = (char *)(((unsigned long)hypercall_stubs_raw & ~0xfff) + PAGE_SIZE);
    printk("XENBLANKET: hypercall_stubs = %p\n", hypercall_stubs);

    for (i = 0; i < pages; i++) {
        mfn = virt_to_mfn((char *)hypercall_stubs + i*PAGE_SIZE);
        wrmsrl(msr, ((u64)mfn << PAGE_SHIFT) + i);
    }

    return 0;
}

long compat_blanket_grant_table_op(unsigned int cmd,
                          XEN_GUEST_HANDLE(void) cmp_uop,
                          unsigned int count)
{
    return do_blanket_grant_table_op(cmd, cmp_uop, count);
}

long compat_blanket_memory_op(int op, XEN_GUEST_HANDLE(void) arg)
{
    switch ( op ){
    case XENMEM_add_to_physmap:{
        struct compat_add_to_physmap cmp;
        struct xen_add_to_physmap *nat = (void *)COMPAT_ARG_XLAT_VIRT_BASE;
        struct xen_add_to_physmap xatp_n;
        struct domain *d;
        long ret;

        if ( copy_from_guest(&cmp, arg, 1) ){
            return -EFAULT;
        }
    
        XLAT_add_to_physmap(nat, &cmp);
        if ( nat->domid != DOMID_SELF ){
            return -EPERM;
        }

        if (( nat->space != XENMAPSPACE_shared_info) && 
            ( nat->space != XENMAPSPACE_grant_table)){
            return -EFAULT;
        }

        d = rcu_lock_current_domain();
        /* LOCKING: shared_info doesn't need to lock */
        spin_lock(&d->grant_table->lock);

        xatp_n.domid = DOMID_SELF;
        xatp_n.idx = nat->idx;
        xatp_n.space = nat->space;
        xatp_n.gpfn = nat->gpfn;  /* gpfn is the mfn */


        ret = HYPERVISOR_nested_memory_op(XENMEM_add_to_physmap, &xatp_n);
        if ( ret ){
            spin_unlock(&d->grant_table->lock);
            printk("memory op failed with %ld\n", ret);
            rcu_unlock_domain(d);
            return -EINVAL;
        }

        spin_unlock(&d->grant_table->lock);
        rcu_unlock_domain(d);

        return ret;
    }
    default:
        printk("XENBLANKET: %s %d nested compat memory op not supported!\n", __FILE__, __LINE__);       
        return -EINVAL;
    }
}



/* simple passthrough */
long do_blanket_memory_op(int cmd, XEN_GUEST_HANDLE(void) arg)
{
    /* map hvm grant table(for ring buf) and shared info page(for evtchn) */
    switch ( cmd )
    {
    /* can map straight through */
    case XENMEM_add_to_physmap:{
        int ret = 0;
        struct xen_add_to_physmap xatp;
        struct xen_add_to_physmap xatp_n;
        struct domain *d;

        if ( copy_from_guest(&xatp, arg, 1) )
            return -EFAULT;

        if ( xatp.domid == DOMID_SELF )
            d = rcu_lock_current_domain();
        else if ( !IS_PRIV(current->domain) ) 
        {
            printk("XENBLANKET: not privileged domain. %s %d\n", __FILE__, __LINE__);
            return -EPERM;
        }
        else if ( (d = rcu_lock_domain_by_id(xatp.domid)) == NULL )
        {
            printk("XENBLANKET: unable to locate domain. %s %d\n", __FILE__, __LINE__);
            return -ESRCH;
        }

        switch ( xatp.space )
        {
        /* LOCKING: shared_info doesn't need to lock */
        case XENMAPSPACE_shared_info:
        case XENMAPSPACE_grant_table:
            xatp_n.domid = DOMID_SELF;
            xatp_n.idx = xatp.idx;
            xatp_n.space = xatp.space;
            xatp_n.gpfn = xatp.gpfn;  /* gpfn is the mfn */
            
            spin_lock(&d->grant_table->lock);

            ret = HYPERVISOR_nested_memory_op(XENMEM_add_to_physmap, &xatp_n);
            if ( ret )
            {
                spin_unlock(&d->grant_table->lock);
                printk("XENBLANKET: memory op failed with %d\n", ret);
                rcu_unlock_domain(d);
                return -EINVAL;
            }
            spin_unlock(&d->grant_table->lock);
            break;
        default:
            printk("XENBLANKET: unknown xatp.space (not shared_info or grant_table)\n");
            rcu_unlock_domain(d);
            return -EINVAL;
            break;
        }
        rcu_unlock_domain(d);
        return 0;
    }
    default:
        printk("%s %d: nested hypercall not implemented!\n", __FILE__, __LINE__);
        return -EINVAL;
    }
}

/* simple passthrough */
long do_blanket_xen_version(int cmd, XEN_GUEST_HANDLE(void) arg)
{
    printk("XENBLANKET: in do_blanket_xen_version.\n");
    switch ( cmd ){
    case XENVER_version:
        return HYPERVISOR_nested_xen_version(XENVER_version, 0);
    case XENVER_get_features:
    {
        xen_feature_info_t fi;

        if ( copy_from_guest(&fi, arg, 1) )
            return -EFAULT;
        
        if ( HYPERVISOR_nested_xen_version(XENVER_get_features, &fi) )
            return -EFAULT;

        if ( copy_to_guest(arg, &fi, 1) )
            return -EFAULT;

        return 0;
    }
    default:
        printk("XENBLANKET: %s %d do_blanket_xen_version - unknown cmd.\n", __FILE__, __LINE__);
        return -EINVAL;
    }
}

/* simple passthrough */
long do_blanket_hvm_op(int op, XEN_GUEST_HANDLE(void) arg)
{
    /* get xenstore mfn and xenstore evtchn num */
    switch (op) 
    {
    case HVMOP_set_param:
    case HVMOP_get_param:
    {
        struct xen_hvm_param a;

        if (copy_from_guest(&a, arg, 1))
            return -EFAULT;

        HYPERVISOR_nested_hvm_op(op, &a);

        if (op == HVMOP_get_param) 
        {
            if (copy_to_guest(arg, &a, 1))
                return -EFAULT;
        }
        
        return 0;
    }
    default:
        printk("XENBLANKET: %s %d, can only perform get_hvm_param!\n", __FILE__, __LINE__);
        return -EINVAL;
    }
}

/* simple passthrough */
long do_blanket_grant_table_op(unsigned int cmd, 
                          XEN_GUEST_HANDLE(void) uop, 
                          unsigned int count){
    long rc = -EINVAL;
    struct domain *d = current->domain;
    struct gnttab_query_size op;


    if ( count > 512 )
        return -EINVAL;
    
    switch ( cmd ) {
    case GNTTABOP_query_size:
        spin_lock(&d->grant_table->lock);
        if ( count != 1 ){
            rc = -EINVAL;
            spin_unlock(&d->grant_table->lock);
            goto out;
        }

        if (copy_from_guest(&op, uop, 1)){
            rc = -EFAULT;
            spin_unlock(&d->grant_table->lock);
            goto out;
        }

        rc = HYPERVISOR_nested_grant_table_op(cmd, &op, count);

        if (copy_to_guest(uop, &op, 1)){
            rc = -EFAULT;
            spin_unlock(&d->grant_table->lock);
            goto out;
        }

        spin_unlock(&d->grant_table->lock);
        break;
    default:
        printk("%s %d: nested hypercall not implemented!\n", __FILE__, __LINE__);
        break;
    }

  out:
    return rc;
}

/* simple passthrough */
long do_blanket_event_channel_op(int cmd, 
                            XEN_GUEST_HANDLE(void) arg)
{
    long rc;

    switch ( cmd ) {
    case EVTCHNOP_unmask: 
    {
        struct evtchn_unmask unmask;
        if ( copy_from_guest(&unmask, arg, 1) != 0 )
            return -EFAULT;
        rc = HYPERVISOR_nested_event_channel_op(cmd, &unmask);
        break;
    }
    case EVTCHNOP_close: 
    {
        struct evtchn_close close;
        if ( copy_from_guest(&close, arg, 1) != 0 )
            return -EFAULT;
        rc = HYPERVISOR_nested_event_channel_op(cmd, &close);
        break;
    }
    case EVTCHNOP_send: 
    {
        struct evtchn_send send;
        if ( copy_from_guest(&send, arg, 1) != 0 )
            return -EFAULT;
        rc = HYPERVISOR_nested_event_channel_op(cmd, &send);
        break;
    }
    case EVTCHNOP_alloc_unbound: 
    {
        struct evtchn_alloc_unbound alloc_unbound;
        if ( copy_from_guest(&alloc_unbound, arg, 1) != 0 )
            return -EFAULT;
        rc = HYPERVISOR_nested_event_channel_op(cmd, &alloc_unbound);
        if ( (rc == 0) && (copy_to_guest(arg, &alloc_unbound, 1) != 0) )
            rc = -EFAULT; /* Cleaning up here would be a mess! */
        break;
    }
    case EVTCHNOP_bind_vcpu: {
	struct evtchn_bind_vcpu bind_vcpu;
	if( copy_from_guest(&bind_vcpu, arg, 1) != 0 )
	    return -EFAULT;
	rc = HYPERVISOR_nested_event_channel_op(cmd, &bind_vcpu);
	if( (rc == 0) && (copy_to_guest(arg, &bind_vcpu, 1) != 0) )
	    rc = -EFAULT;
	break;
    }
    default:
        printk("%s %d: event nested hypercall %d not implemented!\n", 
	    __FILE__, __LINE__, cmd);
        rc = -ENOSYS;
        break;
    }

    return rc;
}

/* simple passthrough */
long do_blanket_sched_op(int cmd, 
                    XEN_GUEST_HANDLE(void) arg)
{
    long ret = 0;

    switch ( cmd )
    {
    case SCHEDOP_shutdown:
    {
        struct sched_shutdown sched_shutdown;

        ret = -EFAULT;
        if ( copy_from_guest(&sched_shutdown, arg, 1) )
            break;

        ret = HYPERVISOR_nested_sched_op(cmd, &sched_shutdown);        

        break;
    }

    default:
        printk("XENBLANKET: %s %d sched op not supported!\n", __FILE__, __LINE__);
        ret = -ENOSYS;
        break;
    }

    return ret;
}
