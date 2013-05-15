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
#include "jana-gtk-note-store.h"
#include "jana-gtk-cell-renderer-note.h"

G_DEFINE_TYPE (JanaGtkCellRendererNote, jana_gtk_cell_renderer_note, \
	       GTK_TYPE_CELL_RENDERER)

#define CELL_RENDERER_NOTE_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), JANA_GTK_TYPE_CELL_RENDERER_NOTE, \
	 JanaGtkCellRendererNotePrivate))

typedef struct _JanaGtkCellRendererNotePrivate JanaGtkCellRendererNotePrivate;

struct _JanaGtkCellRendererNotePrivate
{
	gchar *style_hint;
	gchar *uid;
	gchar **categories;
	gchar *author;
	gchar *recipient;
	gchar *body;
	JanaTime *created;
	JanaTime *modified;
	GdkPixbuf *icon;
	GtkJustification justify;
	gboolean draw_box;
	gboolean show_author;
	gboolean show_recipient;
	gboolean show_body;
	gboolean show_created;
	gboolean show_modified;
	guint xpadi;
	guint ypadi;
	gint cell_width;
	GHashTable *category_color_hash;
	GHashTable *category_icon_hash;
};

enum {
	PROP_STYLE_HINT = 1,
	PROP_UID,
	PROP_CATEGORIES,
	PROP_AUTHOR,
	PROP_RECIPIENT,
	PROP_BODY,
	PROP_CREATED,
	PROP_MODIFIED,
	PROP_ICON,
	PROP_JUSTIFY,
	PROP_DRAW_BOX,
	PROP_SHOW_AUTHOR,
	PROP_SHOW_RECIPIENT,
	PROP_SHOW_BODY,
	PROP_SHOW_CREATED,
	PROP_SHOW_MODIFIED,
	PROP_XPAD_INNER,
	PROP_YPAD_INNER,
	PROP_CELL_WIDTH,
	PROP_CATEGORY_COLOR_HASH,
	PROP_CATEGORY_ICON_HASH,
};

static void
cell_renderer_note_render (GtkCellRenderer *cell, GdkWindow *window,
			   GtkWidget *widget, GdkRectangle *background_area,
			   GdkRectangle *cell_area, GdkRectangle *expose_area,
			   GtkCellRendererState flags);

static void
cell_renderer_note_get_size (GtkCellRenderer *cell, GtkWidget *widget,
			     GdkRectangle *cell_area, gint *x_offset,
			     gint *y_offset, gint *width, gint *height);

