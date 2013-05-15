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


#ifndef _JANA_GTK_DAY_VIEW_H
#define _JANA_GTK_DAY_VIEW_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libjana-gtk/jana-gtk-cell-renderer-event.h>
#include <libjana-gtk/jana-gtk-event-store.h>

G_BEGIN_DECLS

#define JANA_GTK_TYPE_DAY_VIEW jana_gtk_day_view_get_type()

#define JANA_GTK_DAY_VIEW(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	JANA_GTK_TYPE_DAY_VIEW, JanaGtkDayView))

#define JANA_GTK_DAY_VIEW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	JANA_GTK_TYPE_DAY_VIEW, JanaGtkDayViewClass))

#define JANA_GTK_IS_DAY_VIEW(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	JANA_GTK_TYPE_DAY_VIEW))

#define JANA_GTK_IS_DAY_VIEW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	JANA_GTK_TYPE_DAY_VIEW))

#define JANA_GTK_DAY_VIEW_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	JANA_GTK_TYPE_DAY_VIEW, JanaGtkDayViewClass))

typedef struct {
	GtkEventBox parent;
} JanaGtkDayView;

typedef struct {
	GtkEventBoxClass parent_class;

	void	(*set_scroll_adjustments)	(JanaGtkDayView	*self,
						 GtkAdjustment	*hadjustment,
						 GtkAdjustment	*vadjustment);

	/* Signals */
	void	(*selection_changed)	(JanaGtkDayView *self,
					 JanaDuration *range);
	void	(*event_selected)	(JanaGtkDayView *self,
					 GtkTreeRowReference *event);
	void	(*event_activated)	(JanaGtkDayView *self,
					 GtkTreeRowReference *event);
} JanaGtkDayViewClass;

enum {
	JANA_GTK_DAY_VIEW_COL_ROW,
	JANA_GTK_DAY_VIEW_COL_LAST
};

GType jana_gtk_day_view_get_type (void);

GtkWidget *	jana_gtk_day_view_new 		(JanaDuration *range,
						 guint cells);

void	jana_gtk_day_view_add_store		(JanaGtkDayView *self,
						 JanaGtkEventStore *store);
void	jana_gtk_day_view_remove_store		(JanaGtkDayView *self,
						 JanaGtkEventStore *store);

void	jana_gtk_day_view_scroll_to_cell	(JanaGtkDayView *self,
						 guint x, guint y);
void	jana_gtk_day_view_scroll_to_time	(JanaGtkDayView *self,
						 JanaTime *time);

JanaGtkCellRendererEvent *
	jana_gtk_day_view_get_cell_renderer	(JanaGtkDayView *self);

JanaGtkCellRendererEvent *
	jana_gtk_day_view_get_24hr_cell_renderer(JanaGtkDayView *self);

void	jana_gtk_day_view_set_range		(JanaGtkDayView *self,
						 JanaDuration *range);

JanaDuration *	jana_gtk_day_view_get_range	(JanaGtkDayView *self);

void	jana_gtk_day_view_set_cells (JanaGtkDayView *self, guint cells);

guint	jana_gtk_day_view_get_cells (JanaGtkDayView *self);

void	jana_gtk_day_view_set_spacing		(JanaGtkDayView *self,
						 guint spacing);

guint	jana_gtk_day_view_get_spacing		(JanaGtkDayView *self);

JanaDuration *	jana_gtk_day_view_get_selection	(JanaGtkDayView *self);

void	jana_gtk_day_view_set_selection	(JanaGtkDayView *self,
						 JanaDuration *selection);

void	jana_gtk_day_view_set_visible_ratio	(JanaGtkDayView *self,
						 gdouble xratio,
						 gdouble yratio);

void	jana_gtk_day_view_get_visible_ratio	(JanaGtkDayView *self,
						 gdouble *xratio,
						 gdouble *yratio);

GtkTreeRowReference *
	jana_gtk_day_view_get_selected_event	(JanaGtkDayView *self);

void	jana_gtk_day_view_set_selected_event	(JanaGtkDayView *self,
						 GtkTreeRowReference *row);

void	jana_gtk_day_view_set_visible_func	(JanaGtkDayView *self,
						 GtkTreeModelFilterVisibleFunc
						       visible_cb,
						 gpointer data);

void	jana_gtk_day_view_refilter		(JanaGtkDayView *self);

void	jana_gtk_day_view_set_highlighted_time	(JanaGtkDayView *self,
						 JanaTime *time);

void	jana_gtk_day_view_set_active_range	(JanaGtkDayView *self,
						 JanaDuration *range);

G_END_DECLS

#endif /* _JANA_GTK_DAY_VIEW_H */
