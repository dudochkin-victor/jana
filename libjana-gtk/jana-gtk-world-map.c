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

#include "jana-gtk-world-map.h"
#include <libjana/jana-utils.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

G_DEFINE_TYPE (JanaGtkWorldMap, jana_gtk_world_map, GTK_TYPE_EVENT_BOX)

#define WORLD_MAP_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), JANA_GTK_TYPE_WORLD_MAP, \
	JanaGtkWorldMapPrivate))

typedef struct _JanaGtkWorldMapPrivate JanaGtkWorldMapPrivate;

struct _JanaGtkWorldMapPrivate
{
	JanaTime *time;
	
	gint width;
	gint height;
	gboolean scale;
	
	gboolean clicked;
	gboolean in;
	
	/* Data read from the VMF file */
	gdouble phi_min;
	gdouble phi_max;
	gdouble theta_min;
	gdouble theta_max;
	
	/* An array of polygon co-ordinate pairs, separated by NULLs and 
	 * terminated with two NULLs.
	 * i.e. { {0, 0}, {0, 1}, {1, 1}, NULL, NULL }
	 */
	gdouble **map;
	cairo_surface_t *buffer;

	GPtrArray *marks;

	GtkStyle *style;
	JanaTime *time_copy;
	GThread *draw_thread;
	gboolean dirty;
};

enum {
	PROP_TIME = 1,
	PROP_WIDTH,
	PROP_HEIGHT,
	PROP_STATIC
};

