#ifndef __BLANKET_NESTED_HYPERCALL_H__
#define __BLANKET_NESTED_HYPERCALL_H__

/* Make sure this doesn't go over 47 */
#define __HYPERVISOR_blanket_get_cpuid 			39
#define __HYPERVISOR_blanket_init_nested_hypercall     	40	/* 40 */
#define __HYPERVISOR_blanket_xen_version                41
#define __HYPERVISOR_blanket_memory_op                  42
#define __HYPERVISOR_blanket_hvm_op                     43
#define __HYPERVISOR_blanket_grant_table_op             44
#define __HYPERVISOR_blanket_event_channel_op          	45	/* 45 */
#define __HYPERVISOR_blanket_sched_op			46

#define HYPERCALL_STR_nested(name)                         \
    "movabs hypercall_stubs,%%rax; "                       \
    "add $("STR(__HYPERVISOR_##name)" * 32),%%rax; "       \
    "call *%%rax"

#define _hypercall2_nested(type, name, a1, a2)                       \
    ({                                                               \
        long __res, __ign1, __ign2;                                  \
        asm volatile (                                               \
                      HYPERCALL_STR_nested(name)                     \
                      : "=a" (__res), "=D" (__ign1), "=S" (__ign2)   \
                      : "1" ((long)(a1)), "2" ((long)(a2))           \
                      : "memory" );                                  \
        (type)__res;                                                 \
    })

#define _hypercall3_nested(type, name, a1, a2, a3)                      \
    ({                                                                  \
        long __res, __ign1, __ign2, __ign3;                             \
        asm volatile (                                                  \
                      HYPERCALL_STR_nested(name)                        \
                      : "=a" (__res), "=D" (__ign1), "=S" (__ign2),     \
                        "=d" (__ign3)                                   \
                      : "1" ((long)(a1)), "2" ((long)(a2)),             \
                        "3" ((long)(a3))                                \
                      : "memory" );                                     \
        (type)__res;                                                    \
    })


static inline int 
HYPERVISOR_nested_xen_version(int cmd, void *arg){
    return _hypercall2_nested(int, xen_version, cmd, arg);
}
static inline int 
HYPERVISOR_nested_memory_op(int cmd, void *arg){
    return _hypercall2_nested(int, memory_op, cmd, arg);
}
static inline unsigned long
HYPERVISOR_nested_hvm_op(int op, void *arg){
    return _hypercall2_nested(unsigned long, hvm_op, op, arg);
}
static inline int
HYPERVISOR_nested_grant_table_op(unsigned int cmd, void *uop, unsigned int count){
    return _hypercall3_nested(int, grant_table_op, cmd, uop, count);
}
static inline int
HYPERVISOR_nested_event_channel_op(unsigned int cmd, void *arg){
    return _hypercall2_nested(int, event_channel_op, cmd, arg);
}
static inline int
HYPERVISOR_nested_sched_op(unsigned int cmd, void *arg){
    return _hypercall2_nested(int, sched_op, cmd, arg);
}


#endif

/*
 * Local variables:
 * mode: C
 * c-set-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
