#include "xen.h"

#ifndef XEN_BLANKET_HYPERCALL_PARAMS
#define XEN_BLANKET_HYPERCALL_PARAMS

struct blanket_get_cpuid {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
};
typedef struct blanket_get_cpuid blanket_get_cpuid_t;

#endif
