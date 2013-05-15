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


#ifndef _JANA_GTK_CLOCK_H
#define _JANA_GTK_CLOCK_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libjana/jana-time.h>

G_BEGIN_DECLS

#define JANA_GTK_TYPE_CLOCK jana_gtk_clock_get_type()

#define JANA_GTK_CLOCK(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	JANA_GTK_TYPE_CLOCK, JanaGtkClock))

#define JANA_GTK_CLOCK_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	JANA_GTK_TYPE_CLOCK, JanaGtkClockClass))

#define JANA_GTK_IS_CLOCK(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	JANA_GTK_TYPE_CLOCK))

#define JANA_GTK_IS_CLOCK_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	JANA_GTK_TYPE_CLOCK))

#define JANA_GTK_CLOCK_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	JANA_GTK_TYPE_CLOCK, JanaGtkClockClass))

typedef struct {
	GtkEventBox parent;
} JanaGtkClock;

typedef struct {
	GtkEventBoxClass parent_class;

	/* Signals */
	void	(*render_start)	(JanaGtkClock *self);
	void	(*render_stop)	(JanaGtkClock *self);
	void	(*clicked)	(JanaGtkClock *self, GdkEventButton *event);
} JanaGtkClockClass;

GType jana_gtk_clock_get_type (void);

GtkWidget * jana_gtk_clock_new (void);

void jana_gtk_clock_set_time (JanaGtkClock *self, JanaTime *time);

JanaTime *jana_gtk_clock_get_time (JanaGtkClock *self);

void jana_gtk_clock_set_digital (JanaGtkClock *self, gboolean digital);

gboolean jana_gtk_clock_get_digital (JanaGtkClock *self);

void jana_gtk_clock_set_show_seconds (JanaGtkClock *self,
				      gboolean show_seconds);

gboolean jana_gtk_clock_get_show_seconds (JanaGtkClock *self);

void jana_gtk_clock_set_buffer_time (JanaGtkClock *self, gboolean buffer_time);

gboolean jana_gtk_clock_get_buffer_time (JanaGtkClock *self);

void jana_gtk_clock_set_draw_shadow (JanaGtkClock *self, gboolean draw_shadow);

gboolean jana_gtk_clock_get_draw_shadow (JanaGtkClock *self);

G_END_DECLS

#endif /* _JANA_GTK_CLOCK_H */
