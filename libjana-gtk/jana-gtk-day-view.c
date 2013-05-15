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
#include <locale.h>
#include <langinfo.h>
#include <libjana-gtk/jana-gtk-tree-layout.h>
#include <libjana/jana-utils.h>
#include "jana-gtk-day-view.h"

G_DEFINE_TYPE (JanaGtkDayView, jana_gtk_day_view, GTK_TYPE_EVENT_BOX)

#define DAY_VIEW_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), JANA_GTK_TYPE_DAY_VIEW, \
	JanaGtkDayViewPrivate))

typedef struct _JanaGtkDayViewPrivate JanaGtkDayViewPrivate;

struct _JanaGtkDayViewPrivate
{
	GtkWidget *vbox;
	
	GtkWidget *alignment;
	GtkWidget *viewport;
	GtkWidget *layout;
	
	GtkWidget *alignment24hr;
	GtkWidget *viewport24hr;
	GtkWidget *layout24hr;
	
	GtkCellRenderer *event_renderer;
	GtkCellRenderer *event_renderer24hr;
	GtkAllocation allocation;

	PangoLayout **time_layouts;
	PangoLayout **day_layouts;
	PangoLayout *week_layout;
	guint col0_width;
	guint row0_height;

	JanaDuration *range;
	gint visible_days;
	gint cell_minutes;

	JanaTime *day;
	JanaDuration *selection;
	GtkTreeRowReference *selected_event;
	guint cells;
	gchar *style_hint;
	guint spacing;
	gdouble xratio;
	gdouble yratio;
	JanaDuration *active_range;
	JanaTime *highlighted_time;
	
	gint selection_start_x;
	gint selection_start_y;
	gint selection_end_x;
	gint selection_end_y;
	
	gint active_range_start_x;
	gint active_range_start_y;
	gint active_range_end_x;
	gint active_range_end_y;
	
	gint highlighted_time_x;
	gint highlighted_time_y;
	gdouble highlighted_time_pos;
};

enum {
	PROP_RANGE = 1,
	PROP_SELECTION,
	PROP_SELECTED_EVENT,
	PROP_CELLS,
	PROP_SPACING,
	PROP_STYLE_HINT,
	PROP_RATIO_X,
	PROP_RATIO_Y,
	PROP_ACTIVE_RANGE,
	PROP_HIGHLIGHTED_TIME,
};

enum {
	SELECTION_CHANGED,
	EVENT_SELECTED,
	EVENT_ACTIVATED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void relayout (JanaGtkDayView *self);
static gboolean compare_selection (JanaGtkDayView *self,
				   GtkTreeRowReference *row);


static void
set_alignments (JanaGtkDayView *self)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	gtk_alignment_set_padding (GTK_ALIGNMENT (priv->alignment),
		0, 0, priv->col0_width, 0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (priv->alignment24hr),
		0, 0, priv->col0_width, 0);
}

static void
create_layouts (JanaGtkDayView *self)
{
	gint x, text_width, text_height;
	gchar week_string[4];
	JanaTime *time, *end;
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);
	
	if ((!jana_duration_valid (priv->range)) || (priv->cells == 0)) return;
	
	/* Work out how many minutes there are per cell */
	/* TODO: This is an extremely slow way of working
	 *       this out, but this isn't a speed critical
	 *       section of code. Resolve in the future.
	 */
	time = jana_time_duplicate (priv->range->start);
	end = jana_time_duplicate (time);
	jana_time_set_hours (end, jana_time_get_hours (priv->range->end));
	jana_time_set_minutes (end, jana_time_get_minutes (priv->range->end));
	if (jana_utils_time_compare (time, end, FALSE) >= 0)
		jana_time_set_day (end, jana_time_get_day (end) + 1);
	for (priv->cell_minutes = 0;
	     jana_utils_time_compare (time, end, FALSE) < 0;
	     jana_time_set_minutes (time, jana_time_get_minutes (time) +
				    priv->cells),
	     priv->cell_minutes ++) {
		if (jana_time_get_day (time) !=
		    jana_time_get_day (priv->range->start))
			break;
	}
	g_object_unref (end);
	g_object_unref (time);

	priv->row0_height = 0;
	priv->day_layouts = g_slice_alloc (
		sizeof (PangoLayout *) * priv->visible_days);
	time = jana_time_duplicate (priv->range->start);
	for (x = 0; x < priv->visible_days; x++) {
		GDateWeekday day;
		gchar day_markup[48];
		gboolean active;

		/* Draw the highlighted day in bold */
		if (priv->highlighted_time &&
		    (jana_utils_time_compare (
			time, priv->highlighted_time, TRUE) == 0))
			active = TRUE;
		else
			active = FALSE;
		
		day = jana_utils_time_day_of_week (time);
		snprintf (day_markup, 48, "%s%s\n<small>%d/%d</small>%s",
			active ? "<b>" : "",
			jana_utils_ab_day (day - 1),
			jana_time_get_day (time),
			jana_time_get_month (time),
			active ? "</b>" : "");
		
		priv->day_layouts[x] = gtk_widget_create_pango_layout (
			GTK_WIDGET (self), NULL);
		pango_layout_set_markup (priv->day_layouts[x], day_markup, -1);
		pango_layout_set_alignment (
			priv->day_layouts[x], PANGO_ALIGN_CENTER);
		
		pango_layout_get_pixel_size (
			priv->day_layouts[x], &text_width, &text_height);
		
		/* Set the height of the first row */
		priv->row0_height = MAX (priv->row0_height, text_height +
			(priv->spacing * 2));
		jana_time_set_day (time, jana_time_get_day (time) + 1);
	}
	g_object_unref (time);

	priv->col0_width = 0;
	priv->time_layouts = g_slice_alloc (
		sizeof (PangoLayout *) * priv->cells);
	time = jana_time_duplicate (priv->range->start);
	for (x = 0; x < priv->cells; x++) {
		gchar time_string[6];

		snprintf (time_string, 6, "%02d:%02d",
			jana_time_get_hours (time),
			jana_time_get_minutes (time));
		
		priv->time_layouts[x] = gtk_widget_create_pango_layout (
			GTK_WIDGET (self), time_string);
		
		pango_layout_get_pixel_size (
			priv->time_layouts[x], &text_width, &text_height);
		
		/* Set the width of the first column */
		priv->col0_width = MAX (priv->col0_width, text_width +
			(priv->spacing * 2));
		jana_time_set_minutes (time,
			jana_time_get_minutes (time) + priv->cell_minutes);
	}
	g_object_unref (time);

	snprintf (week_string, 4, "W%02d",
		jana_utils_time_week_of_year (priv->range->start, TRUE));
	priv->week_layout = gtk_widget_create_pango_layout (
		GTK_WIDGET (self), NULL);
	pango_layout_set_text (priv->week_layout, week_string, -1);
	pango_layout_get_pixel_size (priv->week_layout,
		&text_width, &text_height);
	
	/* Check the widths/heights, just in case */
	priv->col0_width = MAX (priv->col0_width, text_width);
	priv->row0_height = MAX (priv->row0_height, text_height);

	/* Set the alignments on the tree layouts */
	set_alignments (self);
}

static void
free_layouts (JanaGtkDayView *self)
{
	gint i;
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);
	
	if (priv->day_layouts) {
		for (i = 0; i < priv->visible_days; i++)
			g_object_unref (priv->day_layouts[i]);
		g_slice_free1 (sizeof (PangoLayout*) * priv->visible_days,
			priv->day_layouts);
		priv->day_layouts = NULL;
	}

	if (priv->time_layouts) {
		for (i = 0; i < priv->cells; i++)
			g_object_unref (priv->time_layouts[i]);
		g_slice_free1 (sizeof (PangoLayout*) * priv->cells,
			priv->time_layouts);
	}

	if (priv->week_layout) {
		g_object_unref (priv->week_layout);
		priv->week_layout = NULL;
	}
	
}

static void
time_to_cell_coords (JanaGtkDayView *self, JanaTime *time, gint *x, gint *y)
{
	JanaTime *counter;
	
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	counter = jana_time_duplicate (priv->range->start);
	jana_time_set_offset (counter, jana_time_get_offset (time));
	
	/* Find out what column the time is */
	if (x) {
		for (*x = 0; jana_utils_time_compare (counter, time, TRUE) < 0;
		     jana_time_set_day (counter,
			jana_time_get_day (counter) + 1)) (*x)++;
	}
	
	if (y) {
		for (*y = 0; jana_utils_time_compare (counter, time, FALSE) < 0;
		     jana_time_set_minutes (counter,
			jana_time_get_minutes (counter) +
			priv->cell_minutes)) (*y)++;
	}
	
	g_object_unref (counter);
}

