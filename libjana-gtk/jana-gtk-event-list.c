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


#include <string.h>
#include <time.h>
#include <libjana/jana-utils.h>
#include "jana-gtk-event-list.h"
#include "jana-gtk-cell-renderer-event.h"

G_DEFINE_TYPE (JanaGtkEventList, jana_gtk_event_list, GTK_TYPE_TREE_VIEW)

#define EVENT_LIST_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), JANA_GTK_TYPE_EVENT_LIST, \
	JanaGtkEventListPrivate))

typedef struct _JanaGtkEventListPrivate JanaGtkEventListPrivate;

struct _JanaGtkEventListPrivate
{
	GtkListStore *model;
	GList *stores;
	GtkTreeViewColumn *column;
	GtkCellRenderer *event_renderer;
	GtkCellRenderer *text_renderer;
	gboolean show_headers;
};

enum {
	PROP_COLUMN = 1,
	PROP_EVENT_RENDERER,
	PROP_TEXT_RENDERER,
	PROP_SHOW_HEADERS,
};

static gint
jana_gtk_event_list_compare (GtkTreeModel *model, GtkTreeIter *a,
			     GtkTreeIter *b, gpointer user_data)
{
	GtkTreeRowReference *a_row, *b_row;
	GtkTreePath *path;
	GtkTreeModel *sub_model;
	GtkTreeIter iter;
	JanaTime *start1, *end1, *start2, *end2;
	gchar *summary1, *summary2;
	gint result = 0;
	
	gtk_tree_model_get (model, a,
		JANA_GTK_EVENT_LIST_COL_TIME, &start1,
		JANA_GTK_EVENT_LIST_COL_ROW, &a_row, -1);
	gtk_tree_model_get (model, b,
		JANA_GTK_EVENT_LIST_COL_TIME, &start2,
		JANA_GTK_EVENT_LIST_COL_ROW, &b_row, -1);

	if (a_row) {
		sub_model = gtk_tree_row_reference_get_model (a_row);
		path = gtk_tree_row_reference_get_path (a_row);
		gtk_tree_model_get_iter (sub_model, &iter, path);
		gtk_tree_path_free (path);

		gtk_tree_model_get (sub_model, &iter,
			JANA_GTK_EVENT_STORE_COL_START, &start1,
			JANA_GTK_EVENT_STORE_COL_END, &end1,
			JANA_GTK_EVENT_STORE_COL_SUMMARY, &summary1, -1);
	}
	
	if (b_row) {
		sub_model = gtk_tree_row_reference_get_model (b_row);
		path = gtk_tree_row_reference_get_path (b_row);
		gtk_tree_model_get_iter (sub_model, &iter, path);
		gtk_tree_path_free (path);

		gtk_tree_model_get (sub_model, &iter,
			JANA_GTK_EVENT_STORE_COL_START, &start2,
			JANA_GTK_EVENT_STORE_COL_END, &end2,
			JANA_GTK_EVENT_STORE_COL_SUMMARY, &summary2, -1);
	}
	
	result = jana_utils_time_compare (start1, start2, FALSE);
	if (result == 0) {
		if (a_row && b_row) {
			if (result == 0) result = jana_utils_time_compare (
				end2, end1, FALSE);
			if ((result == 0) && summary1 && summary2)
				result = strcmp (summary1, summary2);
		} else {
			/* Make sure headers sort before events */
			if (!a_row) result = -1;
			else if (!b_row) result = 1;
		}
	}
	
	g_object_unref (start1);
	if (a_row) {
		g_free (summary1);
		g_object_unref (end1);
	}
	
	g_object_unref (start2);
	if (b_row) {
		g_free (summary2);
		g_object_unref (end2);
	}
	
	return result;
}

