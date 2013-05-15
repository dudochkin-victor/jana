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


#ifndef _JANA_GTK_CELL_RENDERER_EVENT_H
#define _JANA_GTK_CELL_RENDERER_EVENT_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define JANA_GTK_TYPE_CELL_RENDERER_EVENT \
	jana_gtk_cell_renderer_event_get_type()

#define JANA_GTK_CELL_RENDERER_EVENT(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	JANA_GTK_TYPE_CELL_RENDERER_EVENT, JanaGtkCellRendererEvent))

#define JANA_GTK_CELL_RENDERER_EVENT_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	JANA_GTK_TYPE_CELL_RENDERER_EVENT, JanaGtkCellRendererEventClass))

#define JANA_GTK_IS_CELL_RENDERER_EVENT(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	JANA_GTK_TYPE_CELL_RENDERER_EVENT))

#define JANA_GTK_IS_CELL_RENDERER_EVENT_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	JANA_GTK_TYPE_CELL_RENDERER_EVENT))

#define JANA_GTK_CELL_RENDERER_EVENT_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	JANA_GTK_TYPE_CELL_RENDERER_EVENT, JanaGtkCellRendererEventClass))

typedef struct {
	GtkCellRenderer parent;
} JanaGtkCellRendererEvent;

typedef struct {
	GtkCellRendererClass parent_class;
} JanaGtkCellRendererEventClass;

GType jana_gtk_cell_renderer_event_get_type (void);

GtkCellRenderer* jana_gtk_cell_renderer_event_new (void);

G_END_DECLS

#endif /* _JANA_GTK_CELL_RENDERER_EVENT_H */

