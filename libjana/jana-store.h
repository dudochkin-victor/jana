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


#ifndef JANA_STORE_H
#define JANA_STORE_H

#include <glib-object.h>

#define JANA_TYPE_STORE			(jana_store_get_type ())
#define JANA_STORE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj),\
					 JANA_TYPE_STORE,\
					 JanaStore))
#define JANA_IS_STORE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
					 JANA_TYPE_STORE))
#define JANA_STORE_GET_INTERFACE(inst)	(G_TYPE_INSTANCE_GET_INTERFACE ((inst),\
					 JANA_TYPE_STORE,\
					 JanaStoreInterface))

/**
 * JanaStore:
 *
 * The #JanaStore struct contains only private data.
 */
typedef struct _JanaStore JanaStore; /* Dummy object */
typedef struct _JanaStoreInterface JanaStoreInterface;

#include <libjana/jana-store-view.h>
#include <libjana/jana-component.h>

struct _JanaStoreInterface {
	GTypeInterface parent;
	
	void			(*open)			(JanaStore *self);
	
	JanaComponent *		(*get_component)	(JanaStore *self,
							 const gchar *uid);
	JanaStoreView *		(*get_view)		(JanaStore *self);
	
	void	(*add_component)	(JanaStore *self, JanaComponent *comp);
	void	(*modify_component)	(JanaStore *self, JanaComponent *comp);
	void	(*remove_component)	(JanaStore *self, JanaComponent *comp);
	
	/* Signals */
	void	(*opened)		(JanaStore *self);
	/*gchar *	(*auth)			(JanaStore *self, */
};

GType jana_store_get_type (void);

void			jana_store_open			(JanaStore *self);

JanaComponent *		jana_store_get_component	(JanaStore *self,
							 const gchar *uid);

JanaStoreView *		jana_store_get_view		(JanaStore *self);

void	jana_store_add_component	(JanaStore *self, JanaComponent *comp);
void	jana_store_modify_component	(JanaStore *self, JanaComponent *comp);
void	jana_store_remove_component	(JanaStore *self, JanaComponent *comp);

#endif /* JANA_STORE_H */

