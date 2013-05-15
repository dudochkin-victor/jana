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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_GLADE
#include <gladeui/glade.h>
#endif

#include "jana-gtk-clock.h"
#include <math.h>

G_DEFINE_TYPE (JanaGtkClock, jana_gtk_clock, GTK_TYPE_EVENT_BOX)

#define CLOCK_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), JANA_GTK_TYPE_CLOCK, JanaGtkClockPrivate))

typedef struct _JanaGtkClockPrivate JanaGtkClockPrivate;

struct _JanaGtkClockPrivate
{
	gboolean digital;
	gboolean show_seconds;
	gboolean buffer_time;
	gboolean draw_shadow;
	
	cairo_surface_t *buffer;
	JanaTime *time;
	
	gboolean in;
	gboolean clicked;

	/* Variables for threaded drawing */
	GtkStyle *style;
	JanaTime *time_copy;
	GThread *draw_thread;
	gboolean dirty;
};

enum {
	PROP_TIME = 1,
	PROP_DIGITAL,
	PROP_SHOW_SECONDS,
	PROP_BUFFER_TIME,
	PROP_DRAW_SHADOW,
};

enum {
	RENDER_START,
	RENDER_STOP,
	CLICKED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
jana_gtk_clock_get_property (GObject *object, guint property_id,
			     GValue *value, GParamSpec *pspec)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (object);
	
	switch (property_id) {
	    case PROP_TIME :
		g_value_take_object (value, priv->time ?
			jana_time_duplicate (priv->time) : NULL);
		break;
	    case PROP_DIGITAL :
		g_value_set_boolean (value, priv->digital);
		break;
	    case PROP_SHOW_SECONDS :
		g_value_set_boolean (value, priv->show_seconds);
		break;
	    case PROP_BUFFER_TIME :
		g_value_set_boolean (value, priv->buffer_time);
		break;
	    case PROP_DRAW_SHADOW :
		g_value_set_boolean (value, priv->draw_shadow);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_clock_set_property (GObject *object, guint property_id,
			     const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	    case PROP_TIME :
		jana_gtk_clock_set_time (JANA_GTK_CLOCK (object),
			g_value_get_object (value));
		break;
	    case PROP_DIGITAL :
		jana_gtk_clock_set_digital (JANA_GTK_CLOCK (object),
			g_value_get_boolean (value));
		break;
	    case PROP_SHOW_SECONDS :
		jana_gtk_clock_set_show_seconds (JANA_GTK_CLOCK (object),
			g_value_get_boolean (value));
		break;
	    case PROP_BUFFER_TIME :
		jana_gtk_clock_set_buffer_time (JANA_GTK_CLOCK (object),
			g_value_get_boolean (value));
		break;
	    case PROP_DRAW_SHADOW :
		jana_gtk_clock_set_draw_shadow (JANA_GTK_CLOCK (object),
			g_value_get_boolean (value));
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
stop_draw_thread (JanaGtkClock *self)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (self);

	if (priv->draw_thread) {
		priv->dirty = TRUE;
		g_thread_join (priv->draw_thread);
		priv->dirty = FALSE;
		priv->draw_thread = NULL;
		g_object_unref (priv->style);
		priv->style = NULL;
		if (priv->time_copy) {
			g_object_unref (priv->time_copy);
			priv->time_copy = NULL;
		}
	}
}

static void
jana_gtk_clock_dispose (GObject *object)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (object);
	
	stop_draw_thread (JANA_GTK_CLOCK (object));
	
	if (priv->time) {
		g_object_unref (priv->time);
		priv->time = NULL;
	}
	
	if (G_OBJECT_CLASS (jana_gtk_clock_parent_class)->dispose)
		G_OBJECT_CLASS (jana_gtk_clock_parent_class)->dispose (object);
}

static void
jana_gtk_clock_finalize (GObject *object)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (object);
	
	if (priv->buffer) {
		cairo_surface_destroy (priv->buffer);
		priv->buffer = NULL;
	}
	
	G_OBJECT_CLASS (jana_gtk_clock_parent_class)->finalize (object);
}