static void
jana_gtk_cell_renderer_note_get_property (GObject *object, guint property_id,
					  GValue *value, GParamSpec *pspec)
{
	JanaGtkCellRendererNotePrivate *priv =
		CELL_RENDERER_NOTE_PRIVATE (object);
		
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
	    case PROP_AUTHOR :
		g_value_set_string (value, priv->author);
		break;
	    case PROP_RECIPIENT :
		g_value_set_string (value, priv->recipient);
		break;
	    case PROP_BODY :
		g_value_set_string (value, priv->body);
		break;
	    case PROP_CREATED :
		g_value_set_object (value, priv->created);
		break;
	    case PROP_MODIFIED :
		g_value_set_object (value, priv->modified);
		break;
	    case PROP_DRAW_BOX :
		g_value_set_boolean (value, priv->draw_box);
		break;
	    case PROP_SHOW_AUTHOR :
		g_value_set_boolean (value, priv->show_author);
		break;
	    case PROP_SHOW_RECIPIENT :
		g_value_set_boolean (value, priv->show_recipient);
		break;
	    case PROP_SHOW_BODY :
		g_value_set_boolean (value, priv->show_body);
		break;
	    case PROP_SHOW_CREATED :
		g_value_set_boolean (value, priv->show_created);
		break;
	    case PROP_SHOW_MODIFIED :
		g_value_set_boolean (value, priv->show_modified);
		break;
	    case PROP_ICON :
		g_value_set_object (value, priv->icon);
		break;
	    case PROP_JUSTIFY :
		g_value_set_enum (value, priv->justify);
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
	    case PROP_CATEGORY_ICON_HASH :
		g_value_set_boxed (value, priv->category_icon_hash);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_cell_renderer_note_set_property (GObject *object, guint property_id,
					  const GValue *value,
					  GParamSpec *pspec)
{
	JanaGtkCellRendererNotePrivate *priv =
		CELL_RENDERER_NOTE_PRIVATE (object);

	switch (property_id) {
	    case PROP_STYLE_HINT :
		g_free (priv->style_hint);
		priv->style_hint = g_value_dup_string (value);
		break;
	    case PROP_UID :
		g_free (priv->uid);
		priv->uid = g_value_dup_string (value);
		break;
	    case PROP_CATEGORIES :
		g_strfreev (priv->categories);
		priv->categories = g_value_dup_boxed (value);
		break;
	    case PROP_AUTHOR :
		g_free (priv->author);
		priv->author = g_value_dup_string (value);
		break;
	    case PROP_RECIPIENT :
		g_free (priv->recipient);
		priv->recipient = g_value_dup_string (value);
		break;
	    case PROP_BODY :
		g_free (priv->body);
		priv->body = g_value_dup_string (value);
		break;
	    case PROP_CREATED :
		if (priv->created) g_object_unref (priv->created);
		priv->created = JANA_TIME (g_value_dup_object (value));
		break;
	    case PROP_MODIFIED :
		if (priv->modified) g_object_unref (priv->modified);
		priv->modified = JANA_TIME (g_value_dup_object (value));
		break;
	    case PROP_DRAW_BOX :
		priv->draw_box = g_value_get_boolean (value);
		break;
	    case PROP_SHOW_AUTHOR :
		priv->show_author = g_value_get_boolean (value);
		break;
	    case PROP_SHOW_RECIPIENT :
		priv->show_recipient = g_value_get_boolean (value);
		break;
	    case PROP_SHOW_BODY :
		priv->show_body = g_value_get_boolean (value);
		break;
	    case PROP_SHOW_CREATED :
		priv->show_created = g_value_get_boolean (value);
		break;
	    case PROP_SHOW_MODIFIED :
		priv->show_modified = g_value_get_boolean (value);
		break;
	    case PROP_ICON :
		if (priv->icon) g_object_unref (priv->icon);
		priv->icon = g_value_dup_object (value);
		break;
	    case PROP_JUSTIFY :
		priv->justify = g_value_get_enum (value);
		break;
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
	    case PROP_CATEGORY_ICON_HASH :
		if (priv->category_icon_hash)
			g_hash_table_unref (priv->category_icon_hash);
		priv->category_icon_hash = g_value_dup_boxed (value);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_cell_renderer_note_dispose (GObject *object)
{
	JanaGtkCellRendererNotePrivate *priv =
		CELL_RENDERER_NOTE_PRIVATE (object);
	
	if (priv->created) {
		g_object_unref (priv->created);
		priv->created = NULL;
	}
	
	if (priv->modified) {
		g_object_unref (priv->modified);
		priv->modified = NULL;
	}

	if (G_OBJECT_CLASS (jana_gtk_cell_renderer_note_parent_class)->dispose)
		G_OBJECT_CLASS (jana_gtk_cell_renderer_note_parent_class)->
			dispose (object);
}

static void
jana_gtk_cell_renderer_note_finalize (GObject *object)
{
	JanaGtkCellRendererNotePrivate *priv =
		CELL_RENDERER_NOTE_PRIVATE (object);
	
	g_free (priv->style_hint);
	g_free (priv->uid);
	g_free (priv->author);
	g_free (priv->recipient);
	g_free (priv->body);
	if (priv->category_color_hash)
		g_hash_table_unref (priv->category_color_hash);
	if (priv->category_icon_hash)
		g_hash_table_unref (priv->category_icon_hash);
	
	G_OBJECT_CLASS (jana_gtk_cell_renderer_note_parent_class)->
		finalize (object);
}

static void
jana_gtk_cell_renderer_note_class_init (JanaGtkCellRendererNoteClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkCellRendererClass *renderer_class = GTK_CELL_RENDERER_CLASS (klass);

	g_type_class_add_private (klass,
		sizeof (JanaGtkCellRendererNotePrivate));

	object_class->get_property = jana_gtk_cell_renderer_note_get_property;
	object_class->set_property = jana_gtk_cell_renderer_note_set_property;
	object_class->dispose = jana_gtk_cell_renderer_note_dispose;
	object_class->finalize = jana_gtk_cell_renderer_note_finalize;
	
	renderer_class->get_size = cell_renderer_note_get_size;
	renderer_class->render = cell_renderer_note_render;

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
			"The UID of the represented note.",
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
		PROP_AUTHOR,
		g_param_spec_string (
			"author",
			"gchar *",
			"The author of the represented note.",
			NULL,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_RECIPIENT,
		g_param_spec_string (
			"recipient",
			"gchar *",
			"The recipient of the represented note.",
			NULL,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_BODY,
		g_param_spec_string (
			"body",
			"gchar *",
			"The body of the represented note.",
			NULL,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_CREATED,
		g_param_spec_object (
			"created",
			"JanaTime *",
			"The creation time of the represented note.",
			G_TYPE_OBJECT,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_MODIFIED,
		g_param_spec_object (
			"modified",
			"JanaTime *",
			"The last modification time of the represented note.",
			G_TYPE_OBJECT,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_ICON,
		g_param_spec_object (
			"icon",
			"GdkPixbuf *",
			"An icon to show alongside the note.",
			GDK_TYPE_PIXBUF,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_JUSTIFY,
		g_param_spec_enum (
			"justify",
			"GtkJustification",
			"The justification to use when laying out the note.",
			GTK_TYPE_JUSTIFICATION, GTK_JUSTIFY_LEFT,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_DRAW_BOX,
		g_param_spec_boolean (
			"draw_box",
			"gboolean",
			"Whether to draw a box surrounding the note.",
			FALSE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_SHOW_AUTHOR,
		g_param_spec_boolean (
			"show_author",
			"gboolean",
			"Whether to show the author of the note.",
			TRUE,
			G_PARAM_READWRITE));
	
	g_object_class_install_property (
		object_class,
		PROP_SHOW_RECIPIENT,
		g_param_spec_boolean (
			"show_recipient",
			"gboolean",
			"Whether to show the recipient of the note.",
			FALSE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_SHOW_BODY,
		g_param_spec_boolean (
			"show_body",
			"gboolean",
			"Whether to show the body of the note.",
			TRUE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_SHOW_CREATED,
		g_param_spec_boolean (
			"show_created",
			"gboolean",
			"Whether to show the creation time of the note.",
			TRUE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_SHOW_MODIFIED,
		g_param_spec_boolean (
			"show_modified",
			"gboolean",
			"Whether to show the modification time of the note.",
			FALSE,
			G_PARAM_READWRITE));

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

	g_object_class_install_property (
		object_class,
		PROP_CATEGORY_ICON_HASH,
		g_param_spec_boxed (
			"category_icon_hash",
			"GHashTable *",
			"A mapping of category strings to GdkPixbufs.",
			G_TYPE_HASH_TABLE,
			G_PARAM_READWRITE));
}

static void
jana_gtk_cell_renderer_note_init (JanaGtkCellRendererNote *self)
{
	JanaGtkCellRendererNotePrivate *priv =
		CELL_RENDERER_NOTE_PRIVATE (self);
	
	priv->style_hint = g_strdup ("button");
	priv->show_author = TRUE;
	priv->show_created = TRUE;
	priv->show_body = TRUE;
	priv->justify = GTK_JUSTIFY_LEFT;
	priv->xpadi = 3;
	priv->ypadi = 3;
	priv->cell_width = -1;
}

static void
layout_set_justification (PangoLayout *layout, GtkJustification justify)
{
	switch (justify) {
	    case GTK_JUSTIFY_RIGHT :
		pango_layout_set_alignment (layout, PANGO_ALIGN_RIGHT);
		break;
	    case GTK_JUSTIFY_CENTER :
		pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
		break;
	    case GTK_JUSTIFY_FILL :
		pango_layout_set_justify (layout, TRUE);
		break;
	    case GTK_JUSTIFY_LEFT :
	    default :
		pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);
		break;
	}
}

static PangoLayout *
get_header_layout (JanaGtkCellRendererNote *cell, GtkWidget *widget,
		   GdkRectangle *area)
{
	GString *string;
	PangoLayout *layout;
	JanaGtkCellRendererNotePrivate *priv =
		CELL_RENDERER_NOTE_PRIVATE (cell);

	if ((!priv->show_created) && (!priv->show_modified) &&
	    (!priv->show_author) && (!priv->show_recipient) &&
	    (!priv->show_body)) return NULL;
	
	string = g_string_new ("<small>");
	
	if (priv->show_created && priv->created) {
		g_string_append_printf (string, "%d/%d/%02d",
			jana_time_get_day (priv->created),
			jana_time_get_month (priv->created),
			jana_time_get_year (priv->created));

		if (priv->show_modified && priv->modified) {
			g_string_append (string, ", ");
		} else
			g_string_append (string, " ");
	}
	if (priv->show_modified && priv->modified) {
		g_string_append_printf (string, "%d/%d/%02d ",
			jana_time_get_day (priv->modified),
			jana_time_get_month (priv->modified),
			jana_time_get_year (priv->modified));
	}
	g_string_append (string, "</small>");
	
	if (priv->show_author && priv->author) {
		g_string_append (string, priv->author);
		
		if (priv->show_recipient && priv->recipient) {
			g_string_append (string, ", to ");
		}
	}
	if (priv->show_recipient && priv->recipient) {
		g_string_append (string, priv->recipient);
	}

	layout = gtk_widget_create_pango_layout (widget, NULL);
	pango_layout_set_width (layout, area->width * PANGO_SCALE);
	pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
	pango_layout_set_markup (layout, string->str, string->len);
	g_string_free (string, TRUE);
	
	layout_set_justification (layout, priv->justify);
	
	return layout;
}

/* FIXME: This code adapted from JanaGtkCellRendererEvent... Could do with 
 *        making this shared, it's not trivial
 */
static PangoLayout *
get_body_layout (JanaGtkCellRendererNote *cell, GtkWidget *widget,
		 GdkRectangle *area)
{
	gint el_width, height;
	PangoLayout *layout;
	JanaGtkCellRendererNotePrivate *priv =
		CELL_RENDERER_NOTE_PRIVATE (cell);

	if ((!priv->body) || (!priv->show_body))
		return NULL;
	
	/* Measure the width of the '...' string */
	layout = gtk_widget_create_pango_layout (widget, "...");
	pango_layout_get_pixel_size (layout, &el_width, NULL);
	g_object_unref (layout);

	/* Make the body layout */
	layout = gtk_widget_create_pango_layout (widget, priv->body);
	
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
				end_string = priv->body +
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

						length = (priv->body +
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

				if (end_string <= priv->body) {
					pango_layout_iter_free (iter);
					g_object_unref (layout);
					return NULL;
				}
				
				string_short = g_strndup (priv->body,
					end_string - priv->body);
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
	
	layout_set_justification (layout, priv->justify);

	return layout;
}

static void
cell_renderer_note_render (GtkCellRenderer *cell, GdkWindow *window,
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
	JanaGtkCellRendererNotePrivate *priv =
		CELL_RENDERER_NOTE_PRIVATE (cell);
	
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
	
	/* Draw box */
	if (priv->draw_box) {
		gtk_paint_box (style, window, state,
			(state == GTK_STATE_ACTIVE) ?
			       GTK_SHADOW_IN : GTK_SHADOW_OUT,
			expose_area, widget, priv->style_hint, x, y, w, h);
	}
	
	/* Calculate text size text */
	text_clip.x = x + priv->xpadi;
	text_clip.y = y + priv->ypadi;
	text_clip.width = w - (priv->xpadi * 2);
	text_clip.height = h - (priv->ypadi * 2);
	
	/* Draw icon */
	if (priv->icon) {
		/* FIXME: Read l10n settings for ltr to decide where to put 
		 *        icon with GTK_JUSTIFY_FILL.
		 */
		switch (priv->justify) {
		    case GTK_JUSTIFY_RIGHT :
			gdk_draw_pixbuf (window, style->black_gc, priv->icon,
				0, 0, (text_clip.x + text_clip.width) -
					 gdk_pixbuf_get_width (priv->icon),
				text_clip.y, -1, -1, GDK_RGB_DITHER_NORMAL,
				0, 0);
			text_clip.width -= gdk_pixbuf_get_width (priv->icon) +
				priv->xpadi;
			break;
		    case GTK_JUSTIFY_CENTER :
			gdk_draw_pixbuf (window, style->black_gc, priv->icon,
				0, 0, text_clip.x + (text_clip.width/2) -
					(gdk_pixbuf_get_width (priv->icon)/2),
				text_clip.y, -1, -1, GDK_RGB_DITHER_NORMAL,
				0, 0);
			text_clip.y += gdk_pixbuf_get_height (priv->icon) +
				priv->ypadi;
			text_clip.height -= gdk_pixbuf_get_height (priv->icon) +
				priv->ypadi;
			break;
		    case GTK_JUSTIFY_FILL :
		    case GTK_JUSTIFY_LEFT :
		    default :
			gdk_draw_pixbuf (window, style->black_gc, priv->icon,
				0, 0, text_clip.x, text_clip.y, -1, -1,
				GDK_RGB_DITHER_NORMAL, 0, 0);
			text_clip.x += gdk_pixbuf_get_width (priv->icon) +
				priv->xpadi;
			text_clip.width -= gdk_pixbuf_get_width (priv->icon) +
				priv->xpadi;
			break;
		}
	}
	
	/* Draw header */
	if ((text_clip.height > 0) && (text_clip.width > 0) &&
	    (layout = get_header_layout (
	    JANA_GTK_CELL_RENDERER_NOTE (cell), widget, &text_clip))) {
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
	
	/* Draw body */
	if ((text_clip.height > 0) && (text_clip.width > 0) &&
	    (layout = get_body_layout (JANA_GTK_CELL_RENDERER_NOTE (cell),
				       widget, &text_clip))) {
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
	
	/* Draw emblems */
	/* TODO: A lot of the logic here is untested... */
	if (priv->category_icon_hash && priv->categories) {
		gint i, cumu_width, cumu_height, max_height;
		
		cumu_height = cumu_width = max_height = 0;
		for (i = 0; priv->categories[i]; i++) {
			GdkPixbuf *pixbuf;
			
			pixbuf = (GdkPixbuf *)g_hash_table_lookup (
				priv->category_icon_hash, priv->categories[i]);
			if (!pixbuf) continue;
			
			w = gdk_pixbuf_get_width (pixbuf);
			h = gdk_pixbuf_get_height (pixbuf);
			x = text_clip.x + cumu_width;
			y = text_clip.y + cumu_height;
			
			if (h > max_height) max_height = h;

			if (cumu_width && (w > text_clip.width)) {
				y += max_height + priv->ypadi;
				cumu_height += h + priv->ypadi;
				x = 0;
			} else {
				cumu_width += w;
				if (cumu_width >= text_clip.width) {
					cumu_height += max_height;
					cumu_width = 0;
				} else {
					cumu_width += priv->xpadi;
				}
			}
			
			gdk_draw_pixbuf (window, style->black_gc,
				pixbuf, 0, 0, x, y,
				MIN (text_clip.width - w, w),
				MIN (text_clip.height - h, h),
				GDK_RGB_DITHER_NORMAL,
				0, 0);
			
			cumu_width += w;
		}
	}
	
	/* Detach style */
	if (priv->draw_box && priv->category_color_hash && priv->categories &&
	    priv->categories[0]) {
		gtk_style_detach (style);
	}
}

static void
cell_renderer_note_get_size (GtkCellRenderer *cell, GtkWidget *widget,
			     GdkRectangle *cell_area, gint *x_offset,
			     gint *y_offset, gint *width, gint *height)
{
	gint text_width, text_height, icon_width, icon_height;
	gboolean layout_visible;
	PangoLayout *layout;
	GdkRectangle area;
	guint xpad, ypad;

	JanaGtkCellRendererNotePrivate *priv =
		CELL_RENDERER_NOTE_PRIVATE (cell);
	
	if (cell_area) {
		/* TODO: Alignment? */
		if (x_offset) *x_offset = 0;
		if (y_offset) *y_offset = 0;
	}

	/* Ask for at least enough room to draw the box with inner padding */
	g_object_get (cell, "xpad", &xpad, "ypad", &ypad, NULL);
	if (width) *width = ((priv->xpadi + xpad)*2);
	if (height) *height = ((priv->ypadi + ypad) * 2);
	
	/* Add room for icon */
	if (priv->icon) {
		icon_width = gdk_pixbuf_get_width (priv->icon);
		icon_height = gdk_pixbuf_get_height (priv->icon);
		if (width) *width += icon_width;
		if (height) *height += icon_height;
	} else {
		icon_width = 0;
		icon_height = 0;
	}
	
	/* TODO: Width for height, natural size */
	/* Calculate available room for text */
	area.x = 0;
	area.y = 0;
	g_object_get (G_OBJECT (cell), "width", &area.width, NULL);
	if (area.width < 0) {
		if (cell_area) {
			area.width = cell_area->width -
				(priv->xpadi + xpad) * 2;
		} else if (priv->cell_width >= 0) {
			area.width = priv->cell_width -
				(priv->xpadi + xpad) * 2;
		} else {
			area.width = G_MAXINT;
		}
	} else {
		area.width -= (priv->xpadi + xpad) * 2;
	}
	area.height = (cell_area && (cell_area->height >= 0)) ?
		cell_area->height : G_MAXINT;

	if (priv->justify == GTK_JUSTIFY_CENTER) {
		if (area.height < G_MAXINT)
			area.height -= icon_height + priv->ypadi;
	} else {
		if (area.width < G_MAXINT)
			area.width -= icon_width + priv->xpadi;
	}
	
	/* Calculate text size */
	/* Header */
	layout_visible = FALSE;
	text_height = 0;
	text_width = 0;
	if ((layout = get_header_layout (JANA_GTK_CELL_RENDERER_NOTE (cell),
	     widget, &area))) {
		pango_layout_get_pixel_size (layout, &text_width, &text_height);

		if (area.width == G_MAXINT) area.width = text_width;
		
		g_object_unref (layout);
		layout_visible = TRUE;
	}
	
	/* Body */
	if ((layout = get_body_layout (
	     JANA_GTK_CELL_RENDERER_NOTE (cell), widget, &area))) {
		gint body_width, body_height;
		pango_layout_get_pixel_size (layout, &body_width, &body_height);
		text_width += body_width;
		text_height += body_height;
		g_object_unref (layout);
		layout_visible = TRUE;
	}
	
	/* Add space for text */
	if (layout_visible) {
		if (priv->icon) {
			if (priv->justify == GTK_JUSTIFY_CENTER) {
				if (*width && (area.width < 0))
					*width += MAX (
						0, text_width - icon_width);
				if (*height) *height += priv->ypadi +
					text_height;
			} else {
				if (*width && (area.width < 0))
					*width += priv->xpadi + text_width;
				if (*height) *height += MAX (
					0, text_height - icon_height);
			}
		} else {
			if (width && (area.width < 0)) *width += text_width;
			if (height) *height += text_height;
		}
	}
	
	/* Add space for emblems */
	if (height && priv->category_icon_hash && priv->categories) {
		gint i, max_height, cumu_width;

		max_height = cumu_width = 0;
		for (i = 0; priv->categories[i]; i++) {
			gint w, h;
			GdkPixbuf *pixbuf = (GdkPixbuf *)
				g_hash_table_lookup (
					priv->category_icon_hash,
					priv->categories[i]);
			if (!pixbuf) continue;

			w = gdk_pixbuf_get_width (pixbuf) + priv->xpadi;
			h = gdk_pixbuf_get_height (pixbuf) + priv->ypadi;
			
			if (!width) {
				/* Add the height of the highest emblem */
				if (h > max_height) {
					*height += h - max_height;
					max_height = h;
				}
			} else {
				/* Check the total height with wrapping */
				if (w > *width) {
					/* Image is too big to fit in bounds, 
					 * if we have icons before it, clip it 
					 * on the next row, otherwise just 
					 * stick it in and wrap immediately 
					 * after.
					 */
					cumu_width = 0;
					*height += h;
				} else {
					cumu_width += w;
					if (cumu_width + priv->xpadi > *width) {
						*height += h;
						cumu_width = w;
						max_height = h;
					} else {
						cumu_width += w;
						if (h > max_height) {
							*height +=
								h - max_height;
							max_height = h;
						}
					}
				}
			}
		}
	}
}

GtkCellRenderer*
jana_gtk_cell_renderer_note_new (void)
{
	return g_object_new (JANA_GTK_TYPE_CELL_RENDERER_NOTE, NULL);
}

