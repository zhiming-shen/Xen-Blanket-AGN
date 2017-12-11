/******************************************************************************
 * xenbus.h
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

#ifndef _XEN_XENBUS_H
#define _XEN_XENBUS_H

#include <linux/device.h>
#include <linux/notifier.h>
#include <linux/mutex.h>
#include <linux/export.h>
#include <linux/completion.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <xen/interface/xen.h>
#include <xen/interface/grant_table.h>
#include <xen/interface/io/xenbus.h>
#include <xen/interface/io/xs_wire.h>

/* Register callback to watch this node. */
struct xenbus_watch
{
	struct list_head list;

	/* Path being watched. */
	const char *node;

	/* Callback (executed in a process context with no locks held). */
	void (*callback)(struct xenbus_watch *,
			 const char **vec, unsigned int len);
};


/* A xenbus device. */
struct xenbus_device {
	const char *devicetype;
	const char *nodename;
	const char *otherend;
	int otherend_id;
	struct xenbus_watch otherend_watch;
	struct device dev;
	enum xenbus_state state;
	struct completion down;
};

static inline struct xenbus_device *to_xenbus_device(struct device *dev)
{
	return container_of(dev, struct xenbus_device, dev);
}

struct xenbus_device_id
{
	/* .../device/<device_type>/<identifier> */
	char devicetype[32]; 	/* General class of device. */
};

/* A xenbus driver. */
struct xenbus_driver {
	const struct xenbus_device_id *ids;
	int (*probe)(struct xenbus_device *dev,
		     const struct xenbus_device_id *id);
	void (*otherend_changed)(struct xenbus_device *dev,
				 enum xenbus_state backend_state);
	int (*remove)(struct xenbus_device *dev);
	int (*suspend)(struct xenbus_device *dev);
	int (*resume)(struct xenbus_device *dev);
	int (*uevent)(struct xenbus_device *, struct kobj_uevent_env *);
	struct device_driver driver;
	int (*read_otherend_details)(struct xenbus_device *dev);
	int (*is_ready)(struct xenbus_device *dev);
};

#define DEFINE_XENBUS_DRIVER(var, drvname, methods...)		\
struct xenbus_driver var ## _driver = {				\
	.driver.name = drvname + 0 ?: var ## _ids->devicetype,	\
	.driver.owner = THIS_MODULE,				\
	.ids = var ## _ids, ## methods				\
}

static inline struct xenbus_driver *to_xenbus_driver(struct device_driver *drv)
{
	return container_of(drv, struct xenbus_driver, driver);
}

int __must_check xenbus_register_frontend_hvm(struct xenbus_driver *);
int __must_check xenbus_register_backend_hvm(struct xenbus_driver *);

void xenbus_unregister_driver_hvm(struct xenbus_driver *drv);

struct xenbus_transaction
{
	u32 id;
};

/* Nil transaction ID. */
#define XBT_NIL ((struct xenbus_transaction) { 0 })

char **xenbus_directory_hvm(struct xenbus_transaction t,
			const char *dir, const char *node, unsigned int *num);
void *xenbus_read_hvm(struct xenbus_transaction t,
		  const char *dir, const char *node, unsigned int *len);
int xenbus_write_hvm(struct xenbus_transaction t,
		 const char *dir, const char *node, const char *string);
int xenbus_mkdir_hvm(struct xenbus_transaction t,
		 const char *dir, const char *node);
int xenbus_exists_hvm(struct xenbus_transaction t,
		  const char *dir, const char *node);
int xenbus_rm_hvm(struct xenbus_transaction t, const char *dir, const char *node);
int xenbus_transaction_start_hvm(struct xenbus_transaction *t);
int xenbus_transaction_end_hvm(struct xenbus_transaction t, int abort);

/* Single read and scanf: returns -errno or num scanned if > 0. */
__scanf(4, 5)
int xenbus_scanf_hvm(struct xenbus_transaction t,
		 const char *dir, const char *node, const char *fmt, ...);

