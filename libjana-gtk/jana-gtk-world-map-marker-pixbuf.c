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


#include "jana-gtk-world-map-marker-pixbuf.h"

G_DEFINE_TYPE (JanaGtkWorldMapMarkerPixbuf, 
	       jana_gtk_world_map_marker_pixbuf, 
	       JANA_GTK_TYPE_WORLD_MAP_MARKER)

static void
dispose (GObject *object)
{
	JanaGtkWorldMapMarkerPixbuf *marker =
		(JanaGtkWorldMapMarkerPixbuf *)object;

	if (marker->pixbuf) {
		g_object_unref (marker->pixbuf);
		marker->pixbuf = NULL;
	}

	((GObjectClass *) jana_gtk_world_map_marker_pixbuf_parent_class)->
		dispose (object);
}

static void
render (JanaGtkWorldMapMarker *marker, GtkStyle *style, cairo_t *cr,
	GtkStateType state, gint x, gint y)
{
	gint real_x, real_y, width, height;

	if (!((JanaGtkWorldMapMarkerPixbuf *)marker)->pixbuf) {
		JANA_GTK_WORLD_MAP_MARKER_CLASS (
			jana_gtk_world_map_marker_pixbuf_parent_class)->
				render (marker, style, cr, state, x, y);
		return;
	}
	
	width = gdk_pixbuf_get_width (
		((JanaGtkWorldMapMarkerPixbuf *)marker)->pixbuf);
	height = gdk_pixbuf_get_height (
		((JanaGtkWorldMapMarkerPixbuf *)marker)->pixbuf);
	real_x = x - (width / 2);
	real_y = y - (height / 2);
	
	gdk_cairo_set_source_pixbuf (cr, ((JanaGtkWorldMapMarkerPixbuf *)
		marker)->pixbuf, real_x, real_y);
	cairo_paint (cr);
}
	     
static void
jana_gtk_world_map_marker_pixbuf_class_init (
				JanaGtkWorldMapMarkerPixbufClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	JanaGtkWorldMapMarkerClass *map_class =
		(JanaGtkWorldMapMarkerClass *) klass;

	object_class->dispose = dispose;
	map_class->render = render;
}

static void
jana_gtk_world_map_marker_pixbuf_init (JanaGtkWorldMapMarkerPixbuf *marker)
{
}

JanaGtkWorldMapMarker *
jana_gtk_world_map_marker_pixbuf_new (GdkPixbuf *pixbuf)
{
	JanaGtkWorldMapMarkerPixbuf *marker;

	marker = g_object_new (JANA_GTK_TYPE_WORLD_MAP_MARKER_PIXBUF, NULL);
	marker->pixbuf = pixbuf ? g_object_ref (pixbuf) : NULL;

	return JANA_GTK_WORLD_MAP_MARKER (marker);
}
