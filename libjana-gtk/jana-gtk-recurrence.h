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


#ifndef _JANA_GTK_RECURRENCE_H
#define _JANA_GTK_RECURRENCE_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libjana/jana-event.h>

G_BEGIN_DECLS

#define JANA_GTK_TYPE_RECURRENCE jana_gtk_recurrence_get_type()

#define JANA_GTK_RECURRENCE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	JANA_GTK_TYPE_RECURRENCE, JanaGtkRecurrence))

#define JANA_GTK_RECURRENCE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	JANA_GTK_TYPE_RECURRENCE, JanaGtkRecurrenceClass))

#define JANA_GTK_IS_RECURRENCE(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	JANA_GTK_TYPE_RECURRENCE))

#define JANA_GTK_IS_RECURRENCE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	JANA_GTK_TYPE_RECURRENCE))

#define JANA_GTK_RECURRENCE_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	JANA_GTK_TYPE_RECURRENCE, JanaGtkRecurrenceClass))

typedef struct {
	GtkVBox parent;
} JanaGtkRecurrence;

typedef struct {
	GtkVBoxClass parent_class;
} JanaGtkRecurrenceClass;

GType jana_gtk_recurrence_get_type (void);

GtkWidget * jana_gtk_recurrence_new (void);

void jana_gtk_recurrence_set_recur (JanaGtkRecurrence *self,
				    JanaRecurrence *recur);

void jana_gtk_recurrence_set_editable (JanaGtkRecurrence *self,
				       gboolean editable);

void jana_gtk_recurrence_set_time (JanaGtkRecurrence *self, JanaTime *time);

JanaRecurrence * jana_gtk_recurrence_get_recur (JanaGtkRecurrence *self);

gboolean jana_gtk_recurrence_get_editable (JanaGtkRecurrence *self);

JanaTime * jana_gtk_recurrence_get_time (JanaGtkRecurrence *self);

G_END_DECLS

#endif /* _JANA_GTK_RECURRENCE_H */

