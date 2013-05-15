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


#include "jana-gtk-world-map-marker.h"
#include <math.h>

G_DEFINE_TYPE (JanaGtkWorldMapMarker, jana_gtk_world_map_marker, \
	G_TYPE_INITIALLY_UNOWNED)


static void
render (JanaGtkWorldMapMarker *marker, GtkStyle *style, cairo_t *cr,
	GtkStateType state, gint x, gint y)
{
	cairo_arc (cr, x, y, 5, 0, 2 * M_PI);
	
	gdk_cairo_set_source_color (cr, &style->mid[state]);
	cairo_fill_preserve (cr);
	gdk_cairo_set_source_color (cr, &style->fg[state]);
	cairo_stroke (cr);
}
	     
static void
jana_gtk_world_map_marker_class_init (JanaGtkWorldMapMarkerClass *klass)
{
	klass->render = render;
}

static void
jana_gtk_world_map_marker_init (JanaGtkWorldMapMarker *marker)
{
}

void
jana_gtk_world_map_marker_render (JanaGtkWorldMapMarker *marker,
				  GtkStyle *style, cairo_t *cr,
				  GtkStateType state, gint x, gint y)
{
	JanaGtkWorldMapMarkerClass *klass =
		JANA_GTK_WORLD_MAP_MARKER_GET_CLASS (marker);
	klass->render (marker, style, cr, state, x, y);
}

JanaGtkWorldMapMarker *
jana_gtk_world_map_marker_new ()
{
	JanaGtkWorldMapMarker *marker;

	marker = g_object_new (JANA_GTK_TYPE_WORLD_MAP_MARKER, NULL);

	return marker;
}