enum {
	RENDER_START,
	RENDER_STOP,
	CLICKED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/*
 * Very basic vmf file reader. It ignores all colour data, open curves, labels,
 * etc. It just reads in the closed, filled curves, the range of the map and 
 * ignores everything else. The code in sunclock is pretty much unreadable, 
 * so wrote my own.
 */
#define VMF_LEN 256
#define VMF_ALLOC 5000
static gboolean
read_vmf (JanaGtkWorldMap *self)
{
	FILE *vmf;
	char string[VMF_LEN];
	gint i;
	glong points, alloc_points;
	gboolean fill;
	gboolean closed;
	
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (self);

	vmf = fopen (PKGDATADIR G_DIR_SEPARATOR_S "landwater.vmf", "rb");
	if (!vmf) {
		g_warning ("Background data not found");
		return FALSE;
	}
	
	/* Check file header */
	fgets (string, VMF_LEN, vmf);
	if (strncmp (string, "%!VMF", 5) != 0) goto vmf_error;
	
	/* Skip past colours and palette */
	for (i = 0; i < 2; i++) do {
		if (!fgets (string, VMF_LEN, vmf)) goto vmf_error;
	} while (string[0] != ';');
	
	/* Get range */
	do {
		if (!fgets (string, VMF_LEN, vmf)) goto vmf_error;
	} while (strncmp (string, "range ", 6) != 0);
	if (sscanf (string, "range %lg %lg %lg %lg", &priv->phi_min, &priv->phi_max,
		&priv->theta_min, &priv->theta_max) != 4) goto vmf_error;
	
	/* Read data */
	closed = FALSE;
	fill = TRUE;
	alloc_points = points = 0;
	while (fgets (string, VMF_LEN, vmf)) {
		if (strncmp (string, "closedcurves", 12) == 0) {
			closed = TRUE;
		} else if (strncmp (string, "opencurves", 10) == 0) {
			closed = FALSE;
		} else if (strncmp (string, "fillmode", 8) == 0) {
			gint fillmode;
			if (sscanf (string, "fillmode %d", &fillmode) == 1)
				fill = (fillmode == 2) ? TRUE : FALSE;
		} else if ((string[0] == '#') && closed && fill) {
			/* Read co-ordinate pairs for polygon */
			gboolean first_point = TRUE;
			if (!fgets (string, VMF_LEN, vmf)) goto vmf_error;
			i = 0;
			while (string[0] != ';') {
				gint j;
				gdouble x, y;
				
				gchar *substring = g_strchug (string);
				
				while (substring && (substring[0] != '\0')) {
					for (j = 0;
					     (substring[j] != ' ') &&
					     (substring[j] != '\0') &&
					     (substring[j] != '\n')&&
					     (substring[j] != '%');
					     j++);
					
					substring[j] = '\0';
					if (sscanf (substring, "%lg",
					    i ? &y : &x) != 1) goto vmf_error;
					
					if (i == 1) {
						/* Normalise theta */
						/* No idea if this is the 
						 * correct way to do this, but
						 * it seems to work correctly
						 */
						if ((!first_point) && (y < 0)) {
							gdouble y2 = y + (
							    priv->theta_max -
							    priv->theta_min);
							if (ABS (priv->map[
							    points-1][1] - y) >
							    ABS (priv->map[
							    points-1][1] - y2))
								y = y2;
						}
						
						/* Allocate memory and
						 * store points - Always 
						 * allocate at least 1 more 
						 * point than is necessary for 
						 * the ending marker.
						 */
						points ++;
						if ((points+1) > alloc_points) {
							priv->map = g_realloc (
								priv->map,
								sizeof (
								gdouble **) *
								(alloc_points+
								VMF_ALLOC));
							alloc_points +=
								VMF_ALLOC;
						}
						priv->map[points-1] = g_malloc (
							sizeof (gdouble) * 2);
						priv->map[points-1][0] = x;
						priv->map[points-1][1] = y;
						i = 0;
						first_point = FALSE;
					} else
						i++;
					
					/* Move onto next value */
					substring = g_strchug (substring+j+1);
				}
				
				if (!fgets (string, VMF_LEN, vmf))
					goto vmf_error;
			}
			if (i != 0) g_warning ("Floating co-ordinate");
			
			/* End polygon */
			points ++;
			priv->map[points-1] = NULL;
			
		} else if (string[0] == '#') do {
			/* Skip polygon */
			if (!fgets (string, VMF_LEN, vmf)) goto vmf_error;
		} while (string[0] != ';');
	}
	
	/* Terminate polygon list and resize allocated memory to what is 
	 * needed.
	 */
	points ++;
	if (alloc_points != points)
		priv->map = g_realloc (priv->map,
			sizeof (gdouble **) * points);
	priv->map[points-1] = NULL;
	
	fclose (vmf);
	
	return TRUE;
	
vmf_error:
	g_warning ("Error reading .vmf file");
	fclose (vmf);
	return FALSE;
}

static void
jana_gtk_world_map_get_property (GObject *object, guint property_id,
				 GValue *value, GParamSpec *pspec)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (object);
	
