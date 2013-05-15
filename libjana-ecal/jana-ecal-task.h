/*
 * Author: Chris Lord <chris@linux.intel.com>
 * Copyright (c) 2007 OpenedHand Ltd
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */


#ifndef JANA_ECAL_TASK_H
#define JANA_ECAL_TASK_H

#include <glib-object.h>
#include <libecal/libecal.h>
#include <libjana/jana-component.h>
#include <libjana/jana-task.h>

#define JANA_ECAL_TYPE_TASK		(jana_ecal_task_get_type ())
#define JANA_ECAL_TASK(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
					 JANA_ECAL_TYPE_TASK, JanaEcalTask))
#define JANA_ECAL_TASK_CLASS(vtable)	(G_TYPE_CHECK_CLASS_CAST ((vtable), \
					 JANA_ECAL_TYPE_TASK, \
					 JanaEcalTaskClass))
#define JANA_ECAL_IS_TASK(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
					 JANA_ECAL_TYPE_TASK))
#define JANA_ECAL_IS_TASK_CLASS(vtable)(G_TYPE_CHECK_CLASS_TYPE ((vtable), \
					JANA_ECAL_TYPE_TASK))
#define JANA_ECAL_TASK_GET_CLASS(inst)	(G_TYPE_INSTANCE_GET_CLASS ((inst), \
					 JANA_ECAL_TYPE_TASK, \
					 JanaEcalTaskClass))

typedef struct _JanaEcalTask JanaEcalTask;
typedef struct _JanaEcalTaskClass JanaEcalTaskClass;

/**
 * JanaEcalTask:
 *
 * The #JanaEcalTask struct contains only private data.
 */
struct _JanaEcalTask {
	GObject parent;
};

struct _JanaEcalTaskClass {
	GObjectClass parent;
};

GType jana_ecal_task_get_type (void);

JanaTask *jana_ecal_task_new ();
JanaTask *jana_ecal_task_new_from_ecalcomp (ECalComponent *task);

#endif /* JANA_ECAL_TASK_H */
