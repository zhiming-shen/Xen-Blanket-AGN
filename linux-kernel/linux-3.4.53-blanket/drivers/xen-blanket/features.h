/******************************************************************************
 * features.h
 *
 * Query the features reported by Xen.
 *
 * Copyright (c) 2006, Ian Campbell
 */

#ifndef __XENBLANKET_FEATURES_H__
#define __XENBLANKET_FEATURES_H__

#include <xen/interface/features.h>

void xen_setup_features_hvm(void);

extern u8 xen_features_hvm[XENFEAT_NR_SUBMAPS * 32];

static inline int xen_feature_hvm(int flag)
{
	return xen_features_hvm[flag];
}

#endif /* __ASM_XEN_FEATURES_H__ */