	switch (property_id) {
	    case PROP_TIME :
		g_value_take_object (value, priv->time ?
			jana_time_duplicate (priv->time) : NULL);
		break;
	    case PROP_STATIC :
		g_value_set_boolean (value, priv->scale);
		break;
	    case PROP_WIDTH :
		g_value_set_int (value, priv->width);
		break;
	    case PROP_HEIGHT :
		g_value_set_int (value, priv->height);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_world_map_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	    case PROP_TIME :
		jana_gtk_world_map_set_time (JANA_GTK_WORLD_MAP (object),
			JANA_TIME (g_value_get_object (value)));
		break;
	    case PROP_STATIC :
		jana_gtk_world_map_set_static (JANA_GTK_WORLD_MAP (object),
			g_value_get_boolean (value));
		break;
	    case PROP_WIDTH :
		jana_gtk_world_map_set_width (JANA_GTK_WORLD_MAP (object),
			g_value_get_int (value));
		break;
	    case PROP_HEIGHT :
		jana_gtk_world_map_set_height (JANA_GTK_WORLD_MAP (object),
			g_value_get_int (value));
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
stop_draw_thread (JanaGtkWorldMap *self)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (self);

	if (priv->draw_thread) {
		g_thread_join (priv->draw_thread);
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
jana_gtk_world_map_dispose (GObject *object)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (object);
	
	stop_draw_thread (JANA_GTK_WORLD_MAP (object));

	if (priv->time) {
		g_object_unref (priv->time);
		priv->time = NULL;
	}
	
	if (G_OBJECT_CLASS (jana_gtk_world_map_parent_class)->dispose)
		G_OBJECT_CLASS (jana_gtk_world_map_parent_class)->
			dispose (object);
}

static void
jana_gtk_world_map_finalize (GObject *object)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (object);
	
	if (priv->map) {
		gint i;
		for (i = 0; priv->map[i] && priv->map[i+1]; i++)
			if (priv->map[i]) g_free (priv->map[i]);
		g_free (priv->map);
		priv->map = NULL;
	}
	
	if (priv->buffer) {
		cairo_surface_destroy (priv->buffer);
		priv->buffer = NULL;
	}

	G_OBJECT_CLASS (jana_gtk_world_map_parent_class)->finalize (object);
}

static gboolean
idle_redraw (GtkWidget *widget)
{
	g_signal_emit (widget, signals[RENDER_STOP], 0);
	gtk_widget_queue_draw (widget);
	return FALSE;
}

static gpointer
draw_map (JanaGtkWorldMap *self)
{
	cairo_pattern_t *bg_pattern;
	double base_color[3], bg_color[3], fg_color[3], mid_color[3];
	gint width, height;

	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (self);
	cairo_t *cr = cairo_create (priv->buffer);

	width = cairo_image_surface_get_width (priv->buffer);
	height = cairo_image_surface_get_height (priv->buffer);

	/* Draw background */
	base_color[0] = ((double)priv->style->bg[GTK_STATE_SELECTED].red)/
		(double)G_MAXUINT16;
	base_color[1] = ((double)priv->style->bg[GTK_STATE_SELECTED].green)/
		(double)G_MAXUINT16;
	base_color[2] = ((double)priv->style->bg[GTK_STATE_SELECTED].blue)/
		(double)G_MAXUINT16;

	bg_color[0] = ((double)priv->style->base[GTK_STATE_NORMAL].red)/
		(double)G_MAXUINT16;
	bg_color[1] = ((double)priv->style->base[GTK_STATE_NORMAL].green)/
		(double)G_MAXUINT16;
	bg_color[2] = ((double)priv->style->base[GTK_STATE_NORMAL].blue)/
		(double)G_MAXUINT16;

	fg_color[0] = ((double)priv->style->text[GTK_STATE_NORMAL].red)/
		(double)G_MAXUINT16;
	fg_color[1] = ((double)priv->style->text[GTK_STATE_NORMAL].green)/
		(double)G_MAXUINT16;
	fg_color[2] = ((double)priv->style->text[GTK_STATE_NORMAL].blue)/
		(double)G_MAXUINT16;
	
	mid_color[0] = (base_color[0] + bg_color[0]) / 2;
	mid_color[1] = (base_color[1] + bg_color[1]) / 2;
	mid_color[2] = (base_color[2] + bg_color[2]) / 2;

	bg_pattern = cairo_pattern_create_linear (0, 0, 0, height);
	cairo_pattern_add_color_stop_rgb (bg_pattern, 0, base_color[0] * 1.1,
		base_color[1] * 1.1, base_color[2] * 1.1);
	cairo_pattern_add_color_stop_rgb (bg_pattern, 0.5, mid_color[0] * 1.05,
		mid_color[1] * 1.05, mid_color[2] * 1.05);
	cairo_pattern_add_color_stop_rgb (bg_pattern, 0.5, mid_color[0],
		mid_color[1], mid_color[2]);
	cairo_pattern_add_color_stop_rgb (bg_pattern, 1, base_color[0],
		base_color[1], base_color[2]);
	cairo_set_source (cr, bg_pattern);
	cairo_paint (cr);
	cairo_pattern_destroy (bg_pattern);

	if (priv->map && priv->map[0]) {
		/*cairo_pattern_t *pattern;*/
		double scale_x, scale_y;
		gint i, j;

		cairo_translate (cr, width/2, height/2);
		cairo_rotate (cr, -M_PI/2);
		scale_x = (double)height /
			(double)(priv->phi_max - priv->phi_min);
		scale_y = (double)width /
			(double)(priv->theta_max - priv->theta_min);
		cairo_scale (cr, scale_x, scale_y);
		
		/*pattern = cairo_pattern_create_radial (priv->phi_max,
			0, 0, priv->phi_max, 0,
			priv->phi_max - priv->phi_min);
		cairo_pattern_add_color_stop_rgb (pattern, 0, bg_color[0] * 1.2,
			bg_color[1] * 1.2, bg_color[2] * 1.2);
		cairo_pattern_add_color_stop_rgb (pattern, 1, bg_color[0] * 0.9,
			bg_color[1] * 0.9, bg_color[2] * 0.9);*/

		/* Loop through polygon co-ordinates and draw */
		for (i = 0; i < 2; i++) {
			cairo_save (cr);
			if (i == 0) {
				/* First run, draw shadow */
				cairo_translate (cr,
					-(priv->phi_max-priv->phi_min)/180,
					(priv->theta_max-priv->theta_min)/360);
				cairo_set_source_rgb (cr, fg_color[0],
					fg_color[1], fg_color[2]);
			} else {
				/*cairo_set_source (cr, pattern);*/
				cairo_set_source_rgb (cr, bg_color[0],
					bg_color[1], bg_color[2]);
			}
			for (j = 0; priv->map[j]; j++) {
				gint k, skip;
				cairo_rectangle_t bounds = {
					priv->phi_max, priv->theta_max,
					priv->phi_min, priv->theta_min };

				/* Count the amount of points in this poly and 
				 * get bounding rectangle
				 */
				for (k = j; priv->map[k]; k++) {
					if (priv->map[k][0] < bounds.x)
						bounds.x = priv->map[k][0];
					if (priv->map[k][1] < bounds.y)
						bounds.y = priv->map[k][1];
					if (priv->map[k][0] > bounds.width)
						bounds.width = priv->map[k][0];
					if (priv->map[k][1] > bounds.height)
						bounds.height = priv->map[k][1];
				}
				bounds.width -= bounds.x;
				bounds.height -= bounds.y;
				
				/* If the polygon is tiny, skip it entirely */
				if ((bounds.width < 1) || (bounds.height < 1)) {
					j = k;
					continue;
				}
				
				/* Decide how many points we can skip depending 
				 * on the length of perimiter of the 
				 * bounding rectangle, multiplied by 1.5.
				 * We at least always draw a triangle.
				 */
				skip = MAX (1, MIN ((k-j)/3,
					(k-j) / (((bounds.width*1.5)+
					(bounds.height*1.5)) *
					MAX (scale_x, scale_y))));

				cairo_new_path (cr);
				cairo_move_to (cr, priv->map[j][0],
					priv->map[j][1]);
				for (j = j + 1; j < k; j += skip) {
					cairo_line_to (cr, priv->map[j][0],
						priv->map[j][1]);
				}
				j = k;
				cairo_close_path (cr);
				cairo_fill (cr);
				
				if (priv->dirty) break;
			}
			cairo_restore (cr);
			if (priv->dirty) break;
		}
		
		/*cairo_pattern_destroy (pattern);*/

		if (priv->time_copy && (!priv->dirty)) {
			/* Draw daylight hours */
			gdouble lat, time_offset, lon, prev_hours = 0;
			gboolean first = TRUE;
			cairo_path_t *path_copy, *path;
			guint day;

			const cairo_matrix_t flip_matrix =
				{ 1, 0,
				  0, -1,
				  0, 0 };
			
			/* Create curve - See:
			 * http://mathforum.org/library/drmath/view/56478.html
			 * for an explanation of the formula used.
			 */
			day = jana_utils_time_day_of_year (priv->time_copy);
			cairo_set_line_width (cr, 1.0);
			
			cairo_new_path (cr);
			cairo_line_to (cr, priv->phi_min, 0);
			for (lat = priv->phi_min; lat < priv->phi_max; lat+=1) {
				gdouble degrees;
				gdouble hours = jana_utils_time_daylight_hours (
					lat, day);
				
				if (isnan (hours)) {
					gdouble flat = lat - 1;
					while (first) {
						/* Read ahead and find 
						 * the next daylight
						 */
						prev_hours =
						jana_utils_time_daylight_hours (
							flat, day);
						flat += 1;
						if (!isnan (prev_hours))
							first = FALSE;
					}
					hours = (prev_hours > 12) ? 24.0 : 0.0;
				}
				
				degrees = (hours/24.0) * 360;
				cairo_line_to (cr, lat, -degrees/2);
				prev_hours = hours;
				first = FALSE;
			}
			cairo_line_to (cr, priv->phi_max, 0);
			cairo_line_to (cr, priv->phi_max, priv->theta_min);
			cairo_line_to (cr, priv->phi_min, priv->theta_min);

			path_copy = cairo_copy_path (cr);
			/* Flip along the vertical axis and draw path again,
			 * then make a copy of the new, full curve so that we 
			 * can draw it repeated.
			 */
			cairo_transform (cr, &flip_matrix);
			cairo_append_path (cr, path_copy);
			path = cairo_copy_path (cr);
			cairo_path_destroy (path_copy);
			
			/* Calculate midday offset */
			time_offset =
				(((((gdouble)jana_time_get_hours (
					priv->time_copy) * 60 * 60) +
				((gdouble)jana_time_get_minutes (
					priv->time_copy) * 60) +
				(gdouble)jana_time_get_seconds (
					priv->time_copy)) / (24.0 * 60.0 *
					60.0)) * 360.0) - 180.0;
			
			/* Draw repeated curve */
			lon = (time_offset > 0) ?
				time_offset - 360 : time_offset;
			cairo_translate (cr, 0, lon);
			cairo_new_path (cr);
			for (; lon < (priv->theta_max - priv->theta_min);
			     lon += 360) {
				cairo_append_path (cr, path);
				cairo_translate (cr, 0, 360);
			}
			cairo_set_source_rgba (cr, 0, 0, 0, 0.5);
			cairo_fill (cr);
			
			cairo_path_destroy (path);
		}
	}
	
	cairo_destroy (cr);
	if (!priv->dirty) {
		g_idle_add_full (G_PRIORITY_HIGH_IDLE,
			(GSourceFunc)idle_redraw, self, NULL);
	}
	
	return NULL;
}

static void
draw_marks (JanaGtkWorldMap *self, cairo_t *cr)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (self);
	GtkWidget *widget = GTK_WIDGET (self);
	int i;