static void
draw_analogue_face (JanaGtkClock *clock, cairo_t *cr, GtkStyle *style,
		    JanaTime *time)
{
	gdouble pi_ratio;
	gint width, height, size, thickness, hours, minutes, seconds;
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (clock);
	
	if (time) {
		hours = jana_time_get_hours (time);
		minutes = jana_time_get_minutes (time);
		seconds = jana_time_get_seconds (time);
	} else {
		hours = minutes = seconds = 0;
	}
	
	width = cairo_image_surface_get_width (priv->buffer);
	height = cairo_image_surface_get_height (priv->buffer);
	if (priv->draw_shadow) height -= height/20;
	size = MIN (width, height);

	gdk_cairo_set_source_color (cr, &style->fg[GTK_STATE_NORMAL]);
	cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
	cairo_set_line_width (cr, MAX (1.5, size / 60));
	thickness = size / 20;
	
	/* Draw hour hand */
	pi_ratio = ((gdouble)((hours*60)+minutes)/60.0)/6.0;
	cairo_new_path (cr);
	cairo_move_to (cr,
		(width/2) + ((size/2 - thickness/2 - size/4) *
			cos ((pi_ratio * M_PI)-(M_PI/2))),
		(height/2) + ((size/2 - thickness/2 - size/4) *
			sin ((pi_ratio * M_PI)-(M_PI/2))));
	cairo_line_to (cr,
		(width/2) + ((size/35) * cos ((pi_ratio * M_PI)-(M_PI/2))),
		(height/2) +((size/35) * sin ((pi_ratio * M_PI)-(M_PI/2))));
	cairo_close_path (cr);
	cairo_stroke (cr);

	if (priv->dirty) return;

	/* Draw minute hand */
	pi_ratio = (gdouble)minutes/30.0;
	cairo_new_path (cr);
	cairo_move_to (cr,
		(width/2) + ((size/2 - thickness/2 - size/8) *
			cos ((pi_ratio * M_PI)-(M_PI/2))),
		(height/2) + ((size/2 - thickness/2 - size/8) *
			sin ((pi_ratio * M_PI)-(M_PI/2))));
	cairo_line_to (cr,
		(width/2) + ((size/35) * cos ((pi_ratio * M_PI)-(M_PI/2))),
		(height/2) +((size/35) * sin ((pi_ratio * M_PI)-(M_PI/2))));
	cairo_close_path (cr);
	cairo_stroke (cr);

	if ((!priv->show_seconds) || priv->dirty) return;
	
	/* Draw second hand */
	gdk_cairo_set_source_color (cr, &style->bg[GTK_STATE_SELECTED]);
	cairo_set_line_width (cr, MAX (1, size / 120));
	pi_ratio = (gdouble)seconds/30.0;
	cairo_new_path (cr);
	cairo_move_to (cr,
		(width/2) + ((size/2 - thickness/2 - size/8) *
			cos ((pi_ratio * M_PI)-(M_PI/2))),
		(height/2) + ((size/2 - thickness/2 - size/8) *
			sin ((pi_ratio * M_PI)-(M_PI/2))));
	cairo_line_to (cr,
		(width/2) + ((size/35) * cos ((pi_ratio * M_PI)-(M_PI/2))),
		(height/2) +((size/35) * sin ((pi_ratio * M_PI)-(M_PI/2))));
	cairo_close_path (cr);
	cairo_stroke (cr);
}

