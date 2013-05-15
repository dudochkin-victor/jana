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
opened_cb (JanaStore *store, JanaGtkEventStore *event_store)
{
	JanaTime *range_start, *range_end;
	
	range_start = jana_ecal_utils_time_today ("UTC");
	range_end = jana_time_duplicate (range_start);

	jana_time_set_day (range_end, jana_time_get_day (range_end) + 5);
	jana_time_set_day (range_start, jana_time_get_day (range_start) - 5);

	store_view = jana_store_get_view (store);
	jana_store_view_set_range (store_view, range_start, range_end);
	jana_gtk_event_store_set_view (event_store, store_view);

	jana_store_view_start (store_view);
	
	g_object_unref (range_start);
	g_object_unref (range_end);
}

int
main (int argc, char **argv)
{
	JanaStore *store;
	GtkTreeModel *event_store;
	GtkWidget *window, *treeview;
	GtkCellRenderer *renderer;
	
	gtk_init (&argc, &argv);
	
	store = jana_ecal_store_new (JANA_COMPONENT_EVENT);
	
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	event_store = jana_gtk_event_store_new ();
	treeview = gtk_tree_view_new_with_model (event_store);
	renderer = jana_gtk_cell_renderer_event_new ();
	g_object_set (renderer, "draw_time", TRUE, "draw_box", FALSE, NULL);
	gtk_tree_view_insert_column_with_attributes (
		GTK_TREE_VIEW (treeview), -1, "Events", renderer,
		"uid", JANA_GTK_EVENT_STORE_COL_UID,
		"summary", JANA_GTK_EVENT_STORE_COL_SUMMARY,
		"description", JANA_GTK_EVENT_STORE_COL_DESCRIPTION,
		"start", JANA_GTK_EVENT_STORE_COL_START,
		"end", JANA_GTK_EVENT_STORE_COL_END,
		NULL);
	gtk_container_add (GTK_CONTAINER (window), treeview);
	gtk_window_set_default_size (GTK_WINDOW (window), 300, 400);
	
	g_signal_connect (G_OBJECT (window), "delete-event",
		G_CALLBACK (gtk_main_quit), NULL);
	
	gtk_widget_show_all (window);
	
	g_signal_connect (G_OBJECT (store), "opened",
		G_CALLBACK (opened_cb), event_store);
	jana_store_open (store);
	
	gtk_main ();
	
	g_object_unref (store);
	if (store_view) g_object_unref (store_view);
	
	return 0;
}

