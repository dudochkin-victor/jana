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


#ifndef _JANA_GTK_EVENT_STORE
#define _JANA_GTK_EVENT_STORE

#include <gtk/gtk.h>
#include <glib-object.h>
#include <libjana/jana-store-view.h>

G_BEGIN_DECLS

#define JANA_GTK_TYPE_EVENT_STORE jana_gtk_event_store_get_type()

#define JANA_GTK_EVENT_STORE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  JANA_GTK_TYPE_EVENT_STORE, JanaGtkEventStore))

#define JANA_GTK_EVENT_STORE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  JANA_GTK_TYPE_EVENT_STORE, JanaGtkEventStoreClass))

#define JANA_GTK_IS_EVENT_STORE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  JANA_GTK_TYPE_EVENT_STORE))

#define JANA_GTK_IS_EVENT_STORE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  JANA_GTK_TYPE_EVENT_STORE))

#define JANA_GTK_EVENT_STORE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  JANA_GTK_TYPE_EVENT_STORE, JanaGtkEventStoreClass))

typedef struct {
	GtkListStore parent;
} JanaGtkEventStore;

typedef struct {
	GtkListStoreClass parent_class;
} JanaGtkEventStoreClass;

enum {
	JANA_GTK_EVENT_STORE_COL_UID,
	JANA_GTK_EVENT_STORE_COL_SUMMARY,
	JANA_GTK_EVENT_STORE_COL_DESCRIPTION,
	JANA_GTK_EVENT_STORE_COL_LOCATION,
	JANA_GTK_EVENT_STORE_COL_CATEGORIES,
	JANA_GTK_EVENT_STORE_COL_START,
	JANA_GTK_EVENT_STORE_COL_END,
	JANA_GTK_EVENT_STORE_COL_FIRST_INSTANCE,
	JANA_GTK_EVENT_STORE_COL_LAST_INSTANCE,
	JANA_GTK_EVENT_STORE_COL_HAS_RECURRENCES,
	JANA_GTK_EVENT_STORE_COL_HAS_ALARM,
	JANA_GTK_EVENT_STORE_COL_RECUR_TYPE,
	JANA_GTK_EVENT_STORE_COL_LAST
};

GType jana_gtk_event_store_get_type (void);

GtkTreeModel * jana_gtk_event_store_new (void);
GtkTreeModel * jana_gtk_event_store_new_full (JanaStoreView *view,
						   glong offset);
void jana_gtk_event_store_set_view (JanaGtkEventStore *store,
				    JanaStoreView *view);
JanaStoreView * jana_gtk_event_store_get_view (JanaGtkEventStore *store);
JanaStore * jana_gtk_event_store_get_store (JanaGtkEventStore *store);
void jana_gtk_event_store_set_offset (JanaGtkEventStore *self, glong offset);
G_END_DECLS

#endif /* _JANA_GTK_EVENT_STORE */