static void
draw_analogue_clock (JanaGtkClock *clock, cairo_t *cr, GtkStyle *style)
{
	cairo_pattern_t *pattern;
	gint width, height, size, thickness, i, shadow_radius;
	double base_color[3];
	double bg_color[3];
	double fg_color[3];
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (clock);
	
	/* Draw a Tango-style analogue clock face */
	
	base_color[0] = ((double)style->bg[GTK_STATE_SELECTED].red)/
		(double)G_MAXUINT16;
	base_color[1] = ((double)style->bg[GTK_STATE_SELECTED].green)/
		(double)G_MAXUINT16;
	base_color[2] = ((double)style->bg[GTK_STATE_SELECTED].blue)/
		(double)G_MAXUINT16;

	bg_color[0] = ((double)style->base[GTK_STATE_NORMAL].red)/
		(double)G_MAXUINT16;
	bg_color[1] = ((double)style->base[GTK_STATE_NORMAL].green)/
		(double)G_MAXUINT16;
	bg_color[2] = ((double)style->base[GTK_STATE_NORMAL].blue)/
		(double)G_MAXUINT16;

	fg_color[0] = ((double)style->text[GTK_STATE_NORMAL].red)/
		(double)G_MAXUINT16;
	fg_color[1] = ((double)style->text[GTK_STATE_NORMAL].green)/
		(double)G_MAXUINT16;
	fg_color[2] = ((double)style->text[GTK_STATE_NORMAL].blue)/
		(double)G_MAXUINT16;
	
	width = cairo_image_surface_get_width (priv->buffer);
	height = cairo_image_surface_get_height (priv->buffer);
	if (priv->draw_shadow) height -= height/20;
	size = MIN (width, height);
	
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	
	if (priv->dirty) return;

	/* Draw shadow */
	if (priv->draw_shadow) {
		cairo_save (cr);
		shadow_radius = MIN (width, cairo_image_surface_get_height (
			priv->buffer))/20;
		cairo_translate (cr, width/2, height/2 + size/2);
		cairo_scale (cr, (gdouble)size /
			(gdouble)(shadow_radius*2), 1.0);
		cairo_new_path (cr);
		cairo_arc (cr, 0, 0, shadow_radius, 0, 2 * M_PI);
		cairo_close_path (cr);
		pattern = cairo_pattern_create_radial (0, 0,
			0, 0, 0, shadow_radius);
		cairo_pattern_add_color_stop_rgba (pattern, 0, 0, 0, 0, 0.5);
		cairo_pattern_add_color_stop_rgba (pattern, 1, 0, 0, 0, 0);
		cairo_set_source (cr, pattern);
		cairo_fill (cr);
		cairo_pattern_destroy (pattern);
		cairo_restore (cr);
	}
	
	if (priv->dirty) return;

	/* Draw clock face */
	thickness = size / 20;
	cairo_new_path (cr);
	cairo_arc (cr, width/2, height/2,
		size/2 - thickness/2, 0, 2 * M_PI);
	cairo_close_path (cr);
	pattern = cairo_pattern_create_radial (width/2, height/3,
		0, width/2, height/2,
		size/2 - thickness/2);
	cairo_pattern_add_color_stop_rgb (pattern, 0, bg_color[0]*2,
		bg_color[1]*2, bg_color[2]*2);
	cairo_pattern_add_color_stop_rgb (pattern, 0.3, bg_color[0],
		bg_color[1], bg_color[2]);
	cairo_pattern_add_color_stop_rgb (pattern, 1, bg_color[0]/1.15,
		bg_color[1]/1.15, bg_color[2]/1.15);
	cairo_set_source (cr, pattern);
	cairo_fill (cr);
	cairo_pattern_destroy (pattern);
	
	if (priv->dirty) return;

	/* Draw tick marks */
	cairo_set_source_rgb (cr, fg_color[0], fg_color[1], fg_color[2]);
	for (i = 0; i < 4; i++) {
		cairo_new_path (cr);
		cairo_arc (cr,
			(width/2) + ((size/2 - thickness/2 - size/6) *
				cos (i * M_PI/2)),
			(height/2) + ((size/2 - thickness/2 - size/6) *
				sin (i * M_PI/2)),
			size/40, 0, 2 * M_PI);
		cairo_close_path (cr);
		cairo_fill (cr);
	}
	
	if (priv->dirty) return;

	/* Draw centre point */
	cairo_new_path (cr);
	cairo_arc (cr, width/2, height/2, size/35, 0, 2 * M_PI);
	cairo_close_path (cr);
	cairo_set_line_width (cr, size/60);
	cairo_stroke (cr);
	
	if (priv->dirty) return;

	/* Draw internal clock-frame shadow */
	thickness = size / 20;
	cairo_new_path (cr);
	cairo_arc (cr, width/2, height/2,
		size/2 - thickness, 0, 2 * M_PI);
	cairo_close_path (cr);
	pattern = cairo_pattern_create_radial ((width/2) - (size/4),
		(height/2) - (size/4),
		0, width/2, height/2,
		size/2 - thickness/2);
	cairo_pattern_add_color_stop_rgb (pattern, 0, bg_color[0]/2,
		bg_color[1]/2, bg_color[2]/2);
	cairo_pattern_add_color_stop_rgb (pattern, 0.5, bg_color[0]/2,
		bg_color[1]/2, bg_color[2]/2);
	cairo_pattern_add_color_stop_rgb (pattern, 1, bg_color[0]*2,
		bg_color[1]*2, bg_color[2]*2);
	cairo_set_source (cr, pattern);
	cairo_set_line_width (cr, thickness);
	cairo_stroke (cr);
	cairo_pattern_destroy (pattern);
	
	if (priv->dirty) return;

	/* Draw internal clock-frame */
	cairo_new_path (cr);
	cairo_arc (cr, width/2, height/2,
		size/2 - thickness/2, 0, 2 * M_PI);
	cairo_close_path (cr);
	pattern = cairo_pattern_create_radial ((width/2) - (size/3),
		(height/2) - (size/3), 0, width/3, height/3, size/2);
	cairo_pattern_add_color_stop_rgb (pattern, 0, base_color[0]*1.2,
		base_color[1]*1.2, base_color[2]*1.2);
	cairo_pattern_add_color_stop_rgb (pattern, 0.7, base_color[0],
		base_color[1], base_color[2]);
	cairo_pattern_add_color_stop_rgb (pattern, 1, base_color[0]/1.2,
		base_color[1]/1.2, base_color[2]/1.2);
	cairo_set_source (cr, pattern);
	cairo_stroke (cr);
	cairo_pattern_destroy (pattern);
	
	if (priv->dirty) return;

	/* Dark outline frame */
	thickness = size / 60;
	cairo_new_path (cr);
	cairo_arc (cr, width/2, height/2,
		size/2 - thickness/2, 0, 2 * M_PI);
	cairo_close_path (cr);
	cairo_set_source_rgb (cr, base_color[0]/2,
		base_color[1]/2, base_color[2]/2);
	cairo_set_line_width (cr, thickness);
	cairo_stroke (cr);
	
	if (priv->dirty) return;

	/* Draw less dark inner outline frame */
	thickness = size / 40;
	cairo_new_path (cr);
	cairo_arc (cr, width/2, height/2, size/2 -
		(size/20)/2 - thickness, 0, 2 * M_PI);
	cairo_close_path (cr);
	cairo_set_source_rgb (cr, base_color[0]/1.5,
		base_color[1]/1.5, base_color[2]/1.5);
	cairo_set_line_width (cr, thickness);
	cairo_stroke (cr);
}