static void
jana_gtk_day_view_get_property (GObject *object, guint property_id,
				GValue *value, GParamSpec *pspec)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (object);

	switch (property_id) {
	    case PROP_RANGE :
		g_value_set_boxed (value, priv->range);
		break;
	    case PROP_SPACING :
		g_value_set_uint (value, priv->spacing);
		break;
	    case PROP_CELLS :
		g_value_set_uint (value, priv->cells);
		break;
	    case PROP_STYLE_HINT :
		g_value_set_string (value, priv->style_hint);
		break;
	    case PROP_SELECTION :
		g_value_set_boxed (value, priv->selection);
		break;
	    case PROP_SELECTED_EVENT :
		g_value_set_boxed (value, priv->selected_event);
		break;
	    case PROP_RATIO_X :
		g_value_set_double (value, priv->xratio);
		break;
	    case PROP_RATIO_Y :
		g_value_set_double (value, priv->yratio);
		break;
	    case PROP_ACTIVE_RANGE :
		g_value_set_boxed (value, priv->active_range);
		break;
	    case PROP_HIGHLIGHTED_TIME :
		if (priv->highlighted_time)
			g_value_take_object (value,
				jana_time_duplicate (priv->highlighted_time));
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_day_view_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (object);

	switch (property_id) {
	    case PROP_RANGE :
		jana_gtk_day_view_set_range (JANA_GTK_DAY_VIEW (object),
			g_value_get_boxed (value));
		break;
	    case PROP_SPACING :
		jana_gtk_day_view_set_spacing (JANA_GTK_DAY_VIEW (object),
			g_value_get_uint (value));
		break;
	    case PROP_CELLS :
		jana_gtk_day_view_set_cells (JANA_GTK_DAY_VIEW (object),
			g_value_get_uint (value));
		break;
	    case PROP_STYLE_HINT :
		if (priv->style_hint) {
			g_free (priv->style_hint);
			priv->style_hint = NULL;
		}
		priv->style_hint = g_value_dup_string (value);
		gtk_widget_queue_draw (GTK_WIDGET (object));
		break;
	    case PROP_SELECTION :
		jana_gtk_day_view_set_selection (JANA_GTK_DAY_VIEW (object),
			g_value_get_boxed (value));
		break;
	    case PROP_SELECTED_EVENT :
		jana_gtk_day_view_set_selected_event (
			JANA_GTK_DAY_VIEW (object), g_value_get_boxed (value));
		break;
	    case PROP_RATIO_X :
		priv->xratio = g_value_get_double (value);
		gtk_widget_queue_resize (GTK_WIDGET (object));
		break;
	    case PROP_RATIO_Y :
		priv->yratio = g_value_get_double (value);
		gtk_widget_queue_resize (GTK_WIDGET (object));
		break;
	    case PROP_HIGHLIGHTED_TIME :
		jana_gtk_day_view_set_highlighted_time (
			JANA_GTK_DAY_VIEW (object), g_value_get_object (value));
		break;
	    case PROP_ACTIVE_RANGE :
		jana_gtk_day_view_set_active_range (
			JANA_GTK_DAY_VIEW (object), g_value_get_boxed (value));
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_day_view_dispose (GObject *object)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (object);

	if (priv->event_renderer) {
		g_object_unref (priv->event_renderer);
		priv->event_renderer = NULL;
	}
	
	if (priv->event_renderer24hr) {
		g_object_unref (priv->event_renderer24hr);
		priv->event_renderer24hr = NULL;
	}
	
	if (G_OBJECT_CLASS (jana_gtk_day_view_parent_class)->dispose)
		G_OBJECT_CLASS (jana_gtk_day_view_parent_class)->
			dispose (object);
}

static void
jana_gtk_day_view_finalize (GObject *object)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (object);

	if (priv->range) {
		jana_duration_free (priv->range);
		priv->range = NULL;
	}
	
	if (priv->selection) {
		jana_duration_free (priv->selection);
		priv->selection = NULL;
	}
	
	if (priv->style_hint) {
		g_free (priv->style_hint);
		priv->style_hint = NULL;
	}
	
	if (priv->highlighted_time) {
		g_object_unref (priv->highlighted_time);
		priv->highlighted_time = NULL;
	}
	
	if (priv->active_range) {
		jana_duration_free (priv->active_range);
		priv->active_range = NULL;
	}
	
	free_layouts (JANA_GTK_DAY_VIEW (object));

	G_OBJECT_CLASS (jana_gtk_day_view_parent_class)->finalize (object);
}

static void
paint_box (GtkStyle *style, GdkWindow *window, GtkStateType state_type,
	   GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget,
	   const gchar *detail, gint x, gint y, gint width, gint height)
{
	gtk_paint_flat_box (style, window, state_type, shadow_type, area,
		widget, detail, x, y, width, height);
	gtk_paint_shadow (style, window, state_type, shadow_type, area,
		widget, detail, x, y, width, height);
}

static gboolean
layout_expose_event_cb (GtkWidget *widget, GdkEventExpose *event,
			JanaGtkDayView *self)
{
	gint x, y, box_x, box_y, row_height, col_width, selection_direction;
	GtkStyle *style;

	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);
	
	if ((!jana_duration_valid (priv->range)) || (priv->cells == 0))
		return FALSE;
	
	/* Draw background */
	/* Make a style with base as bg */
	style = gtk_style_copy (widget->style);
	style->bg[GTK_STATE_NORMAL] = style->base[GTK_STATE_NORMAL];
	style->bg[GTK_STATE_SELECTED] = style->base[GTK_STATE_SELECTED];
	style = gtk_style_attach (style, widget->window);

	/* Draw event boxes */
	row_height = widget->allocation.height / priv->cells;
	col_width = widget->allocation.width / priv->visible_days;
	
	if ((priv->selection_start_x < priv->selection_end_x) ||
	    ((priv->selection_start_x == priv->selection_end_x) &&
	     (priv->selection_start_y < priv->selection_end_y)))
		selection_direction = 1;
	else
		selection_direction = 0;

	box_x = 0;
	for (x = 0; x < priv->visible_days; x++) {
		box_y = 0;
		
		for (y = 0; y < priv->cells; y++) {
			GtkStateType state = GTK_STATE_NORMAL;

			if (priv->active_range) {
				if ((x < priv->active_range_start_x) ||
				    (x >= priv->active_range_end_x) ||
				    (y < priv->active_range_start_y) ||
				    (y >= priv->active_range_end_y))
					state = GTK_STATE_INSENSITIVE;
			}
			
			if (priv->selection) {
				gint start_x, start_y, end_x, end_y;
				
				if (selection_direction) {
					start_x = priv->selection_start_x;
					start_y = priv->selection_start_y;
					end_x = priv->selection_end_x;
					end_y = priv->selection_end_y;
				} else {
					start_x = priv->selection_end_x;
					start_y = priv->selection_end_y - 1;
					end_x = priv->selection_start_x;
					end_y = priv->selection_start_y + 1;
				}
				
				/* These can be combined, but you end
				 * up with a horrible unreadable mess.
				 */
				if ((x > start_x) && (x < end_x))
					state = GTK_STATE_SELECTED;
				else
				if ((x == start_x) && (x < end_x) &&
				    (y >= start_y))
					state = GTK_STATE_SELECTED;
				else
				if ((x == end_x) && (x > start_x) &&
				    (y < end_y))
					state = GTK_STATE_SELECTED;
				else
				if ((x == start_x) && (x == end_x) &&
				    (y >= start_y) && (y < end_y))
					state = GTK_STATE_SELECTED;
			}

			paint_box (style, widget->window,
				state, GTK_SHADOW_IN,
				&event->area, widget, priv->style_hint,
				box_x, box_y, col_width, row_height);
			
			if (priv->highlighted_time &&
			    (x == priv->highlighted_time_x) &&
			    (y == priv->highlighted_time_y)) {
				/* Draw time line */
				gtk_paint_hline (style, widget->window,
					GTK_STATE_SELECTED,
					&event->area, widget,
					priv->style_hint, box_x,
					box_x + col_width,
					(gdouble)row_height *
					(gdouble)priv->cells *
					priv->highlighted_time_pos);
			}
			
			box_y += row_height;
		}
		box_x += col_width;
	}
	gtk_style_detach (style);

	return FALSE;
}

static gboolean
layout24hr_expose_event_cb (GtkWidget *widget, GdkEventExpose *event,
			JanaGtkDayView *self)
{
	gint x, box_x, col_width, text_width, text_height;
	GtkStyle *dark_style;

	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);
	
	if ((!jana_duration_valid (priv->range)) || (priv->cells == 0))
		return FALSE;
	
	col_width = priv->layout->allocation.width / priv->visible_days;

	/* Draw headers */
	/* Create darker style */
	dark_style = gtk_style_copy (widget->style);
	dark_style->bg[GTK_STATE_NORMAL] = dark_style->bg[GTK_STATE_ACTIVE];
	dark_style->fg[GTK_STATE_NORMAL] = dark_style->fg[GTK_STATE_ACTIVE];
	dark_style = gtk_style_attach (dark_style, widget->window);

	/* Week/day headers */
	for (x = 0, box_x = 0; x < priv->visible_days; x++) {
		PangoLayout *layout = priv->day_layouts[x];
		pango_layout_get_pixel_size (layout, &text_width, &text_height);

		paint_box (dark_style, widget->window,
			GTK_STATE_NORMAL, GTK_SHADOW_OUT,
			&event->area, widget, priv->style_hint,
			box_x, 0,
			col_width, priv->layout24hr->allocation.height);

		gtk_paint_layout (dark_style, widget->window,
			GTK_STATE_NORMAL, FALSE, &event->area,
			widget, priv->style_hint,
			box_x + (col_width / 2) - (text_width / 2),
			(priv->row0_height / 2) - (text_height / 2),
			layout);
		
		box_x += col_width;
	}
	gtk_style_detach (dark_style);
	
	return FALSE;
}

static gboolean
jana_gtk_day_view_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	gint y, box_y, row_height, col_width,
		row_offset, text_width, text_height;
	GtkAdjustment *adjustment;
	GtkStyle *dark_style;

	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (widget);
	
	if ((!jana_duration_valid (priv->range)) || (priv->cells == 0))
		return FALSE;
	
	row_height = priv->layout->allocation.height / priv->cells;
	col_width = priv->layout->allocation.width / priv->visible_days;

	/* Draw headers */
	/* Create darker style */
	dark_style = gtk_style_copy (widget->style);
	dark_style->bg[GTK_STATE_NORMAL] = dark_style->bg[GTK_STATE_ACTIVE];
	dark_style->fg[GTK_STATE_NORMAL] = dark_style->fg[GTK_STATE_ACTIVE];
	dark_style = gtk_style_attach (dark_style, widget->window);

	/* Calculate header offsets */
	adjustment = gtk_viewport_get_vadjustment (
		GTK_VIEWPORT (priv->viewport));
	row_offset = (gint)(((adjustment->value - adjustment->lower) /
		(adjustment->upper - adjustment->lower)) *
		(gdouble)priv->layout->allocation.height);

	/* Time headers */
	box_y = priv->layout24hr->allocation.height - row_offset;
	for (y = 0; y < priv->cells; y++) {
		PangoLayout *layout = priv->time_layouts[y];
		pango_layout_get_pixel_size (layout, &text_width, &text_height);

		paint_box (dark_style, widget->window,
			GTK_STATE_NORMAL, GTK_SHADOW_OUT,
			&event->area, widget, priv->style_hint,
			0, box_y,
			priv->col0_width, row_height);

		gtk_paint_layout (dark_style, widget->window,
			GTK_STATE_NORMAL, FALSE, &event->area,
			widget, priv->style_hint,
			(priv->col0_width / 2) - (text_width / 2),
			box_y + (row_height / 2) - (text_height / 2),
			layout);
		
		box_y += row_height;
	}
	
	/* Week */
	paint_box (widget->style, widget->window,
		GTK_STATE_NORMAL, GTK_SHADOW_OUT,
		&event->area, widget, priv->style_hint, 0, 0,
		priv->col0_width, priv->layout24hr->allocation.height);

	pango_layout_get_pixel_size (priv->week_layout,
		&text_width, &text_height);

	gtk_paint_layout (widget->style, widget->window,
		GTK_STATE_NORMAL, FALSE, &event->area,
		widget, priv->style_hint,
		(priv->col0_width / 2) - (text_width / 2),
		(priv->row0_height / 2) - (text_height / 2),
		priv->week_layout);

	gtk_style_detach (dark_style);
	
	return FALSE;
}

