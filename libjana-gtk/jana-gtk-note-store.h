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


#ifndef _JANA_GTK_NOTE_STORE_H
#define _JANA_GTK_NOTE_STORE_H

#include <gtk/gtk.h>
#include <glib-object.h>
#include <libjana/jana-store-view.h>

G_BEGIN_DECLS

#define JANA_GTK_TYPE_NOTE_STORE jana_gtk_note_store_get_type()

#define JANA_GTK_NOTE_STORE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	JANA_GTK_TYPE_NOTE_STORE, JanaGtkNoteStore))

#define JANA_GTK_NOTE_STORE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	JANA_GTK_TYPE_NOTE_STORE, JanaGtkNoteStoreClass))

#define JANA_GTK_IS_NOTE_STORE(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	JANA_GTK_TYPE_NOTE_STORE))

#define JANA_GTK_IS_NOTE_STORE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	JANA_GTK_TYPE_NOTE_STORE))

#define JANA_GTK_NOTE_STORE_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	JANA_GTK_TYPE_NOTE_STORE, JanaGtkNoteStoreClass))

typedef struct {
	GtkListStore parent;
} JanaGtkNoteStore;

typedef struct {
	GtkListStoreClass parent_class;
} JanaGtkNoteStoreClass;

enum {
	JANA_GTK_NOTE_STORE_COL_UID,
	JANA_GTK_NOTE_STORE_COL_CATEGORIES,
	JANA_GTK_NOTE_STORE_COL_AUTHOR,
	JANA_GTK_NOTE_STORE_COL_RECIPIENT,
	JANA_GTK_NOTE_STORE_COL_BODY,
	JANA_GTK_NOTE_STORE_COL_CREATED,
	JANA_GTK_NOTE_STORE_COL_MODIFIED,
	JANA_GTK_NOTE_STORE_COL_LAST
};

GType jana_gtk_note_store_get_type (void);

GtkTreeModel * jana_gtk_note_store_new (void);
GtkTreeModel * jana_gtk_note_store_new_full (JanaStoreView *view);
void jana_gtk_note_store_set_view (JanaGtkNoteStore *store,
				   JanaStoreView *view);
JanaStoreView * jana_gtk_note_store_get_view (JanaGtkNoteStore *store);
JanaStore * jana_gtk_note_store_get_store (JanaGtkNoteStore *store);

G_END_DECLS

#endif /* _JANA_GTK_NOTE_STORE_H */