	for (i = 0; i < priv->marks->len; i++) {
		JanaGtkWorldMapMarker *marker = priv->marks->pdata[i];
		gint x, y;

		jana_gtk_world_map_get_xy (self,
			marker->lat, marker->lon, &x, &y);
		
		cairo_save (cr);
		jana_gtk_world_map_marker_render (marker, widget->style, cr,
			GTK_WIDGET_STATE (widget), x, y);
		cairo_restore (cr);
	}
}

static gboolean
jana_gtk_world_map_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	cairo_t *cr = gdk_cairo_create (widget->window);
	
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (widget);
	
	cairo_translate (cr, widget->allocation.x, widget->allocation.y);
	if (priv->buffer) {
		cairo_pattern_t *pattern = cairo_pattern_create_for_surface (
			priv->buffer);
		if (priv->scale) {
			cairo_matrix_t matrix;
			cairo_matrix_init_scale (&matrix,
				(priv->width > 0) ?
				((double)priv->width/
				 (double)widget->allocation.width) : 1.0,
				(priv->height > 0) ?
				((double)priv->height/
				 (double)widget->allocation.height) : 1.0);
			cairo_pattern_set_matrix (pattern, &matrix);
		}
		cairo_set_source (cr, pattern);
		cairo_paint (cr);
		cairo_pattern_destroy (pattern);
	}

	draw_marks ((JanaGtkWorldMap *)widget, cr);

	cairo_destroy (cr);
	
	return GTK_WIDGET_CLASS (jana_gtk_world_map_parent_class)->
		expose_event (widget, event);
}