static void
relayout (JanaGtkDayView *self)
{
	JanaTime *time;
	GList *cell, *cells;
	gint cell_width, min_time, max_time, alloc_height, event_y, day,
		max_event_y;
	gfloat min_per_pixel;

	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);
	
	if ((!jana_duration_valid (priv->range)) || (priv->cells == 0))
		return;

	cell_width = priv->layout->allocation.width / priv->visible_days;
	min_time = (jana_time_get_hours (priv->range->start) * 60) +
		jana_time_get_minutes (priv->range->start);
	max_time = ((jana_time_get_hours (priv->range->end) ?
		     jana_time_get_hours (priv->range->end) : 24) * 60) +
		jana_time_get_minutes (priv->range->end);
	
	/* TODO: Use integer math for this */
	alloc_height = priv->layout->allocation.height -
		(priv->layout->allocation.height % priv->cells);
	min_per_pixel = (gfloat)alloc_height / (gfloat)(max_time - min_time);
	
	cells = jana_gtk_tree_layout_get_cells (
		JANA_GTK_TREE_LAYOUT (priv->layout));
	
	for (cell = g_list_last (cells); cell; cell = cell->prev) {
		JanaGtkTreeLayoutCellInfo *info =
			(JanaGtkTreeLayoutCellInfo *)cell->data;
		GtkTreeModel *model;
		GtkTreePath *path;
		GtkTreeIter iter;
		JanaTime *start, *end, *time;
		gint day;

		/* Work out what day the cell is in */
		model = gtk_tree_row_reference_get_model (info->row);
		path = gtk_tree_row_reference_get_path (info->row);
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_path_free (path);
	
		gtk_tree_model_get (model, &iter,
			JANA_GTK_EVENT_STORE_COL_START, &start,
			JANA_GTK_EVENT_STORE_COL_END, &end, -1);
		
		if (!start) {
			if (end) g_object_unref (end);
			continue;
		}
		if (!end) {
			end = jana_time_duplicate (start);
			jana_time_set_minutes (
				end, jana_time_get_minutes (end) + 30);
		}
		
		/* Get to the correct day */
		for (time = jana_time_duplicate (priv->range->start), day = 0;
		     jana_utils_time_compare (time, start, TRUE) < 0;
		     jana_time_set_day (time, jana_time_get_day (time) + 1),
		     day ++);
		if (jana_utils_time_compare (time, start, TRUE) == 0) {
			GList *prev_cell;
			JanaGtkTreeLayoutCellInfo *ovl_info = NULL;
			gint x, y, width, height, minutes;
			
			x = (cell_width * day) + priv->spacing;
			width = cell_width - (priv->spacing * 2);
			
			minutes = (jana_time_get_hours (start) * 60) +
				jana_time_get_minutes (start);
			minutes -= min_time;
			
			y = min_per_pixel * minutes;

			minutes = (((jana_time_get_hours (end) ||
				jana_time_get_minutes (end)) ?
				    jana_time_get_hours (end) : 24) * 60) +
				jana_time_get_minutes (end);
			minutes -= min_time;
			
			height = (min_per_pixel * minutes) - y;
			
			if (y < 0) {
				height += y;
				y = 0;
			}
			
			if ((width <= 0) || (height <= 0) ||
			    (y > alloc_height)) {
				/* Hide off-screen events */
				x = y = width = height = 0;
			} else if (cell->next) {
				/* Check if we've overlapped with any previous
				 * events in this day.
				 */
				for (prev_cell = cell->next; prev_cell;
				     prev_cell = prev_cell->next) {
					JanaGtkTreeLayoutCellInfo *prev_info =
						(JanaGtkTreeLayoutCellInfo *)
						prev_cell->data;
					     
					/* Check if info is on a previous day */
					if (prev_info->x < x) break;
					
					/* Check if event is hidden */
					if ((prev_info->width == 0) ||
					    (prev_info->height == 0))
						continue;
					     
					/* Check if we're overlapping */
					if (y < (prev_info->y +
					    prev_info->height)) {
						ovl_info = prev_info;
						break;
					}
				}
			}
			
			/* Resize on overlap */
			if (ovl_info) {
				/* 1/2 event width is enough room to squeeze
				 * it in without resizing the last event.
				 */
				if (ovl_info->x >= (x + width/2)) {
					width = ovl_info->x - x -
						priv->spacing;
				} else {
					/* Reduce the size of the last event
					 * and shuffle this event along
					 * slightly.
					 */
					width = (ovl_info->width * 2)/3;
					x = ovl_info->x + (ovl_info->width -
							    width);
					ovl_info->width = width;

					/* Resize overlapped event */
					jana_gtk_tree_layout_move_cell (
						JANA_GTK_TREE_LAYOUT (
							priv->layout),
						ovl_info->row, ovl_info->x,
						ovl_info->y, ovl_info->width,
						ovl_info->height);
				}
			}

			jana_gtk_tree_layout_move_cell (
				JANA_GTK_TREE_LAYOUT (priv->layout),
				info->row, x, y, width, height);
		} else {
			/* Hide out-of-range events */
			jana_gtk_tree_layout_move_cell (
				JANA_GTK_TREE_LAYOUT (priv->layout),
				info->row, 0, 0, 0, 0);
		}
		
		g_object_unref (time);
		g_object_unref (start);
		g_object_unref (end);
	}
	g_list_free (cells);

	/* Relayout the 24-hour events */
	cells = jana_gtk_tree_layout_get_cells (
		JANA_GTK_TREE_LAYOUT (priv->layout24hr));
	time = jana_time_duplicate (priv->range->start);
	day = 0;
	max_event_y = priv->row0_height;
	event_y = priv->spacing + priv->row0_height;
	for (cell = g_list_last (cells); cell; cell = cell->prev) {
		JanaGtkTreeLayoutCellInfo *info =
			(JanaGtkTreeLayoutCellInfo *)cell->data;
		GtkTreeModel *model;
		GtkTreePath *path;
		GtkTreeIter iter;
		JanaTime *start;
		gint height;
		GdkRectangle area;

		/* Work out what day the cell is in */
		model = gtk_tree_row_reference_get_model (info->row);
		path = gtk_tree_row_reference_get_path (info->row);
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_path_free (path);
	
		gtk_tree_model_get (model, &iter,
			JANA_GTK_EVENT_STORE_COL_START, &start, -1);
		
		if (!start) continue;
		
		if (!jana_utils_duration_contains (priv->range, start)) {
			/* Hide out-of-range events */
			jana_gtk_tree_layout_move_cell (
				JANA_GTK_TREE_LAYOUT (priv->layout24hr),
				info->row, 0, 0, 0, 0);
			g_object_unref (start);
			continue;
		}
		
		/* Get to the correct day */
		for (;jana_utils_time_compare (time, start, TRUE) < 0;
		     jana_time_set_day (time, jana_time_get_day (time) + 1),
		     day ++) {
			event_y = priv->spacing + priv->row0_height;
		}
		g_object_unref (start);
		
		/* Place event */
		area.x = (day * cell_width) + priv->spacing; area.y = 0;
		area.width = cell_width - (priv->spacing * 2);
		area.height = G_MAXINT;
		jana_gtk_tree_layout_move_cell (
			(JanaGtkTreeLayout *)priv->layout24hr,
			info->row, area.x, event_y, area.width, -1);
		g_object_set (G_OBJECT (priv->event_renderer24hr),
			"row", info->row, NULL);
		gtk_cell_renderer_get_size (priv->event_renderer24hr,
			priv->layout24hr, NULL, NULL, NULL, NULL, &height);
		event_y += height + priv->spacing;
		max_event_y = MAX (event_y, max_event_y);
	}
	
	gtk_widget_set_size_request (priv->alignment24hr,
		priv->layout->allocation.width + priv->col0_width, max_event_y);
	
	g_list_free (cells);
	g_object_unref (time);
}

static void
size_request_cb (GtkWidget *widget, GtkRequisition *requisition,
		 JanaGtkDayView *self)
{
	requisition->width = 0;
	requisition->height = 0;
}

static void
size_allocate_cb (GtkWidget *widget, GtkAllocation *allocation,
		  JanaGtkDayView *self)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	if ((allocation->width != priv->allocation.width) ||
	    (allocation->height != priv->allocation.height)) {
		priv->allocation.width = allocation->width;
		priv->allocation.height = allocation->height;
		relayout (self);
	}
}

/*static void
redraw_cell (JanaGtkDayView *self, gint x, gint y)
{
	GdkRectangle rect;
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);
	
	rect.width = priv->layout->allocation.width / priv->visible_days;
	rect.height = priv->layout->allocation.height / priv->cells;
	rect.x = x - (x % rect.width);
	rect.y = y - (y % rect.height);
	
	gdk_window_invalidate_rect (priv->layout->window, &rect, TRUE);
}*/

