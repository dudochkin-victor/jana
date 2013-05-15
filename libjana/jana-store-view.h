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


#ifndef JANA_STORE_VIEW_H
#define JANA_STORE_VIEW_H

#include <glib-object.h>

#define JANA_TYPE_STORE_VIEW		(jana_store_view_get_type ())
#define JANA_STORE_VIEW(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj),\
					 JANA_TYPE_STORE_VIEW,\
					 JanaStoreView))
#define JANA_IS_STORE_VIEW(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
					 JANA_TYPE_STORE_VIEW))
#define JANA_STORE_VIEW_GET_INTERFACE(inst)\
					(G_TYPE_INSTANCE_GET_INTERFACE ((inst),\
					 JANA_TYPE_STORE_VIEW,\
					 JanaStoreViewInterface))

/**
 * JanaStoreView:
 *
 * The #JanaStoreView struct contains only private data.
 */
typedef struct _JanaStoreView JanaStoreView; /* Dummy object */
typedef struct _JanaStoreViewInterface JanaStoreViewInterface;

#include <libjana/jana-time.h>
#include <libjana/jana-store.h>

/**
 * JanaStoreViewField:
 * @JANA_STORE_VIEW_SUMMARY: An event summary
 * @JANA_STORE_VIEW_LOCATION: An event location
 * @JANA_STORE_VIEW_DESCRIPTION: An event description
 * @JANA_STORE_VIEW_AUTHOR: A note author
 * @JANA_STORE_VIEW_RECIPIENT: A note recipient
 * @JANA_STORE_VIEW_BODY: A note body
 * @JANA_STORE_VIEW_CATEGORY: A component category
 * @JANA_STORE_VIEW_ANYFIELD: Match any field
 *
 * Enum values for different types of field to use in the 
 * jana_store_view_set_match() function. Using field values for incorrect 
 * #JanaComponent types can have undefined results.
 **/
typedef enum {
	JANA_STORE_VIEW_SUMMARY,
	JANA_STORE_VIEW_LOCATION,
	JANA_STORE_VIEW_DESCRIPTION,
	JANA_STORE_VIEW_AUTHOR,
	JANA_STORE_VIEW_RECIPIENT,
	JANA_STORE_VIEW_BODY,
	JANA_STORE_VIEW_CATEGORY,
	JANA_STORE_VIEW_ANYFIELD,
} JanaStoreViewField;

/**
 * JanaStoreViewMatch:
 * @field: The #JanaStoreViewField to match against
 * @data: The string to match with
 *
 * struct that represents a matching query, returned by 
 * jana_store_view_add_match().
 */
typedef struct {
	JanaStoreViewField field;
	gchar *data;
} JanaStoreViewMatch;

struct _JanaStoreViewInterface {
	GTypeInterface parent;
	
	void	(*get_range)	(JanaStoreView *self, JanaTime **start,
					 JanaTime **end);
	
	void	(*set_range)	(JanaStoreView *self, JanaTime *start,
				 JanaTime *end);
	
	JanaStoreViewMatch *	(*add_match)	(JanaStoreView *self,
						 JanaStoreViewField field,
						 const gchar *data);
	
	GList *	(*get_matches)	(JanaStoreView *self);

	void	(*remove_match)	(JanaStoreView *self,
				 JanaStoreViewMatch *match);
	
	void	(*clear_matches)(JanaStoreView *self);
	
	void	(*start)	(JanaStoreView *self);
	
	JanaStore *	(*get_store)	(JanaStoreView *self);
	
	/* Signals */
	void	(*added)	(JanaStoreView *self, GList *components);
	void	(*modified)	(JanaStoreView *self, GList *components);
	void	(*removed)	(JanaStoreView *self, GList *uids);
	void	(*progress)	(JanaStoreView *self, gint percent);
};

GType jana_store_view_get_type (void);

void	jana_store_view_get_range	(JanaStoreView *self, JanaTime **start,
					 JanaTime **end);

void	jana_store_view_set_range	(JanaStoreView *self, JanaTime *start,
					 JanaTime *end);

JanaStoreViewMatch *jana_store_view_add_match (JanaStoreView *self,
					       JanaStoreViewField field,
					       const gchar *data);

GList * jana_store_view_get_matches	(JanaStoreView *self);

void	jana_store_view_remove_match	(JanaStoreView *self,
					 JanaStoreViewMatch *match);

void	jana_store_view_clear_matches	(JanaStoreView *self);

void	jana_store_view_start		(JanaStoreView *self);

JanaStore * jana_store_view_get_store	(JanaStoreView *self);

#endif /* JANA_STORE_VIEW_H */

