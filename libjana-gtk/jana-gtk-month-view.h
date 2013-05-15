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


#ifndef _JANA_GTK_MONTH_VIEW_H
#define _JANA_GTK_MONTH_VIEW_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libjana-gtk/jana-gtk-cell-renderer-event.h>
#include <libjana-gtk/jana-gtk-event-store.h>

G_BEGIN_DECLS

#define JANA_GTK_TYPE_MONTH_VIEW jana_gtk_month_view_get_type()

#define JANA_GTK_MONTH_VIEW(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	JANA_GTK_TYPE_MONTH_VIEW, JanaGtkMonthView))

#define JANA_GTK_MONTH_VIEW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	JANA_GTK_TYPE_MONTH_VIEW, JanaGtkMonthViewClass))

#define JANA_GTK_IS_MONTH_VIEW(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	JANA_GTK_TYPE_MONTH_VIEW))

#define JANA_GTK_IS_MONTH_VIEW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	JANA_GTK_TYPE_MONTH_VIEW))

#define JANA_GTK_MONTH_VIEW_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	JANA_GTK_TYPE_MONTH_VIEW, JanaGtkMonthViewClass))

typedef struct {
	GtkEventBox parent;
} JanaGtkMonthView;

typedef struct {
	GtkEventBoxClass parent_class;

	/* Signals */
	void	(*selection_changed)	(JanaGtkMonthView *self, JanaTime *day);
} JanaGtkMonthViewClass;

enum {
	JANA_GTK_MONTH_VIEW_COL_ROW,
	JANA_GTK_MONTH_VIEW_COL_LAST
};

GType jana_gtk_month_view_get_type (void);

GtkWidget *	jana_gtk_month_view_new 	(JanaTime *month);

void	jana_gtk_month_view_add_store		(JanaGtkMonthView *self,
						 JanaGtkEventStore *store);
void	jana_gtk_month_view_remove_store	(JanaGtkMonthView *self,
						 JanaGtkEventStore *store);
JanaGtkCellRendererEvent *
	jana_gtk_month_view_get_cell_renderer	(JanaGtkMonthView *self);

void	jana_gtk_month_view_set_month		(JanaGtkMonthView *self,
						 JanaTime *month);

JanaTime *	jana_gtk_month_view_get_month	(JanaGtkMonthView *self);

void	jana_gtk_month_view_set_spacing		(JanaGtkMonthView *self,
						 guint spacing);

guint	jana_gtk_month_view_get_spacing		(JanaGtkMonthView *self);

void	jana_gtk_month_view_set_selection	(JanaGtkMonthView *self,
						 JanaTime *day);

JanaTime * jana_gtk_month_view_get_selection	(JanaGtkMonthView *self);

void	jana_gtk_month_view_set_visible_func	(JanaGtkMonthView *self,
						 GtkTreeModelFilterVisibleFunc
						       visible_cb,
						 gpointer data);

void	jana_gtk_month_view_refilter		(JanaGtkMonthView *self);

void	jana_gtk_month_view_set_highlighted_time(JanaGtkMonthView *self,
						 JanaTime *time);
G_END_DECLS

#endif /* _JANA_GTK_MONTH_VIEW_H */