static void
refresh_buffer (JanaGtkWorldMap *self)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (self);

	priv->dirty = TRUE;
	if (!GTK_WIDGET_MAPPED (self)) return;
	
	stop_draw_thread (self);
	priv->dirty = FALSE;
	
	if (!priv->buffer) return;
	
	priv->style = gtk_style_copy (GTK_WIDGET (self)->style);
	priv->time_copy = priv->time ? jana_time_duplicate (priv->time) : NULL;
	g_signal_emit (self, signals[RENDER_START], 0);
	priv->draw_thread = g_thread_create ((GThreadFunc)draw_map,
		self, TRUE, NULL);
}

static void
jana_gtk_world_map_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	gint width, height;

	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (widget);

	GTK_WIDGET_CLASS (jana_gtk_world_map_parent_class)->
		size_allocate (widget, allocation);

	if (!GTK_WIDGET_REALIZED (widget)) gtk_widget_realize (widget);

	if (priv->scale) {
		width = (priv->width > 0) ? priv->width : allocation->width;
		height = (priv->height > 0) ? priv->height : allocation->height;
	} else {
		width = allocation->width;
		height = allocation->height;
	}
	
	if ((!priv->buffer) || (width !=
	     cairo_image_surface_get_width (priv->buffer)) ||
	    (height !=
	     cairo_image_surface_get_height (priv->buffer))) {
		stop_draw_thread (JANA_GTK_WORLD_MAP (widget));
		if (priv->buffer) cairo_surface_destroy (priv->buffer);
		priv->buffer = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
			width, height);
		refresh_buffer (JANA_GTK_WORLD_MAP (widget));
	}
}

