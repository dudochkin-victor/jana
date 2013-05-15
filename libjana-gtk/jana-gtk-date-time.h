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


#ifndef _JANA_GTK_DATE_TIME_H
#define _JANA_GTK_DATE_TIME_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libjana/jana-time.h>

G_BEGIN_DECLS

#define JANA_GTK_TYPE_DATE_TIME jana_gtk_date_time_get_type()

#define JANA_GTK_DATE_TIME(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	JANA_GTK_TYPE_DATE_TIME, JanaGtkDateTime))

#define JANA_GTK_DATE_TIME_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	JANA_GTK_TYPE_DATE_TIME, JanaGtkDateTimeClass))

#define JANA_GTK_IS_DATE_TIME(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	JANA_GTK_TYPE_DATE_TIME))

#define JANA_GTK_IS_DATE_TIME_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	JANA_GTK_TYPE_DATE_TIME))

#define JANA_GTK_DATE_TIME_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	JANA_GTK_TYPE_DATE_TIME, JanaGtkDateTimeClass))

typedef struct {
	GtkVBox parent;
} JanaGtkDateTime;

typedef struct {
	GtkVBoxClass parent_class;
	
	/* Signals */
	void	(*changed)	(JanaGtkDateTime *self);
} JanaGtkDateTimeClass;

GType jana_gtk_date_time_get_type (void);

GtkWidget* jana_gtk_date_time_new (JanaTime *time);

JanaTime *jana_gtk_date_time_get_time (JanaGtkDateTime *self);
void jana_gtk_date_time_set_time (JanaGtkDateTime *self, JanaTime *time);

void		jana_gtk_date_time_set_editable	(JanaGtkDateTime *self,
						 gboolean editable);
gboolean	jana_gtk_date_time_get_editable	(JanaGtkDateTime *self);

G_END_DECLS

#endif /* _JANA_GTK_DATE_TIME_H */