static void
start_selection (JanaGtkDayView *self, gint x, gint y)
{
	JanaTime *start, *end;
	gint alloc_height;

	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);
	
	alloc_height = priv->layout->allocation.height -
		(priv->layout->allocation.height % priv->cells);
	x = x + 1;
	y = MIN (y, alloc_height - 1);

	priv->selection_start_x = (x * priv->visible_days) /
		priv->layout->allocation.width;
	priv->selection_start_y = (y * priv->cells) / alloc_height;
	priv->selection_end_x = priv->selection_start_x;
	priv->selection_end_y = priv->selection_start_y + 1;
	
	start = jana_time_duplicate (priv->range->start);
	jana_time_set_day (start, jana_time_get_day (start) +
		priv->selection_start_x);
	jana_time_set_minutes (start, jana_time_get_minutes (start) +
		(priv->selection_start_y * priv->cell_minutes));
	end = jana_time_duplicate (start);
	jana_time_set_minutes (end, jana_time_get_minutes (end) +
		priv->cell_minutes);
	
	if (priv->selection) jana_duration_free (priv->selection);
	priv->selection = jana_duration_new (start, end);
	
	g_object_unref (start);
	g_object_unref (end);
}

static void
end_selection (JanaGtkDayView *self, gint x, gint y)
{
	static gint last_day_offset = 0, last_minute_offset = 0;
	gint alloc_height;
	
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);
	
	alloc_height = priv->layout->allocation.height -
		(priv->layout->allocation.height % priv->cells);
	x = MAX (MIN (x + 1, priv->layout->allocation.width), 0);
	y = MAX (MIN (y + 1, alloc_height), 0);

	/* Check against the last end offset so we don't change/redraw the
	 * selection unnecessarily.
	 */
	priv->selection_end_x = (x * priv->visible_days) /
		priv->layout->allocation.width;
	priv->selection_end_y = (((y * priv->cells) / alloc_height) + 1);
	if ((priv->selection_end_x == last_day_offset) &&
	    (priv->selection_end_y == last_minute_offset)) return;
	last_day_offset = priv->selection_end_x;
	last_minute_offset = priv->selection_end_y;
	
	jana_time_set_day (priv->selection->end,
		jana_time_get_day (priv->range->start) + priv->selection_end_x);
	jana_time_set_hours (priv->selection->end,
		jana_time_get_hours (priv->range->start));
	jana_time_set_minutes (priv->selection->end,
		jana_time_get_minutes (priv->range->start) +
		(priv->selection_end_y * priv->cell_minutes));
	
	if (jana_utils_time_compare (
	    priv->selection->end, priv->selection->start, FALSE) <= 0)
		jana_time_set_minutes (priv->selection->end,
			jana_time_get_minutes (priv->selection->end) -
			priv->cell_minutes);

	gtk_widget_queue_draw (priv->layout);
}

static gboolean
button_press_event_cb (GtkWidget *layout, GdkEventButton *event,
		       JanaGtkDayView *self)
{
	GList *selection;
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);
	
	if ((event->button != 1) || (!jana_duration_valid (priv->range)) ||
	    (priv->cells == 0)) return FALSE;
	
	if (priv->selection) {
		jana_duration_free (priv->selection);
		priv->selection = NULL;
		gtk_widget_queue_draw (GTK_WIDGET (self));
	}
	
	selection = jana_gtk_tree_layout_get_selection (
		JANA_GTK_TREE_LAYOUT (layout));
	
	if (!selection) {
		start_selection (self, event->x, event->y);
		gtk_widget_queue_draw (layout);
	} else {
		g_list_free (selection);
	}
	
	jana_gtk_tree_layout_set_selection (
		JANA_GTK_TREE_LAYOUT (priv->layout24hr), NULL);
	
	return FALSE;
}

static gboolean
button_release_event_cb (GtkWidget *layout, GdkEventButton *event,
			 JanaGtkDayView *self)
{
	gint result;
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	if ((event->button != 1) || (!priv->selection)) return FALSE;
	
	end_selection (self, event->x, event->y);

	/* Swap start/end if end is before start */
	result = jana_utils_time_compare (priv->selection->end,
		priv->selection->start, FALSE);
	if (result <= 0) {
		JanaTime *start;
		start = priv->selection->start;
		priv->selection->start = priv->selection->end;
		priv->selection->end = start;
		jana_time_set_minutes (priv->selection->end,
			jana_time_get_minutes (priv->selection->end) +
			priv->cell_minutes);
	}

	gtk_widget_queue_draw (layout);
	g_signal_emit (self, signals[SELECTION_CHANGED], 0, priv->selection);
	
	return FALSE;
}

static gboolean
motion_notify_event_cb (GtkWidget *layout, GdkEventMotion *event,
			JanaGtkDayView *self)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	if ((event->state & GDK_BUTTON1_MASK) && (priv->selection)) {
		end_selection (self, event->x, event->y);
	}
	
	/* Request more motion events */
#if GTK_CHECK_VERSION(2, 12, 0)
	gdk_event_request_motions (event);
#else
	gdk_window_get_pointer (layout->window, NULL, NULL, NULL);
#endif
	
	return FALSE;
}

static void
adjustment_changed_cb (GtkAdjustment *adjustment, JanaGtkDayView *self)
{
	GdkRectangle rect;
	JanaGtkDayViewPrivate *priv;
	
	if (!GTK_WIDGET (self)->window) return;
	priv = DAY_VIEW_PRIVATE (self);
	
	/* Invalidate the headers */
	rect.x = 0;
	rect.y = 0;
	rect.width = GTK_WIDGET (self)->allocation.width;
	rect.height = priv->row0_height;
	
	gdk_window_invalidate_rect (GTK_WIDGET (self)->window, &rect, FALSE);

	rect.width = priv->col0_width;
	rect.height = GTK_WIDGET (self)->allocation.height;

	gdk_window_invalidate_rect (GTK_WIDGET (self)->window, &rect, FALSE);
	gdk_window_process_updates (GTK_WIDGET (self)->window, FALSE);
}

static void
jana_gtk_day_view_set_scroll_adjustments (JanaGtkDayView *self,
					  GtkAdjustment *hadjustment,
					  GtkAdjustment *vadjustment)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);
	GtkAdjustment *adjustment;
	
	adjustment = gtk_viewport_get_hadjustment (
		GTK_VIEWPORT (priv->viewport));
	if (adjustment) {
		g_signal_handlers_disconnect_by_func (adjustment,
			adjustment_changed_cb, self);
	}
	adjustment = gtk_viewport_get_vadjustment (
		GTK_VIEWPORT (priv->viewport));
	if (adjustment) {
		g_signal_handlers_disconnect_by_func (adjustment,
			adjustment_changed_cb, self);
	}
	
	gtk_viewport_set_hadjustment (
		GTK_VIEWPORT (priv->viewport24hr), hadjustment);
	gtk_viewport_set_hadjustment (
		GTK_VIEWPORT (priv->viewport), hadjustment);
	gtk_viewport_set_vadjustment (
		GTK_VIEWPORT (priv->viewport), vadjustment);
	
	if (hadjustment) {
		g_signal_connect (hadjustment, "changed",
			G_CALLBACK (adjustment_changed_cb), self);
		g_signal_connect (hadjustment, "value-changed",
			G_CALLBACK (adjustment_changed_cb), self);
	}
	if (vadjustment) {
		g_signal_connect (vadjustment, "changed",
			G_CALLBACK (adjustment_changed_cb), self);
		g_signal_connect (vadjustment, "value-changed",
			G_CALLBACK (adjustment_changed_cb), self);
	}
}

/* Generated with glib-genmarshal and tidied */
static void
marshal_VOID__OBJECT_OBJECT (GClosure *closure, GValue *return_value,
			     guint n_param_values, const GValue *param_values,
			     gpointer invocation_hint, gpointer marshal_data)
{
	typedef void (*GMarshalFunc_VOID__OBJECT_OBJECT) (
		gpointer data1, gpointer arg_1, gpointer arg_2, gpointer data2);
	register GMarshalFunc_VOID__OBJECT_OBJECT callback;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;

	g_return_if_fail (n_param_values == 3);

	if (G_CCLOSURE_SWAP_DATA (closure)) {
		data1 = closure->data;
		data2 = g_value_peek_pointer (param_values + 0);
	} else {
		data1 = g_value_peek_pointer (param_values + 0);
		data2 = closure->data;
	}
	callback = (GMarshalFunc_VOID__OBJECT_OBJECT)
		(marshal_data ? marshal_data : cc->callback);

	callback (data1, g_value_get_object (param_values + 1),
		g_value_get_object (param_values + 2), data2);
}

static void
jana_gtk_day_view_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	GTK_WIDGET_CLASS (jana_gtk_day_view_parent_class)->
		size_request (widget, requisition);
	
	requisition->width = 0;
	requisition->height = 0;
}