static gboolean
jana_gtk_world_map_map_event (GtkWidget *widget, GdkEventAny *event)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (widget);
	
	if (priv->dirty) refresh_buffer (JANA_GTK_WORLD_MAP (widget));
	
	return FALSE;
}

static void
jana_gtk_world_map_style_set (GtkWidget *widget, GtkStyle *previous_style)
{
	GTK_WIDGET_CLASS (jana_gtk_world_map_parent_class)->
		style_set (widget, previous_style);
	
	refresh_buffer (JANA_GTK_WORLD_MAP (widget));
}

static gboolean
jana_gtk_world_map_enter_notify_event (GtkWidget *widget,
				       GdkEventCrossing *event)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (widget);

	priv->in = TRUE;

	return FALSE;
}

static gboolean
jana_gtk_world_map_leave_notify_event (GtkWidget *widget,
				       GdkEventCrossing *event)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (widget);

	priv->in = FALSE;

	return FALSE;
}

static gboolean
jana_gtk_world_map_button_press_event (GtkWidget *widget,
				       GdkEventButton *event)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (widget);

	priv->clicked = TRUE;

	return FALSE;
}

static gboolean
jana_gtk_world_map_button_release_event (GtkWidget *widget,
					 GdkEventButton *event)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (widget);

	if (priv->clicked && priv->in)
		g_signal_emit (widget, signals[CLICKED], 0, event);
	priv->clicked = FALSE;

	return FALSE;
}

