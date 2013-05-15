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


#ifndef _JANA_GTK_WORLD_MAP_H
#define _JANA_GTK_WORLD_MAP_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libjana/jana-time.h>
#include <libjana-gtk/jana-gtk-world-map-marker.h>

G_BEGIN_DECLS

#define JANA_GTK_TYPE_WORLD_MAP jana_gtk_world_map_get_type()

#define JANA_GTK_WORLD_MAP(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	JANA_GTK_TYPE_WORLD_MAP, JanaGtkWorldMap))

#define JANA_GTK_WORLD_MAP_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	JANA_GTK_TYPE_WORLD_MAP, JanaGtkWorldMapClass))

#define JANA_GTK_IS_WORLD_MAP(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	JANA_GTK_TYPE_WORLD_MAP))

#define JANA_GTK_IS_WORLD_MAP_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	JANA_GTK_TYPE_WORLD_MAP))

#define JANA_GTK_WORLD_MAP_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	JANA_GTK_TYPE_WORLD_MAP, JanaGtkWorldMapClass))

typedef struct {
	GtkEventBox parent;
} JanaGtkWorldMap;

typedef struct {
	GtkEventBoxClass parent_class;

	/* Signals */
	void	(*render_start)	(JanaGtkWorldMap *self);
	void	(*render_stop)	(JanaGtkWorldMap *self);
	void	(*clicked)	(JanaGtkWorldMap *self, GdkEventButton *event);
} JanaGtkWorldMapClass;

GType jana_gtk_world_map_get_type (void);

GtkWidget * jana_gtk_world_map_new (void);

void jana_gtk_world_map_set_time (JanaGtkWorldMap *self, JanaTime *time);

void jana_gtk_world_map_get_latlon (JanaGtkWorldMap *self, gint x, gint y,
				    gdouble *lat, gdouble *lon);

void jana_gtk_world_map_get_xy (JanaGtkWorldMap *self, gdouble lat, gdouble lon,
				gint *x, gint *y);

JanaGtkWorldMapMarker *
	jana_gtk_world_map_add_marker (JanaGtkWorldMap *self,
				       JanaGtkWorldMapMarker *mark,
				       gdouble lat, gdouble lon);

void jana_gtk_world_map_remove_marker (JanaGtkWorldMap *self,
				       JanaGtkWorldMapMarker *mark);

void jana_gtk_world_map_move_marker (JanaGtkWorldMap *self,
				     JanaGtkWorldMapMarker *mark,
				     gdouble lat, gdouble lon);

GList * jana_gtk_world_map_get_markers (JanaGtkWorldMap *self);

void jana_gtk_world_map_set_static (JanaGtkWorldMap *self, gboolean set_static);

gboolean jana_gtk_world_map_get_static (JanaGtkWorldMap *self);

void jana_gtk_world_map_set_width (JanaGtkWorldMap *self, gint width);

gint jana_gtk_world_map_get_width (JanaGtkWorldMap *self);

void jana_gtk_world_map_set_height (JanaGtkWorldMap *self, gint height);

gint jana_gtk_world_map_get_height (JanaGtkWorldMap *self);

G_END_DECLS

#endif /* _JANA_GTK_WORLD_MAP_H */
