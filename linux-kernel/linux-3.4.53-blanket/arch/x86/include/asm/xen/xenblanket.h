#ifndef XENBLANKET
#define XENBLANKET

#define __HYPERVISOR_blanket_get_cpuid    39
#define __HYPERVISOR_blanket_init_nested_hypercall      40      /* 40 */
#define __HYPERVISOR_blanket_xen_version                41               
#define __HYPERVISOR_blanket_memory_op                  42                 
#define __HYPERVISOR_blanket_hvm_op                     43                   
#define __HYPERVISOR_blanket_grant_table_op             44           
#define __HYPERVISOR_blanket_event_channel_op           45      /* 45 */
#define __HYPERVISOR_blanket_sched_op                   46

extern int xenblanket_platform;
extern struct shared_info *HYPERVISOR_shared_info_hvm;

#endif