static void
jana_gtk_world_map_class_init (JanaGtkWorldMapClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	g_type_class_add_private (klass, sizeof (JanaGtkWorldMapPrivate));

	object_class->get_property = jana_gtk_world_map_get_property;
	object_class->set_property = jana_gtk_world_map_set_property;
	object_class->dispose = jana_gtk_world_map_dispose;
	object_class->finalize = jana_gtk_world_map_finalize;
	
	widget_class->expose_event = jana_gtk_world_map_expose_event;
	widget_class->size_allocate = jana_gtk_world_map_size_allocate;
	widget_class->map_event = jana_gtk_world_map_map_event;
	widget_class->style_set = jana_gtk_world_map_style_set;
	widget_class->enter_notify_event =
		jana_gtk_world_map_enter_notify_event;
	widget_class->leave_notify_event =
		jana_gtk_world_map_leave_notify_event;
	widget_class->button_press_event =
		jana_gtk_world_map_button_press_event;
	widget_class->button_release_event =
		jana_gtk_world_map_button_release_event;

	g_object_class_install_property (
		object_class,
		PROP_TIME,
		g_param_spec_object (
			"time",
			"Time",
			"The JanaTime the world map will show.",
			G_TYPE_OBJECT,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_WIDTH,
		g_param_spec_int (
			"width",
			"Width",
			"The width to use when using a static size.",
			0, G_MAXINT, 0,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_HEIGHT,
		g_param_spec_int (
			"height",
			"Height",
			"The height to use when using a static size.",
			0, G_MAXINT, 0,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_STATIC,
		g_param_spec_boolean (
			"static",
			"Static",
			"Whether to render a static size once and scale to the "
			"widget's allocated size.",
			FALSE,
			G_PARAM_READWRITE));
	
	signals[RENDER_START] =
		g_signal_new ("render_start",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaGtkWorldMapClass,
					 render_start),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	signals[RENDER_STOP] =
		g_signal_new ("render_stop",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaGtkWorldMapClass,
					 render_stop),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	signals[CLICKED] =
		g_signal_new ("clicked",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaGtkWorldMapClass,
					 clicked),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
jana_gtk_world_map_init (JanaGtkWorldMap *self)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (self);

	if (!g_thread_supported ()) g_thread_init (NULL);

	gtk_widget_add_events (GTK_WIDGET (self),
		GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
	gtk_widget_set_app_paintable (GTK_WIDGET (self), TRUE);
	gtk_event_box_set_visible_window (GTK_EVENT_BOX (self), FALSE);
	read_vmf (self);
	
	priv->marks = g_ptr_array_new ();
}

GtkWidget *
jana_gtk_world_map_new (void)
{
	return GTK_WIDGET (g_object_new (JANA_GTK_TYPE_WORLD_MAP, NULL));
}

void
jana_gtk_world_map_set_time (JanaGtkWorldMap *self, JanaTime *time)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (self);
	
	if (priv->time) {
		g_object_unref (priv->time);
		priv->time = NULL;
	}
	if (time) {
		priv->time = jana_time_duplicate (time);
		/* Set time to UTC */
		jana_time_set_offset (priv->time, 0);
	}
	refresh_buffer (self);
}

void
jana_gtk_world_map_get_latlon (JanaGtkWorldMap *self, gint x, gint y,
			       gdouble *lat, gdouble *lon)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (self);
	
	if (lon) {
		*lon = (((gdouble)x / (gdouble)GTK_WIDGET (self)->
			allocation.width) * (priv->theta_max -
			priv->theta_min)) + priv->theta_min;
	}

	if (lat) {
		*lat = -((((gdouble)y / (gdouble)GTK_WIDGET (self)->
			allocation.height) * (priv->phi_max - priv->phi_min)) +
			priv->phi_min);
	}
}

void
jana_gtk_world_map_get_xy (JanaGtkWorldMap *self, gdouble lat, gdouble lon,
			   gint *x, gint *y)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (self);
	
	if (x) {
		*x = ((lon / (priv->theta_max - priv->theta_min)) *
		       (gdouble)GTK_WIDGET (self)->allocation.width) +
			GTK_WIDGET (self)->allocation.width/2;
	}

	if (y) {
		*y = (((-lat) / (priv->phi_max - priv->phi_min)) *
		       (gdouble)GTK_WIDGET (self)->allocation.height) +
			GTK_WIDGET (self)->allocation.height/2;
	}
}

JanaGtkWorldMapMarker *
jana_gtk_world_map_add_marker (JanaGtkWorldMap *self,
			       JanaGtkWorldMapMarker *marker,
			       gdouble lat, gdouble lon)

{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (self);

	marker->lat = lat;
	marker->lon = lon;

	g_object_ref_sink (G_OBJECT (marker));
	g_ptr_array_add (priv->marks, marker);

	gtk_widget_queue_draw (GTK_WIDGET (self));

	return marker;
}

void
jana_gtk_world_map_remove_marker (JanaGtkWorldMap *self,
				  JanaGtkWorldMapMarker *marker)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (self);

	g_ptr_array_remove (priv->marks, marker);
	g_object_unref (marker);
			
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
jana_gtk_world_map_move_marker (JanaGtkWorldMap *self,
				JanaGtkWorldMapMarker *marker,
				gdouble lat, gdouble lon)
{
	marker->lat = lat;
	marker->lon = lon;

	gtk_widget_queue_draw (GTK_WIDGET (self));
}

GList *
jana_gtk_world_map_get_markers (JanaGtkWorldMap *self)
{
	gint i;
	GList *markers;
	
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (self);
	
	for (i = 0, markers = NULL; i < priv->marks->len; i++) {
		markers = g_list_prepend (markers, priv->marks->pdata[i]);
	}
	
	return markers;
}

void
jana_gtk_world_map_set_static (JanaGtkWorldMap *self, gboolean set_static)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (self);
	
	if (priv->scale != set_static) {
		priv->scale = set_static;
		refresh_buffer (self);
	}
}