static void
draw_digital_number (cairo_t *cr, gint number, double *bg, double *fg)
{
	double padding = 0.01;

	/* Draw a segmented number, like on an old digital LCD display */
	
	/* Top */
	switch (number) {
	    case 0 :
	    case 2 :
	    case 3 :
	    case 5 :
	    case 6 :
	    case 7 :
	    case 8 :
	    case 9 :
		cairo_set_source_rgb (cr, fg[0], fg[1], fg[2]);
		break;	
	    default :
		cairo_set_source_rgb (cr, bg[0], bg[1], bg[2]);
		break;
	}
	cairo_new_path (cr);
	cairo_move_to (cr, 0, 0);
	cairo_line_to (cr, 8.0/8.0, 0);
	cairo_line_to (cr, 4.0/5.0 - padding, 1.0/8.0 - padding);
	cairo_line_to (cr, 1.0/5.0 + padding, 1.0/8.0 - padding);
	cairo_close_path (cr);
	cairo_fill (cr);
	
	/* Top-left */
	switch (number) {
	    case 0 :
	    case 4 :
	    case 5 :
	    case 6 :
	    case 8 :
	    case 9 :
		cairo_set_source_rgb (cr, fg[0], fg[1], fg[2]);
		break;	
	    default :
		cairo_set_source_rgb (cr, bg[0], bg[1], bg[2]);
		break;
	}
	cairo_new_path (cr);
	cairo_move_to (cr, 0, 0);
	cairo_line_to (cr, 0, 4.0/8.0);
	cairo_line_to (cr, 1.0/5.0 - padding, 3.5/8.0 - padding);
	cairo_line_to (cr, 1.0/5.0 - padding, 1.0/8.0 + padding);
	cairo_close_path (cr);
	cairo_fill (cr);
	
	/* Top-right */
	switch (number) {
	    case 0 :
	    case 1 :
	    case 2 :
	    case 3 :
	    case 4 :
	    case 7 :
	    case 8 :
	    case 9 :
		cairo_set_source_rgb (cr, fg[0], fg[1], fg[2]);
		break;	
	    default :
		cairo_set_source_rgb (cr, bg[0], bg[1], bg[2]);
		break;
	}
	cairo_new_path (cr);
	cairo_move_to (cr, 5.0/5.0, 0);
	cairo_line_to (cr, 5.0/5.0, 4.0/8.0);
	cairo_line_to (cr, 4.0/5.0 + padding, 3.5/8.0 - padding);
	cairo_line_to (cr, 4.0/5.0 + padding, 1.0/8.0 + padding);
	cairo_close_path (cr);
	cairo_fill (cr);
	
	/* Middle */
	switch (number) {
	    case 2 :
	    case 3 :
	    case 4 :
	    case 5 :
	    case 6 :
	    case 8 :
	    case 9 :
		cairo_set_source_rgb (cr, fg[0], fg[1], fg[2]);
		break;	
	    default :
		cairo_set_source_rgb (cr, bg[0], bg[1], bg[2]);
		break;
	}
	cairo_new_path (cr);
	cairo_move_to (cr, 0, 4.0/8.0);
	cairo_line_to (cr, 1.0/5.0 + padding, 3.5/8.0 + padding);
	cairo_line_to (cr, 4.0/5.0 - padding, 3.5/8.0 + padding);
	cairo_line_to (cr, 5.0/5.0, 4.0/8.0);
	cairo_line_to (cr, 4.0/5.0 - padding, 4.5/8.0 - padding);
	cairo_line_to (cr, 1.0/5.0 + padding, 4.5/8.0 - padding);
	cairo_close_path (cr);
	cairo_fill (cr);
	
	/* Bottom-left */
	switch (number) {
	    case 0 :
	    case 2 :
	    case 6 :
	    case 8 :
		cairo_set_source_rgb (cr, fg[0], fg[1], fg[2]);
		break;	
	    default :
		cairo_set_source_rgb (cr, bg[0], bg[1], bg[2]);
		break;
	}
	cairo_new_path (cr);
	cairo_move_to (cr, 0, 4.0/8.0);
	cairo_line_to (cr, 0, 8.0/8.0);
	cairo_line_to (cr, 1.0/5.0 - padding, 7.0/8.0 - padding);
	cairo_line_to (cr, 1.0/5.0 - padding, 4.5/8.0 + padding);
	cairo_close_path (cr);
	cairo_fill (cr);
	
	/* Bottom-right */
	switch (number) {
	    case 0 :
	    case 1 :
	    case 3 :
	    case 4 :
	    case 5 :
	    case 6 :
	    case 7 :
	    case 8 :
	    case 9 :
		cairo_set_source_rgb (cr, fg[0], fg[1], fg[2]);
		break;	
	    default :
		cairo_set_source_rgb (cr, bg[0], bg[1], bg[2]);
		break;
	}
	cairo_new_path (cr);
	cairo_move_to (cr, 5.0/5.0, 4.0/8.0);
	cairo_line_to (cr, 5.0/5.0, 8.0/8.0);
	cairo_line_to (cr, 4.0/5.0 + padding, 7.0/8.0 - padding);
	cairo_line_to (cr, 4.0/5.0 + padding, 4.5/8.0 + padding);
	cairo_close_path (cr);
	cairo_fill (cr);
	
	/* Bottom */
	switch (number) {
	    case 0 :
	    case 2 :
	    case 3 :
	    case 5 :
	    case 6 :
	    case 8 :
	    case 9 :
		cairo_set_source_rgb (cr, fg[0], fg[1], fg[2]);
		break;	
	    default :
		cairo_set_source_rgb (cr, bg[0], bg[1], bg[2]);
		break;
	}
	cairo_new_path (cr);
	cairo_move_to (cr, 0, 8.0/8.0);
	cairo_line_to (cr, 5.0/5.0, 8.0/8.0);
	cairo_line_to (cr, 4.0/5.0 - padding, 7.0/8.0 + padding);
	cairo_line_to (cr, 1.0/5.0 + padding, 7.0/8.0 + padding);
	cairo_close_path (cr);
	cairo_fill (cr);
}