static void
recalculate_headers (JanaGtkEventList *self)
{
	GtkTreeIter iter;
	JanaGtkEventListPrivate *priv = EVENT_LIST_PRIVATE (self);
	gboolean skip;
	JanaTime *day = NULL;
	GtkTreeModelFilter *filter;

	/* Remove all header rows, then iterate through and add unique days
	 * to table.
	 */
	if (!gtk_tree_model_get_iter_first ((GtkTreeModel *)priv->model, &iter))
		return;
	do {
		GtkTreeRowReference *row;
		gtk_tree_model_get ((GtkTreeModel *)priv->model, &iter,
			JANA_GTK_EVENT_LIST_COL_ROW, &row, -1);
		skip = FALSE;
		if (!row) {
			if (gtk_list_store_remove (priv->model, &iter))
				skip = TRUE;
			else
				break;
		}
	} while (skip || gtk_tree_model_iter_next (
		 (GtkTreeModel *)priv->model, &iter));
	
	if (!priv->show_headers) return;
	
	filter = (GtkTreeModelFilter *)gtk_tree_view_get_model (
		GTK_TREE_VIEW (self));
	if (!gtk_tree_model_get_iter_first ((GtkTreeModel *)filter, &iter))
		return;
	do {
		GtkTreeModel *model;
		GtkTreeRowReference *row;
		GtkTreePath *path;
		GtkTreeIter inner_iter;
		JanaTime *start;

		gtk_tree_model_get ((GtkTreeModel *)filter, &iter,
			JANA_GTK_EVENT_LIST_COL_ROW, &row, -1);
		
		if (!gtk_tree_row_reference_valid (row)) continue;
		
		model = gtk_tree_row_reference_get_model (row);
		path = gtk_tree_row_reference_get_path (row);
		gtk_tree_model_get_iter (model, &inner_iter, path);
		gtk_tree_path_free (path);
		
		gtk_tree_model_get (model, &inner_iter,
			JANA_GTK_EVENT_STORE_COL_START, &start, -1);
		
		if ((!day)||(jana_utils_time_compare (start, day, TRUE) > 0)) {
			JanaTime *day_copy;
			gchar *header;
			GtkTreeIter child_iter;

			if (day) g_object_unref (day);
			day = jana_time_duplicate (start);
			jana_time_set_isdate (day, TRUE);

			header = jana_utils_strftime (day, "%A %-d %B %Y");
			
			day_copy = jana_time_duplicate (day);
			gtk_tree_model_filter_convert_iter_to_child_iter (
				filter, &child_iter, &iter);
			gtk_list_store_insert_before (priv->model,
				&child_iter, &child_iter);
			gtk_list_store_set (priv->model, &child_iter,
				JANA_GTK_EVENT_LIST_COL_HEADER, header,
				JANA_GTK_EVENT_LIST_COL_TIME, day_copy,
				JANA_GTK_EVENT_LIST_COL_IS_HEADER, TRUE, -1);
			g_object_unref (day_copy);
			gtk_tree_model_filter_convert_child_iter_to_iter (
				filter, &iter, &child_iter);
			
			g_free (header);
		}
		
		g_object_unref (start);
	} while (gtk_tree_model_iter_next ((GtkTreeModel *)filter, &iter));

	if (day) g_object_unref (day);
}

static void
row_deleted_cb (GtkTreeModel *tree_model, GtkTreePath *path,
		JanaGtkEventList *self)
{
	GtkTreeIter iter;
	JanaGtkEventListPrivate *priv = EVENT_LIST_PRIVATE (self);
	gboolean skip = FALSE;
	
	if (!gtk_tree_model_get_iter_first ((GtkTreeModel *)priv->model, &iter))
		return;
	
	do {
		GtkTreeRowReference *row;
		GtkTreePath *stored_path;
		GtkTreeModel *model;
		skip = FALSE;
		
		gtk_tree_model_get ((GtkTreeModel *)priv->model, &iter,
			JANA_GTK_EVENT_LIST_COL_ROW, &row, -1);

		if (!row) continue;
		
		/* We check the path in case we're manually deleting rows,
		 * as is done when we remove a store from the list.
		 */
		model = gtk_tree_row_reference_get_model (row);
		stored_path = gtk_tree_row_reference_get_path (row);
		if ((!gtk_tree_row_reference_valid (row)) ||
		    ((model == tree_model) &&
		     (gtk_tree_path_compare (stored_path, path) == 0))) {
			if (gtk_list_store_remove (priv->model, &iter))
				skip = TRUE;
			gtk_tree_row_reference_free (row);
			if (!skip) break;
		}
	} while (skip || gtk_tree_model_iter_next (
		 (GtkTreeModel *)priv->model, &iter));

	recalculate_headers (self);
}