/* Single printf and write: returns -errno or 0. */
__printf(4, 5)
int xenbus_printf_hvm(struct xenbus_transaction t,
		  const char *dir, const char *node, const char *fmt, ...);

/* Generic read function: NULL-terminated triples of name,
 * sprintf-style type string, and pointer. Returns 0 or errno.*/
int xenbus_gather_hvm(struct xenbus_transaction t, const char *dir, ...);

/* notifer routines for when the xenstore comes up */
extern int xenstored_ready_hvm;
int register_xenstore_notifier_hvm(struct notifier_block *nb);
void unregister_xenstore_notifier_hvm(struct notifier_block *nb);

int register_xenbus_watch_hvm(struct xenbus_watch *watch);
void unregister_xenbus_watch_hvm(struct xenbus_watch *watch);
void xs_suspend_hvm(void);
void xs_resume_hvm(void);
void xs_suspend_cancel_hvm(void);

/* Used by xenbus_dev to borrow kernel's store connection. */
void *xenbus_dev_request_and_reply_hvm(struct xsd_sockmsg *msg);

struct work_struct;

/* Prepare for domain suspend: then resume or cancel the suspend. */
void xenbus_suspend_hvm(void);
void xenbus_resume_hvm(void);
void xenbus_probe_hvm(struct work_struct *);
void xenbus_suspend_cancel_hvm(void);

#define XENBUS_IS_ERR_READ(str) ({			\
	if (!IS_ERR(str) && strlen(str) == 0) {		\
		kfree(str);				\
		str = ERR_PTR(-ERANGE);			\
	}						\
	IS_ERR(str);					\
})

#define XENBUS_EXIST_ERR(err) ((err) == -ENOENT || (err) == -ERANGE)

int xenbus_watch_path_hvm(struct xenbus_device *dev, const char *path,
		      struct xenbus_watch *watch,
		      void (*callback)(struct xenbus_watch *,
				       const char **, unsigned int));
__printf(4, 5)
int xenbus_watch_pathfmt_hvm(struct xenbus_device *dev, struct xenbus_watch *watch,
			 void (*callback)(struct xenbus_watch *,
					  const char **, unsigned int),
			 const char *pathfmt, ...);

int xenbus_switch_state_hvm(struct xenbus_device *dev, enum xenbus_state new_state);
int xenbus_grant_ring_hvm(struct xenbus_device *dev, unsigned long ring_mfn);
int xenbus_map_ring_valloc_hvm(struct xenbus_device *dev,
			   int gnt_ref, void **vaddr);
int xenbus_map_ring_hvm(struct xenbus_device *dev, int gnt_ref,
			   grant_handle_t *handle, void *vaddr);

int xenbus_unmap_ring_vfree_hvm(struct xenbus_device *dev, void *vaddr);
int xenbus_unmap_ring_hvm(struct xenbus_device *dev,
		      grant_handle_t handle, void *vaddr);

int xenbus_alloc_evtchn_hvm(struct xenbus_device *dev, int *port);
int xenbus_bind_evtchn_hvm(struct xenbus_device *dev, int remote_port, int *port);
int xenbus_free_evtchn_hvm(struct xenbus_device *dev, int port);

enum xenbus_state xenbus_read_driver_state_hvm(const char *path);

__printf(3, 4)
void xenbus_dev_error_hvm(struct xenbus_device *dev, int err, const char *fmt, ...);
__printf(3, 4)
void xenbus_dev_fatal_hvm(struct xenbus_device *dev, int err, const char *fmt, ...);

const char *xenbus_strstate_hvm(enum xenbus_state state);
int xenbus_dev_is_online_hvm(struct xenbus_device *dev);
int xenbus_frontend_closed_hvm(struct xenbus_device *dev);

int __init xenbus_init_hvm(void);
int __init xenbus_probe_backend_init_hvm(void);
int __init xenbus_probe_frontend_init_hvm(void);

#endif /* _XEN_XENBUS_H */