static void
draw_digital_face (JanaGtkClock *clock, cairo_t *cr, GtkStyle *style,
		   JanaTime *time)
{
	gint x, y, width, height, thickness, hours, minutes, seconds;
	double bg_color[3];
	double fg_color[3];

	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (clock);

	if (time) {
		hours = jana_time_get_hours (time);
		minutes = jana_time_get_minutes (time);
		seconds = jana_time_get_seconds (time);
	} else {
		hours = minutes = seconds = 0;
	}

	bg_color[0] = ((double)style->text[GTK_STATE_NORMAL].red)/
		(double)G_MAXUINT16;
	bg_color[1] = ((double)style->text[GTK_STATE_NORMAL].green)/
		(double)G_MAXUINT16;
	bg_color[2] = ((double)style->text[GTK_STATE_NORMAL].blue)/
		(double)G_MAXUINT16;

	fg_color[0] = ((double)style->base[GTK_STATE_NORMAL].red)/
		(double)G_MAXUINT16;
	fg_color[1] = ((double)style->base[GTK_STATE_NORMAL].green)/
		(double)G_MAXUINT16;
	fg_color[2] = ((double)style->base[GTK_STATE_NORMAL].blue)/
		(double)G_MAXUINT16;

	height = cairo_image_surface_get_height (priv->buffer);
	if (priv->draw_shadow) height -= height/10;
	width = cairo_image_surface_get_width (priv->buffer);
	if (priv->draw_shadow) width -= width/10;
	width = MIN (width, height * 2);
	height = width / 2;
	x = (cairo_image_surface_get_width (priv->buffer) - width)/2;
	y = (cairo_image_surface_get_height (priv->buffer) - height)/2;

	thickness = width/28;

	if (priv->dirty) return;
	
	cairo_translate (cr, x + thickness*3, y + thickness*3);
	cairo_scale (cr, (double)(width - thickness*6)/5.0,
		(double)(height - thickness*6));
	
	draw_digital_number (cr, time ? hours/10 : -1, bg_color, fg_color);
	cairo_translate (cr, 1.1, 0);

	if (priv->dirty) return;

	draw_digital_number (cr, time ? hours%10 : -1, bg_color, fg_color);
	cairo_translate (cr, 1.1, 0);

	if (priv->dirty) return;
	
	/* Draw separator */
	if (time && priv->show_seconds && ((seconds % 2) == 1))
		cairo_set_source_rgb (
			cr, bg_color[0], bg_color[1], bg_color[2]);
	else
		cairo_set_source_rgb (
			cr, fg_color[0], fg_color[1], fg_color[2]);
	cairo_new_path (cr);
	cairo_rectangle (cr, 0.15, 2.0/8.0, 0.3, 1.0/8.0);
	cairo_rectangle (cr, 0.15, 5.0/8.0, 0.3, 1.0/8.0);
	cairo_fill (cr);

	if (priv->dirty) return;
	cairo_translate (cr, 0.7, 0);
	
	draw_digital_number (cr, time ? minutes/10 : -1, bg_color, fg_color);
	cairo_translate (cr, 1.1, 0);

	if (priv->dirty) return;

	draw_digital_number (cr, time ? minutes%10 : -1, bg_color, fg_color);
}

