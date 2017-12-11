#ifndef _ASM_X86_XENBLANKET_HYPERCALL_H
#define _ASM_X86_XENBLANKET_HYPERCALL_H

#include "xenblanket.h"

struct blanket_get_cpuid {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
};


static inline long
HYPERVISOR_blanket_get_cpuid(
        uint32_t base,
        struct blanket_get_cpuid *arg)
{
        return _hypercall2(long, blanket_get_cpuid, base, arg);
}

static inline long
HYPERVISOR_blanket_init_nested_hypercall(void)
{
        return _hypercall0(long, blanket_init_nested_hypercall);
}

static inline long
HYPERVISOR_blanket_xen_version(int cmd, void *arg)
{
        return _hypercall2(long, blanket_xen_version, cmd, arg);
}

static inline long
HYPERVISOR_blanket_grant_table_op(unsigned int cmd, void *uop, unsigned int count)
{
        return _hypercall3(long, blanket_grant_table_op, cmd, uop, count);
}

static inline long
HYPERVISOR_blanket_memory_op(unsigned int cmd, void *arg)
{
        return _hypercall2(long, blanket_memory_op, cmd, arg);
}

static inline long
HYPERVISOR_blanket_hvm_op(int op, void *arg)
{
       return _hypercall2(long, blanket_hvm_op, op, arg);
}

static inline long
HYPERVISOR_blanket_event_channel_op(int cmd, void *arg)
{
        return _hypercall2(long, blanket_event_channel_op, cmd, arg);
}

static inline long
HYPERVISOR_blanket_sched_op(int cmd, void *arg)
{
        return _hypercall2(long, blanket_sched_op, cmd, arg);
}

#endif

