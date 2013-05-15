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
#include <libjana/jana-time.h>
#include "jana-gtk-event-store.h"
#include "jana-gtk-cell-renderer-event.h"

G_DEFINE_TYPE (JanaGtkCellRendererEvent, jana_gtk_cell_renderer_event, \
	       GTK_TYPE_CELL_RENDERER)

#define CELL_RENDERER_EVENT_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), JANA_GTK_TYPE_CELL_RENDERER_EVENT, \
	 JanaGtkCellRendererEventPrivate))

typedef struct _JanaGtkCellRendererEventPrivate JanaGtkCellRendererEventPrivate;

struct _JanaGtkCellRendererEventPrivate
{
	gchar *style_hint;
	gchar *uid;
	gchar **categories;
	gchar *summary;
	gchar *location;
	gchar *description;
	JanaTime *start;
	JanaTime *end;
	gboolean first_instance;
	gboolean last_instance;
	gboolean has_recurrences;
	gboolean has_alarm;
	gboolean draw_text;
	gboolean draw_detail;
	gboolean draw_time;
	gboolean draw_box;
	gboolean draw_resize;
	guint xpadi;
	guint ypadi;
	gint cell_width;
	GHashTable *category_color_hash;
};

enum {
	PROP_STYLE_HINT = 1,
	PROP_UID,
	PROP_CATEGORIES,
	PROP_SUMMARY,
	PROP_LOCATION,
	PROP_DESCRIPTION,
	PROP_START,
	PROP_END,
	PROP_FIRST_INSTANCE,
	PROP_LAST_INSTANCE,
	PROP_HAS_RECURRENCES,
	PROP_HAS_ALARM,
	PROP_DRAW_TEXT,
	PROP_DRAW_DETAIL,
	PROP_DRAW_TIME,
	PROP_DRAW_BOX,
	PROP_DRAW_RESIZE,
	PROP_ROW,
	PROP_XPAD_INNER,
	PROP_YPAD_INNER,
	PROP_CELL_WIDTH,
	PROP_CATEGORY_COLOR_HASH,
};

static void
cell_renderer_event_render (GtkCellRenderer *cell, GdkWindow *window,
			    GtkWidget *widget, GdkRectangle *background_area,
			    GdkRectangle *cell_area, GdkRectangle *expose_area,
			    GtkCellRendererState flags);

static void
cell_renderer_event_get_size (GtkCellRenderer *cell, GtkWidget *widget,
			      GdkRectangle *cell_area, gint *x_offset,
			      gint *y_offset, gint *width, gint *height);

