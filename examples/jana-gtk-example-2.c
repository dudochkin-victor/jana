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


#include <math.h>
#include <glib.h>
#include <libjana/jana.h>
#include <libjana-ecal/jana-ecal.h>
#include <libjana-gtk/jana-gtk.h>

#define DEG2RAD(x)	((x)*(M_PI/180))

static JanaStoreView *store_view = NULL;
static GtkCellRenderer *renderer;

static const gint r = 240;
static gdouble angle = 0;

static gboolean
animate_cb (gpointer data)
{
	GList *cell, *cells, *selected;
	gint n_cells, i;
	JanaGtkTreeLayout *layout = (JanaGtkTreeLayout *)data;
	
	if (!(cells = jana_gtk_tree_layout_get_cells (layout))) return TRUE;
	selected = jana_gtk_tree_layout_get_selection (layout);
	n_cells = g_list_length (cells);

	for (i = 0, cell = cells; cell; cell = cell->next, i++) {
		gint x, y;
		gdouble local_angle;
		JanaGtkTreeLayoutCellInfo *info =
			(JanaGtkTreeLayoutCellInfo *)cell->data;
		
		local_angle = angle + ((360 / n_cells) * i);
		while (local_angle > 360) local_angle -= 360;

		if (g_list_find (selected, info)) {
			x = 240; y = 240;
		} else {
			x = (r * cos (DEG2RAD (local_angle))) + 240;
			y = (r * sin (DEG2RAD (local_angle))) + 240;
		}
		
		jana_gtk_tree_layout_move_cell (layout, info->row,
			(info->x + x) / 2, (info->y + y) / 2,
			info->width, info->height);
	}

	angle ++;
	if (angle > 360) angle -= 360;
	
	g_list_free (cells);
	g_list_free (selected);
	
	return TRUE;
}

static void
row_inserted_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
		 JanaGtkTreeLayout *layout)
{
	GtkTreeRowReference *row = gtk_tree_row_reference_new (model, path);
	
	jana_gtk_tree_layout_add_cell (JANA_GTK_TREE_LAYOUT (layout),
		row, -150, 240, 150, -1, renderer,
		"uid", JANA_GTK_EVENT_STORE_COL_UID,
		"summary", JANA_GTK_EVENT_STORE_COL_SUMMARY,
		"description", JANA_GTK_EVENT_STORE_COL_DESCRIPTION,
		"start", JANA_GTK_EVENT_STORE_COL_START,
		"end", JANA_GTK_EVENT_STORE_COL_END,
		NULL);
	gtk_tree_row_reference_free (row);
}

static void
opened_cb (JanaStore *store, JanaGtkEventStore *event_store)
{
	JanaTime *range_start, *range_end;
	
	range_start = jana_ecal_utils_time_today ("UTC");
	range_end = jana_time_duplicate (range_start);
	
	jana_time_set_day (range_end, jana_time_get_day (range_end) + 1);

	store_view = jana_store_get_view (store);
	jana_store_view_set_range (store_view, range_start, range_end);
	jana_gtk_event_store_set_view (event_store, store_view);
	jana_store_view_start (store_view);
	
	g_object_unref (range_start);
	g_object_unref (range_end);
}

static gboolean
delete_event_cb (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	g_source_remove_by_user_data (user_data);
	gtk_main_quit ();
	return FALSE;
}

int
main (int argc, char **argv)
{
	JanaStore *store;
	GtkWidget *window;
	GtkWidget *tree_layout;
	GtkTreeModel *event_store;
	guint animate_id;
	
	gtk_init (&argc, &argv);
	
	store = jana_ecal_store_new (JANA_COMPONENT_EVENT);
	
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	event_store = jana_gtk_event_store_new ();
	tree_layout = jana_gtk_tree_layout_new ();
	/* These allow the widget to resize correctly when sizing the window,
	 * but looks weird due to the variable required size in this example.
	 */
	/*g_object_set (tree_layout, "fill_width", TRUE, "fill_height", TRUE,
		NULL);*/
	renderer = jana_gtk_cell_renderer_event_new ();
	g_object_set (renderer, "draw_time", TRUE, NULL);
	gtk_container_add (GTK_CONTAINER (window), tree_layout);
	gtk_window_set_default_size (GTK_WINDOW (window), 630, 630);
	gtk_window_set_title (GTK_WINDOW (window), "Today's Events");
	
	g_signal_connect (event_store, "row-inserted",
		G_CALLBACK (row_inserted_cb), tree_layout);
	g_signal_connect (window, "delete-event",
		G_CALLBACK (delete_event_cb), tree_layout);
	
	gtk_widget_show_all (window);
	
	g_signal_connect (G_OBJECT (store), "opened",
		G_CALLBACK (opened_cb), event_store);
	jana_store_open (store);
	
	animate_id = g_timeout_add (1000/30, animate_cb, tree_layout);
	
	gtk_main ();
	
	g_object_unref (store);
	if (store_view) g_object_unref (store_view);
	
	return 0;
}

