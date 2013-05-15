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


#ifndef _JANA_GTK_EVENT_LIST_H
#define _JANA_GTK_EVENT_LIST_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libjana-gtk/jana-gtk-event-store.h>

G_BEGIN_DECLS

#define JANA_GTK_TYPE_EVENT_LIST jana_gtk_event_list_get_type()

#define JANA_GTK_EVENT_LIST(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	JANA_GTK_TYPE_EVENT_LIST, JanaGtkEventList))

#define JANA_GTK_EVENT_LIST_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	JANA_GTK_TYPE_EVENT_LIST, JanaGtkEventListClass))

#define JANA_GTK_IS_EVENT_LIST(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	JANA_GTK_TYPE_EVENT_LIST))

#define JANA_GTK_IS_EVENT_LIST_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	JANA_GTK_TYPE_EVENT_LIST))

#define JANA_GTK_EVENT_LIST_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	JANA_GTK_TYPE_EVENT_LIST, JanaGtkEventListClass))

typedef struct {
	GtkTreeView parent;
} JanaGtkEventList;

typedef struct {
	GtkTreeViewClass parent_class;
} JanaGtkEventListClass;

enum {
	JANA_GTK_EVENT_LIST_COL_ROW,
	JANA_GTK_EVENT_LIST_COL_TIME,
	JANA_GTK_EVENT_LIST_COL_HEADER,
	JANA_GTK_EVENT_LIST_COL_IS_EVENT,
	JANA_GTK_EVENT_LIST_COL_IS_HEADER,
	JANA_GTK_EVENT_LIST_COL_LAST
};

GType jana_gtk_event_list_get_type (void);

GtkWidget* jana_gtk_event_list_new (void);

void	jana_gtk_event_list_add_store		(JanaGtkEventList *self,
						 JanaGtkEventStore *store);
void	jana_gtk_event_list_remove_store	(JanaGtkEventList *self,
						 JanaGtkEventStore *store);
GtkTreeModelFilter *	jana_gtk_event_list_get_filter
						(JanaGtkEventList *self);
void	jana_gtk_event_list_refilter		(JanaGtkEventList *self);

void	jana_gtk_event_list_set_show_headers	(JanaGtkEventList *self,
						 gboolean show_headers);

gboolean	jana_gtk_event_list_get_show_headers
						(JanaGtkEventList *self);

G_END_DECLS

#endif /* _JANA_GTK_EVENT_LIST_H */