static void
row_changed_cb (GtkTreeModel *tree_model, GtkTreePath *path,
		GtkTreeIter *iter, JanaGtkEventList *self)
{
	JanaGtkEventListPrivate *priv = EVENT_LIST_PRIVATE (self);
	
	/* Re-sort */
	/* http://bugzilla.gnome.org/show_bug.cgi?id=316152 */
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (priv->model),
		JANA_GTK_EVENT_LIST_COL_ROW, jana_gtk_event_list_compare,
		self, NULL);

	/* Re-filter */
	/* Note, this isn't really necessary as recalculating headers will
	 * trigger a re-filter, but that may change...
	*/
	gtk_tree_model_filter_refilter ((GtkTreeModelFilter *)
		gtk_tree_view_get_model (GTK_TREE_VIEW (self)));
	
	recalculate_headers (self);
}

static void
row_inserted_cb (GtkTreeModel *tree_model, GtkTreePath *path,
		 GtkTreeIter *iter, JanaGtkEventList *self)
{
	GtkTreeRowReference *row;
	JanaGtkEventListPrivate *priv = EVENT_LIST_PRIVATE (self);
	
	row = gtk_tree_row_reference_new (tree_model, path);
	gtk_list_store_insert_with_values (priv->model, NULL, 0,
		JANA_GTK_EVENT_LIST_COL_ROW, row,
		JANA_GTK_EVENT_LIST_COL_IS_EVENT, TRUE,
		-1);
	
	recalculate_headers (self);
}

static void
size_allocate_cb (GtkWidget *self, GtkAllocation *allocation,
		  gpointer user_data)
{
	static gint last_width[] = { 0, 0 };
	gint width;

	JanaGtkEventListPrivate *priv = EVENT_LIST_PRIVATE (self);

	/* Store the last two negotiated widths to stop oscillation when
	 * crossing the threshold for which scroll-bars are required.
	 */
	width = gtk_tree_view_column_get_width (priv->column);
	if ((width != last_width[0]) && (width != last_width[1])) {
		last_width[1] = last_width[0];
		last_width[0] = width;
		g_object_set (G_OBJECT (priv->event_renderer), "cell_width",
			width, NULL);
		gtk_tree_view_columns_autosize (GTK_TREE_VIEW (self));
	}
}