static void
jana_gtk_day_view_class_init (JanaGtkDayViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	g_type_class_add_private (klass, sizeof (JanaGtkDayViewPrivate));

	object_class->get_property = jana_gtk_day_view_get_property;
	object_class->set_property = jana_gtk_day_view_set_property;
	object_class->dispose = jana_gtk_day_view_dispose;
	object_class->finalize = jana_gtk_day_view_finalize;
	
	widget_class->size_request = jana_gtk_day_view_size_request;
	widget_class->expose_event = jana_gtk_day_view_expose_event;
	widget_class->set_scroll_adjustments_signal =
		g_signal_new ("set_scroll_adjustments",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET (JanaGtkDayViewClass,
					 set_scroll_adjustments),
			NULL, NULL,
			marshal_VOID__OBJECT_OBJECT,
			G_TYPE_NONE, 2,
			GTK_TYPE_ADJUSTMENT,
			GTK_TYPE_ADJUSTMENT);
	
	klass->set_scroll_adjustments =
		jana_gtk_day_view_set_scroll_adjustments;
	
	g_object_class_install_property (
		object_class,
		PROP_RANGE,
		g_param_spec_boxed (
			"range",
			"Range",
			"The range shown by this day-view, as a JanaDuration.",
			JANA_TYPE_DURATION,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_CELLS,
		g_param_spec_uint (
			"cells",
			"Cells",
			"Amount of cells to partition time into, per day.",
			0, G_MAXUINT, 0,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_SPACING,
		g_param_spec_uint (
			"spacing",
			"Spacing",
			"Spacing to use inside day boxes.",
			0, G_MAXUINT, 2,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_STYLE_HINT,
		g_param_spec_string (
			"style_hint",
			"Style hint",
			"The style hint to use when drawing.",
			NULL,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_SELECTION,
		g_param_spec_boxed (
			"selection",
			"Selection",
			"The currently selected range, as a JanaDuration.",
			JANA_TYPE_DURATION,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_SELECTED_EVENT,
		g_param_spec_boxed (
			"selected_event",
			"Selected event",
			"A reference to the currently selected event in a "
			"JanaGtkEventStore.",
			GTK_TYPE_TREE_ROW_REFERENCE,
			G_PARAM_READWRITE));
	
	g_object_class_install_property (
		object_class,
		PROP_RATIO_X,
		g_param_spec_double (
			"xratio",
			"X-ratio",
			"The proportion of the calendar that should be "
			"visible, on the x-axis.",
			G_MINDOUBLE, 1.0, 1.0,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_RATIO_Y,
		g_param_spec_double (
			"yratio",
			"Y-ratio",
			"The proportion of the calendar that should be "
			"visible, on the y-axis.",
			G_MINDOUBLE, 1.0, 1.0,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_HIGHLIGHTED_TIME,
		g_param_spec_object (
			"highlighted_time",
			"Highlighted time",
			"A time that should be highlighted, "
			"for example, the current time.",
			G_TYPE_OBJECT,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_ACTIVE_RANGE,
		g_param_spec_boxed (
			"active_range",
			"Active range",
			"The range that should be considered 'active'.",
			JANA_TYPE_DURATION,
			G_PARAM_READWRITE));

	signals[SELECTION_CHANGED] =
		g_signal_new ("selection_changed",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaGtkDayViewClass,
					 selection_changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__BOXED,
			G_TYPE_NONE, 1, JANA_TYPE_DURATION);

	signals[EVENT_SELECTED] =
		g_signal_new ("event_selected",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaGtkDayViewClass,
					 event_selected),
			NULL, NULL,
			g_cclosure_marshal_VOID__BOXED,
			G_TYPE_NONE, 1, GTK_TYPE_TREE_ROW_REFERENCE);

	signals[EVENT_ACTIVATED] =
		g_signal_new ("event_activated",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaGtkDayViewClass,
					 event_activated),
			NULL, NULL,
			g_cclosure_marshal_VOID__BOXED,
			G_TYPE_NONE, 1, GTK_TYPE_TREE_ROW_REFERENCE);
}

static gint
sort_cells_cb (JanaGtkTreeLayoutCellInfo *info_a,
	       JanaGtkTreeLayoutCellInfo *info_b)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	JanaTime *start_a, *end_a, *start_b, *end_b;
	gchar *summary_a, *summary_b;
	gint result = 0;

	model = gtk_tree_row_reference_get_model (info_a->row);
	path = gtk_tree_row_reference_get_path (info_a->row);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);
	
	gtk_tree_model_get (model, &iter,
		JANA_GTK_EVENT_STORE_COL_START, &start_a,
		JANA_GTK_EVENT_STORE_COL_END, &end_a,
		JANA_GTK_EVENT_STORE_COL_SUMMARY, &summary_a, -1);

	model = gtk_tree_row_reference_get_model (info_b->row);
	path = gtk_tree_row_reference_get_path (info_b->row);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);
	
	gtk_tree_model_get (model, &iter,
		JANA_GTK_EVENT_STORE_COL_START, &start_b,
		JANA_GTK_EVENT_STORE_COL_END, &end_b,
		JANA_GTK_EVENT_STORE_COL_SUMMARY, &summary_b, -1);
	
	if (start_a && start_b)
		result = jana_utils_time_compare (start_b, start_a, FALSE);
	if (result == 0) {
		if (end_a && end_b)
			result = jana_utils_time_compare (end_a, end_b, FALSE);
		if ((result == 0) && summary_a && summary_b)
			result = g_utf8_collate (summary_b, summary_a);
	}
	
	g_free (summary_a);
	g_free (summary_b);
	if (start_a) g_object_unref (start_a);
	if (start_b) g_object_unref (start_b);
	if (end_a) g_object_unref (end_a);
	if (end_b) g_object_unref (end_b);
	
	return result;
}

static void
adjustment_changed_after_size_allocate_cb (GtkAdjustment *adjustment,
					   JanaGtkDayView *self)
{
	gdouble lower, upper, diff1, diff2;

	/* FIXME: Do this in a more sane way */
	
	/* It's ok to call this as the signals we want will already be in 
	 * the queue when we get here (it seems)
	 */
	g_signal_handlers_disconnect_by_func (adjustment,
		adjustment_changed_after_size_allocate_cb, self);

	lower = *((gdouble *)g_object_get_data (
		G_OBJECT (adjustment), "lower"));
	upper = *((gdouble *)g_object_get_data (
		G_OBJECT (adjustment), "upper"));
	
	/* If the adjustment really has changed, change the value to 
	 * reflect the new visible size
	 */
	diff1 = upper - lower;
	diff2 = adjustment->upper - adjustment->lower;
	if (diff1 != diff2) {
		gtk_adjustment_set_value (adjustment,
			adjustment->value * (diff2 / diff1));
	}
}

static void
viewport_size_allocate_cb (GtkWidget *widget, GtkAllocation *allocation,
			   JanaGtkDayView *self)
{
	static gdouble hlower, hupper, vlower, vupper;
	gint size_x, size_y;

	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);
	
	size_x = (gint)((gdouble)allocation->width / priv->xratio);
	size_y = (gint)((gdouble)allocation->height / priv->yratio);
	
	if ((priv->layout->allocation.width != size_x) ||
	    (priv->layout->allocation.height != size_y)) {
		GtkAdjustment *hadjustment, *vadjustment;
		hadjustment = gtk_viewport_get_hadjustment (
			GTK_VIEWPORT (widget));
		vadjustment = gtk_viewport_get_vadjustment (
			GTK_VIEWPORT (widget));
		
		hlower = hadjustment->lower;
		hupper = hadjustment->upper;
		vlower = vadjustment->lower;
		vupper = vadjustment->upper;

		g_object_set_data (G_OBJECT (hadjustment), "lower", &hlower);
		g_object_set_data (G_OBJECT (hadjustment), "upper", &hupper);
		g_object_set_data (G_OBJECT (vadjustment), "lower", &vlower);
		g_object_set_data (G_OBJECT (vadjustment), "upper", &vupper);

		g_signal_connect (hadjustment, "changed",
			G_CALLBACK (adjustment_changed_after_size_allocate_cb),
			self);
		g_signal_connect (vadjustment, "changed",
			G_CALLBACK (adjustment_changed_after_size_allocate_cb),
			self);
		
		gtk_widget_set_size_request (priv->layout, size_x, size_y);
		gtk_widget_set_size_request (priv->layout24hr, size_x, -1);
	}
}

static gboolean
compare_selection (JanaGtkDayView *self, GtkTreeRowReference *row)
{
	GtkTreePath *old_path, *path;
	gboolean result = TRUE;

	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	old_path = gtk_tree_row_reference_get_path (priv->selected_event);
	path = gtk_tree_row_reference_get_path (row);
	
	if ((gtk_tree_row_reference_get_model (row) !=
	    gtk_tree_row_reference_get_model (priv->selected_event)) ||
	    (gtk_tree_path_compare (path, old_path) != 0)) result = FALSE;
	
	gtk_tree_path_free (path);
	gtk_tree_path_free (old_path);
	
	return result;
}

static void
layout_selection_changed_cb (JanaGtkTreeLayout *layout,
			     JanaGtkDayView *self)
{
	GList *info_list;
	JanaGtkTreeLayoutCellInfo *info;
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	info_list = jana_gtk_tree_layout_get_selection (layout);
	
	if (!info_list) {
		if (priv->selected_event) {
			gtk_tree_row_reference_free (priv->selected_event);
			priv->selected_event = NULL;
			g_signal_emit (self, signals[EVENT_SELECTED], 0, NULL);
		}
		
		return;
	}
	
	info = (JanaGtkTreeLayoutCellInfo  *)info_list->data;
	if ((!priv->selected_event) || (!compare_selection (self, info->row))) {
		if (priv->selected_event)
			gtk_tree_row_reference_free (priv->selected_event);
		priv->selected_event = gtk_tree_row_reference_copy (info->row);
		g_signal_emit (self, signals[EVENT_SELECTED],
			0, priv->selected_event);
	}
	
	g_list_free (info_list);
}

static void
layout24hr_selection_changed_cb (JanaGtkTreeLayout *layout,
				 JanaGtkDayView *self)
{
	GList *info_list;
	JanaGtkTreeLayoutCellInfo *info;
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	info_list = jana_gtk_tree_layout_get_selection (layout);
	
	if (!info_list) {
		if (priv->selected_event) {
			gtk_tree_row_reference_free (priv->selected_event);
			priv->selected_event = NULL;
			g_signal_emit (self, signals[EVENT_SELECTED], 0, NULL);
		}
		jana_gtk_tree_layout_set_selection (
			JANA_GTK_TREE_LAYOUT (priv->layout), NULL);
		
		return;
	}

	/* Get rid of cell selection */
	if (priv->selection) {
		jana_duration_free (priv->selection);
		priv->selection = NULL;
		gtk_widget_queue_draw (priv->layout);
		g_signal_emit (self, signals[SELECTION_CHANGED], 0, NULL);
	}
	
	info = (JanaGtkTreeLayoutCellInfo  *)info_list->data;
	if ((!priv->selected_event) || (!compare_selection (self, info->row))) {
		if (priv->selected_event)
			gtk_tree_row_reference_free (priv->selected_event);
		priv->selected_event = NULL;

		/* Clear selection on other layout */
		jana_gtk_tree_layout_set_selection (
			JANA_GTK_TREE_LAYOUT (priv->layout), NULL);

		priv->selected_event = gtk_tree_row_reference_copy (info->row);
		g_signal_emit (self, signals[EVENT_SELECTED],
			0, priv->selected_event);
	}
	
	g_list_free (info_list);
}

static void
cell_activated_cb (JanaGtkTreeLayout *layout,
		   const JanaGtkTreeLayoutCellInfo *info,
		   JanaGtkDayView *self)
{
	g_signal_emit (self, signals[EVENT_ACTIVATED], 0, info->row);
}

static void
jana_gtk_day_view_init (JanaGtkDayView *self)
{
	static gboolean locale_set = FALSE;
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	if (!locale_set) {
		setlocale (LC_TIME, "");
		locale_set = TRUE;
	}

	priv->spacing = 2;
	priv->xratio = 1.0;
	priv->yratio = 1.0;
	
	priv->vbox = gtk_vbox_new (FALSE, 0);
	
	priv->alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
	priv->viewport = gtk_viewport_new (NULL, NULL);
	gtk_viewport_set_shadow_type (
		GTK_VIEWPORT (priv->viewport), GTK_SHADOW_NONE);

	priv->alignment24hr = gtk_alignment_new (0.5, 0.5, 1, 1);
	priv->viewport24hr = gtk_viewport_new (NULL, NULL);
	gtk_viewport_set_shadow_type (
		GTK_VIEWPORT (priv->viewport24hr), GTK_SHADOW_NONE);
	
	gtk_widget_set_app_paintable (GTK_WIDGET (self), TRUE);

	priv->layout = jana_gtk_tree_layout_new ();
	g_object_set (G_OBJECT (priv->layout),
		"sort_cb", sort_cells_cb,
		"select_mode", GTK_SELECTION_SINGLE,
		NULL);
	gtk_widget_add_events (priv->layout, GDK_POINTER_MOTION_HINT_MASK);

	priv->layout24hr = jana_gtk_tree_layout_new ();
	g_object_set (G_OBJECT (priv->layout24hr),
		"sort_cb", sort_cells_cb,
		"select_mode", GTK_SELECTION_SINGLE,
		NULL);
	
	gtk_container_add (GTK_CONTAINER (priv->viewport), priv->layout);
	gtk_container_add (GTK_CONTAINER (priv->alignment), priv->viewport);
	
	gtk_container_add (GTK_CONTAINER (priv->viewport24hr),
		priv->layout24hr);
	gtk_container_add (GTK_CONTAINER (priv->alignment24hr),
		priv->viewport24hr);
	
	gtk_box_pack_start (GTK_BOX (priv->vbox),
		priv->alignment24hr, FALSE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (priv->vbox),
		priv->alignment, TRUE, TRUE, 0);

	gtk_container_add (GTK_CONTAINER (self), priv->vbox);
	
	gtk_widget_show (priv->alignment);
	gtk_widget_show (priv->viewport);
	gtk_widget_show (priv->layout);

	gtk_widget_show (priv->alignment24hr);
	gtk_widget_show (priv->viewport24hr);
	gtk_widget_show (priv->layout24hr);

	gtk_widget_show (priv->vbox);
	
	gtk_widget_set_no_show_all (priv->vbox, TRUE);
	
	priv->event_renderer = g_object_ref_sink (
		jana_gtk_cell_renderer_event_new ());
	priv->event_renderer24hr = g_object_ref_sink (
		jana_gtk_cell_renderer_event_new ());
	g_object_set (G_OBJECT (priv->event_renderer24hr),
		"draw_detail", FALSE, NULL);
	
	/* Resizing events */
	g_signal_connect (priv->layout, "size-request",
		G_CALLBACK (size_request_cb), self);
	g_signal_connect (priv->layout24hr, "size-request",
		G_CALLBACK (size_request_cb), self);
	g_signal_connect (priv->layout, "size-allocate",
		G_CALLBACK (size_allocate_cb), self);

	g_signal_connect (priv->viewport, "size-allocate",
		G_CALLBACK (viewport_size_allocate_cb), self);
	
	/* Selection callbacks */
	g_signal_connect_after (priv->layout, "button-press-event",
		G_CALLBACK (button_press_event_cb), self);
	g_signal_connect (priv->layout, "button-release-event",
		G_CALLBACK (button_release_event_cb), self);
	g_signal_connect (priv->layout, "motion-notify-event",
		G_CALLBACK (motion_notify_event_cb), self);
	
	g_signal_connect (priv->layout, "selection_changed",
		G_CALLBACK (layout_selection_changed_cb), self);
	g_signal_connect (priv->layout24hr, "selection_changed",
		G_CALLBACK (layout24hr_selection_changed_cb), self);
	g_signal_connect (priv->layout24hr, "cell_activated",
		G_CALLBACK (cell_activated_cb), self);
	g_signal_connect (priv->layout, "cell_activated",
		G_CALLBACK (cell_activated_cb), self);
	
	/* Drawing callbacks */
	g_signal_connect (priv->layout, "expose-event",
		G_CALLBACK (layout_expose_event_cb), self);
	g_signal_connect (priv->layout24hr, "expose-event",
		G_CALLBACK (layout24hr_expose_event_cb), self);
}

/**
 * jana_gtk_day_view_new:
 * @range: The visible range on the day view
 * @cells: How many cells to use vertically
 *
 * Creates a new #JanaGtkDayView, which is a view of a single or multiple 
 * days on any amount of #JanaGtkEventStore objects. @range will be used to 
 * determine a rectangular viewing area, where the start of the duration is 
 * used to determine the earliest visible time each day, and the end of the 
 * duration is used to determine the latest visible time each day.
 *
 * Returns: A new #JanaGtkDayView.
 */
GtkWidget *
jana_gtk_day_view_new (JanaDuration *range, guint cells)
{
	return GTK_WIDGET (g_object_new (JANA_GTK_TYPE_DAY_VIEW,
		"range", range, "cells", cells, NULL));
}

static void
row_changed_cb (GtkTreeModel *tree_model, GtkTreePath *path,
		GtkTreeIter *iter, JanaGtkDayView *self)
{
	GtkTreeRowReference *row;
	JanaGtkTreeLayout *layout, *old_layout;
	gboolean first, last;
	JanaTime *start, *end;
	GtkCellRenderer *renderer;
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	gtk_tree_model_get (tree_model, iter,
		JANA_GTK_EVENT_STORE_COL_FIRST_INSTANCE, &first,
		JANA_GTK_EVENT_STORE_COL_LAST_INSTANCE, &last,
		JANA_GTK_EVENT_STORE_COL_START, &start,
		JANA_GTK_EVENT_STORE_COL_END, &end, -1);
	
	if (((!last) ||
	     ((end && jana_time_get_hours (end) == 0) &&
	      (jana_time_get_minutes (end) == 0) &&
	      (jana_time_get_seconds (end) == 0))) &&
	    ((!first) ||
	     ((start && jana_time_get_hours (start) == 0) &&
	      (jana_time_get_minutes (start) == 0) &&
	      (jana_time_get_seconds (start) == 0)))) {
		layout = JANA_GTK_TREE_LAYOUT (priv->layout24hr);
		old_layout = JANA_GTK_TREE_LAYOUT (priv->layout);
		renderer = priv->event_renderer24hr;
	} else {
		layout = JANA_GTK_TREE_LAYOUT (priv->layout);
		old_layout = JANA_GTK_TREE_LAYOUT (priv->layout24hr);
		renderer = priv->event_renderer;
	}
	
	if (start) g_object_unref (start);
	if (end) g_object_unref (end);
	
	row = gtk_tree_row_reference_new (tree_model, path);

	if (!jana_gtk_tree_layout_get_cell (layout, row)) {
		jana_gtk_tree_layout_remove_cell (old_layout, row);
		jana_gtk_tree_layout_add_cell (layout,
			row, 0, 0, 0, 0, renderer,
			"uid", JANA_GTK_EVENT_STORE_COL_UID,
			"categories", JANA_GTK_EVENT_STORE_COL_CATEGORIES,
			"summary", JANA_GTK_EVENT_STORE_COL_SUMMARY,
			"description", JANA_GTK_EVENT_STORE_COL_DESCRIPTION,
			"start", JANA_GTK_EVENT_STORE_COL_START,
			"end", JANA_GTK_EVENT_STORE_COL_END,
			"first_instance", JANA_GTK_EVENT_STORE_COL_FIRST_INSTANCE,
			"last_instance", JANA_GTK_EVENT_STORE_COL_LAST_INSTANCE,
			"has_recurrences", JANA_GTK_EVENT_STORE_COL_HAS_RECURRENCES,
			"has_alarm", JANA_GTK_EVENT_STORE_COL_HAS_ALARM,
			NULL);
	}
	
	gtk_tree_row_reference_free (row);
	relayout (self);
}

static void
row_inserted_cb (GtkTreeModel *tree_model, GtkTreePath *path,
		 GtkTreeIter *iter, JanaGtkDayView *self)
{
	JanaGtkTreeLayout *layout;
	gboolean first, last;
	JanaTime *start, *end;
	GtkTreeRowReference *row;
	GtkCellRenderer *renderer;
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	gtk_tree_model_get (tree_model, iter,
		JANA_GTK_EVENT_STORE_COL_FIRST_INSTANCE, &first,
		JANA_GTK_EVENT_STORE_COL_LAST_INSTANCE, &last,
		JANA_GTK_EVENT_STORE_COL_START, &start,
		JANA_GTK_EVENT_STORE_COL_END, &end, -1);
	
	if (((!last) ||
	     ((end && jana_time_get_hours (end) == 0) &&
	      (jana_time_get_minutes (end) == 0) &&
	      (jana_time_get_seconds (end) == 0))) &&
	    ((!first) ||
	     ((start && jana_time_get_hours (start) == 0) &&
	      (jana_time_get_minutes (start) == 0) &&
	      (jana_time_get_seconds (start) == 0)))) {
		layout = JANA_GTK_TREE_LAYOUT (priv->layout24hr);
		renderer = priv->event_renderer24hr;
	} else {
		layout = JANA_GTK_TREE_LAYOUT (priv->layout);
		renderer = priv->event_renderer;
	}
	
	if (start) g_object_unref (start);
	if (end) g_object_unref (end);
	
	/* Add row to layout */
	row = gtk_tree_row_reference_new (tree_model, path);
	
	jana_gtk_tree_layout_add_cell (layout,
		row, 0, 0, 0, 0, renderer,
		"uid", JANA_GTK_EVENT_STORE_COL_UID,
		"categories", JANA_GTK_EVENT_STORE_COL_CATEGORIES,
		"summary", JANA_GTK_EVENT_STORE_COL_SUMMARY,
		"location", JANA_GTK_EVENT_STORE_COL_LOCATION,
		"description", JANA_GTK_EVENT_STORE_COL_DESCRIPTION,
		"start", JANA_GTK_EVENT_STORE_COL_START,
		"end", JANA_GTK_EVENT_STORE_COL_END,
		"first_instance", JANA_GTK_EVENT_STORE_COL_FIRST_INSTANCE,
		"last_instance", JANA_GTK_EVENT_STORE_COL_LAST_INSTANCE,
		"has_recurrences", JANA_GTK_EVENT_STORE_COL_HAS_RECURRENCES,
		"has_alarm", JANA_GTK_EVENT_STORE_COL_HAS_ALARM,
		NULL);
	
	gtk_tree_row_reference_free (row);
	
	relayout (self);
}

static void
row_deleted_cb (GtkTreeModel *tree_model, GtkTreePath *path,
		JanaGtkDayView *self)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	if (priv->selected_event &&
	    (!gtk_tree_row_reference_valid (priv->selected_event))) {
		gtk_tree_row_reference_free (priv->selected_event);
		priv->selected_event = NULL;
		g_signal_emit (self, signals[EVENT_SELECTED], 0, NULL);
	}
	
	relayout (self);
}

static void
insert_rows (JanaGtkDayView *self, JanaGtkEventStore *store)
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
remove_rows (JanaGtkDayView *self, JanaGtkEventStore *store)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);
	GList *cell, *cells = jana_gtk_tree_layout_get_cells (
		JANA_GTK_TREE_LAYOUT (priv->layout));
	
	for (cell = cells; cell; cell = cell->next) {
		JanaGtkTreeLayoutCellInfo *info =
			(JanaGtkTreeLayoutCellInfo *)cell->data;
		if (gtk_tree_row_reference_get_model (info->row) ==
		    (GtkTreeModel *)store)
			jana_gtk_tree_layout_remove_cell (
				(JanaGtkTreeLayout *)self, info->row);
	}
	
	g_list_free (cells);
}

