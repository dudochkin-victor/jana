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


#ifndef JANA_COMPONENT_H
#define JANA_COMPONENT_H

#include <glib-object.h>

#define JANA_TYPE_COMPONENT		(jana_component_get_type ())
#define JANA_COMPONENT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj),\
					 JANA_TYPE_COMPONENT, JanaComponent))
#define JANA_IS_COMPONENT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
					 JANA_TYPE_COMPONENT))
#define JANA_COMPONENT_GET_INTERFACE(inst)\
					(G_TYPE_INSTANCE_GET_INTERFACE ((inst),\
					 JANA_TYPE_COMPONENT,\
					 JanaComponentInterface))

/**
 * JanaComponent:
 *
 * The #JanaComponent struct contains only private data.
 */
typedef struct _JanaComponent JanaComponent; /* Dummy object */
typedef struct _JanaComponentInterface JanaComponentInterface;

/**
 * JanaComponentType:
 * @JANA_COMPONENT_NULL: No specific type
 * @JANA_COMPONENT_EVENT: Event type. Component can be cast to a #JanaEvent
 * @JANA_COMPONENT_NOTE: Note type. Component can be cast to a #JanaNote
 * @JANA_COMPONENT_TASK: Task type. Component can be cast to a #JanaTask
 *
 * Enum values for different types of component.
 *
 **/
typedef enum {
	JANA_COMPONENT_NULL,
	JANA_COMPONENT_EVENT,
	JANA_COMPONENT_NOTE,
	JANA_COMPONENT_TASK,
} JanaComponentType;

struct _JanaComponentInterface {
	GTypeInterface parent;
	
	JanaComponentType	(*get_component_type)	(JanaComponent *self);
	gboolean		(*is_fully_represented)	(JanaComponent *self);

	gchar * 	(*get_uid)			(JanaComponent *self);
	const gchar * 	(*peek_uid)			(JanaComponent *self);
	gchar **	(*get_categories)		(JanaComponent *self);
	void		(*set_categories)	(JanaComponent *self,
						 const gchar **categories);
	
	gboolean	(*supports_custom_props)	(JanaComponent *self);
	GList *		(*get_custom_props_list)	(JanaComponent *self);
	gchar *		(*get_custom_prop)		(JanaComponent *self,
							 const gchar *name);
	gboolean	(*set_custom_prop)		(JanaComponent *self,
							 const gchar *name,
							 const gchar *value);
};

GType jana_component_get_type (void);

JanaComponentType 	jana_component_get_component_type
						(JanaComponent *self);
gboolean		jana_component_is_fully_represented
						(JanaComponent *self);

gchar *		jana_component_get_uid		(JanaComponent *self);
const gchar *	jana_component_peek_uid		(JanaComponent *self);
gchar **	jana_component_get_categories	(JanaComponent *self);
void		jana_component_set_categories	(JanaComponent *self,
						 const gchar **categories);

gboolean	jana_component_supports_custom_props	(JanaComponent *self);
GList *		jana_component_get_custom_props_list	(JanaComponent *self);
gchar *		jana_component_get_custom_prop		(JanaComponent *self,
							 const gchar *name);
gboolean	jana_component_set_custom_prop		(JanaComponent *self,
							 const gchar *name,
							 const gchar *value);

/* Props list is a list of key-name pairs as gchar **'s */
void		jana_component_props_list_free		(GList *props);

#endif /* JANA_COMPONENT_H */

