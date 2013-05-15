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


#ifndef _JANA_GTK_YEAR_VIEW_H
#define _JANA_GTK_YEAR_VIEW_H

#include <gtk/gtk.h>
#include <glib-object.h>
#include <libjana/jana-time.h>
#include <libjana-gtk/jana-gtk-event-store.h>

G_BEGIN_DECLS

#define JANA_GTK_TYPE_YEAR_VIEW jana_gtk_year_view_get_type()

#define JANA_GTK_YEAR_VIEW(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	JANA_GTK_TYPE_YEAR_VIEW, JanaGtkYearView))

#define JANA_GTK_YEAR_VIEW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	JANA_GTK_TYPE_YEAR_VIEW, JanaGtkYearViewClass))

#define JANA_GTK_IS_YEAR_VIEW(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	JANA_GTK_TYPE_YEAR_VIEW))

#define JANA_GTK_IS_YEAR_VIEW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	JANA_GTK_TYPE_YEAR_VIEW))

#define JANA_GTK_YEAR_VIEW_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	JANA_GTK_TYPE_YEAR_VIEW, JanaGtkYearViewClass))

typedef struct {
	GtkTable parent;
} JanaGtkYearView;

typedef struct {
	GtkTableClass parent_class;

	/* Signals */
	void	(*selection_changed)	(JanaGtkYearView *self, gint month);
} JanaGtkYearViewClass;

GType jana_gtk_year_view_get_type (void);

GtkWidget * jana_gtk_year_view_new (guint months_per_row, JanaTime *year);

void		jana_gtk_year_view_add_store	(JanaGtkYearView *self,
						 JanaGtkEventStore *store);

void		jana_gtk_year_view_remove_store	(JanaGtkYearView *self,
						 JanaGtkEventStore *store);

void		jana_gtk_year_view_set_year	(JanaGtkYearView *self,
						 JanaTime *year);

JanaTime *	jana_gtk_year_view_get_year	(JanaGtkYearView *self);

void		jana_gtk_year_view_set_months_per_row
						(JanaGtkYearView *self,
						 guint months_per_row);

gint		jana_gtk_year_view_get_selected_month
						(JanaGtkYearView *self);

void		jana_gtk_year_view_set_selected_month
						(JanaGtkYearView *self,
						 gint month);

void		jana_gtk_year_view_set_highlighted_time
						(JanaGtkYearView *self,
						 JanaTime *time);
G_END_DECLS

#endif /* _JANA_GTK_YEAR_VIEW_H */