gboolean
jana_gtk_world_map_get_static (JanaGtkWorldMap *self)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (self);
	return priv->scale;
}

void
jana_gtk_world_map_set_width (JanaGtkWorldMap *self, gint width)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (self);
	
	if (priv->width != width) {
		priv->width = width;
		if (priv->scale) refresh_buffer (self);
	}
}

gint
jana_gtk_world_map_get_width (JanaGtkWorldMap *self)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (self);
	return priv->width;
}

void
jana_gtk_world_map_set_height (JanaGtkWorldMap *self, gint height)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (self);
	
	if (priv->height != height) {
		priv->height = height;
		if (priv->scale) refresh_buffer (self);
	}
}

gint
jana_gtk_world_map_get_height (JanaGtkWorldMap *self)
{
	JanaGtkWorldMapPrivate *priv = WORLD_MAP_PRIVATE (self);
	return priv->height;
}

#ifdef HAVE_GLADE
/* The following is a nasty hack to get the widget to display correctly in 
 * Glade-3. Taken from JanaGtkClock.
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
jana_gtk_world_map_glade_add_cb (GtkContainer *container,
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
jana_gtk_world_map_glade_remove_cb (GtkContainer *container,
				    GtkWidget    *widget,
				    gpointer      user_data)
{
	if (!GLADE_IS_PLACEHOLDER (widget)) return;
	
	g_signal_handlers_disconnect_by_func (
		widget, placeholder_realize_cb, NULL);
}

void
jana_gtk_world_map_glade_post_create (GladeWidgetAdaptor *adaptor,
				      GObject	     *object,
				      GladeCreateReason   reason)
{
	GList *children;
	g_return_if_fail (GTK_IS_CONTAINER (object));

	g_signal_connect (object, "add",
		G_CALLBACK (jana_gtk_world_map_glade_add_cb), NULL);
	g_signal_connect (object, "remove",
		G_CALLBACK (jana_gtk_world_map_glade_remove_cb), NULL);
	
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
