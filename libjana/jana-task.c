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


/**
 * SECTION:jana-task
 * @short_description: A task/todo component interface
 * @see_also: #JanaComponent
 *
 * #JanaTask is the interface for components that store task information.
 */

#include "jana-task.h"

static void
jana_task_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

GType
jana_task_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (JanaTaskInterface),
			jana_task_base_init,   /* base_init */
			NULL,

		};
		type = g_type_register_static (G_TYPE_INTERFACE,
			"JanaTask", &info, 0);
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}
	return type;
}

gchar *
jana_task_get_summary (JanaTask *self)
{
	return JANA_TASK_GET_INTERFACE (self)->get_summary (self);
}

gchar *
jana_task_get_description (JanaTask *self)
{
	return JANA_TASK_GET_INTERFACE (self)->get_description (self);
}

gboolean
jana_task_get_completed (JanaTask *self)
{
	return JANA_TASK_GET_INTERFACE (self)->get_completed (self);
}

JanaTime *
jana_task_get_due_date (JanaTask *self)
{
	return JANA_TASK_GET_INTERFACE (self)->get_due_date (self);
}

gint
jana_task_get_priority (JanaTask *self)
{
	return JANA_TASK_GET_INTERFACE (self)->get_priority (self);
}

void
jana_task_set_summary (JanaTask *self, const gchar *summary)
{
	JANA_TASK_GET_INTERFACE (self)->set_summary (self, summary);
}

void
jana_task_set_description (JanaTask *self, const gchar *description)
{
	JANA_TASK_GET_INTERFACE (self)->set_description (self, description);
}

void
jana_task_set_completed (JanaTask *self, gboolean completed)
{
	JANA_TASK_GET_INTERFACE (self)->set_completed (self, completed);
}

void
jana_task_set_due_date (JanaTask *self, JanaTime *time)
{
	JANA_TASK_GET_INTERFACE (self)->set_due_date (self, time);
}

void
jana_task_set_priority (JanaTask *self, gint priority)
{
	JANA_TASK_GET_INTERFACE (self)->set_priority (self, priority);
}