/**
 * jana_gtk_day_view_add_store:
 * @self: A #JanaGtkDayView
 * @store: The #JanaGtkEventStore
 *
 * Adds a #JanaGtkEventStore to the view. Added stores will be monitored and 
 * visualised events will be kept up-to-date.
 */
void
jana_gtk_day_view_add_store (JanaGtkDayView *self, JanaGtkEventStore *store)
{
	insert_rows (self, store);
	
	g_signal_connect (store, "row-inserted",
		G_CALLBACK (row_inserted_cb), self);
	g_signal_connect_after (store, "row-changed",
		G_CALLBACK (row_changed_cb), self);
	g_signal_connect_after (store, "row-deleted",
		G_CALLBACK (row_deleted_cb), self);
}

/**
 * jana_gtk_day_view_remove_store:
 * @self: A #JanaGtkDayView
 * @store: The #JanaGtkEventStore
 *
 * Removes a #JanaGtkEventStore from the view.
 */
void
jana_gtk_day_view_remove_store (JanaGtkDayView *self,
				  JanaGtkEventStore *store)
{
	g_signal_handlers_disconnect_by_func (store, row_inserted_cb, self);
	g_signal_handlers_disconnect_by_func (store, row_changed_cb, self);
	g_signal_handlers_disconnect_by_func (store, row_deleted_cb, self);
	
	remove_rows (self, store);
}

