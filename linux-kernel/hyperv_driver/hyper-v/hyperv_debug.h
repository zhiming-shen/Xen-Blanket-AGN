#ifndef _XEN_BLANKET_HV_DEBUG
#define _XEN_BLANKET_HV_DEBUG

#define XENBLANKET_ENABLED 1

#if XENBLANKET_ENABLED
#include <asm/xen/page.h>
#endif


#define XENBLANKET_DEBUG 1
#if XENBLANKET_DEBUG
#define xenblanket_printk( s, arg... ) printk("=====XEN-BLANKET-HV=====" s, ##arg)
#else
#define xenblanket_printk( s, arg... )
#endif




#endif
