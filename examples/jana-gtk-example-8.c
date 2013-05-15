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


#include <glib.h>
#include <libjana/jana.h>
#include <libjana-ecal/jana-ecal.h>
#include <libjana-gtk/jana-gtk.h>

static JanaStoreView *store_view = NULL;

static void
opened_cb (JanaStore *store, JanaGtkNoteStore *note_store)
{
	store_view = jana_store_get_view (store);
	jana_gtk_note_store_set_view (note_store, store_view);

	jana_store_view_start (store_view);
}

int
main (int argc, char **argv)
{
	JanaStore *store;
	GtkTreeModel *note_store;
	GtkWidget *window, *treeview, *scroll;
	GtkCellRenderer *renderer;
	GdkPixbuf *pixbuf;
	
	gtk_init (&argc, &argv);
	
	store = jana_ecal_store_new (JANA_COMPONENT_NOTE);
	
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	note_store = jana_gtk_note_store_new ();
	treeview = gtk_tree_view_new_with_model (note_store);
	renderer = jana_gtk_cell_renderer_note_new ();
	pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
		GTK_STOCK_EDIT, 48, 0, NULL);
	g_object_set (G_OBJECT (renderer), "icon", pixbuf,
		"justify", GTK_JUSTIFY_CENTER, NULL);
	g_object_unref (pixbuf);
	gtk_tree_view_insert_column_with_attributes (
		GTK_TREE_VIEW (treeview), -1, "Notes", renderer,
		"author", JANA_GTK_NOTE_STORE_COL_AUTHOR,
		"recipient", JANA_GTK_NOTE_STORE_COL_RECIPIENT,
		"body", JANA_GTK_NOTE_STORE_COL_BODY,
		"created", JANA_GTK_NOTE_STORE_COL_CREATED,
		"modified", JANA_GTK_NOTE_STORE_COL_MODIFIED,
		NULL);
	g_signal_connect (treeview, "size-allocate",
		G_CALLBACK (jana_gtk_utils_treeview_resize), renderer);
	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scroll), treeview);
	gtk_container_add (GTK_CONTAINER (window), scroll);
	gtk_window_set_default_size (GTK_WINDOW (window), 300, 400);
	
	g_signal_connect (G_OBJECT (window), "delete-event",
		G_CALLBACK (gtk_main_quit), NULL);
	
	gtk_widget_show_all (window);
	
	g_signal_connect (store, "opened",
		G_CALLBACK (opened_cb), note_store);
	jana_store_open (store);
	
	gtk_main ();
	
	g_object_unref (store);
	if (store_view) g_object_unref (store_view);
	
	return 0;
}

