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


#include "jana-gtk-tree-layout.h"

G_DEFINE_TYPE (JanaGtkTreeLayout, jana_gtk_tree_layout, GTK_TYPE_EVENT_BOX)

#define TREE_LAYOUT_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), JANA_GTK_TYPE_TREE_LAYOUT, \
	JanaGtkTreeLayoutPrivate))

typedef struct _JanaGtkTreeLayoutPrivate JanaGtkTreeLayoutPrivate;

struct _JanaGtkTreeLayoutPrivate {
	GCompareDataFunc sort_cb;
	gpointer sort_data;
	gboolean single_click;
	GtkSelectionMode select_mode;
	gboolean fill_width;
	gboolean fill_height;
	GtkTreeModelFilterVisibleFunc visible_cb;
	gpointer visible_data;

	GList *cells;
	GList *visible_cells;
	GList **cells_ptr;
	GHashTable *models;
	
	JanaGtkTreeLayoutCellInfo *hover;
	GList *select;
	guint select_idle;
};

enum {
	PROP_SORT_CB = 1,
	PROP_SORT_DATA,
	PROP_SINGLE_CLICK,
	PROP_SELECT_MODE,
	PROP_FILL_WIDTH,
	PROP_FILL_HEIGHT,
	PROP_VISIBLE_CB,
	PROP_VISIBLE_DATA,
};

