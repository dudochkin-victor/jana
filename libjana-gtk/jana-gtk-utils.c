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


#include "jana-gtk-utils.h"

/**
 * jana_gtk_utils_treeview_resize:
 * @tree_view: A #GtkTreeView
 * @allocation: The widget's allocated size
 * @cell_renderer: A #GtkCellRenderer
 *
 * A convenience callback that can be used in conjunction with 
 * #GtkTreeView::size-allocate that will set the cell renderer width to the 
 * width of the first column in @tree_view. This can be useful if you want 
 * correctly sized rows when using #JanaGtkCellRendererEvent or 
 * #JanaGtkCellRendererNote.
 */
void
jana_gtk_utils_treeview_resize (GtkWidget *tree_view, GtkAllocation *allocation,
				gpointer cell_renderer)
{
	gint last_width0, last_width1, width;

	/* Store the last two negotiated widths to stop oscillation when
	 * crossing the threshold for which scroll-bars are required.
	 */
	last_width0 = GPOINTER_TO_INT (
		g_object_get_data (G_OBJECT (tree_view), "last_width0"));
	last_width1 = GPOINTER_TO_INT (
		g_object_get_data (G_OBJECT (tree_view), "last_width1"));
	width = gtk_tree_view_column_get_width (
		gtk_tree_view_get_column (GTK_TREE_VIEW (tree_view), 0));

	if ((width != last_width0) && (width != last_width1)) {
		g_object_set_data (G_OBJECT (tree_view), "last_width1",
			GINT_TO_POINTER (last_width0));
		g_object_set_data (G_OBJECT (tree_view), "last_width0",
			GINT_TO_POINTER (width));

		g_object_set (G_OBJECT (cell_renderer), "cell_width",
			width, NULL);
		gtk_tree_view_columns_autosize (GTK_TREE_VIEW (tree_view));
	}
}
