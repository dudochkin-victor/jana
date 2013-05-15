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
 * SECTION:jana-store-view
 * @short_description: A store query interface
 * @see_also: #JanaStore
 *
 * #JanaStoreView is the interface for a query, or 'view' on a #JanaStore. A 
 * store view has functions to query a particular time range of components.
 */

#include "jana-store-view.h"

enum {
	ADDED,
	MODIFIED,
	REMOVED,
	LAST_SIGNAL
};

static void
jana_store_view_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/**
		 * JanaStoreView::added:
		 * @store_view: the store view that received the signal
		 * @components: A list of #JanaComponent<!-- -->s
		 *
		 * The ::added signal is emitted when new components become
		 * visible in the scope of the store view.
		 **/
		g_signal_new ("added",
			G_OBJECT_CLASS_TYPE (g_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaStoreViewInterface, added),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE, 1, G_TYPE_POINTER);

		/**
		 * JanaStoreView::modified:
		 * @store_view: the store view that received the signal
		 * @components: A list of #JanaComponent<!-- -->s
		 *
		 * The ::modified signal is emitted when components that were 
		 * previously in view have been modified in some way.
		 **/
		g_signal_new ("modified",
			G_OBJECT_CLASS_TYPE (g_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaStoreViewInterface, modified),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE, 1, G_TYPE_POINTER);

		/**
		 * JanaStoreView::removed:
		 * @store_view: the store view that received the signal
		 * @uids: A list of uids
		 *
		 * The ::removed signal is emitted when components that were 
		 * previously in view have been removed from the underling 
		 * #JanaStore.
		 **/
		g_signal_new ("removed",
			G_OBJECT_CLASS_TYPE (g_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaStoreViewInterface, removed),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE, 1, G_TYPE_POINTER);

		/**
		 * JanaStoreView::progress:
		 * @store_view: the store view that received the signal
		 * @percent: Percentage completion of the current running query
		 *
		 * The ::progress signal is emitted periodically as the 
		 * query progresses. When the query is complete, @percent will 
		 * be 100.
		 **/
		g_signal_new ("progress",
			G_OBJECT_CLASS_TYPE (g_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaStoreViewInterface, progress),
			NULL, NULL,
			g_cclosure_marshal_VOID__INT,
			G_TYPE_NONE, 1, G_TYPE_INT);
		
		initialized = TRUE;
	}
}

GType
jana_store_view_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (JanaStoreViewInterface),
			jana_store_view_base_init,   /* base_init */
			NULL,

		};
		type = g_type_register_static (G_TYPE_INTERFACE,
			"JanaStoreView", &info, 0);
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}
	return type;
}

/**
 * jana_store_view_get_range:
 * @self: A #JanaStoreView
 * @start: Return location for the start of the view range, or %NULL
 * @end: Return location for the end of the view range, or %NULL
 *
 * Retrieves the range over which this #JanaStoreView is set to span. A %NULL
 * start or end indicate an unbounded range.
 */
void
jana_store_view_get_range (JanaStoreView *self,
			   JanaTime **start, JanaTime **end)
{
	JANA_STORE_VIEW_GET_INTERFACE (self)->get_range (self, start, end);
}

/**
 * jana_store_view_set_range:
 * @self: A #JanaStoreView
 * @start: The start of the range, or %NULL
 * @end: The end of the range, or %NULL
 *
 * Sets the range for the #JanaStoreView to span. A %NULL parameter indicates 
 * that that end of the range should be unbounded (i.e. extended infinitely 
 * into the past or future). For event stores, this query will match against 
 * events that have occurences within this range; for note stores, this query 
 * will match against notes that were created or modified within this range.
 */
void
jana_store_view_set_range (JanaStoreView *self,
			   JanaTime *start, JanaTime *end)
{
	JANA_STORE_VIEW_GET_INTERFACE (self)->set_range (self, start, end);
}

/**
 * jana_store_view_add_match:
 * @self: A #JanaStoreView
 * @field: The field to match against
 * @data: The matching string to use
 *
 * Adds a matching query to the store view. This allows to search for components
 * whose fields contain a particular string. The matching is done disregarding 
 * the case of letters in the string. @field must contain a field that matches 
 * the component type being store by the underlying #JanaStore, otherwise 
 * unexpected results may occur.
 *
 * Returns: A #JanaStoreViewMatch, representing the matching query.
 */
JanaStoreViewMatch *
jana_store_view_add_match (JanaStoreView *self, JanaStoreViewField field,
			   const gchar *data)
{
	return JANA_STORE_VIEW_GET_INTERFACE (self)->
		add_match (self, field, data);
}

/**
 * jana_store_view_get_matches:
 * @self: A #JanaStoreView
 *
 * Retrieves all matching queries that have been added to the store view. See 
 * jana_store_view_add_match().
 *
 * Returns: A newly allocated #GList, containing all the #JanaStoreViewMatch 
 * structures added to the store view. This list must be freed with 
 * g_list_free().
 */
GList *
jana_store_view_get_matches (JanaStoreView *self)
{
	return JANA_STORE_VIEW_GET_INTERFACE (self)->get_matches (self);
}

/**
 * jana_store_view_remove_match:
 * @self: A #JanaStoreView
 * @match: A #JanaStoreViewMatch
 *
 * Removes a matching query from a store view. @match must have been returned 
 * by jana_store_view_add_match().
 */
void
jana_store_view_remove_match (JanaStoreView *self, JanaStoreViewMatch *match)
{
	JANA_STORE_VIEW_GET_INTERFACE (self)->remove_match (self, match);
}

/**
 * jana_store_view_clear_matches:
 * @self: A #JanaStoreView
 *
 * Removes all matching queries from a store view.
 */
void
jana_store_view_clear_matches (JanaStoreView *self)
{
	JANA_STORE_VIEW_GET_INTERFACE (self)->clear_matches (self);
}

/**
 * jana_store_view_start:
 * @self: A #JanaStoreView
 * 
 * Starts the view. No signals will be emitted prior to calling this function.
 */
void
jana_store_view_start (JanaStoreView *self)
{
	JANA_STORE_VIEW_GET_INTERFACE (self)->start (self);
}

/**
 * jana_store_view_get_store:
 * @self: A #JanaStoreView
 *
 * Retrieves the #JanaStore that this view operates on.
 *
 * Returns: The #JanaStore that this view operates on.
 */
JanaStore *
jana_store_view_get_store (JanaStoreView *self)
{
	return JANA_STORE_VIEW_GET_INTERFACE (self)->get_store (self);
}