static void
jana_gtk_event_list_get_property (GObject *object, guint property_id,
				  GValue *value, GParamSpec *pspec)
{
	JanaGtkEventListPrivate *priv = EVENT_LIST_PRIVATE (object);

	switch (property_id) {
	    case PROP_COLUMN :
		g_value_set_object (value, priv->column);
		break;
	    case PROP_EVENT_RENDERER :
		g_value_set_object (value, priv->event_renderer);
		break;
	    case PROP_TEXT_RENDERER :
		g_value_set_object (value, priv->text_renderer);
		break;
	    case PROP_SHOW_HEADERS :
		g_value_set_boolean (value, priv->show_headers);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_event_list_set_property (GObject *object, guint property_id,
				  const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	    case PROP_SHOW_HEADERS :
		jana_gtk_event_list_set_show_headers (JANA_GTK_EVENT_LIST (
			object), g_value_get_boolean (value));
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_event_list_dispose (GObject *self)
{
	JanaGtkEventListPrivate *priv = EVENT_LIST_PRIVATE (self);
	
	while (priv->stores) {
		GObject *object = (GObject *)priv->stores->data;

		g_signal_handlers_disconnect_by_func (
			object, row_inserted_cb, self);
		g_signal_handlers_disconnect_by_func (
			object, row_changed_cb, self);
		g_signal_handlers_disconnect_by_func (
			object, row_deleted_cb, self);
		g_object_unref (object);

		priv->stores = g_list_delete_link (priv->stores, priv->stores);
	}
	
	if (G_OBJECT_CLASS (jana_gtk_event_list_parent_class)->dispose)
		G_OBJECT_CLASS (jana_gtk_event_list_parent_class)->
			dispose (self);
}

static void
jana_gtk_event_list_finalize (GObject *object)
{
	/*JanaGtkEventListPrivate *priv = EVENT_LIST_PRIVATE (object);*/
	
	G_OBJECT_CLASS (jana_gtk_event_list_parent_class)->finalize (object);
}

static void
jana_gtk_event_list_class_init (JanaGtkEventListClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (JanaGtkEventListPrivate));

	object_class->get_property = jana_gtk_event_list_get_property;
	object_class->set_property = jana_gtk_event_list_set_property;
	object_class->dispose = jana_gtk_event_list_dispose;
	object_class->finalize = jana_gtk_event_list_finalize;

	g_object_class_install_property (
		object_class,
		PROP_COLUMN,
		g_param_spec_object (
			"column",
			"Column",
			"The column used to display events and headers.",
			GTK_TYPE_TREE_VIEW_COLUMN,
			G_PARAM_READABLE));

	g_object_class_install_property (
		object_class,
		PROP_EVENT_RENDERER,
		g_param_spec_object (
			"event-renderer",
			"Event renderer",
			"The cell renderer used to render event objects.",
			JANA_GTK_TYPE_CELL_RENDERER_EVENT,
			G_PARAM_READABLE));

	g_object_class_install_property (
		object_class,
		PROP_TEXT_RENDERER,
		g_param_spec_object (
			"text-renderer",
			"Text renderer",
			"The cell renderer used to render headers.",
			GTK_TYPE_CELL_RENDERER_TEXT,
			G_PARAM_READABLE));

	g_object_class_install_property (
		object_class,
		PROP_SHOW_HEADERS,
		g_param_spec_boolean (
			"show_headers",
			"Show headers",
			"Whether or not to display unique day headers.",
			TRUE,
			G_PARAM_READWRITE));
}

static void
style_set_cb (GtkWidget *widget, GtkStyle *previous_style, gpointer user_data)
{
	JanaGtkEventListPrivate *priv = EVENT_LIST_PRIVATE (widget);
	
	gtk_widget_realize (widget);
	g_object_set (G_OBJECT (priv->text_renderer), "background-gdk",
		&gtk_widget_get_style (widget)->text_aa[GTK_STATE_NORMAL],
		"foreground-gdk",
		&gtk_widget_get_style (widget)->base[GTK_STATE_NORMAL],
		"weight", 800, NULL);
}

static void
jana_gtk_event_list_init (JanaGtkEventList *self)
{
	GtkTreeModel *filter;
	JanaGtkEventListPrivate *priv = EVENT_LIST_PRIVATE (self);

	priv->show_headers = TRUE;
	priv->model = gtk_list_store_new (JANA_GTK_EVENT_LIST_COL_LAST,
		G_TYPE_POINTER,			/* ROW */
		G_TYPE_OBJECT,			/* TIME */
		G_TYPE_STRING,			/* HEADER */
		G_TYPE_BOOLEAN,			/* IS_EVENT */
		G_TYPE_BOOLEAN);		/* IS_HEADER */
	filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (priv->model), NULL);

	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (priv->model),
		JANA_GTK_EVENT_LIST_COL_ROW, jana_gtk_event_list_compare,
		self, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (priv->model),
		JANA_GTK_EVENT_LIST_COL_ROW, GTK_SORT_ASCENDING);
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (self), filter);
	g_object_unref (filter);
	
	priv->event_renderer = jana_gtk_cell_renderer_event_new ();
	priv->text_renderer = gtk_cell_renderer_text_new ();

	g_object_set (G_OBJECT (priv->event_renderer), "draw_time", TRUE,
		"draw_box", FALSE, NULL);
	g_object_set (G_OBJECT (priv->text_renderer), "weight", 800,
		"ellipsize", PANGO_ELLIPSIZE_END, NULL);
	
	priv->column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (priv->column, "Events");
	gtk_tree_view_column_pack_start (priv->column,
		priv->event_renderer, TRUE);
	gtk_tree_view_column_pack_end (priv->column, priv->text_renderer, TRUE);

	gtk_tree_view_column_add_attribute (priv->column, priv->event_renderer,
		"row", JANA_GTK_EVENT_LIST_COL_ROW);
	gtk_tree_view_column_add_attribute (priv->column, priv->event_renderer,
		"visible", JANA_GTK_EVENT_LIST_COL_IS_EVENT);
	gtk_tree_view_column_add_attribute (priv->column, priv->text_renderer,
		"text", JANA_GTK_EVENT_LIST_COL_HEADER);
	gtk_tree_view_column_add_attribute (priv->column, priv->text_renderer,
		"visible", JANA_GTK_EVENT_LIST_COL_IS_HEADER);
	gtk_tree_view_append_column (GTK_TREE_VIEW (self), priv->column);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self), FALSE);
	
	g_signal_connect (G_OBJECT (self), "size-allocate",
		G_CALLBACK (size_allocate_cb), NULL);
	g_signal_connect (G_OBJECT (self), "style-set",
		G_CALLBACK (style_set_cb), NULL);
}

