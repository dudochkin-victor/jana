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


#ifndef _JANA_GTK_WORLD_MAP_MARKER_H
#define _JANA_GTK_WORLD_MAP_MARKER_H

#include <glib-object.h>
#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

#define JANA_GTK_TYPE_WORLD_MAP_MARKER jana_gtk_world_map_marker_get_type()

#define JANA_GTK_WORLD_MAP_MARKER(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        JANA_GTK_TYPE_WORLD_MAP_MARKER, JanaGtkWorldMapMarker))

#define JANA_GTK_WORLD_MAP_MARKER_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST ((klass), \
        JANA_GTK_TYPE_WORLD_MAP_MARKER, JanaGtkWorldMapMarkerClass))

#define JANA_GTK_IS_WORLD_MAP_MARKER(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        JANA_GTK_TYPE_WORLD_MAP_MARKER))

#define JANA_GTK_IS_WORLD_MAP_MARKER_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), \
        JANA_GTK_TYPE_WORLD_MAP_MARKER))

#define JANA_GTK_WORLD_MAP_MARKER_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        JANA_GTK_TYPE_WORLD_MAP_MARKER, JanaGtkWorldMapMarkerClass))

typedef struct {
	GObject parent;
	
	gdouble lat;
	gdouble lon;
} JanaGtkWorldMapMarker;

typedef struct {
	GObjectClass parent_class;

	void (* render) (JanaGtkWorldMapMarker *marker, GtkStyle *style,
			 cairo_t *cr, GtkStateType state, gint x, gint y);
} JanaGtkWorldMapMarkerClass;

GType jana_gtk_world_map_marker_get_type (void);

JanaGtkWorldMapMarker * jana_gtk_world_map_marker_new ();

void
jana_gtk_world_map_marker_render (JanaGtkWorldMapMarker *marker,
				  GtkStyle *style, cairo_t *cr,
				  GtkStateType state, gint x, gint y);

G_END_DECLS

#endif
