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


#ifndef _JANA_GTK_TREE_LAYOUT_H
#define _JANA_GTK_TREE_LAYOUT_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define JANA_GTK_TYPE_TREE_LAYOUT jana_gtk_tree_layout_get_type()

#define JANA_GTK_TREE_LAYOUT(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	JANA_GTK_TYPE_TREE_LAYOUT, JanaGtkTreeLayout))

#define JANA_GTK_TREE_LAYOUT_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	JANA_GTK_TYPE_TREE_LAYOUT, JanaGtkTreeLayoutClass))

#define JANA_GTK_IS_TREE_LAYOUT(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	JANA_GTK_TYPE_TREE_LAYOUT))

#define JANA_GTK_IS_TREE_LAYOUT_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	JANA_GTK_TYPE_TREE_LAYOUT))

#define JANA_GTK_TREE_LAYOUT_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	JANA_GTK_TYPE_TREE_LAYOUT, JanaGtkTreeLayoutClass))

typedef struct {
	GtkTreeRowReference *row;
	gint x;
	gint y;
	gint width;
	gint height;
	gint real_x;
	gint real_y;
	gint real_width;
	gint real_height;
	gboolean sensitive;
	GtkCellRenderer *renderer;
	GList *attributes;
} JanaGtkTreeLayoutCellInfo;

typedef struct {
	GtkEventBox parent;
} JanaGtkTreeLayout;

typedef struct {
	GtkEventBoxClass parent_class;
	
	void	(*add_cell)	(JanaGtkTreeLayout *self,
				 GtkTreeRowReference *row,
				 gint x,
				 gint y,
				 gint width,
				 gint height,
				 GtkCellRenderer *renderer,
				 va_list args);
	void	(*move_cell)	(JanaGtkTreeLayout *self,
				 GtkTreeRowReference *row,
				 gint x,
				 gint y,
				 gint width,
				 gint height);
	void	(*remove_cell)	(JanaGtkTreeLayout *self,
				 GtkTreeRowReference *row);
	void	(*clear)	(JanaGtkTreeLayout *self);

	/* Signals */
	void	(*selection_changed)	(JanaGtkTreeLayout *self);
	void	(*cell_activated)	(JanaGtkTreeLayout *self,
					 const JanaGtkTreeLayoutCellInfo *info);
} JanaGtkTreeLayoutClass;

GType jana_gtk_tree_layout_get_type (void);

GtkWidget* jana_gtk_tree_layout_new (void);

void	jana_gtk_tree_layout_add_cell		(JanaGtkTreeLayout *self,
						 GtkTreeRowReference *row,
						 gint x, gint y,
						 gint width, gint height,
						 GtkCellRenderer *renderer,
						 ...);
void	jana_gtk_tree_layout_move_cell		(JanaGtkTreeLayout *self,
						 GtkTreeRowReference *row,
						 gint x, gint y,
						 gint width, gint height);
void	jana_gtk_tree_layout_remove_cell	(JanaGtkTreeLayout *self,
						 GtkTreeRowReference *row);
void	jana_gtk_tree_layout_clear		(JanaGtkTreeLayout *self);

GList *	jana_gtk_tree_layout_get_selection	(JanaGtkTreeLayout *self);
GList *	jana_gtk_tree_layout_get_cells		(JanaGtkTreeLayout *self);

void	jana_gtk_tree_layout_set_selection	(JanaGtkTreeLayout *self,
						 GList *selection);

const JanaGtkTreeLayoutCellInfo *
	jana_gtk_tree_layout_get_cell		(JanaGtkTreeLayout *self,
						 GtkTreeRowReference *row);

void	jana_gtk_tree_layout_set_cell_sensitive	(JanaGtkTreeLayout *self,
						 GtkTreeRowReference *row,
						 gboolean sensitive);

void	jana_gtk_tree_layout_set_visible_func	(JanaGtkTreeLayout *self,
						 GtkTreeModelFilterVisibleFunc
							  visible_cb,
						 gpointer data);

void	jana_gtk_tree_layout_refilter		(JanaGtkTreeLayout *self);

G_END_DECLS

#endif /* _JANA_GTK_TREE_LAYOUT_H */

