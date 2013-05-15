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
#include "jana-gtk-month-view.h"

G_DEFINE_TYPE (JanaGtkMonthView, jana_gtk_month_view, GTK_TYPE_EVENT_BOX)

#define MONTH_VIEW_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), JANA_GTK_TYPE_MONTH_VIEW, \
	JanaGtkMonthViewPrivate))

typedef struct _JanaGtkMonthViewPrivate JanaGtkMonthViewPrivate;

struct _JanaGtkMonthViewPrivate
{
	GtkWidget *alignment;
	GtkWidget *layout;
	GtkCellRenderer *event_renderer;
	GtkAllocation allocation;

	gint visible_weeks;
	JanaTime *start;
	JanaTime *end;
	JanaTime *selection;

	JanaTime *month;
	gchar *style_hint;
	guint spacing;
	JanaTime *highlighted_time;
};

enum {
	PROP_MONTH = 1,
	PROP_SPACING,
	PROP_STYLE_HINT,
	PROP_SELECTION,
	PROP_HIGHLIGHTED_TIME,
};

enum {
	SELECTION_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void relayout (JanaGtkMonthView *self);


static void
jana_gtk_month_view_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (object);

	switch (property_id) {
	    case PROP_MONTH :
		if (priv->month)
			g_value_take_object (value,
				jana_time_duplicate (priv->month));
		break;
	    case PROP_SPACING :
		g_value_set_uint (value, priv->spacing);
		break;
	    case PROP_STYLE_HINT :
		g_value_set_string (value, priv->style_hint);
		break;
	    case PROP_SELECTION :
		g_value_take_object (value,
			priv->selection ?
				jana_time_duplicate (priv->selection) : NULL);
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
jana_gtk_month_view_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (object);

	switch (property_id) {
	    case PROP_MONTH :
		jana_gtk_month_view_set_month (JANA_GTK_MONTH_VIEW (object),
			g_value_get_object (value));
		break;
	    case PROP_SPACING :
		jana_gtk_month_view_set_spacing (
			JANA_GTK_MONTH_VIEW (object), g_value_get_uint (value));
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
		jana_gtk_month_view_set_selection (JANA_GTK_MONTH_VIEW (object),
			g_value_get_object (value));
		break;
	    case PROP_HIGHLIGHTED_TIME :
		jana_gtk_month_view_set_highlighted_time (
			JANA_GTK_MONTH_VIEW (object),
			g_value_get_object (value));
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_month_view_dispose (GObject *object)
{
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (object);

	if (priv->event_renderer) {
		g_object_unref (priv->event_renderer);
		priv->event_renderer = NULL;
	}
	
	if (G_OBJECT_CLASS (jana_gtk_month_view_parent_class)->dispose)
		G_OBJECT_CLASS (jana_gtk_month_view_parent_class)->
			dispose (object);
}

static void
jana_gtk_month_view_finalize (GObject *object)
{
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (object);

	if (priv->highlighted_time) {
		g_object_unref (priv->highlighted_time);
		priv->highlighted_time = NULL;
	}
	
	if (priv->start) {
		g_object_unref (priv->start);
		priv->start = NULL;
	}

	if (priv->end) {
		g_object_unref (priv->end);
		priv->end = NULL;
	}
	
	if (priv->month) {
		g_object_unref (priv->month);
		priv->month = NULL;
	}

	if (priv->selection) {
		g_object_unref (priv->selection);
		priv->selection = NULL;
	}
	
	if (priv->style_hint) {
		g_free (priv->style_hint);
		priv->style_hint = NULL;
	}

	G_OBJECT_CLASS (jana_gtk_month_view_parent_class)->finalize (object);
}

static gboolean
jana_gtk_month_view_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	JanaTime *time;
	gint week;
	gint x, y, box_y, row0_height, row_height = 0,
		col0_width, col_width = 0;
	GtkStyle *dark_style, *light_style, *style;
	PangoLayout *layout;
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (widget);
	
	if (!priv->month) return FALSE;
	
	/* Draw background */
	box_y = 0;
	row_height = widget->allocation.height / (priv->visible_weeks + 1);

	layout = gtk_widget_create_pango_layout (widget, NULL);

	dark_style = gtk_style_copy (widget->style);
	dark_style->bg[GTK_STATE_NORMAL] = dark_style->bg[GTK_STATE_ACTIVE];
	dark_style->fg[GTK_STATE_NORMAL] = dark_style->fg[GTK_STATE_ACTIVE];
	dark_style = gtk_style_attach (dark_style, widget->window);

	light_style = gtk_style_copy (widget->style);
	light_style->bg[GTK_STATE_NORMAL] = light_style->base[GTK_STATE_NORMAL];
	light_style->bg[GTK_STATE_SELECTED] =
		light_style->base[GTK_STATE_SELECTED];
	light_style = gtk_style_attach (light_style, widget->window);
	
	time = jana_time_duplicate (priv->start);

	for (y = 0; y < priv->visible_weeks + 1; y++) {
		gint box_x = 0, box_height = 0;
		week = ((gint)jana_utils_time_week_of_year (time, TRUE));
		for (x = 0; x < 8; x++) {
			gint box_width, text_width, text_height;
			GtkShadowType shadow = GTK_SHADOW_IN;
			GtkStateType state = GTK_STATE_NORMAL;
			style = light_style;
			
			if (y == 0) {
				if (x == 0) {
					guint top, left;
					
					/* Month label */
					pango_layout_set_text (layout,
						nl_langinfo (ABMON_1 +
							(jana_time_get_month (
							 priv->month) - 1)),
						-1);
					
					pango_layout_get_pixel_size (
						layout, &col0_width,
						&row0_height);
					
					/* The first column/row will fit to the
					 * text size (+ spacing)
					 */
					col0_width += priv->spacing * 2;
					row0_height += priv->spacing * 2;
					col_width = (widget->allocation.width -
						col0_width) / 7;
					row_height = (widget->allocation.height-
						row0_height) /
						priv->visible_weeks;
					
					/* Align the tree layout to the events
					 * area.
					 */
					gtk_alignment_get_padding (
						GTK_ALIGNMENT (priv->alignment),
						&top, NULL, &left, NULL);
					if ((top != row0_height) ||
					    (left != col0_width)) {
						gtk_alignment_set_padding (
							GTK_ALIGNMENT (priv->
								alignment),
							row0_height, 0,
							col0_width, 0);
					}
				} else {
					/* Day label */
					pango_layout_set_text (layout,
						jana_utils_ab_day (x - 1),
						-1);
				}
			} else if (x == 0) {
				gchar week_string[3];
				/* Week label */
				snprintf (week_string, 3, "%02d", week);
				pango_layout_set_text (layout, week_string, -1);
			} else {
				/* We're in a date box, so check if we're in
				 * the month (and if not, set insensitive),
				 * and increment day.
				 */
				if ((priv->selection) &&
				    (jana_utils_time_compare (time,
				     priv->selection, TRUE) == 0)) {
					state = GTK_STATE_SELECTED;
				} else if (jana_time_get_month (time) !=
					   jana_time_get_month (priv->month)) {
					state = GTK_STATE_INSENSITIVE;
				}

				jana_time_set_day (time,
					jana_time_get_day (time) + 1);
			}
			
			if ((x == 0) || (y == 0)) {
				shadow = GTK_SHADOW_OUT;
				style = dark_style;
			}
			
			if (x == 0) box_width = col0_width;
			else box_width = col_width;
			if (y == 0) box_height = row0_height;
			else box_height = row_height;
			
			gtk_paint_flat_box (style, widget->window,
				state, shadow,
				&event->area, widget, priv->style_hint,
				box_x, box_y,
				box_width, box_height);
			gtk_paint_shadow (style, widget->window,
				state, shadow,
				&event->area, widget, priv->style_hint,
				box_x, box_y,
				box_width, box_height);
			
			pango_layout_get_pixel_size (
				layout, &text_width, &text_height);
			
			if ((x == 0) || (y == 0))
				gtk_paint_layout (style, widget->window,
					state, FALSE, &event->area,
					widget, priv->style_hint,
					box_x + (box_width / 2) -
						  (text_width / 2),
					box_y + (box_height / 2) -
						  (text_height / 2),
					layout);
			
			box_x += box_width;
		}
		box_y += box_height;
	}
	gtk_style_detach (dark_style);
	
	GTK_WIDGET_CLASS (jana_gtk_month_view_parent_class)->
		expose_event (widget, event);
	
	/* Draw date labels */
	jana_utils_time_copy (priv->start, time);
	for (y = 0; y < priv->visible_weeks; y++) {
		for (x = 0; x < 7; x++) {
			gint text_width, text_height;
			GtkStateType state;

			/* Date label */
			if (priv->highlighted_time && (jana_utils_time_compare (
			    priv->highlighted_time, time, TRUE) == 0)) {
				gchar *markup = g_strdup_printf ("<b>%d</b>",
					jana_time_get_day (time));
				pango_layout_set_markup (layout, markup, -1);
				g_free (markup);
			} else {
				gchar date_string[3];
				snprintf (date_string, 3, "%d",
					jana_time_get_day (time));
				pango_layout_set_markup (
					layout, date_string, -1);
			}
			
			if ((priv->selection) && (jana_utils_time_compare (
			    priv->selection, time, TRUE) == 0)) {
				state = GTK_STATE_SELECTED;
			} else if (jana_time_get_month (time) !=
			    jana_time_get_month (priv->month))
				state = GTK_STATE_INSENSITIVE;
			else
				state = GTK_STATE_NORMAL;

			pango_layout_get_pixel_size (
				layout, &text_width, &text_height);
			
			gtk_paint_layout (light_style, widget->window,
				state, TRUE, &event->area,
				widget, priv->style_hint,
				(x * col_width) + (col_width / 2) +
					col0_width - (text_width / 2),
				(y * row_height) + row0_height +
					(row_height / 2) - (text_height / 2),
				layout);
			
			jana_time_set_day (time,
				jana_time_get_day (time) + 1);
		}
	}
	gtk_style_detach (light_style);

	g_object_unref (layout);
	g_object_unref (time);
	
	return FALSE;
}

static void
relayout (JanaGtkMonthView *self)
{
	gint x, y, width, height;
	JanaTime *time;
	GList *cell, *cells;
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (self);

	if (!priv->month) return;
	
	cells = jana_gtk_tree_layout_get_cells (
		JANA_GTK_TREE_LAYOUT (priv->layout));
	
	if (!cells) return;
	
	/* Place cells in their day boxes, 4 events per box */
	width = priv->layout->allocation.width / 7;
	height = priv->layout->allocation.height / priv->visible_weeks;
	cell = cells;
	time = jana_time_duplicate (priv->start);
	for (y = 0; (y < priv->visible_weeks) && (cell); y++) {
		for (x = 0; (x < 7) && (cell); x++) {
			gint events = 0;
			do {
				JanaGtkTreeLayoutCellInfo *info =
					(JanaGtkTreeLayoutCellInfo *)cell->data;
				GtkTreeModel *model;
				GtkTreePath *path;
				GtkTreeIter iter;
				JanaTime *start;

				/* Work out what month the cell is in */
				model = gtk_tree_row_reference_get_model (info->row);
				path = gtk_tree_row_reference_get_path (info->row);
				gtk_tree_model_get_iter (model, &iter, path);
				gtk_tree_path_free (path);
			
				gtk_tree_model_get (model, &iter,
					JANA_GTK_EVENT_STORE_COL_START, &start,
					-1);

				if (start && (jana_utils_time_compare (
				    start, time, TRUE) == 0)) {
					gint event_x, event_y, event_width,
						event_height;

					event_width = (width/2) -
						(priv->spacing * 1.5);
					event_height = (height/2) -
						(priv->spacing * 1.5);
					switch (events) {
					    case 0 :
						/* Upper-left quadrant */
						event_x = (x * width) +
							priv->spacing;
						event_y = (y * height) +
							priv->spacing;
						break;
					    case 1 :
						/* Upper-right quadrant */
						event_x = (x * width) +
							(width/2) +
							(priv->spacing / 2);
						event_y = (y * height) +
							priv->spacing;
						break;
					    case 2 :
						/* Lower-left quadrant */
						event_x = (x * width) +
							priv->spacing;
						event_y = (y * height) +
							(height / 2) +
							(priv->spacing / 2);
						break;
					    case 3 :
					    default :
						/* Lower-right quadrant */
						event_x = (x * width) +
							(width/2) +
							(priv->spacing / 2);
						event_y = (y * height) +
							(height / 2) +
							(priv->spacing / 2);
						break;
					}
					if (jana_time_get_month (start) ==
					    jana_time_get_month (priv->month))
						jana_gtk_tree_layout_set_cell_sensitive (
							JANA_GTK_TREE_LAYOUT (
								priv->layout),
							info->row, TRUE);
					else
						jana_gtk_tree_layout_set_cell_sensitive (
							JANA_GTK_TREE_LAYOUT (
								priv->layout),
							info->row, FALSE);
					
					jana_gtk_tree_layout_move_cell (
						JANA_GTK_TREE_LAYOUT (
							priv->layout),
						info->row, event_x, event_y,
						event_width, event_height);
					events ++;
					g_object_unref (start);
				} else if (start) {
					/* Make cell invisible (it isn't in
					 * range).
					 */
					jana_gtk_tree_layout_move_cell (
						JANA_GTK_TREE_LAYOUT (
							priv->layout),
						info->row, 0, 0, 0, 0);
					g_object_unref (start);
					break;
				}
				cell = cell->next;
			} while (cell);
			jana_time_set_day (time, jana_time_get_day (time) + 1);
		}
	}
	
	g_object_unref (time);
	g_list_free (cells);
}

static void
size_request_cb (GtkWidget *widget, GtkRequisition *requisition,
		 JanaGtkMonthView *self)
{
	requisition->width = 0;
	requisition->height = 0;
}

static void
size_allocate_cb (GtkWidget *widget, GtkAllocation *allocation,
		  JanaGtkMonthView *self)
{
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (self);

	if ((allocation->width != priv->allocation.width) ||
	    (allocation->height != priv->allocation.height)) {
		priv->allocation.width = allocation->width;
		priv->allocation.height = allocation->height;
		relayout (self);
	}
}

static gboolean
button_press_event_cb (GtkWidget *layout, GdkEventButton *event,
		       JanaGtkMonthView *self)
{
	JanaTime *date;
	gint day_increment;
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (self);

	if (!priv->month) return FALSE;
	
	/* Work out which day box (and thereby, date) was clicked */
	date = jana_time_duplicate (priv->start);
	day_increment = 6 - (((layout->allocation.width -
		(gint)event->x + 1) * 7) / layout->allocation.width);
	day_increment += ((priv->visible_weeks - 1) -
		(((layout->allocation.height - (gint)event->y + 1) *
		  priv->visible_weeks) / layout->allocation.height)) * 7;
	jana_time_set_day (date, jana_time_get_day (date) + day_increment);
	
	if (priv->selection)
		g_object_unref (priv->selection);
	priv->selection = date;
	
	g_signal_emit (self, signals[SELECTION_CHANGED], 0, priv->selection);
	gtk_widget_queue_draw (GTK_WIDGET (self));
	
	return TRUE;
}

static void
jana_gtk_month_view_class_init (JanaGtkMonthViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	g_type_class_add_private (klass, sizeof (JanaGtkMonthViewPrivate));

	object_class->get_property = jana_gtk_month_view_get_property;
	object_class->set_property = jana_gtk_month_view_set_property;
	object_class->dispose = jana_gtk_month_view_dispose;
	object_class->finalize = jana_gtk_month_view_finalize;
	
	widget_class->expose_event = jana_gtk_month_view_expose_event;

	g_object_class_install_property (
		object_class,
		PROP_MONTH,
		g_param_spec_object (
			"month",
			"Month",
			"A JanaTime in the month this view should show.",
			G_TYPE_OBJECT,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (
		object_class,
		PROP_SPACING,
		g_param_spec_uint (
			"spacing",
			"Spacing",
			"Spacing to use inside month boxes.",
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
		g_param_spec_object (
			"selection",
			"Selection",
			"The currently selected JanaTime.",
			G_TYPE_OBJECT,
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

	signals[SELECTION_CHANGED] =
		g_signal_new ("selection_changed",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaGtkMonthViewClass,
					 selection_changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__OBJECT,
			G_TYPE_NONE, 1, G_TYPE_OBJECT);
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
		result = jana_utils_time_compare (start_a, start_b, FALSE);
	if (result == 0) {
		if (end_a && end_b)
			result = jana_utils_time_compare (end_b, end_a, FALSE);
		if ((result == 0) && summary_a && summary_b)
			result = g_utf8_collate (summary_a, summary_b);
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
jana_gtk_month_view_init (JanaGtkMonthView *self)
{
	static gboolean locale_set = FALSE;
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (self);

	if (!locale_set) {
		setlocale (LC_TIME, "");
		locale_set = TRUE;
	}
	
	gtk_widget_set_app_paintable (GTK_WIDGET (self), TRUE);

	priv->spacing = 2;
	priv->alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
	priv->layout = jana_gtk_tree_layout_new ();
	g_object_set (G_OBJECT (priv->layout),
		"sort_cb", sort_cells_cb,
		"select_mode", GTK_SELECTION_NONE,
		NULL);
	
	gtk_container_add (GTK_CONTAINER (priv->alignment), priv->layout);
	gtk_container_add (GTK_CONTAINER (self), priv->alignment);
	gtk_widget_show (priv->alignment);
	gtk_widget_show (priv->layout);
	
	priv->event_renderer = g_object_ref_sink (
		jana_gtk_cell_renderer_event_new ());
	g_object_set (G_OBJECT (priv->event_renderer),
		"draw_text", FALSE, NULL);
	
	g_signal_connect (priv->layout, "size-request",
		G_CALLBACK (size_request_cb), self);
	g_signal_connect (priv->layout, "size-allocate",
		G_CALLBACK (size_allocate_cb), self);
	g_signal_connect (priv->layout, "button-press-event",
		G_CALLBACK (button_press_event_cb), self);
}

GtkWidget *
jana_gtk_month_view_new (JanaTime *month)
{
	return GTK_WIDGET (g_object_new (
		JANA_GTK_TYPE_MONTH_VIEW, "month", month, NULL));
}

static void
row_changed_cb (GtkTreeModel *tree_model, GtkTreePath *path,
		GtkTreeIter *iter, JanaGtkMonthView *self)
{
	relayout (self);
}

static void
row_inserted_cb (GtkTreeModel *tree_model, GtkTreePath *path,
		 GtkTreeIter *iter, JanaGtkMonthView *self)
{
	GtkTreeRowReference *row;
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (self);
	
	/* Add row to layout */
	row = gtk_tree_row_reference_new (tree_model, path);
	
	jana_gtk_tree_layout_add_cell (JANA_GTK_TREE_LAYOUT (priv->layout),
		row, 0, 0, 0, 0, priv->event_renderer,
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
	
	gtk_tree_row_reference_free (row);
	
	relayout (self);
}

static void
row_deleted_cb (GtkTreeModel *tree_model, GtkTreePath *path,
		JanaGtkMonthView *self)
{
	relayout (self);
}

static void
insert_rows (JanaGtkMonthView *self, JanaGtkEventStore *store)
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
remove_rows (JanaGtkMonthView *self, JanaGtkEventStore *store)
{
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (self);
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

void
jana_gtk_month_view_add_store (JanaGtkMonthView *self, JanaGtkEventStore *store)
{
	insert_rows (self, store);
	
	g_signal_connect (store, "row-inserted",
		G_CALLBACK (row_inserted_cb), self);
	g_signal_connect_after (store, "row-changed",
		G_CALLBACK (row_changed_cb), self);
	g_signal_connect_after (store, "row-deleted",
		G_CALLBACK (row_deleted_cb), self);
}

void
jana_gtk_month_view_remove_store (JanaGtkMonthView *self,
				  JanaGtkEventStore *store)
{
	g_signal_handlers_disconnect_by_func (store, row_inserted_cb, self);
	g_signal_handlers_disconnect_by_func (store, row_changed_cb, self);
	g_signal_handlers_disconnect_by_func (store, row_deleted_cb, self);
	
	remove_rows (self, store);
}

JanaGtkCellRendererEvent *
jana_gtk_month_view_get_cell_renderer (JanaGtkMonthView *self)
{
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (self);
	
	return (JanaGtkCellRendererEvent *)priv->event_renderer;
}

void
jana_gtk_month_view_set_month (JanaGtkMonthView *self, JanaTime *month)
{
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (self);
	
	if (priv->month) {
		g_object_unref (priv->month);
		priv->month = NULL;
	}
	if (month) {
		GDateWeekday start_day, end_day, day;
		JanaTime *time;
		
		priv->month = jana_time_duplicate (month);
		jana_time_set_isdate (priv->month, TRUE);
		jana_time_set_day (priv->month, 1);

		priv->start = jana_time_duplicate (priv->month);
		start_day = jana_utils_time_set_start_of_week (priv->start);
		priv->end = jana_time_duplicate (priv->month);
		jana_time_set_day (priv->end,
			jana_utils_time_days_in_month (
				jana_time_get_year (priv->end),
				jana_time_get_month (priv->end)));
		end_day = jana_utils_time_set_end_of_week (priv->end);
		
		/* Count number of rows (weeks) */
		for (priv->visible_weeks = 0, day = start_day,
		     time = jana_time_duplicate (priv->start);
		     jana_utils_time_compare (time, priv->end, TRUE) < 0;
		     jana_time_set_day (time,
			jana_time_get_day (time) + 1), day++) {
			if (day > G_DATE_SUNDAY) day -= G_DATE_SUNDAY;
			if (day == start_day) priv->visible_weeks++;
		}
		
		g_object_unref (time);
	}
	if (priv->selection) {
		g_object_unref (priv->selection);
		priv->selection = NULL;
		g_signal_emit (self, signals[SELECTION_CHANGED], 0, NULL);
	}
	relayout (self);
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

JanaTime *
jana_gtk_month_view_get_month (JanaGtkMonthView *self)
{
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (self);
	
	return priv->month ? jana_time_duplicate (priv->month) : NULL;
}

void
jana_gtk_month_view_set_spacing (JanaGtkMonthView *self, guint spacing)
{
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (self);
	
	priv->spacing = spacing;
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

guint
jana_gtk_month_view_get_spacing (JanaGtkMonthView *self)
{
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (self);
	
	return priv->spacing;
}

void
jana_gtk_month_view_set_selection (JanaGtkMonthView *self, JanaTime *day)
{
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (self);
	
	if (priv->selection) {
		if (day &&
		    jana_utils_time_compare (priv->selection, day, TRUE) == 0)
			return;
		
		g_object_unref (priv->selection);
		priv->selection = NULL;
	}
	if (day) priv->selection = jana_time_duplicate (day);
	g_signal_emit (self, signals[SELECTION_CHANGED], 0, priv->selection);
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

JanaTime *
jana_gtk_month_view_get_selection (JanaGtkMonthView *self)
{
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (self);
	
	return priv->selection ? jana_time_duplicate (priv->selection) : NULL;
}

void
jana_gtk_month_view_set_visible_func (JanaGtkMonthView *self,
				      GtkTreeModelFilterVisibleFunc visible_cb,
				      gpointer data)
{
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (self);
	
	jana_gtk_tree_layout_set_visible_func (
		JANA_GTK_TREE_LAYOUT (priv->layout), visible_cb, data);
}

void
jana_gtk_month_view_refilter (JanaGtkMonthView *self)
{
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (self);
	
	jana_gtk_tree_layout_refilter (JANA_GTK_TREE_LAYOUT (priv->layout));
}

void
jana_gtk_month_view_set_highlighted_time (JanaGtkMonthView *self,
					  JanaTime *time)
{
	JanaGtkMonthViewPrivate *priv = MONTH_VIEW_PRIVATE (self);
	
	if (priv->highlighted_time) {
		g_object_unref (priv->highlighted_time);
		priv->highlighted_time = NULL;
	}
	if (time) priv->highlighted_time = jana_time_duplicate (time);
	
	gtk_widget_queue_draw (GTK_WIDGET (self));
}