static void
draw_digital_clock (JanaGtkClock *clock, cairo_t *cr, GtkStyle *style)
{
	cairo_pattern_t *pattern;
	gint x, y, width, height, thickness, shadow_radius;
	double base_color[3];
	double bg_color[3];
	double fg_color[3];

	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (clock);
	
	/* Draw a Tango-style digital clock face */
	
	base_color[0] = ((double)style->bg[GTK_STATE_SELECTED].red)/
		(double)G_MAXUINT16;
	base_color[1] = ((double)style->bg[GTK_STATE_SELECTED].green)/
		(double)G_MAXUINT16;
	base_color[2] = ((double)style->bg[GTK_STATE_SELECTED].blue)/
		(double)G_MAXUINT16;

	bg_color[0] = ((double)style->base[GTK_STATE_NORMAL].red)/
		(double)G_MAXUINT16;
	bg_color[1] = ((double)style->base[GTK_STATE_NORMAL].green)/
		(double)G_MAXUINT16;
	bg_color[2] = ((double)style->base[GTK_STATE_NORMAL].blue)/
		(double)G_MAXUINT16;

	fg_color[0] = ((double)style->text[GTK_STATE_NORMAL].red)/
		(double)G_MAXUINT16;
	fg_color[1] = ((double)style->text[GTK_STATE_NORMAL].green)/
		(double)G_MAXUINT16;
	fg_color[2] = ((double)style->text[GTK_STATE_NORMAL].blue)/
		(double)G_MAXUINT16;
	
	height = cairo_image_surface_get_height (priv->buffer);
	if (priv->draw_shadow) height -= height/10;
	width = cairo_image_surface_get_width (priv->buffer);
	if (priv->draw_shadow) width -= width/10;
	width = MIN (width, height * 2);
	height = width / 2;
	x = (cairo_image_surface_get_width (priv->buffer) - width)/2;
	y = (cairo_image_surface_get_height (priv->buffer) - height)/2;
	
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	
	if (priv->dirty) return;
	
	cairo_translate (cr, x, y);
	
	if (priv->draw_shadow) {
		/* Draw ground shadow */
		cairo_save (cr);
		cairo_translate (cr, width/2, height);
		cairo_scale (cr, 1.0, ((double)height/(double)width)/10);
		
		cairo_new_path (cr);
		shadow_radius = ((10*width)/9)/2;
		cairo_arc (cr, 0, 0, shadow_radius, 0, 2 * M_PI);
		cairo_close_path (cr);
		pattern = cairo_pattern_create_radial (0, 0, 0,
			0, 0, shadow_radius);
		cairo_pattern_add_color_stop_rgba (pattern, 0, 0, 0, 0, 0.5);
		cairo_pattern_add_color_stop_rgba (pattern, 0.5, 0, 0, 0, 0.5);
		cairo_pattern_add_color_stop_rgba (pattern, 1, 0, 0, 0, 0);
		cairo_set_source (cr, pattern);
		cairo_fill (cr);
		cairo_pattern_destroy (pattern);
		
		cairo_restore (cr);
	}

	if (priv->dirty) return;

	/* Draw internal frame shadow */
	thickness = width/28;
	cairo_new_path (cr);
	cairo_rectangle (cr, thickness*2, thickness*2,
		width - thickness*4, height - thickness*4);
	pattern = cairo_pattern_create_radial (
		width - thickness * 4, height - thickness * 4, 0,
		width - thickness * 4, height - thickness * 4, height);
	cairo_pattern_add_color_stop_rgb (pattern, 0,
		(2*fg_color[0]+bg_color[0])/3,
		(2*fg_color[1]+bg_color[1])/3,
		(2*fg_color[2]+bg_color[2])/3);
	cairo_pattern_add_color_stop_rgb (pattern, 0.5, fg_color[0],
		fg_color[1], fg_color[2]);
	cairo_pattern_add_color_stop_rgb (pattern, 1, fg_color[0],
		fg_color[1], fg_color[2]);
	cairo_set_source (cr, pattern);
	cairo_set_line_width (cr, thickness);
	cairo_stroke (cr);
	cairo_pattern_destroy (pattern);
	
	if (priv->dirty) return;

	/* Draw clock face */
	cairo_new_path (cr);
	cairo_rectangle (cr, thickness*2.5, thickness*2.5,
		width - thickness*5, height - thickness*5);
	cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_source_rgb (cr, (15*fg_color[0]+bg_color[0])/16,
		(15*fg_color[1]+bg_color[1])/16,
		(15*fg_color[2]+bg_color[2])/16);
	cairo_set_line_width (cr, thickness/2);
	cairo_stroke_preserve (cr);
	cairo_fill (cr);

	if (priv->dirty) return;

	/* Draw dark outline frame */
	cairo_new_path (cr);
	cairo_rectangle (cr, thickness/2, thickness/2,
		width - thickness, height - thickness);
	cairo_set_line_width (cr, thickness);
	cairo_set_source_rgb (cr, base_color[0]/2,
		base_color[1]/2, base_color[2]/2);
	cairo_stroke (cr);
	
	if (priv->dirty) return;

	/* Draw main outline frame */
	cairo_new_path (cr);
	cairo_rectangle (cr, thickness, thickness,
		width - thickness*2, height - thickness*2);
	pattern = cairo_pattern_create_radial (thickness/2,
		thickness/2, 0, thickness/2, thickness/2, width);
	cairo_pattern_add_color_stop_rgb (pattern, 0, base_color[0]*1.2,
		base_color[1]*1.2, base_color[2]*1.2);
	cairo_pattern_add_color_stop_rgb (pattern, 0.7, base_color[0],
		base_color[1], base_color[2]);
	cairo_pattern_add_color_stop_rgb (pattern, 1, base_color[0]/1.2,
		base_color[1]/1.2, base_color[2]/1.2);
	cairo_set_source (cr, pattern);
	cairo_stroke (cr);
	cairo_pattern_destroy (pattern);

	if (priv->dirty) return;

	/* Draw less dark inner outline frame */
	cairo_new_path (cr);
	cairo_rectangle (cr, thickness*1.5, thickness*1.5,
		width - thickness*3, height - thickness*3);
	cairo_set_source_rgb (cr, base_color[0]/1.5,
		base_color[1]/1.5, base_color[2]/1.5);
	cairo_set_line_width (cr, thickness/2);
	cairo_stroke (cr);
}

static gboolean
jana_gtk_clock_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (widget);
	
	/* Don't draw anything until the entire clock is ready to draw */
	if (!priv->draw_thread) {
		cairo_t *cr = gdk_cairo_create (widget->window);
		cairo_translate (cr, widget->allocation.x,
			widget->allocation.y);

		/* Draw background */
		if (priv->buffer) {
			cairo_set_source_surface (cr, priv->buffer, 0, 0);
			cairo_paint_with_alpha (cr, 1.0);
		}

		/* Draw face */
		if (!priv->buffer_time) {
			if (priv->digital)
				draw_digital_face ((JanaGtkClock *)widget, cr,
					widget->style, priv->time);
			else
				draw_analogue_face ((JanaGtkClock *)widget, cr,
					widget->style, priv->time);
		}
		
		cairo_destroy (cr);
	}
	
	return GTK_WIDGET_CLASS (jana_gtk_clock_parent_class)->
		expose_event (widget, event);
}

static gboolean
idle_redraw (GtkWidget *widget)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (widget);
	
	priv->draw_thread = NULL;
	
	g_signal_emit (widget, signals[RENDER_STOP], 0);
	gtk_widget_queue_draw (widget);
	
	return FALSE;
}

