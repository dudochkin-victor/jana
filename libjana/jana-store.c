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


/**
 * SECTION:jana-store
 * @short_description: A component store interface
 * @see_also: #JanaComponent, #JanaStoreView
 *
 * #JanaStore is the interface for a component storage. A component store 
 * has functions to add, modify and remove components, as well as query the 
 * storage itself and retrieve specific components.
 */

#include "jana-store.h"

static void
jana_store_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/**
		 * JanaStore::opened:
		 * @store: the store that received the signal
		 *
		 * The ::opened signal is emitted after jana_store_open() 
		 * is called and the store is ready to be queried.
		 **/
		g_signal_new ("opened",
			G_OBJECT_CLASS_TYPE (g_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaStoreInterface, opened),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);
		initialized = TRUE;
	}
}

GType
jana_store_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (JanaStoreInterface),
			jana_store_base_init,   /* base_init */
			NULL,

		};
		type = g_type_register_static (G_TYPE_INTERFACE,
			"JanaStore", &info, 0);
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}
	return type;
}

/**
 * jana_store_open:
 * @self: A #JanaStore
 *
 * Opens the store. Prior to opening a store, any #JanaStoreView on the store 
 * will not receive any components and retrieval of components will fail. 
 * This is an asynchronous call. When the store is open and ready, the ::opened 
 * signal will be fired. Implementations of this function should return as 
 * immediately as possible and do any lengthy processing in idle time, or in 
 * a thread.
 */
void
jana_store_open (JanaStore *self)
{
	JANA_STORE_GET_INTERFACE (self)->open (self);
}

/**
 * jana_store_get_component:
 * @self: A #JanaStore
 * @uid: The UID of a #JanaComponent
 *
 * Retrieves a particular #JanaComponent using its unique identifier.
 *
 * Returns: The #JanaComponent requested, or %NULL if it does not exist in this 
 * store.
 */
JanaComponent *
jana_store_get_component (JanaStore *self, const gchar *uid)
{
	return JANA_STORE_GET_INTERFACE (self)->get_component (self, uid);
}

/**
 * jana_store_get_view:
 * @self: A #JanaStore
 *
 * Retrieves a #JanaStoreView on this #JanaStore.
 *
 * Returns: A new #JanaStoreView.
 */
JanaStoreView *
jana_store_get_view (JanaStore *self)
{
	return JANA_STORE_GET_INTERFACE (self)->get_view (self);
}

/**
 * jana_store_add_component:
 * @self: A #JanaStore
 * @comp: The #JanaComponent
 *
 * Adds a component to the store.
 */
void
jana_store_add_component (JanaStore *self, JanaComponent *comp)
{
	return JANA_STORE_GET_INTERFACE (self)->add_component (self, comp);
}

/**
 * jana_store_modify_component:
 * @self: A #JanaStore
 * @comp: The #JanaComponent
 *
 * Updates the stored component with any changes made. Does nothing if the 
 * component does not exist in the store.
 */
void
jana_store_modify_component (JanaStore *self, JanaComponent *comp)
{
	return JANA_STORE_GET_INTERFACE (self)->modify_component (self, comp);
}

/**
 * jana_store_remove_component:
 * @self: A #JanaStore
 * @comp: The #JanaComponent
 *
 * Removes a component from the store. Does nothing if the component does not 
 * exist in the store.
 */
void
jana_store_remove_component (JanaStore *self, JanaComponent *comp)
{
	return JANA_STORE_GET_INTERFACE (self)->remove_component (self, comp);
}