/**
 * jana_gtk_day_view_scroll_to_cell:
 * @self: A #JanaGtkDayView
 * @x: The cell's x coordinate
 * @y: The cell's y coordinate
 *
 * Sets adjustments on @self so that the cell at @x , @y is visible.
 */
void
jana_gtk_day_view_scroll_to_cell (JanaGtkDayView *self, guint x, guint y)
{
	GtkAdjustment *hadjustment, *vadjustment;

	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);
	
	hadjustment = gtk_viewport_get_hadjustment (
		GTK_VIEWPORT (priv->viewport));
	vadjustment = gtk_viewport_get_vadjustment (
		GTK_VIEWPORT (priv->viewport));
	
	x *= (priv->layout->allocation.width -
	      (priv->layout->allocation.width % priv->visible_days)) /
		priv->visible_days;
	y *= (priv->layout->allocation.height -
	      (priv->layout->allocation.width % priv->cells)) /
		priv->cells;
	
	hadjustment->value = MIN (x,
		hadjustment->upper - hadjustment->page_size);
	vadjustment->value = MIN (y + vadjustment->lower,
		vadjustment->upper - vadjustment->page_size);
	
	gtk_adjustment_value_changed (hadjustment);
	gtk_adjustment_value_changed (vadjustment);
}

/**
 * jana_gtk_day_view_scroll_to_time:
 * @self: A #JanaGtkDayView
 * @time: The #JanaTime to scroll to
 *
 * Sets adjustments on @self so that @time is visible, assuming that @time is
 * in the visible range.
 */
void
jana_gtk_day_view_scroll_to_time (JanaGtkDayView *self, JanaTime *time)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);
	gint x, hours, y;

	/* Check if the time is visible */
	if ((jana_utils_time_compare (time, priv->range->start, FALSE) < 0) ||
	    (jana_utils_time_compare (time, priv->range->end, FALSE) >= 0))
		return;
	
	if (jana_time_get_hours (time) <
	    jana_time_get_hours (priv->range->start)) return;
	
	if ((jana_time_get_hours (time) ==
	     jana_time_get_hours (priv->range->start)) &&
	    (jana_time_get_minutes (time) <
	     jana_time_get_minutes (priv->range->start)))
		return;

	hours = jana_time_get_hours (priv->range->end);
	if (hours == 0) hours = 24;
	if (jana_time_get_hours (time) > hours) return;
	
	if ((jana_time_get_hours (time) == hours) &&
	    (jana_time_get_minutes (time) >=
	     jana_time_get_minutes (priv->range->end)))
		return;
	
	time_to_cell_coords (self, time, &x, &y);
	
	jana_gtk_day_view_scroll_to_cell (self, x, y);
}

/**
 * jana_gtk_day_view_get_cell_renderer:
 * @self: A #JanaGtkDayView
 *
 * Retrieves the #JanaGtkCellRendererEvent used to draw standard events.
 *
 * Returns: the #JanaGtkCellRendererEvent used to draw standard events.
 */
JanaGtkCellRendererEvent *
jana_gtk_day_view_get_cell_renderer (JanaGtkDayView *self)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);
	
	return (JanaGtkCellRendererEvent *)priv->event_renderer;
}

/**
 * jana_gtk_day_view_get_24hr_cell_renderer:
 * @self: A #JanaGtkDayView
 *
 * Retrieves the #JanaGtkCellRendererEvent used to draw all-day events.
 *
 * Returns: the #JanaGtkCellRendererEvent used to draw all-day events.
 */
JanaGtkCellRendererEvent *
jana_gtk_day_view_get_24hr_cell_renderer (JanaGtkDayView *self)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);
	
	return (JanaGtkCellRendererEvent *)priv->event_renderer24hr;
}

static void
recalculate_cell_coords (JanaGtkDayView *self, gboolean active,
			 gboolean selection, gboolean highlight)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	if (priv->range && priv->cell_minutes) {
		if (priv->active_range) {
			time_to_cell_coords (self, priv->active_range->start,
				&priv->active_range_start_x,
				&priv->active_range_start_y);
			time_to_cell_coords (self, priv->active_range->end,
				&priv->active_range_end_x,
				&priv->active_range_end_y);
		}

		if (priv->selection) {
			time_to_cell_coords (self, priv->selection->start,
				&priv->selection_start_x,
				&priv->selection_start_y);
			time_to_cell_coords (self, priv->selection->end,
				&priv->selection_end_x,
				&priv->selection_end_y);
		}
		
		if (priv->highlighted_time) {
			gint days, mins, mins_in_day, alloc_height;
			/* Get how many minutes into the day the highlighted 
			 * time is, how many minutes there are per day visible, 
			 * and the height of the layout, so we can find out 
			 * where the time-line should occur.
			 */
			jana_utils_time_diff (priv->range->start,
				priv->highlighted_time, NULL, NULL, &days,
				NULL, &mins, NULL);
			jana_utils_time_diff (priv->range->start,
				priv->range->end, NULL, NULL, &days,
				NULL, &mins_in_day, NULL);
			if (!mins_in_day) mins_in_day = 24 * 60;
			alloc_height = priv->layout->allocation.height -
				(priv->layout->allocation.height % priv->cells);
			
			time_to_cell_coords (self, priv->highlighted_time,
				&priv->highlighted_time_x,
				&priv->highlighted_time_y);
			priv->highlighted_time_pos = (gdouble)mins /
				(gdouble)mins_in_day;
		}
	}
}

/**
 * jana_gtk_day_view_set_range:
 * @self: A #JanaGtkDayView
 * @range: The visible range
 *
 * Set the rectangular viewing area, where the start of @range is 
 * used to determine the earliest visible time each day, and the end 
 * is used to determine the latest visible time each day.
 */