static gpointer
draw_clock (JanaGtkClock *self)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (self);
	cairo_t *cr = cairo_create (priv->buffer);
	
	if (priv->digital) {
		draw_digital_clock (self, cr, GTK_WIDGET (self)->style);
		if (priv->buffer_time && (!priv->dirty))
			draw_digital_face (self, cr,
				GTK_WIDGET (self)->style, priv->time_copy);
	} else {
		draw_analogue_clock (self, cr, GTK_WIDGET (self)->style);
		if (priv->buffer_time && (!priv->dirty))
			draw_analogue_face (self, cr,
				GTK_WIDGET (self)->style, priv->time_copy);
	}

	cairo_destroy (cr);

	if (!priv->dirty) {
		g_idle_add_full (G_PRIORITY_HIGH_IDLE,
			(GSourceFunc)idle_redraw, self, NULL);
	}

	return NULL;
}

static void
refresh_buffer (JanaGtkClock *self)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (self);
	
	if ((!GTK_WIDGET_MAPPED (self)) || (!priv->buffer)) return;
	
	stop_draw_thread (self);
	priv->style = gtk_style_copy (GTK_WIDGET (self)->style);
	priv->time_copy = priv->time ? jana_time_duplicate (priv->time) : NULL;
	g_signal_emit (self, signals[RENDER_START], 0);
	priv->draw_thread = g_thread_create ((GThreadFunc)draw_clock,
		self, TRUE, NULL);
}

static void
jana_gtk_clock_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (widget);

	/* Note: Calling this after the below breaks things, not sure why */
	GTK_WIDGET_CLASS (jana_gtk_clock_parent_class)->
		size_allocate (widget, allocation);

	if (!GTK_WIDGET_REALIZED (widget)) gtk_widget_realize (widget);

	if ((!priv->buffer) || (allocation->width !=
	     cairo_image_surface_get_width (priv->buffer)) ||
	    (allocation->height !=
	     cairo_image_surface_get_height (priv->buffer))) {
		stop_draw_thread (JANA_GTK_CLOCK (widget));
		if (priv->buffer) cairo_surface_destroy (priv->buffer);
		priv->buffer = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
			allocation->width, allocation->height);
		refresh_buffer (JANA_GTK_CLOCK (widget));
	}
	
}

static void
jana_gtk_clock_map (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (jana_gtk_clock_parent_class)->map (widget);
	GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
	refresh_buffer (JANA_GTK_CLOCK (widget));
}

static void
jana_gtk_clock_unmap (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (jana_gtk_clock_parent_class)->unmap (widget);

	GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
}

static void
jana_gtk_clock_style_set (GtkWidget *widget, GtkStyle *previous_style)
{
	GTK_WIDGET_CLASS (jana_gtk_clock_parent_class)->
		style_set (widget, previous_style);
	
	refresh_buffer (JANA_GTK_CLOCK (widget));
}

static gboolean
jana_gtk_clock_enter_notify_event (GtkWidget *widget, GdkEventCrossing *event)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (widget);

	priv->in = TRUE;

	return FALSE;
}

static gboolean
jana_gtk_clock_leave_notify_event (GtkWidget *widget, GdkEventCrossing *event)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (widget);

	priv->in = FALSE;

	return FALSE;
}

static gboolean
jana_gtk_clock_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (widget);

	priv->clicked = TRUE;

	return FALSE;
}

static gboolean
jana_gtk_clock_button_release_event (GtkWidget *widget, GdkEventButton *event)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (widget);

	if (priv->clicked && priv->in)
		g_signal_emit (widget, signals[CLICKED], 0, event);
	priv->clicked = FALSE;

	return FALSE;
}