static void
jana_gtk_cell_renderer_event_get_property (GObject *object, guint property_id,
					   GValue *value, GParamSpec *pspec)
{
	JanaGtkCellRendererEventPrivate *priv =
		CELL_RENDERER_EVENT_PRIVATE (object);
		
	switch (property_id) {
	    case PROP_STYLE_HINT :
		g_value_set_string (value, priv->style_hint);
		break;
	    case PROP_UID :
		g_value_set_string (value, priv->uid);
		break;
	    case PROP_CATEGORIES :
		g_value_set_boxed (value, priv->categories);
		break;
	    case PROP_SUMMARY :
		g_value_set_string (value, priv->summary);
		break;
	    case PROP_LOCATION :
		g_value_set_string (value, priv->location);
		break;
	    case PROP_DESCRIPTION :
		g_value_set_string (value, priv->description);
		break;
	    case PROP_START :
		g_value_set_object (value, priv->start);
		break;
	    case PROP_END :
		g_value_set_object (value, priv->end);
		break;
	    case PROP_FIRST_INSTANCE :
		g_value_set_boolean (value, priv->first_instance);
		break;
	    case PROP_LAST_INSTANCE :
		g_value_set_boolean (value, priv->last_instance);
		break;
	    case PROP_HAS_RECURRENCES :
		g_value_set_boolean (value, priv->has_recurrences);
		break;
	    case PROP_HAS_ALARM :
		g_value_set_boolean (value, priv->has_alarm);
		break;
	    case PROP_DRAW_TEXT :
		g_value_set_boolean (value, priv->draw_text);
		break;
	    case PROP_DRAW_DETAIL :
		g_value_set_boolean (value, priv->draw_detail);
		break;
	    case PROP_DRAW_TIME :
		g_value_set_boolean (value, priv->draw_time);
		break;
	    case PROP_DRAW_BOX :
		g_value_set_boolean (value, priv->draw_box);
		break;
	    case PROP_DRAW_RESIZE :
		g_value_set_boolean (value, priv->draw_resize);
		break;
	    case PROP_XPAD_INNER :
		g_value_set_uint (value, priv->xpadi);
		break;
	    case PROP_YPAD_INNER :
		g_value_set_uint (value, priv->ypadi);
		break;
	    case PROP_CELL_WIDTH :
		g_value_set_int (value, priv->cell_width);
		break;
	    case PROP_CATEGORY_COLOR_HASH :
		g_value_set_boxed (value, priv->category_color_hash);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_cell_renderer_event_set_property (GObject *object, guint property_id,
					   const GValue *value,
					   GParamSpec *pspec)
{
	JanaGtkCellRendererEventPrivate *priv =
		CELL_RENDERER_EVENT_PRIVATE (object);

	switch (property_id) {
	    case PROP_STYLE_HINT :
		if (priv->style_hint) g_free (priv->style_hint);
		priv->style_hint = g_value_dup_string (value);
		break;
	    case PROP_UID :
		if (priv->uid) g_free (priv->uid);
		priv->uid = g_value_dup_string (value);
		break;
	    case PROP_CATEGORIES :
		if (priv->categories) g_strfreev (priv->categories);
		priv->categories = g_value_dup_boxed (value);
		break;
	    case PROP_SUMMARY :
		if (priv->summary) g_free (priv->summary);
		priv->summary = g_value_dup_string (value);
		break;
	    case PROP_LOCATION :
		if (priv->location) g_free (priv->location);
		priv->location = g_value_dup_string (value);
		break;
	    case PROP_DESCRIPTION :
		if (priv->description) g_free (priv->description);
		priv->description = g_value_dup_string (value);
		break;
	    case PROP_START :
		if (priv->start) g_object_unref (priv->start);
		priv->start = JANA_TIME (g_value_dup_object (value));
		break;
	    case PROP_END :
		if (priv->end) g_object_unref (priv->end);
		priv->end = JANA_TIME (g_value_dup_object (value));
		break;
	    case PROP_FIRST_INSTANCE :
		priv->first_instance = g_value_get_boolean (value);
		break;
	    case PROP_LAST_INSTANCE :
		priv->last_instance = g_value_get_boolean (value);
		break;
	    case PROP_HAS_RECURRENCES :
		priv->has_recurrences = g_value_get_boolean (value);
		break;
	    case PROP_HAS_ALARM :
		priv->has_alarm = g_value_get_boolean (value);
		break;
	    case PROP_DRAW_TEXT :
		priv->draw_text = g_value_get_boolean (value);
		break;
	    case PROP_DRAW_DETAIL :
		priv->draw_detail = g_value_get_boolean (value);
		break;
	    case PROP_DRAW_TIME :
		priv->draw_time = g_value_get_boolean (value);
		break;
	    case PROP_DRAW_BOX :
		priv->draw_box = g_value_get_boolean (value);
		break;
	    case PROP_DRAW_RESIZE :
		priv->draw_resize = g_value_get_boolean (value);
		break;
	    case PROP_ROW : {
		GtkTreeModel *model;
		GtkTreePath *path;
		GtkTreeIter iter;
		GtkTreeRowReference *row = g_value_get_pointer (value);
		
		g_object_set (object, "uid", NULL, "categories", NULL,
			"summary", NULL, "location", NULL, "description", NULL,
			"start", NULL, "end", NULL, NULL);

		if (!row) break;
		
		model = gtk_tree_row_reference_get_model (row);
		
		path = gtk_tree_row_reference_get_path (row);
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_path_free (path);
		
		gtk_tree_model_get (model, &iter,
			JANA_GTK_EVENT_STORE_COL_UID, &priv->uid,
			JANA_GTK_EVENT_STORE_COL_CATEGORIES, &priv->categories,
			JANA_GTK_EVENT_STORE_COL_SUMMARY, &priv->summary,
			JANA_GTK_EVENT_STORE_COL_LOCATION, &priv->location,
			JANA_GTK_EVENT_STORE_COL_DESCRIPTION,
				&priv->description,
			JANA_GTK_EVENT_STORE_COL_START, &priv->start,
			JANA_GTK_EVENT_STORE_COL_END, &priv->end,
			JANA_GTK_EVENT_STORE_COL_FIRST_INSTANCE,
				&priv->first_instance,
			JANA_GTK_EVENT_STORE_COL_LAST_INSTANCE,
				&priv->last_instance,
			JANA_GTK_EVENT_STORE_COL_HAS_RECURRENCES,
				&priv->has_recurrences,
			JANA_GTK_EVENT_STORE_COL_HAS_ALARM, &priv->has_alarm,
			-1);
		
		break;
	    }
	    case PROP_XPAD_INNER :
		priv->xpadi = g_value_get_uint (value);
		break;
	    case PROP_YPAD_INNER :
		priv->ypadi = g_value_get_uint (value);
		break;
	    case PROP_CELL_WIDTH :
		priv->cell_width = g_value_get_int (value);
		break;
	    case PROP_CATEGORY_COLOR_HASH :
		if (priv->category_color_hash)
			g_hash_table_unref (priv->category_color_hash);
		priv->category_color_hash = g_value_dup_boxed (value);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_cell_renderer_event_dispose (GObject *object)
{
	JanaGtkCellRendererEventPrivate *priv =
		CELL_RENDERER_EVENT_PRIVATE (object);
	
	if (priv->start) {
		g_object_unref (priv->start);
		priv->start = NULL;
	}
	
	if (priv->end) {
		g_object_unref (priv->end);
		priv->end = NULL;		
	}

	if (G_OBJECT_CLASS (jana_gtk_cell_renderer_event_parent_class)->dispose)
		G_OBJECT_CLASS (jana_gtk_cell_renderer_event_parent_class)->
			dispose (object);
}

static void
jana_gtk_cell_renderer_event_finalize (GObject *object)
{
	JanaGtkCellRendererEventPrivate *priv =
		CELL_RENDERER_EVENT_PRIVATE (object);
	
	g_free (priv->style_hint);
	g_free (priv->uid);
	g_free (priv->summary);
	g_free (priv->location);
	g_free (priv->description);
	if (priv->category_color_hash)
		g_hash_table_unref (priv->category_color_hash);
	
	G_OBJECT_CLASS (jana_gtk_cell_renderer_event_parent_class)->
		finalize (object);
}

static void
jana_gtk_cell_renderer_event_class_init (JanaGtkCellRendererEventClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkCellRendererClass *renderer_class = GTK_CELL_RENDERER_CLASS (klass);

	g_type_class_add_private (klass,
		sizeof (JanaGtkCellRendererEventPrivate));

	object_class->get_property = jana_gtk_cell_renderer_event_get_property;
	object_class->set_property = jana_gtk_cell_renderer_event_set_property;
	object_class->dispose = jana_gtk_cell_renderer_event_dispose;
	object_class->finalize = jana_gtk_cell_renderer_event_finalize;
	
	renderer_class->get_size = cell_renderer_event_get_size;
	renderer_class->render = cell_renderer_event_render;

	g_object_class_install_property (
		object_class,
		PROP_STYLE_HINT,
		g_param_spec_string (
			"style_hint",
			"gchar *",
			"The style hint to use when drawing.",
			"button",
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_UID,
		g_param_spec_string (
			"uid",
			"gchar *",
			"The UID of the represented event.",
			NULL,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_CATEGORIES,
		g_param_spec_boxed (
			"categories",
			"gchar **",
			"A list of category strings.",
			G_TYPE_STRV,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_SUMMARY,
		g_param_spec_string (
			"summary",
			"gchar *",
			"The summary of the represented event.",
			NULL,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_LOCATION,
		g_param_spec_string (
			"location",
			"gchar *",
			"The location of the represented event.",
			NULL,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_DESCRIPTION,
		g_param_spec_string (
			"description",
			"gchar *",
			"The description of the represented event.",
			NULL,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_START,
		g_param_spec_object (
			"start",
			"JanaTime *",
			"The starting time of the represented event.",
			G_TYPE_OBJECT,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_END,
		g_param_spec_object (
			"end",
			"JanaTime *",
			"The ending time of the represented event.",
			G_TYPE_OBJECT,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_FIRST_INSTANCE,
		g_param_spec_boolean (
			"first-instance",
			"gboolean",
			"If this is the first instance of this event.",
			TRUE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_LAST_INSTANCE,
		g_param_spec_boolean (
			"last-instance",
			"gboolean",
			"If this is the last instance of this event.",
			TRUE,
			G_PARAM_READWRITE));
	
	g_object_class_install_property (
		object_class,
		PROP_HAS_RECURRENCES,
		g_param_spec_boolean (
			"has_recurrences",
			"gboolean",
			"Whether the represented event has recurrences.",
			FALSE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_HAS_ALARM,
		g_param_spec_boolean (
			"has_alarm",
			"gboolean",
			"Whether the represented event has an alarm.",
			FALSE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_DRAW_TEXT,
		g_param_spec_boolean (
			"draw_text",
			"gboolean",
			"Whether to draw text.",
			TRUE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_DRAW_DETAIL,
		g_param_spec_boolean (
			"draw_detail",
			"gboolean",
			"Whether to draw description.",
			TRUE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_DRAW_TIME,
		g_param_spec_boolean (
			"draw_time",
			"gboolean",
			"Whether to draw the time of the represented event "
			"textually.",
			FALSE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_DRAW_BOX,
		g_param_spec_boolean (
			"draw_box",
			"gboolean",
			"Whether to draw a box around the represented event.",
			TRUE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_DRAW_RESIZE,
		g_param_spec_boolean (
			"draw_resize",
			"gboolean",
			"Whether to draw a resize grip.",
			FALSE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_ROW,
		g_param_spec_pointer (
			"row",
			"GtkTreeRowReference *",
			"A reference to a row in a JanaGtkEventStore to get "
			"data from.",
			G_PARAM_WRITABLE));

	g_object_class_install_property (
		object_class,
		PROP_XPAD_INNER,
		g_param_spec_uint (
			"xpad_inner",
			"gint",
			"Inner padding on the x-axis, in pixels.",
			0, G_MAXUINT, 3,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_YPAD_INNER,
		g_param_spec_uint (
			"ypad_inner",
			"gint",
			"Inner padding on the y-axis, in pixels.",
			0, G_MAXUINT, 3,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_CELL_WIDTH,
		g_param_spec_int (
			"cell_width",
			"gint",
			"Width the cell should try to size itself for.",
			-G_MAXINT, G_MAXINT, -1,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_CATEGORY_COLOR_HASH,
		g_param_spec_boxed (
			"category_color_hash",
			"GHashTable *",
			"A mapping of category strings to GdkColors.",
			G_TYPE_HASH_TABLE,
			G_PARAM_READWRITE));
}

static void
jana_gtk_cell_renderer_event_init (JanaGtkCellRendererEvent *self)
{
	JanaGtkCellRendererEventPrivate *priv =
		CELL_RENDERER_EVENT_PRIVATE (self);
	
	priv->style_hint = g_strdup ("button");
	priv->draw_text = TRUE;
	priv->draw_detail = TRUE;
	priv->draw_box = TRUE;
	priv->xpadi = 3;
	priv->ypadi = 3;
	priv->cell_width = -1;
	priv->first_instance = TRUE;
	priv->last_instance = TRUE;
}

static PangoLayout *
get_summary_layout (JanaGtkCellRendererEvent *cell, GtkWidget *widget,
		    GdkRectangle *area)
{
	gchar *string, *time_string;
	PangoLayout *layout;
	JanaGtkCellRendererEventPrivate *priv =
		CELL_RENDERER_EVENT_PRIVATE (cell);

	if ((!priv->summary) && (!priv->draw_time)) return NULL;

	if (priv->draw_time) {
		if (priv->start && priv->first_instance) {
			if (priv->end && priv->last_instance) {
				time_string = g_strdup_printf (
					"<span stretch=\"ultracondensed\">"
					"<small>[%02d:%02d]-[%02d:%02d]</small>"
					"</span> ",
					jana_time_get_hours (priv->start),
					jana_time_get_minutes (priv->start),
					jana_time_get_hours (priv->end),
					jana_time_get_minutes (priv->end));
			} else {
				time_string = g_strdup_printf (
					"<span stretch=\"ultracondensed\">"
					"<small>[%02d:%02d]-[...]</small>"
					"</span> ",
					jana_time_get_hours (priv->start),
					jana_time_get_minutes (priv->start));
			}
		} else {
			if (priv->end && priv->last_instance) {
				time_string = g_strdup_printf (
					"<span stretch=\"ultracondensed\">"
					"<small>[...]-[%02d:%02d]</small>"
					"</span> ",
					jana_time_get_hours (priv->end),
					jana_time_get_minutes (priv->end));
			} else {
				time_string = g_strdup_printf (
					"<span stretch=\"ultracondensed\">"
					"<small>[...]</small></span> ");
			}
		}
	} else {
		time_string = NULL;
	}
	
	string = g_strconcat (time_string ? time_string : "",
		priv->summary ? priv->summary : "", NULL);
	
	layout = gtk_widget_create_pango_layout (widget, NULL);
	pango_layout_set_width (layout, area->width * PANGO_SCALE);
	pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
	pango_layout_set_markup (layout, string, -1);
	
	return layout;
}

static PangoLayout *
get_description_layout (JanaGtkCellRendererEvent *cell, GtkWidget *widget,
			GdkRectangle *area)
{
	gint el_width, height;
	PangoLayout *layout;
	PangoAttrList *attrs;
	PangoAttribute *size;
	JanaGtkCellRendererEventPrivate *priv =
		CELL_RENDERER_EVENT_PRIVATE (cell);

	if (((!priv->description) && (!priv->location)) || (!priv->draw_detail))
		return NULL;
	
	/* Make Pango attribute list for small text */
	attrs = pango_attr_list_new ();
	size = pango_attr_size_new (pango_font_description_get_size (
		widget->style->font_desc) * PANGO_SCALE_SMALL);
	size->start_index = 0;
	size->end_index = G_MAXUINT;
	pango_attr_list_change (attrs, size);

	/* Measure the width of the '...' string */
	layout = gtk_widget_create_pango_layout (widget, "...");
	pango_layout_set_attributes (layout, attrs);
	pango_layout_get_pixel_size (layout, &el_width, NULL);
	g_object_unref (layout);

	/* Make the description layout */
	if (priv->location) {
		gchar *string = g_strdup_printf ("%s%s%s",
			priv->location, priv->description ? "\n" : "",
			priv->description ? priv->description : "");
		layout = gtk_widget_create_pango_layout (widget, string);
		g_free (string);
	} else {
		layout = gtk_widget_create_pango_layout (
			widget, priv->description);
	}
	
	pango_layout_set_attributes (layout, attrs);
	pango_attr_list_unref (attrs);
	pango_layout_set_width (layout, area->width * PANGO_SCALE);
	pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);

	/* Manual ellipsis for multiple-line layouts */
	/* TODO: Width for height, natural size */
	pango_layout_get_pixel_size (layout, NULL, &height);
	if (height > area->height) {
		PangoLayoutIter *iter = pango_layout_get_iter (layout);
		gint line_n = 0;
		height = 0;
		if (pango_layout_get_line_count (layout) <= 1) {
			pango_layout_set_ellipsize (layout,
				PANGO_ELLIPSIZE_END);
		} else do {
			PangoRectangle rect;
			PangoLayoutLine *line =
				pango_layout_iter_get_line_readonly (iter);
			gint descent;
			
			pango_layout_line_get_pixel_extents (line, NULL, &rect);
			height += rect.height;
			descent = PANGO_DESCENT (rect);
			if (descent < 0) descent = 0;
			
			if (height + descent > area->height) {
				gchar *string_short, *string_ellip, *end_string;
				gint width;
				
				if (line_n == 0) {
					pango_layout_set_ellipsize (layout,
						PANGO_ELLIPSIZE_END);
					break;
				}

				/* Get previous line */
				line = pango_layout_get_line (layout,
					line_n - 1);
				pango_layout_line_ref (line);
				
				/* Do ellipsis */
				end_string = priv->description +
					line->start_index + line->length;

				pango_layout_line_get_pixel_extents (line, NULL,
					&rect);

				/* Find out if the line wrapped */
				if (area->width - (rect.x + rect.width) <=
				    el_width) {
					do {
						gint length;
						/* FIXME: Something faster..? */
						/* Cut off characters from the
						 * end of the string until we
						 * have enough space to append
						 * ellipsis.
						 */
						
						/* Cut off a character */
						end_string = 
						  g_utf8_offset_to_pointer (
							end_string, -1);

						length = (priv->description +
							line->start_index +
							line->length) -
							end_string;

						/* Safe-guard - if we reach
						 * the start of the line, break
						 */
						if (length <= 1) break; 
						
						/* Measure the string we've
						 * cut off.
						 */
						pango_layout_set_text (layout,
							end_string, length);
						pango_layout_get_pixel_extents (
							layout, NULL, &rect);
						width = rect.width;
					} while (width <= el_width);
				}
				
				pango_layout_line_unref (line);

				if (end_string <= priv->description) {
					pango_layout_iter_free (iter);
					g_object_unref (layout);
					return NULL;
				}
				
				string_short = g_strndup (priv->description,
					end_string - priv->description);
				string_ellip = g_strconcat (
					string_short, "...", NULL);
				pango_layout_set_text (
					layout, string_ellip, -1);
				g_free (string_short);
				g_free (string_ellip);
				break;
			}
			line_n ++;
		} while (pango_layout_iter_next_line (iter));
		pango_layout_iter_free (iter);
	}
	
	return layout;
}

static void
cell_renderer_event_render (GtkCellRenderer *cell, GdkWindow *window,
			    GtkWidget *widget, GdkRectangle *background_area,
			    GdkRectangle *cell_area, GdkRectangle *expose_area,
			    GtkCellRendererState flags)
{
	GtkStateType state;
	guint xpad, ypad;
	gint w, h, x, y;
	GtkStyle *style;
	PangoLayout *layout;
	GdkRectangle text_clip;
	JanaGtkCellRendererEventPrivate *priv =
		CELL_RENDERER_EVENT_PRIVATE (cell);
	
	if ((!priv->draw_box) && (!priv->draw_text)) return;

	if (flags & GTK_CELL_RENDERER_INSENSITIVE)
		state = GTK_STATE_INSENSITIVE;
	else if (flags & GTK_CELL_RENDERER_SELECTED)
		state = GTK_STATE_ACTIVE;
	else if (flags & GTK_CELL_RENDERER_PRELIT)
		state = GTK_STATE_PRELIGHT;
	else
		state = GTK_STATE_NORMAL;
	
	g_object_get (cell, "xpad", &xpad, "ypad", &ypad, NULL);
	
	/* Check if there's enough allocated space to draw in */
	w = cell_area->width - (xpad * 2);
	h = cell_area->height - (ypad * 2);
	x = cell_area->x + xpad;
	y = cell_area->y + ypad;
	if ((w <= 0) || (h <= 0)) return;
	
	if ((!priv->draw_box) || (!priv->category_color_hash) ||
	    (!priv->categories) || (!priv->categories[0])) {
		style = widget->style;
	} else {
		GdkColor *color;
		
		color = g_hash_table_lookup (
			priv->category_color_hash, priv->categories[0]);
		
		style = gtk_style_copy (widget->style);
		if (color) {
			gint intensity, avg;
			style->bg[state] = *color;
			switch (state) {
			    case GTK_STATE_INSENSITIVE :
				/* De-saturated */
				avg = (((gint)color->red*3)+
					((gint)color->green*6)+
				       (gint)color->blue)/10;
				style->bg[state].red =
					(style->bg[state].red + avg)/2;
				style->bg[state].green =
					(style->bg[state].green + avg)/2;
				style->bg[state].blue =
					(style->bg[state].blue + avg)/2;
				break;
			    case GTK_STATE_ACTIVE :
				/* Darkened */
				style->bg[state].red =
					((gint)style->bg[state].red*4)/5;
				style->bg[state].green =
					((gint)style->bg[state].green*4)/5;
				style->bg[state].blue =
					((gint)style->bg[state].blue*4)/5;
				break;
			    case GTK_STATE_PRELIGHT :
				/* Brightened */
				avg = G_MAXUINT16;
				style->bg[state].red =
					(style->bg[state].red + avg)/2;
				style->bg[state].green =
					(style->bg[state].green + avg)/2;
				style->bg[state].blue =
					(style->bg[state].blue + avg)/2;
				break;
			    default :
				break;
			}
			
			/* Grey out text if it's insensitive */
			if (state == GTK_STATE_INSENSITIVE) {
				style->fg[state].red = G_MAXUINT16/2;
				style->fg[state].green = G_MAXUINT16/2;
				style->fg[state].blue = G_MAXUINT16/2;
			} else {
				/* Choose text colour (black/white) based on
				 * rough background colour intensity
				 */
				intensity = (((gint)style->bg[state].red)+
					((gint)style->bg[state].green)+
					(gint)style->bg[state].blue)/3;

				if (intensity > G_MAXUINT16/2) {
					style->fg[state] = style->black;
				} else {
					style->fg[state] = style->white;
				}
			}
		}
		style = gtk_style_attach (style, widget->window);
	}
	
	/* Draw event box */
	if (priv->draw_box) {
		gtk_paint_box (style, window, state,
			(state == GTK_STATE_ACTIVE) ?
			       GTK_SHADOW_IN : GTK_SHADOW_OUT,
			expose_area, widget, priv->style_hint, x, y, w, h);
	}
	
	/* Draw event text */
	if (priv->draw_text) {
		text_clip.x = x + priv->xpadi;
		text_clip.y = y + priv->ypadi;
		text_clip.width = w - (priv->xpadi * 2);
		text_clip.height = h - (priv->ypadi * 2);
		if ((text_clip.height > 0) && (text_clip.width > 0) &&
		    (layout = get_summary_layout (
		    JANA_GTK_CELL_RENDERER_EVENT (cell), widget, &text_clip))) {
			gtk_paint_layout (style, window, state,
				!priv->draw_box,
				&text_clip, widget, priv->style_hint,
				text_clip.x, text_clip.y,
				layout);

			pango_layout_get_pixel_size (layout, NULL, &h);
			text_clip.y += h;
			text_clip.height -= h;
			g_object_unref (layout);
		}
		
		if ((text_clip.height > 0) && (text_clip.width > 0) &&
		    (priv->draw_detail) && (layout = get_description_layout (
		    JANA_GTK_CELL_RENDERER_EVENT (cell), widget, &text_clip))) {
			gtk_paint_layout (style, window, state,
				!priv->draw_box,
				&text_clip, widget, priv->style_hint,
				text_clip.x, text_clip.y,
				layout);

			g_object_unref (layout);
		}
	}
	
	if ((priv->draw_box) && (priv->draw_resize)) {
		/* Draw resize grip */
		gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
		
		x = cell_area->x + cell_area->width - w -
			xpad - (priv->xpadi/2);
		y = cell_area->y + cell_area->height - h -
			ypad - (priv->ypadi/2);
		
		if ((w + xpad + (priv->xpadi/2) <= cell_area->width) &&
		    (h + ypad + (priv->ypadi/2) <= cell_area->height))
			gtk_paint_resize_grip (style, window, state,
				expose_area, widget, priv->style_hint,
				GDK_WINDOW_EDGE_SOUTH_EAST, x, y, w, h);
	}
	
	if (priv->draw_box && priv->category_color_hash && priv->categories &&
	    priv->categories[0]) {
		gtk_style_detach (style);
	}
}

static void
cell_renderer_event_get_size (GtkCellRenderer *cell, GtkWidget *widget,
			      GdkRectangle *cell_area, gint *x_offset,
			      gint *y_offset, gint *width, gint *height)
{
	JanaGtkCellRendererEventPrivate *priv =
		CELL_RENDERER_EVENT_PRIVATE (cell);
	guint xpad, ypad;
	gint text_width, text_height/*, icon_width, icon_height*/;
	GdkRectangle area;
	PangoLayout *layout;
	
	if ((!priv->draw_box) && (!priv->draw_text)) {
		if (width) *width = 0;
		if (height) *height = 0;
		if (x_offset) *x_offset = 0;
		if (y_offset) *y_offset = 0;
		return;
	}
	
	if (cell_area) {
		/* TODO: Alignment? */
		if (x_offset) *x_offset = 0;
		if (y_offset) *y_offset = 0;
	}

	/* Ask for at least enough room to draw the box with inner padding */
	g_object_get (cell, "xpad", &xpad, "ypad", &ypad, NULL);
	if (width) *width = ((priv->xpadi + xpad)*2);
	if (height) *height = ((priv->ypadi + ypad) * 2);
	
	if (!priv->draw_text) return;

	/* TODO: Width for height, natural size */
	area.x = 0;
	area.y = 0;
	g_object_get (G_OBJECT (cell), "width", &area.width, NULL);
	if (area.width < 0) {
		if (cell_area) {
			area.width = cell_area->width;
		} else if (priv->cell_width >= 0) {
			area.width = priv->cell_width -
				(priv->xpadi + xpad) * 2;
		} else {
			area.width = G_MAXINT;
		}
	} else area.width -= (priv->xpadi + xpad) * 2;
	area.height = (cell_area && (cell_area->height >= 0)) ?
		cell_area->height : G_MAXINT;
	
	if ((layout = get_summary_layout (JANA_GTK_CELL_RENDERER_EVENT (cell),
	     widget, &area))) {
		pango_layout_get_pixel_size (layout, &text_width, &text_height);

		if (height) *height += text_height;
		if (area.width == G_MAXINT) {
			area.width = text_width;
			if (width) *width += text_width;
		}
		
		g_object_unref (layout);
	}
	
	/* TODO: Alarm / recurrence icons */
	/*gtk_icon_size_lookup (GTK_ICON_SIZE_MENU,
		&icon_width, &icon_height);*/
	
	if (!priv->draw_detail) return;
	
	if ((layout = get_description_layout (
	     JANA_GTK_CELL_RENDERER_EVENT (cell), widget, &area))) {
		pango_layout_get_pixel_size (layout, &text_width, &text_height);
		if (height) *height += text_height;
		g_object_unref (layout);
	}
}

GtkCellRenderer*
jana_gtk_cell_renderer_event_new (void)
{
	return g_object_new (JANA_GTK_TYPE_CELL_RENDERER_EVENT, NULL);
}

