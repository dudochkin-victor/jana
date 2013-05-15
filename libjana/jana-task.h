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


#ifndef JANA_TASK_H
#define JANA_TASK_H

#include <glib-object.h>
#include <libjana/jana-time.h>

#define JANA_TYPE_TASK		(jana_task_get_type ())
#define JANA_TASK(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj),\
				 JANA_TYPE_TASK, JanaTask))
#define JANA_IS_TASK(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
				 JANA_TYPE_TASK))
#define JANA_TASK_GET_INTERFACE(inst)	(G_TYPE_INSTANCE_GET_INTERFACE ((inst),\
					 JANA_TYPE_TASK, JanaTaskInterface))

/**
 * JanaTask:
 *
 * The #JanaTask struct contains only private data.
 */
typedef struct _JanaTask JanaTask; /* Dummy object */
typedef struct _JanaTaskInterface JanaTaskInterface;

struct _JanaTaskInterface {
	GTypeInterface parent;

	gchar *				(*get_summary)		(JanaTask *self);
	gchar *				(*get_description)	(JanaTask *self);
	gboolean			(*get_completed)	(JanaTask *self);
	JanaTime *			(*get_due_date)		(JanaTask *self);
	gint				(*get_priority)		(JanaTask *self);

	void		(*set_summary)		(JanaTask *self, const gchar *summary);
	void		(*set_description)	(JanaTask *self, const gchar *description);
	void		(*set_completed)	(JanaTask *self, gboolean completed);
	void		(*set_due_date)		(JanaTask *self, JanaTime *time);
	void		(*set_priority)		(JanaTask *self, gint priority);
};

GType jana_task_get_type (void);

gchar *		jana_task_get_summary			(JanaTask *self);
gchar *		jana_task_get_description		(JanaTask *self);
gboolean	jana_task_get_completed			(JanaTask *self);
JanaTime *	jana_task_get_due_date			(JanaTask *self);
gint		jana_task_get_priority			(JanaTask *self);

void		jana_task_set_summary			(JanaTask *self, const gchar *summary);
void		jana_task_set_description		(JanaTask *self, const gchar *description);
void		jana_task_set_completed			(JanaTask *self, gboolean completed);
void		jana_task_set_due_date			(JanaTask *self, JanaTime *time);
void		jana_task_set_priority			(JanaTask *self, gint priority);

#endif /* JANA_TASK_H */