static void
jana_gtk_clock_class_init (JanaGtkClockClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	g_type_class_add_private (klass, sizeof (JanaGtkClockPrivate));

	object_class->get_property = jana_gtk_clock_get_property;
	object_class->set_property = jana_gtk_clock_set_property;
	object_class->dispose = jana_gtk_clock_dispose;
	object_class->finalize = jana_gtk_clock_finalize;
	
	widget_class->expose_event = jana_gtk_clock_expose_event;
	widget_class->size_allocate = jana_gtk_clock_size_allocate;
	widget_class->style_set = jana_gtk_clock_style_set;
	widget_class->map = jana_gtk_clock_map;
	widget_class->unmap = jana_gtk_clock_unmap;
	widget_class->enter_notify_event = jana_gtk_clock_enter_notify_event;
	widget_class->leave_notify_event = jana_gtk_clock_leave_notify_event;
	widget_class->button_press_event = jana_gtk_clock_button_press_event;
	widget_class->button_release_event =
		jana_gtk_clock_button_release_event;

	g_object_class_install_property (
		object_class,
		PROP_TIME,
		g_param_spec_object (
			"time",
			"Time",
			"The JanaTime the clock will show.",
			G_TYPE_OBJECT,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_DIGITAL,
		g_param_spec_boolean (
			"digital",
			"Digital",
			"Whether to show a digital clock, as opposed to an "
			"analogue clock.",
			FALSE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_SHOW_SECONDS,
		g_param_spec_boolean (
			"show_seconds",
			"Show seconds",
			"Whether to show seconds on the clock face.",
			FALSE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_BUFFER_TIME,
		g_param_spec_boolean (
			"buffer_time",
			"Buffer time",
			"Whether to double-buffer the time. Set this to "
			"%TRUE if you don't expect to change the time "
			"frequently.",
			FALSE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_DRAW_SHADOW,
		g_param_spec_boolean (
			"draw_shadow",
			"Draw shadow",
			"Whether to draw a shadow underneath the clock.",
			FALSE,
			G_PARAM_READWRITE));

	signals[RENDER_START] =
		g_signal_new ("render_start",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaGtkClockClass,
					 render_start),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	signals[RENDER_STOP] =
		g_signal_new ("render_stop",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaGtkClockClass,
					 render_stop),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	signals[CLICKED] =
		g_signal_new ("clicked",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaGtkClockClass,
					 clicked),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
jana_gtk_clock_init (JanaGtkClock *self)
{
	if (!g_thread_supported ()) g_thread_init (NULL);

	gtk_widget_add_events (GTK_WIDGET (self),
		GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
	gtk_widget_set_app_paintable (GTK_WIDGET (self), TRUE);
	gtk_event_box_set_visible_window (GTK_EVENT_BOX (self), FALSE);
}

GtkWidget *
jana_gtk_clock_new (void)
{
	return GTK_WIDGET (g_object_new (JANA_GTK_TYPE_CLOCK, NULL));
}

void
jana_gtk_clock_set_time (JanaGtkClock *self, JanaTime *time)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (self);
	
	if (priv->time) {
		g_object_unref (priv->time);
		priv->time = NULL;
	}
	if (time) priv->time = jana_time_duplicate (time);
	if (priv->buffer_time) refresh_buffer (self);
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

JanaTime *
jana_gtk_clock_get_time (JanaGtkClock *self)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (self);
	
	return priv->time ? jana_time_duplicate (priv->time) : NULL;
}

void
jana_gtk_clock_set_digital (JanaGtkClock *self, gboolean digital)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (self);
	
	if (priv->digital != digital) {
		priv->digital = digital;
		refresh_buffer (self);
		gtk_widget_queue_draw (GTK_WIDGET (self));
	}
}

gboolean
jana_gtk_clock_get_digital (JanaGtkClock *self)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (self);
	return priv->digital;
}

void
jana_gtk_clock_set_show_seconds (JanaGtkClock *self, gboolean show_seconds)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (self);
	
	if (priv->show_seconds != show_seconds) {
		priv->show_seconds = show_seconds;
		if (priv->buffer_time) refresh_buffer (self);
		gtk_widget_queue_draw (GTK_WIDGET (self));
	}
}

gboolean
jana_gtk_clock_get_show_seconds (JanaGtkClock *self)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (self);
	return priv->show_seconds;
}

void
jana_gtk_clock_set_buffer_time (JanaGtkClock *self, gboolean buffer_time)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (self);
	
	if (priv->buffer_time != buffer_time) {
		priv->buffer_time = buffer_time;
		refresh_buffer (self);
		gtk_widget_queue_draw (GTK_WIDGET (self));
	}
}

gboolean
jana_gtk_clock_get_buffer_time (JanaGtkClock *self)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (self);
	return priv->buffer_time;
}

void
jana_gtk_clock_set_draw_shadow (JanaGtkClock *self, gboolean draw_shadow)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (self);
	
	if (priv->draw_shadow != draw_shadow) {
		priv->draw_shadow = draw_shadow;
		refresh_buffer (self);
		gtk_widget_queue_draw (GTK_WIDGET (self));
	}
}

gboolean
jana_gtk_clock_get_draw_shadow (JanaGtkClock *self)
{
	JanaGtkClockPrivate *priv = CLOCK_PRIVATE (self);
	return priv->draw_shadow;
}

#ifdef HAVE_GLADE
/* The following is a nasty hack to get the widget to display correctly in 
 * Glade-3.
 */
static void
placeholder_realize_cb (GtkWidget *widget)
{
	GTK_WIDGET_SET_FLAGS (widget, GTK_NO_WINDOW);
	if (widget->window) g_object_unref (widget->window);
	widget->window = g_object_ref (gtk_widget_get_parent (widget)->window);
	gtk_widget_queue_draw (gtk_widget_get_parent (widget));
}

static void
jana_gtk_clock_glade_add_cb (GtkContainer *container,
			     GtkWidget    *widget,
			     gpointer      user_data)
{
	if (!GLADE_IS_PLACEHOLDER (widget)) return;
	
	g_signal_connect_after (widget, "realize",
		G_CALLBACK (placeholder_realize_cb), NULL);

	if (GTK_WIDGET_REALIZED (widget))
		placeholder_realize_cb (widget);
}

static void
jana_gtk_clock_glade_remove_cb (GtkContainer *container,
				GtkWidget    *widget,
				gpointer      user_data)
{
	if (!GLADE_IS_PLACEHOLDER (widget)) return;
	
	g_signal_handlers_disconnect_by_func (
		widget, placeholder_realize_cb, NULL);
}

void
jana_gtk_clock_glade_post_create (GladeWidgetAdaptor *adaptor,
				  GObject	     *object,
				  GladeCreateReason   reason)
{
	GList *children;
	g_return_if_fail (GTK_IS_CONTAINER (object));

	g_signal_connect (object, "add",
		G_CALLBACK (jana_gtk_clock_glade_add_cb), NULL);
	g_signal_connect (object, "remove",
		G_CALLBACK (jana_gtk_clock_glade_remove_cb), NULL);
	
	if (reason == GLADE_CREATE_USER) {
		GtkContainer *container = GTK_CONTAINER (object);
		children = gtk_container_get_children (container);
		if (!children)
			gtk_container_add (container, glade_placeholder_new ());
		else
			g_list_free (children);
	}
}
#endif
