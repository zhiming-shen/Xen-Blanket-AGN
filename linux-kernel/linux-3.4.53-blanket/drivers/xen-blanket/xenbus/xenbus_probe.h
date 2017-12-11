/******************************************************************************
 * xenbus_probe.h
 *
 * Talks to Xen Store to figure out what devices we have.
 *
 * Copyright (C) 2005 Rusty Russell, IBM Corporation
 * Copyright (C) 2005 XenSource Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation; or, when distributed
 * separately from the Linux kernel or incorporated into other
 * software packages, subject to the following license:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef _XENBUS_PROBE_H
#define _XENBUS_PROBE_H

#define XEN_BUS_ID_SIZE			20

struct xen_bus_type {
	char *root;
	unsigned int levels;
	int (*get_bus_id)(char bus_id[XEN_BUS_ID_SIZE], const char *nodename);
	int (*probe)(struct xen_bus_type *bus, const char *type,
		     const char *dir);
	void (*otherend_changed)(struct xenbus_watch *watch, const char **vec,
				 unsigned int len);
	struct bus_type bus;
};

extern struct device_attribute xenbus_dev_attrs_hvm[];

extern int xenbus_match_hvm(struct device *_dev, struct device_driver *_drv);
extern int xenbus_dev_probe_hvm(struct device *_dev);
extern int xenbus_dev_remove_hvm(struct device *_dev);
extern int xenbus_register_driver_common_hvm(struct xenbus_driver *drv,
					 struct xen_bus_type *bus);
extern int xenbus_probe_node_hvm(struct xen_bus_type *bus,
			     const char *type,
			     const char *nodename);
extern int xenbus_probe_devices_hvm(struct xen_bus_type *bus);

extern void xenbus_dev_changed_hvm(const char *node, struct xen_bus_type *bus);

extern void xenbus_dev_shutdown_hvm(struct device *_dev);

extern int xenbus_dev_suspend_hvm(struct device *dev);
extern int xenbus_dev_resume_hvm(struct device *dev);
extern int xenbus_dev_cancel_hvm(struct device *dev);

extern void xenbus_otherend_changed_hvm(struct xenbus_watch *watch,
				    const char **vec, unsigned int len,
				    int ignore_on_shutdown);

extern int xenbus_read_otherend_details_hvm(struct xenbus_device *xendev,
					char *id_node, char *path_node);

void xenbus_ring_ops_init_hvm(void);

#endif