void
jana_gtk_day_view_set_range (JanaGtkDayView *self, JanaDuration *range)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	free_layouts (self);

	if (priv->range) {
		jana_duration_free (priv->range);
		priv->range = NULL;
	}
	
	if (range) {
		JanaTime *time;
		
		priv->range = jana_duration_copy (range);
		jana_time_set_isdate (priv->range->start, FALSE);
		jana_time_set_isdate (priv->range->end, FALSE);
		
		/* Count visible days */
		for (priv->visible_days = 0,
		     time = jana_time_duplicate (priv->range->start);
		     jana_utils_time_compare (
			time, priv->range->end, TRUE) < 0;
		     jana_time_set_day (
			time, jana_time_get_day (time) + 1),
		     priv->visible_days ++);

		g_object_unref (time);
	}
	
	if (priv->selection) {
		jana_duration_free (priv->selection);
		priv->selection = NULL;
		g_signal_emit (self, signals[SELECTION_CHANGED], 0, NULL);
	}
	
	create_layouts (self);
	relayout (self);
	recalculate_cell_coords (self, TRUE, TRUE, TRUE);
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
 * jana_gtk_day_view_get_range:
 * @self: A #JanaGtkDayView
 *
 * Get the visible time-range, as a #JanaDuration. 
 * See jana_gtk_day_view_set_range().
 *
 * Returns: The visible range.
 */
JanaDuration *
jana_gtk_day_view_get_range (JanaGtkDayView *self)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	return priv->range ? jana_duration_copy (priv->range) : NULL;
}

/**
 * jana_gtk_day_view_get_selection:
 * @self: A #JanaGtkDayView
 *
 * Get the selected time-range, as a #JanaDuration. 
 * See jana_gtk_day_view_set_selection().
 *
 * Returns: The selected time-range, or %NULL if no range is selected.
 */
JanaDuration *
jana_gtk_day_view_get_selection (JanaGtkDayView *self)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	return priv->selection ? jana_duration_copy (priv->selection) : NULL;
}

/**
 * jana_gtk_day_view_set_selection:
 * @self: A #JanaGtkDayView
 * @selection: The time-range to select
 *
 * Sets the selected time range.
 */
void
jana_gtk_day_view_set_selection (JanaGtkDayView *self, JanaDuration *selection)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	if (priv->selection) {
		jana_duration_free (priv->selection);
		priv->selection = NULL;
	}
	if (selection) {
		priv->selection = jana_duration_copy (selection);
		jana_time_set_isdate (priv->selection->start, FALSE);
		jana_time_set_isdate (priv->selection->end, FALSE);
		recalculate_cell_coords (self, FALSE, TRUE, FALSE);
	}
	g_signal_emit (self, signals[SELECTION_CHANGED], 0, priv->selection);
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
 * jana_gtk_day_view_set_spacing:
 * @self: A #JanaGtkDayView
 * @spacing: Spacing, in pixels
 *
 * Sets the spacing used around events and text, in pixels.
 */
void
jana_gtk_day_view_set_spacing (JanaGtkDayView *self, guint spacing)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	priv->row0_height -= (priv->spacing * 2);
	priv->col0_width -= (priv->spacing * 2);
	priv->spacing = spacing;
	priv->row0_height += (priv->spacing * 2);
	priv->col0_width += (priv->spacing * 2);

	set_alignments (self);
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
 * jana_gtk_day_view_set_cells:
 * @self: A #JanaGtkDayView
 * @cells: Amount of cells to use, vertically.
 *
 * Sets the amount of cells to divide time up into, per day; i.e. the amount
 * of cells to display vertically.
 */
void
jana_gtk_day_view_set_cells (JanaGtkDayView *self, guint cells)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	priv->cells = cells;
	free_layouts (self);
	create_layouts (self);
	relayout (self);
	recalculate_cell_coords (self, TRUE, TRUE, TRUE);
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
 * jana_gtk_day_view_get_cells:
 * @self: A #JanaGtkDayView
 *
 * Gets the amount of cells used vertically.
 * See jana_gtk_day_view_set_cells().
 *
 * Returns: Amount of cells used vertically.
 */
guint
jana_gtk_day_view_get_cells (JanaGtkDayView *self)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	return priv->cells;
}

/**
 * jana_gtk_day_view_get_spacing:
 * @self: A #JanaGtkDayView
 *
 * Gets the spacing used around events and text.
 * See jana_gtk_day_view_set_spacing().
 *
 * Returns: Spacing used around events and text, in pixels.
 */
guint
jana_gtk_day_view_get_spacing (JanaGtkDayView *self)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	return priv->spacing;
}

/**
 * jana_gtk_day_view_set_visible_ratio:
 * @self: A #JanaGtkDayView
 * @xratio: The fraction of the set visible range that should be visible 
 * without scrolling, on the x-axis. 
 * @yratio: The fraction of the set visible range that should be visible 
 * without scrolling, on the y-axis.
 *
 * Sets the amount of the visible range that should be viewable at once,
 * without scrolling. This is represented as a fraction, from G_MINFLOAT to
 * 1.0, inclusive. If, for example, a #JanaGtkDayView was placed in a 
 * #GtkScrolledWindow, and the xratio and yratio were set to 1.0 and 0.5, 
 * respectively, a horizontal scroll-bar would not be required, but the 
 * #JanaGtkDayView would size itself so that only half of itself were visible 
 * and a vertical scroll-bar would be required. Ratios below 1.0 must not be 
 * set when the scrolling policy of the containing widget disallows scrolling.
 */
void
jana_gtk_day_view_set_visible_ratio (JanaGtkDayView *self,
				     gdouble xratio, gdouble yratio)
{
	g_object_set (G_OBJECT (self),
		"xratio", xratio, "yratio", yratio, NULL);
}

/**
 * jana_gtk_day_view_get_visible_ratio:
 * @self: A #JanaGtkDayView
 * @xratio: Return location for xratio, or %NULL
 * @yratio: Return location for yratio, or %NULL
 *
 * Retrieves the amount of the visible range that should be viewable at once,
 * on each axis. See jana_gtk_day_view_set_visible_ratio().
 */
void
jana_gtk_day_view_get_visible_ratio (JanaGtkDayView *self,
				     gdouble *xratio, gdouble *yratio)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	if (xratio) *xratio = priv->xratio;
	if (yratio) *yratio = priv->yratio;
}

/**
 * jana_gtk_day_view_get_selected_event:
 * @self: A #JanaGtkDayView
 *
 * Retrieves the selected event, returned as a #GtkTreeRowReference on a 
 * #JanaGtkEventStore. See jana_gtk_day_view_set_selected_event().
 *
 * Returns: the selected event, as a row reference on a #JanaGtkEventStore.
 */
GtkTreeRowReference *
jana_gtk_day_view_get_selected_event (JanaGtkDayView *self)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	return priv->selected_event ?
		gtk_tree_row_reference_copy (priv->selected_event) : NULL;
}

/**
 * jana_gtk_day_view_set_selected_event:
 * @self: A #JanaGtkDayView
 * @row: A #GtkTreeRowReference on a #JanaGtkEventStore
 *
 * Sets the selected event, if the specified @row is visible on @self.
 */
void
jana_gtk_day_view_set_selected_event (JanaGtkDayView *self,
				      GtkTreeRowReference *row)
{
	const JanaGtkTreeLayoutCellInfo *info;
	JanaGtkTreeLayout *layout;

	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	if (row && compare_selection (self, row)) return;
	
	if (row) {
		layout = JANA_GTK_TREE_LAYOUT (priv->layout);
		info = jana_gtk_tree_layout_get_cell (layout, row);
		if (!info) {
			layout = JANA_GTK_TREE_LAYOUT (priv->layout24hr);
			info = jana_gtk_tree_layout_get_cell (layout, row);
		}

		if (info) {
			GList *selection = g_list_prepend (
				NULL, (gpointer)info);
			jana_gtk_tree_layout_set_selection (
				layout, selection);
			g_list_free (selection);
			return;
		}
	}
	
	jana_gtk_tree_layout_set_selection (
		JANA_GTK_TREE_LAYOUT (priv->layout), NULL);
	jana_gtk_tree_layout_set_selection (
		JANA_GTK_TREE_LAYOUT (priv->layout24hr), NULL);
}

void
jana_gtk_day_view_set_visible_func (JanaGtkDayView *self,
				    GtkTreeModelFilterVisibleFunc visible_cb,
				    gpointer data)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);
	
	jana_gtk_tree_layout_set_visible_func (
		JANA_GTK_TREE_LAYOUT (priv->layout), visible_cb, data);
	jana_gtk_tree_layout_set_visible_func (
		JANA_GTK_TREE_LAYOUT (priv->layout24hr), visible_cb, data);
}

void
jana_gtk_day_view_refilter (JanaGtkDayView *self)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);
	
	jana_gtk_tree_layout_refilter (JANA_GTK_TREE_LAYOUT (priv->layout));
	jana_gtk_tree_layout_refilter (JANA_GTK_TREE_LAYOUT (priv->layout24hr));
}

void
jana_gtk_day_view_set_highlighted_time (JanaGtkDayView *self, JanaTime *time)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	if (priv->highlighted_time) {
		g_object_unref (priv->highlighted_time);
		priv->highlighted_time = NULL;
	}
	if (time) {
		priv->highlighted_time = jana_time_duplicate (time);
		if (priv->range) {
			recalculate_cell_coords (self, FALSE, FALSE, TRUE);
		}
	}

	free_layouts (self);
	create_layouts (self);
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
jana_gtk_day_view_set_active_range (JanaGtkDayView *self, JanaDuration *range)
{
	JanaGtkDayViewPrivate *priv = DAY_VIEW_PRIVATE (self);

	if (priv->active_range) {
		jana_duration_free (priv->active_range);
		priv->active_range = NULL;
	}
	if (range) {
		priv->active_range = jana_duration_copy (range);
		if (priv->range) {
			recalculate_cell_coords (self, TRUE, FALSE, FALSE);
		}
	}

	gtk_widget_queue_draw (priv->layout);
}