GtkWidget*
jana_gtk_event_list_new (void)
{
	return GTK_WIDGET (g_object_new (JANA_GTK_TYPE_EVENT_LIST, NULL));
}

static void
insert_rows (JanaGtkEventList *self, JanaGtkEventStore *store)
{
	GtkTreeIter iter;
	
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter)) do {
		GtkTreePath *path = gtk_tree_model_get_path (
			(GtkTreeModel *)store, &iter);
		row_inserted_cb ((GtkTreeModel *)store, path, &iter, self);
		gtk_tree_path_free (path);
	} while (gtk_tree_model_iter_next ((GtkTreeModel *)store, &iter));
}

static void
delete_rows (JanaGtkEventList *self, JanaGtkEventStore *store)
{
	GtkTreeIter iter;
	
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter)) do {
		GtkTreePath *path = gtk_tree_model_get_path (
			(GtkTreeModel *)store, &iter);
		row_deleted_cb ((GtkTreeModel *)store, path, self);
		gtk_tree_path_free (path);
	} while (gtk_tree_model_iter_next ((GtkTreeModel *)store, &iter));
}

void
jana_gtk_event_list_add_store (JanaGtkEventList *self, JanaGtkEventStore *store)
{
	JanaGtkEventListPrivate *priv = EVENT_LIST_PRIVATE (self);
	
	priv->stores = g_list_prepend (priv->stores, g_object_ref (store));
	
	insert_rows (self, store);

	g_signal_connect (store, "row-inserted",
		G_CALLBACK (row_inserted_cb), self);
	g_signal_connect (store, "row-changed",
		G_CALLBACK (row_changed_cb), self);
	g_signal_connect (store, "row-deleted",
		G_CALLBACK (row_deleted_cb), self);
}

void
jana_gtk_event_list_remove_store (JanaGtkEventList *self,
				  JanaGtkEventStore *store)
{
	GList *link;
	JanaGtkEventListPrivate *priv = EVENT_LIST_PRIVATE (self);
	
	if (!(link = g_list_find (priv->stores, store))) return;
	
	g_signal_handlers_disconnect_by_func (store, row_inserted_cb, self);
	g_signal_handlers_disconnect_by_func (store, row_changed_cb, self);
	g_signal_handlers_disconnect_by_func (store, row_deleted_cb, self);
	
	delete_rows (self, store);
	g_object_unref (store);
	
	priv->stores = g_list_delete_link (priv->stores, link);
}

GtkTreeModelFilter *
jana_gtk_event_list_get_filter (JanaGtkEventList *self)
{
	return GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (
		GTK_TREE_VIEW (self)));
}

void
jana_gtk_event_list_refilter (JanaGtkEventList *self)
{
	gtk_tree_model_filter_refilter ((GtkTreeModelFilter *)
		gtk_tree_view_get_model (GTK_TREE_VIEW (self)));
	recalculate_headers (self);
}

/**
 * jana_gtk_event_list_set_show_headers:
 * @self: A #JanaGtkEventList
 * @show_headers: %TRUE to show date headers, %FALSE otherwise
 *
 * Sets whether to display unique day-header rows for each day at least one 
 * event falls on.
 */
void
jana_gtk_event_list_set_show_headers (JanaGtkEventList *self,
				      gboolean show_headers)
{
	JanaGtkEventListPrivate *priv = EVENT_LIST_PRIVATE (self);
	
	if (priv->show_headers != show_headers) {
		priv->show_headers = show_headers;
		recalculate_headers (self);
	}
}

/**
 * jana_gtk_event_list_get_show_headers:
 * @self: A #JanaGtkEventList
 *
 * Determines whether unique day headers are to be displayed. See 
 * jana_gtk_event_list_set_show_headers().
 *
 * Returns: %TRUE if day headers are being displayed, %FALSE otherwise.
 */
gboolean
jana_gtk_event_list_get_show_headers (JanaGtkEventList *self)
{
	JanaGtkEventListPrivate *priv = EVENT_LIST_PRIVATE (self);
	return priv->show_headers;
}