enum {
	SELECTION_CHANGED,
	CELL_ACTIVATED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


static void	tree_layout_add_cell		(JanaGtkTreeLayout *self,
						 GtkTreeRowReference *row,
						 gint x, gint y,
						 gint width, gint height,
						 GtkCellRenderer *renderer,
						 va_list args);
static void	tree_layout_move_cell		(JanaGtkTreeLayout *self,
						 GtkTreeRowReference *row,
						 gint x, gint y,
						 gint width, gint height);
static void	tree_layout_remove_cell		(JanaGtkTreeLayout *self,
						 GtkTreeRowReference *row);
static void	tree_layout_clear		(JanaGtkTreeLayout *self);


static void	tree_layout_remove_cell_with_list (JanaGtkTreeLayout *self,
						   GList *info_list);

static gint
find_point_cb (gconstpointer a, gconstpointer b)
{
	JanaGtkTreeLayoutCellInfo *info = (JanaGtkTreeLayoutCellInfo *)a;
	GdkPoint *point = (GdkPoint *)b;
	
	if ((point->x < info->real_x) || (point->y < info->real_y)) return 1;
	else if ((point->x > info->real_x + info->real_width) ||
		 (point->y > info->real_y + info->real_height)) return -1;
	else return 0;
}

static gint
find_row_cb (gconstpointer a, gconstpointer b)
{
	gint result;
	GtkTreePath *path_a, *path_b;
	
	path_a = gtk_tree_row_reference_get_path (
		((JanaGtkTreeLayoutCellInfo *)a)->row);
	path_b = gtk_tree_row_reference_get_path ((GtkTreeRowReference *)b);
	
	result = gtk_tree_path_compare (path_a, path_b);
	
	gtk_tree_path_free (path_a);
	gtk_tree_path_free (path_b);
	
	return result;
}

static void
free_info (JanaGtkTreeLayoutCellInfo *info)
{
	g_object_unref (info->renderer);
	while (info->attributes) {
		/* Column ID */
		info->attributes = g_list_delete_link (info->attributes,
			info->attributes);
		
		/* Property name */
		g_free (info->attributes->data);
		info->attributes = g_list_delete_link (info->attributes,
			info->attributes);
	}
	g_slice_free (JanaGtkTreeLayoutCellInfo, info);
}

static void
tree_layout_row_changed_cb (GtkTreeModel *model, GtkTreePath *path,
			    GtkTreeIter *iter, JanaGtkTreeLayout *self)
{
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (self);
	GtkTreeRowReference *row;
	GList *info_list;
	
	row = gtk_tree_row_reference_new (model, path);
	info_list = g_list_find_custom (
		priv->cells, row, find_row_cb);
	gtk_tree_row_reference_free (row);
	
	if (info_list) {
		JanaGtkTreeLayoutCellInfo *info =
			(JanaGtkTreeLayoutCellInfo *)info_list->data;
		
		if (priv->sort_cb) {
			priv->cells = g_list_sort_with_data (priv->cells,
				priv->sort_cb, priv->sort_data);
		}
		
		gtk_widget_queue_draw_area (GTK_WIDGET (self),
			info->real_x + GTK_WIDGET (self)->allocation.x,
			info->real_y + GTK_WIDGET (self)->allocation.y,
			info->real_width, info->real_height);
		
		if (priv->visible_cb) {
			info_list = g_list_find (priv->visible_cells, info);
			if (priv->visible_cb (model, iter, priv->visible_data)){
				if (info_list) {
					if (priv->sort_cb) {
						priv->visible_cells =
							g_list_sort_with_data (
							    priv->visible_cells,
							    priv->sort_cb,
							    priv->sort_data);
					}
				} else {
					priv->visible_cells =
						g_list_insert_sorted_with_data (
							priv->visible_cells,
							info, priv->sort_cb,
							priv->sort_data);
				}
			} else if (info_list) {
				priv->visible_cells =
					g_list_delete_link (
						priv->visible_cells,
						info_list);
			}
		}
	}
}

static void
tree_layout_row_deleted_cb (GtkTreeModel *model, GtkTreePath *path,
			    JanaGtkTreeLayout *self)
{
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (self);
	GList *info_list = priv->cells;
	
	/* Iterate through cells and remove invalid - we can't search for
	 * a particular row as the row reference is now invalid :(
	 */
	while (info_list) {
		GList *prev_info = info_list;
		JanaGtkTreeLayoutCellInfo *info =
			(JanaGtkTreeLayoutCellInfo *)info_list->data;
		info_list = info_list->next;
		if (!gtk_tree_row_reference_valid (info->row)) {
			/* No need to queue a redraw, the widget will be
			 * redrawn due to a possible new size request
			 */
			tree_layout_remove_cell_with_list (
				self, prev_info);
		}
	}
}

static GList *
get_visible_cells (JanaGtkTreeLayout *self)
{
	GList *cell, *cells = NULL;
	
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (self);

	for (cell = priv->cells; cell; cell = cell->next) {
		JanaGtkTreeLayoutCellInfo *info;
		GtkTreeModel *model;
		GtkTreePath *path;
		GtkTreeIter iter;
		
		info = (JanaGtkTreeLayoutCellInfo *)cell->data;
		model = gtk_tree_row_reference_get_model (info->row);
		path = gtk_tree_row_reference_get_path (info->row);
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_path_free (path);
		
		if (priv->visible_cb (model, &iter, priv->visible_data))
			cells = g_list_append (cells, info);
	}
	
	return cells;
}

static void
jana_gtk_tree_layout_get_property (GObject *object, guint property_id,
				   GValue *value, GParamSpec *pspec)
{
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (object);
	
	switch (property_id) {
	    case PROP_SORT_CB :
		g_value_set_pointer (value, priv->sort_cb);
		break;
	    case PROP_SORT_DATA :
		g_value_set_pointer (value, priv->sort_data);
		break;
	    case PROP_SINGLE_CLICK :
		g_value_set_boolean (value, priv->single_click);
		break;
	    case PROP_SELECT_MODE :
		g_value_set_enum (value, priv->select_mode);
		break;
	    case PROP_FILL_WIDTH :
		g_value_set_boolean (value, priv->fill_width);
		break;
	    case PROP_FILL_HEIGHT :
		g_value_set_boolean (value, priv->fill_height);
		break;
	    case PROP_VISIBLE_CB :
		g_value_set_pointer (value, priv->visible_cb);
		break;
	    case PROP_VISIBLE_DATA :
		g_value_set_pointer (value, priv->visible_data);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_tree_layout_set_property (GObject *object, guint property_id,
				   const GValue *value, GParamSpec *pspec)
{
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (object);
	
	switch (property_id) {
	    case PROP_SORT_CB :
		priv->sort_cb = g_value_get_pointer (value);
		if (priv->sort_cb) {
			priv->cells = g_list_sort_with_data (priv->cells,
				priv->sort_cb, priv->sort_data);
			if (priv->visible_cb)
				priv->visible_cells = g_list_sort_with_data (
					priv->visible_cells, priv->sort_cb,
					priv->sort_data);
		}
		break;
	    case PROP_SORT_DATA :
		priv->sort_data = g_value_get_pointer (value);
		break;
	    case PROP_SINGLE_CLICK :
		priv->single_click = g_value_get_boolean (value);
		break;
	    case PROP_SELECT_MODE :
		priv->select_mode = g_value_get_enum (value);
		break;
	    case PROP_FILL_WIDTH :
		priv->fill_width = g_value_get_boolean (value);
		break;
	    case PROP_FILL_HEIGHT :
		priv->fill_height = g_value_get_boolean (value);
		break;
	    case PROP_VISIBLE_CB :
		jana_gtk_tree_layout_set_visible_func (
			JANA_GTK_TREE_LAYOUT (object),
			g_value_get_pointer (value), priv->visible_data);
		break;
	    case PROP_VISIBLE_DATA :
		priv->visible_data = g_value_get_pointer (value);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_tree_layout_dispose (GObject *object)
{
	jana_gtk_tree_layout_clear (JANA_GTK_TREE_LAYOUT (object));
	
	if (G_OBJECT_CLASS (jana_gtk_tree_layout_parent_class)->dispose)
		G_OBJECT_CLASS (jana_gtk_tree_layout_parent_class)->
			dispose (object);
}

static void
jana_gtk_tree_layout_finalize (GObject *object)
{
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (object);
	
	g_hash_table_destroy (priv->models);
	
	G_OBJECT_CLASS (jana_gtk_tree_layout_parent_class)->finalize (object);
}

static void
tree_layout_set_properties (JanaGtkTreeLayoutCellInfo *info)
{
	GList *attr;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	GValue value = { 0, };

	model = gtk_tree_row_reference_get_model (info->row);
	path = gtk_tree_row_reference_get_path (info->row);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);
		
	/* Set properties */
	for (attr = info->attributes; attr && attr->next;
	     attr = attr->next->next) {
		gtk_tree_model_get_value (model, &iter,
			GPOINTER_TO_INT (attr->data), &value);
		g_object_set_property (G_OBJECT (info->renderer),
			(gchar *)attr->next->data, &value);
		g_value_unset (&value);
	}
}

static gboolean
jana_gtk_tree_layout_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	GList *c;
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (widget);
	
	/* Draw these in the reverse order to how we handle mouse operations,
	 * that way cells that obscure other cells appear to have priority.
	 */
	for (c = g_list_last (*priv->cells_ptr); c; c = c->prev) {
		GdkRectangle cell_area;
		GtkCellRendererState state = 0;
		JanaGtkTreeLayoutCellInfo *info =
			(JanaGtkTreeLayoutCellInfo *)c->data;
		
		if ((info->real_width < 0) || (info->real_height < 0)) continue;
		
		tree_layout_set_properties (info);
		
		cell_area.x = info->real_x + widget->allocation.x;
		cell_area.y = info->real_y + widget->allocation.y;
		cell_area.width = info->real_width;
		cell_area.height = info->real_height;
		
		if (info == priv->hover) state |= GTK_CELL_RENDERER_PRELIT;
		if (g_list_find (priv->select, info))
			state |= GTK_CELL_RENDERER_SELECTED;
		if (!info->sensitive)
			state |= GTK_CELL_RENDERER_INSENSITIVE;
		
		gtk_cell_renderer_render (info->renderer, widget->window,
			widget, &cell_area, &cell_area, &event->area, state);
	}
	
	return FALSE;
}

static void
tree_layout_get_size (JanaGtkTreeLayout *self, gint *width, gint *height)
{
	GList *c;
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (self);

	if (width) *width = 0;
	if (height) *height = 0;

	for (c = *priv->cells_ptr; c; c = c->next) {
		gint size;
		JanaGtkTreeLayoutCellInfo *info =
			(JanaGtkTreeLayoutCellInfo *)c->data;
		
		if (width) {
			size = info->real_x + info->real_width;
			if (size > *width) *width = size;
		}
		if (height) {
			size = info->real_y + info->real_height;
			if (size > *height) *height = size;
		}
	}
}

static void
jana_gtk_tree_layout_size_request (GtkWidget *widget,
				   GtkRequisition *requisition)
{
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (widget);

	requisition->width = 0;
	requisition->height = 0;
	
	if (priv->fill_width && priv->fill_height) return;
	
	tree_layout_get_size (JANA_GTK_TREE_LAYOUT (widget),
		priv->fill_width ? NULL : &requisition->width,
		priv->fill_height ? NULL : &requisition->height);
}

static void
jana_gtk_tree_layout_size_allocate (GtkWidget *widget,
				    GtkAllocation *allocation)
{
	GList *c;
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (widget);
	
	GTK_WIDGET_CLASS (jana_gtk_tree_layout_parent_class)->size_allocate (
		widget, allocation);

	/* Work out real size of cells (necessary for width|height == -1) */
	for (c = *priv->cells_ptr; c; c = c->next) {
		gint x_offset, y_offset, old_width, old_height, width, height;
		GdkRectangle cell_area;
		JanaGtkTreeLayoutCellInfo *info =
			(JanaGtkTreeLayoutCellInfo *)c->data;
		
		if ((info->width >= 0) && (info->height >= 0))
			continue;
		if ((allocation->width == 0) || (allocation->height == 0))
			continue;

		/* Find real width/height */
		cell_area.x = info->real_x;
		cell_area.y = info->real_y;
		cell_area.width = (info->width < 0) ?
			(allocation->width - info->real_x) :
				info->real_width;
		cell_area.height = (info->height < 0) ?
			(allocation->height - info->real_y) :
				info->real_height;
		
		old_width = info->real_width;
		old_height = info->real_height;
		
		tree_layout_set_properties (info);

		gtk_cell_renderer_set_fixed_size (info->renderer,
			info->width, info->height);
		width = 0; height = 0; x_offset = 0; y_offset = 0;
		gtk_cell_renderer_get_size (info->renderer, widget,
			&cell_area,
			(info->width < 0) ? &x_offset : NULL,
			(info->height < 0) ? &y_offset : NULL,
			(info->width < 0) ? &width : NULL,
			(info->height < 0) ? &height : NULL);
		if (info->width < 0) {
			info->real_x += x_offset;
			info->real_width = width;
		}
		if (info->height < 0) {
			info->real_y += y_offset;
			info->real_height = height;
		}
		
		/* If we've changed since last size, resize the widget */
		if (((!priv->fill_width) && (info->real_width != old_width)) ||
		    ((!priv->fill_height) && (info->real_height != old_height)))
			gtk_widget_queue_resize (widget);
	}

	/* Work out scaled position (for fill-width|fill-height) */
	if (priv->fill_width || priv->fill_height) {
		gint width, height;
		gfloat scale_x = 1.f, scale_y = 1.f;

		tree_layout_get_size (JANA_GTK_TREE_LAYOUT (widget),
			&width, &height);
		scale_x = (gfloat)allocation->width / (gfloat)width;
		scale_y = (gfloat)allocation->height / (gfloat)height;
		
		for (c = *priv->cells_ptr; c; c = c->next) {
			JanaGtkTreeLayoutCellInfo *info =
				(JanaGtkTreeLayoutCellInfo *)c->data;
			if (priv->fill_width) {
				info->real_x = (gint)
					((gfloat)info->real_x*scale_x);
				info->real_width = (gint)((gfloat)
					info->real_width * scale_x);
			}
			if (priv->fill_height) {
				info->real_y = (gint)
					((gfloat)info->real_y * scale_y);
				info->real_height = (gint)((gfloat)
					info->real_height * scale_y);
			}
		}
	}
}

static gboolean
select_idle_cb (gpointer data)
{
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (data);

	g_signal_emit (data, signals[SELECTION_CHANGED], 0);

	priv->select_idle = 0;

	return FALSE;
}

static gboolean
jana_gtk_tree_layout_button_press_event (GtkWidget *widget,
					 GdkEventButton *event)
{
	GdkPoint point;
	GList *info_list;
	JanaGtkTreeLayoutCellInfo *info;
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (widget);

	if (priv->select_mode == GTK_SELECTION_NONE) {
		GdkPoint point;

		/* Activate the event if there is one, and we're in single
		 * click mode or this was a double-click.
		 */
		point.x = event->x;
		point.y = event->y;
		if ((priv->single_click || (event->type == GDK_2BUTTON_PRESS))
		    && (info_list = g_list_find_custom (
		     *priv->cells_ptr, &point, find_point_cb))) {
			gchar *path_string;
			GdkRectangle cell_area;
			info = (JanaGtkTreeLayoutCellInfo *)info_list->data;
			GtkTreePath *path;
			
			if (!info->sensitive) return FALSE;
			
			path = gtk_tree_row_reference_get_path (info->row);
			path_string = gtk_tree_path_to_string (path);
			gtk_tree_path_free (path);

			cell_area.x = info->real_x;
			cell_area.y = info->real_y;
			cell_area.width = info->real_width;
			cell_area.height = info->real_height;

			gtk_cell_renderer_activate (info->renderer,
				(GdkEvent *)event, widget, path_string,
				&cell_area, &cell_area,
				GTK_CELL_RENDERER_PRELIT);
			g_free (path_string);
			g_signal_emit (widget, signals[CELL_ACTIVATED],
				0, info);
		}
		
		return FALSE;
	}

	/* Find and select new cell(s), redraw */
	point.x = event->x;
	point.y = event->y;
	info_list = g_list_find_custom (*priv->cells_ptr, &point, find_point_cb);
	info = info_list ? (JanaGtkTreeLayoutCellInfo *)info_list->data : NULL;

	/* Deselect old cell(s) if necessary and queue a redraw */
	if (((priv->select_mode != GTK_SELECTION_BROWSE) &&
	     (!(event->state & GDK_CONTROL_MASK))) ||
	     ((priv->select_mode != GTK_SELECTION_MULTIPLE) &&
		   priv->select && info && (priv->select->data != info))) {
		for (info_list = priv->select; info_list;
		     info_list = info_list->next) {
			JanaGtkTreeLayoutCellInfo *old_info =
				(JanaGtkTreeLayoutCellInfo *)
					info_list->data;
			gtk_widget_queue_draw_area (widget,
				old_info->real_x + widget->allocation.x,
				old_info->real_y + widget->allocation.y,
				old_info->real_width,
				old_info->real_height);
		}
		g_list_free (priv->select);
		priv->select = NULL;
		if (!priv->select_idle) priv->select_idle = g_idle_add (
			select_idle_cb, widget);
	}

	/* Select new cell, activate if necessary and redraw */
	if (info) {
		if ((priv->select_mode != GTK_SELECTION_BROWSE) &&
		    (event->state & GDK_CONTROL_MASK)) {
			/* Deselect row if it's already selected */
			GList *selected = g_list_find (priv->select, info);
			if (selected) {
				priv->select = g_list_remove (
					priv->select, info);
				gtk_widget_queue_draw_area (widget,
					info->real_x + widget->allocation.x,
					info->real_y + widget->allocation.y,
					info->real_width, info->real_height);

				if (!priv->select_idle) priv->select_idle =
					g_idle_add (select_idle_cb, widget);
				return FALSE;
			}
		}
		
		/* Activate cell(s) */
		/* Activates on double-click, or if in single-click mode,
		 * only when there's a single event selected (otherwise
		 * selecting multiple events would become near-impossible)
		 */
		if ((event->type == GDK_2BUTTON_PRESS) ||
		    (priv->single_click && (!priv->select))) {
			for (info_list = priv->select; info_list;
			     info_list = info_list->next) {
				gchar *path_string;
				GdkRectangle cell_area;
				GtkCellRendererState state =
					GTK_CELL_RENDERER_SELECTED;
				JanaGtkTreeLayoutCellInfo *old_info =
					(JanaGtkTreeLayoutCellInfo *)
						info_list->data;
				GtkTreePath *path;
				
				if (!info->sensitive) continue;
				
				path = gtk_tree_row_reference_get_path (
					info->row);
				path_string = gtk_tree_path_to_string (path);
				gtk_tree_path_free (path);

				cell_area.x = info->real_x;
				cell_area.y = info->real_y;
				cell_area.width = info->real_width;
				cell_area.height = info->real_height;

				if (old_info == info) state |=
					GTK_CELL_RENDERER_PRELIT;
				gtk_cell_renderer_activate (old_info->renderer,
					(GdkEvent *)event, widget, path_string,
					&cell_area, &cell_area, state);
				g_free (path_string);
				g_signal_emit (widget, signals[CELL_ACTIVATED],
					0, old_info);
			}
		}
		
		/* Add new cell to selection */
		if (info->sensitive) {
			priv->select = g_list_prepend (priv->select, info);
			gtk_widget_queue_draw_area (widget,
				info->real_x + widget->allocation.x,
				info->real_y + widget->allocation.y,
				info->real_width, info->real_height);
			if (!priv->select_idle) priv->select_idle =
				g_idle_add (select_idle_cb, widget);
		}
	}
	
	return FALSE;
}

static gboolean
jana_gtk_tree_layout_motion_notify_event (GtkWidget *widget,
					  GdkEventMotion *event)
{
	GdkPoint point;
	GList *info_list;
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (widget);

	/* Find the cell we're hovered over and redraw it if necessary */
	point.x = event->x;
	point.y = event->y;
	if ((info_list = g_list_find_custom (
	     *priv->cells_ptr, &point, find_point_cb))) {
		JanaGtkTreeLayoutCellInfo *info =
			(JanaGtkTreeLayoutCellInfo *)info_list->data;
		if (priv->hover != info) {
			if (priv->hover)
				gtk_widget_queue_draw_area (
					widget,
					priv->hover->real_x +
						widget->allocation.x,
					priv->hover->real_y +
						widget->allocation.y,
					priv->hover->real_width,
					priv->hover->real_height);
			priv->hover = info;
			gtk_widget_queue_draw_area (widget,
				priv->hover->real_x + widget->allocation.x,
				priv->hover->real_y + widget->allocation.y,
				priv->hover->real_width,
				priv->hover->real_height);
		}
	} else if (priv->hover) {
		gtk_widget_queue_draw_area (widget,
			priv->hover->real_x + widget->allocation.x,
			priv->hover->real_y + widget->allocation.y,
			priv->hover->real_width,
			priv->hover->real_height);
		priv->hover = NULL;
		return FALSE;
	}
	
	return FALSE;
}

static void
jana_gtk_tree_layout_class_init (JanaGtkTreeLayoutClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	g_type_class_add_private (klass, sizeof (JanaGtkTreeLayoutPrivate));

	object_class->get_property = jana_gtk_tree_layout_get_property;
	object_class->set_property = jana_gtk_tree_layout_set_property;
	object_class->dispose = jana_gtk_tree_layout_dispose;
	object_class->finalize = jana_gtk_tree_layout_finalize;
	
	widget_class->expose_event = jana_gtk_tree_layout_expose_event;
	widget_class->size_request = jana_gtk_tree_layout_size_request;
	widget_class->size_allocate = jana_gtk_tree_layout_size_allocate;
	widget_class->button_press_event =
		jana_gtk_tree_layout_button_press_event;
	widget_class->motion_notify_event =
		jana_gtk_tree_layout_motion_notify_event;
	
	klass->add_cell = tree_layout_add_cell;
	klass->move_cell = tree_layout_move_cell;
	klass->remove_cell = tree_layout_remove_cell;
	klass->clear = tree_layout_clear;

	g_object_class_install_property (
		object_class,
		PROP_SORT_CB,
		g_param_spec_pointer (
			"sort_cb",
			"Sort callback",
			"A comparison function to sort the cells into drawing "
			"order.",
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_SORT_DATA,
		g_param_spec_pointer (
			"sort_data",
			"Sort data",
			"User data for cell sorting function.",
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_SINGLE_CLICK,
		g_param_spec_boolean (
			"single_click",
			"Single click",
			"Whether to activate cells on a single click.",
			FALSE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_SELECT_MODE,
		g_param_spec_enum (
			"select_mode",
			"Selection mode",
			"Selection mode.",
			GTK_TYPE_SELECTION_MODE,
			GTK_SELECTION_SINGLE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_FILL_WIDTH,
		g_param_spec_boolean (
			"fill_width",
			"Fill width",
			"Scale to fit available width.",
			FALSE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_FILL_HEIGHT,
		g_param_spec_boolean (
			"fill_height",
			"Fill height",
			"Scale to fit available height.",
			FALSE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_VISIBLE_CB,
		g_param_spec_pointer (
			"visible_cb",
			"Visible callback",
			"A filter function to determine which cells are "
			"visible.", G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_VISIBLE_DATA,
		g_param_spec_pointer (
			"visible_data",
			"gpointer",
			"User data for cell visibility filter function.",
			G_PARAM_READWRITE));

	signals[SELECTION_CHANGED] =
		g_signal_new ("selection_changed",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaGtkTreeLayoutClass,
					 selection_changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	signals[CELL_ACTIVATED] =
		g_signal_new ("cell_activated",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaGtkTreeLayoutClass,
					 cell_activated),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
tree_layout_style_set_cb (GtkWidget *widget, GtkStyle *previous_style,
			  gpointer user_data)
{
	gtk_widget_queue_resize (widget);
	gtk_widget_queue_draw (widget);
}

static void
jana_gtk_tree_layout_init (JanaGtkTreeLayout *self)
{
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (self);

	priv->models = g_hash_table_new (g_direct_hash, g_int_equal);
	priv->select_mode = GTK_SELECTION_SINGLE;
	priv->cells_ptr = &priv->cells;

	gtk_widget_set_app_paintable (GTK_WIDGET (self), TRUE);
	gtk_event_box_set_visible_window (GTK_EVENT_BOX (self), FALSE);

	gtk_widget_add_events (GTK_WIDGET (self), GDK_POINTER_MOTION_MASK |
		GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
		GDK_BUTTON1_MOTION_MASK | GDK_KEY_PRESS_MASK |
		GDK_KEY_RELEASE_MASK | GDK_SCROLL_MASK);
	
	g_signal_connect (G_OBJECT (self), "style-set",
		G_CALLBACK (tree_layout_style_set_cb), NULL);
}

static void
tree_layout_add_cell (JanaGtkTreeLayout *self,
		      GtkTreeRowReference *row, gint x, gint y, gint width,
		      gint height, GtkCellRenderer *renderer, va_list args)
{
	gint refs;
	const gchar *prop;
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	JanaGtkTreeLayoutCellInfo *info;
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (self);
	
	info = g_slice_new (JanaGtkTreeLayoutCellInfo);
	info->row = gtk_tree_row_reference_copy (row);
	info->x = x;
	info->y = y;
	info->width = width;
	info->height = height;
	info->real_x = x;
	info->real_y = y;
	info->real_width = width;
	info->real_height = height;
	info->sensitive = TRUE;
	info->renderer = g_object_ref (renderer);
	info->attributes = NULL;
	
	for (prop = va_arg (args, const gchar *); prop;
	     prop = va_arg (args, const gchar *)) {
		gint col_id = va_arg (args, gint);
		info->attributes = g_list_prepend (info->attributes,
			g_strdup (prop));
		info->attributes = g_list_prepend (info->attributes,
			GINT_TO_POINTER (col_id));
	}
	
	model = gtk_tree_row_reference_get_model (info->row);
	if ((refs = GPOINTER_TO_INT (g_hash_table_lookup (
	     priv->models, model)))) {
		/* Increment local ref-count */
		g_hash_table_replace (priv->models, model,
			GINT_TO_POINTER (refs + 1));
	} else {
		/* Attach to signals for row updates */
		g_signal_connect (G_OBJECT (model), "row-changed",
			G_CALLBACK (tree_layout_row_changed_cb), self);

		g_signal_connect (G_OBJECT (model), "row-deleted",
			G_CALLBACK (tree_layout_row_deleted_cb), self);

		g_hash_table_insert (priv->models,
			g_object_ref (model), GINT_TO_POINTER (1));
	}
	
	if (priv->sort_cb)
		priv->cells = g_list_insert_sorted_with_data (priv->cells, info,
			priv->sort_cb, priv->sort_data);
	else
		priv->cells = g_list_prepend (priv->cells, info);
	
	path = gtk_tree_row_reference_get_path (row);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);
	
	if (priv->visible_cb &&
	    priv->visible_cb (model, &iter, priv->visible_data)) {
		if (priv->sort_cb)
			priv->visible_cells = g_list_insert_sorted_with_data (
				priv->visible_cells, info, priv->sort_cb,
				priv->sort_data);
		else
			priv->visible_cells = g_list_prepend (
				priv->visible_cells, info);
	}
	
	gtk_widget_queue_resize (GTK_WIDGET (self));
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
tree_layout_move_cell (JanaGtkTreeLayout *self,
		      GtkTreeRowReference *row, gint x, gint y, gint width,
		      gint height)
{
	JanaGtkTreeLayoutCellInfo *info;
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (self);
	GList *info_list = g_list_find_custom (priv->cells, row, find_row_cb);
	
	if (!info_list) return;
	
	info = (JanaGtkTreeLayoutCellInfo *)info_list->data;
	info->x = x;
	info->y = y;
	info->width = width;
	info->height = height;
	info->real_x = x;
	info->real_y = y;
	info->real_width = width;
	info->real_height = height;

	if (priv->hover == info) priv->hover = NULL;

	if (priv->sort_cb)
		priv->cells = g_list_sort_with_data (priv->cells,
			priv->sort_cb, priv->sort_data);

	gtk_widget_queue_resize (GTK_WIDGET (self));
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
tree_layout_remove_cell_with_list (JanaGtkTreeLayout *self, GList *info_list)
{
	gint refs;
	GList *selected;
	GtkTreeModel *model;
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (self);
	JanaGtkTreeLayoutCellInfo *info =
		(JanaGtkTreeLayoutCellInfo *)info_list->data;

	if ((selected = g_list_find (priv->select, info))) {
		priv->select = g_list_delete_link (priv->select, selected);
		if (!priv->select_idle) priv->select_idle = g_idle_add (
			select_idle_cb, self);
	}
	if (priv->hover == info) priv->hover = NULL;
	
	model = gtk_tree_row_reference_get_model (info->row);
	if ((refs = GPOINTER_TO_INT (g_hash_table_lookup (
	     priv->models, model)))) {
		refs --;
		if (refs) {
			/* Decrement local ref-count */
			g_hash_table_replace (priv->models, model,
				GINT_TO_POINTER (refs));
		} else {
			/* Remove signals and unref model */
			g_signal_handlers_disconnect_by_func (model,
				tree_layout_row_changed_cb, self);
			g_signal_handlers_disconnect_by_func (model,
				tree_layout_row_deleted_cb, self);
			
			g_hash_table_remove (priv->models, model);
			g_object_unref (model);
		}
	}
	
	free_info ((JanaGtkTreeLayoutCellInfo *)info_list->data);
	priv->cells = g_list_delete_link (priv->cells, info_list);
	
	if (priv->visible_cb) {
		GList *link = g_list_find (
			priv->visible_cells, info);
		if (link)
			priv->visible_cells = g_list_delete_link (
				priv->visible_cells, link);
	}

	gtk_widget_queue_resize (GTK_WIDGET (self));
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
tree_layout_remove_cell (JanaGtkTreeLayout *self, GtkTreeRowReference *row)
{
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (self);
	GList *info_list = g_list_find_custom (priv->cells, row, find_row_cb);
	
	if (info_list) tree_layout_remove_cell_with_list (self, info_list);
}

static void
tree_layout_clear (JanaGtkTreeLayout *self)
{
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (self);
	
	while (priv->cells) tree_layout_remove_cell_with_list (
		self, priv->cells);
}

GtkWidget*
jana_gtk_tree_layout_new (void)
{
	return (GtkWidget *)g_object_new (JANA_GTK_TYPE_TREE_LAYOUT, NULL);
}

GList *
jana_gtk_tree_layout_get_selection (JanaGtkTreeLayout *self)
{
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (self);
	return g_list_copy (priv->select);
}

GList *
jana_gtk_tree_layout_get_cells (JanaGtkTreeLayout *self)
{
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (self);
	return g_list_copy (priv->cells);
}

void
jana_gtk_tree_layout_add_cell (JanaGtkTreeLayout *self,
			       GtkTreeRowReference *row,
			       gint x, gint y, gint width, gint height,
			       GtkCellRenderer *renderer, ...)
{
	va_list args;

	va_start (args, renderer);

	JANA_GTK_TREE_LAYOUT_GET_CLASS (self)->add_cell (self, row, x, y,
		width, height, renderer, args);

	va_end (args);
}

void
jana_gtk_tree_layout_move_cell (JanaGtkTreeLayout *self,
				GtkTreeRowReference *row, gint x, gint y,
				gint width, gint height)
{
	JANA_GTK_TREE_LAYOUT_GET_CLASS (self)->
		move_cell (self, row, x, y, width, height);
}

void
jana_gtk_tree_layout_remove_cell (JanaGtkTreeLayout *self,
				  GtkTreeRowReference *row)
{
	JANA_GTK_TREE_LAYOUT_GET_CLASS (self)->remove_cell (self, row);
}

void
jana_gtk_tree_layout_clear (JanaGtkTreeLayout *self)
{
	JANA_GTK_TREE_LAYOUT_GET_CLASS (self)->clear (self);
}

void
jana_gtk_tree_layout_set_selection (JanaGtkTreeLayout *self,
				    GList *selection)
{
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (self);

	g_list_free (priv->select);
	priv->select = g_list_copy (selection);
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

const JanaGtkTreeLayoutCellInfo *
jana_gtk_tree_layout_get_cell (JanaGtkTreeLayout *self,
			       GtkTreeRowReference *row)
{
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (self);
	GList *info_list = g_list_find_custom (priv->cells, row, find_row_cb);
	
	if (info_list)
		return (const JanaGtkTreeLayoutCellInfo *)info_list->data;
	else
		return NULL;
}

void
jana_gtk_tree_layout_set_cell_sensitive (JanaGtkTreeLayout *self,
					 GtkTreeRowReference *row,
					 gboolean sensitive)
{
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (self);
	GList *info_list = g_list_find_custom (priv->cells, row, find_row_cb);
	
	if (info_list) {
		JanaGtkTreeLayoutCellInfo *info;
		
		info = (JanaGtkTreeLayoutCellInfo *)info_list->data;
		if (info->sensitive != sensitive) {
			info->sensitive = sensitive;
			gtk_widget_queue_draw_area (GTK_WIDGET (self),
				info->real_x + GTK_WIDGET (self)->allocation.x,
				info->real_y + GTK_WIDGET (self)->allocation.y,
				info->real_width, info->real_height);
		}
	}
}

void
jana_gtk_tree_layout_set_visible_func (JanaGtkTreeLayout *self,
				       GtkTreeModelFilterVisibleFunc visible_cb,
				       gpointer data)
{
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (self);
	
	if (priv->visible_cb) g_list_free (priv->visible_cells);
	priv->visible_cb = visible_cb;
	priv->visible_data = data;
	
	if (priv->visible_cb) {
		priv->visible_cells = get_visible_cells (self);
		priv->cells_ptr = &priv->visible_cells;
	} else
		priv->cells_ptr = &priv->cells;
	
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
jana_gtk_tree_layout_refilter (JanaGtkTreeLayout *self)
{
	JanaGtkTreeLayoutPrivate *priv = TREE_LAYOUT_PRIVATE (self);
	
	if (priv->visible_cb) {
		g_list_free (priv->visible_cells);
		priv->visible_cells = get_visible_cells (self);
		gtk_widget_queue_draw (GTK_WIDGET (self));
	}
}
