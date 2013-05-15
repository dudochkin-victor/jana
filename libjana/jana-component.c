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
 * SECTION:jana-component
 * @short_description: A basic store component interface
 *
 * #JanaComponent is the basic interface for a component in a #JanaStore. 
 * A component is uniquely identifiable, has a type and optionally, can store 
 * custom properties as key-value pairs.
 */

#include "jana-component.h"

static void
jana_component_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

GType
jana_component_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (JanaComponentInterface),
			jana_component_base_init,   /* base_init */
			NULL,

		};
		type = g_type_register_static (G_TYPE_INTERFACE,
			"JanaComponent", &info, 0);
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}
	return type;
}

/**
 * jana_component_get_component_type:
 * @self: A #JanaComponent
 *
 * Get the #JanaComponentType of @self. The component type will 
 * tell you if the component can be cast to a more specific type.
 *
 * Returns: The #JanaComponentType of @self.
 */
JanaComponentType
jana_component_get_component_type (JanaComponent *self)
{
	return JANA_COMPONENT_GET_INTERFACE (self)->get_component_type (self);
}

/**
 * jana_component_is_fully_represented:
 * @self: A #JanaComponent
 *
 * Determines whether the underlying data of @self is fully 
 * represented by the libjana interface. If it isn't, there may be data 
 * in the component that is not reachable via libjana and modifying the 
 * object may destroy this data.
 *
 * Returns: %TRUE if the underlying data of @self is fully represented by the 
 * libjana interface.
 */
gboolean
jana_component_is_fully_represented (JanaComponent *self)
{
	return JANA_COMPONENT_GET_INTERFACE (self)->is_fully_represented (self);
}

/**
 * jana_component_get_uid:
 * @self: A #JanaComponent
 *
 * Get a unique identifying string for @self. This can be used as the 
 * key in a hash table and does not change when modifying the component. A 
 * #JanaComponent that is not a part of a #JanaStore may not have a uid.
 *
 * This function returns a newly allocated string. To avoid this allocation
 * please use jana_component_peek_uid().
 *
 * Returns: The unique identifier of @self.
 */
gchar *
jana_component_get_uid (JanaComponent *self)
{
	return JANA_COMPONENT_GET_INTERFACE (self)->get_uid (self);
}

/**
 * jana_component_peek_uid:
 * @self: A #JanaComponent
 *
 * Get a unique identifying string for @self. This can be used as the 
 * key in a hash table and does not change when modifying the component. A 
 * #JanaComponent that is not a part of a #JanaStore may not have a uid.
 *
 * Unlike jana_component_get_uid() this function does not return a newly
 * allocated string. It is owned internally and must not be freed or
 * modified.
 *
 * Returns: The unique identifier of @self.
 */
const gchar *
jana_component_peek_uid (JanaComponent *self)
{
	return JANA_COMPONENT_GET_INTERFACE (self)->peek_uid (self);
}

/**
 * jana_component_get_categories:
 * @self: A #JanaComponent
 *
 * Retrieves the list of categories this component is filed under. See
 * jana_component_set_categories().
 *
 * Returns: A newly allocated, NULL-terminated string array, containing the 
 * component categories, to be freed with g_strfreev().
 */
gchar **
jana_component_get_categories (JanaComponent *self)
{
	return JANA_COMPONENT_GET_INTERFACE (self)->get_categories (self);
}

/**
 * jana_component_set_categories:
 * @self: A #JanaComponent
 * @categories: A %NULL-terminated array of strings, or %NULL
 *
 * Sets or clears the component's category list, overriding any previous set. 
 * list. @categories should be an array of %NULL-terminated UTF-8 strings, and  
 * the final member of the array should be %NULL.
 */
void
jana_component_set_categories (JanaComponent *self, const gchar **categories)
{
	JANA_COMPONENT_GET_INTERFACE (self)->set_categories (self, categories);
}

/**
 * jana_component_supports_custom_props:
 * @self: A #JanaComponent
 *
 * Determines whether @self supports the setting and retrieval 
 * of custom properties.
 *
 * Returns: %TRUE if @self supports custom properties, %FALSE otherwise.
 */
gboolean
jana_component_supports_custom_props (JanaComponent *self)
{
	return JANA_COMPONENT_GET_INTERFACE (self)->
		supports_custom_props (self);
}

/**
 * jana_component_get_custom_props_list:
 * @self: A #JanaComponent
 *
 * Get a #GList of properties set on @self. The data component of each  
 * list element contains an array of two strings. The first string is the 
 * property name, the second the value. This list can be freed using
 * jana_component_props_list_free().
 *
 * Returns: A #GList containing the properties set on @self.
 */
GList *
jana_component_get_custom_props_list (JanaComponent *self)
{
	return JANA_COMPONENT_GET_INTERFACE (self)->
		get_custom_props_list (self);
}

/**
 * jana_component_get_custom_prop:
 * @self: A #JanaComponent
 * @name: The key name of the property
 *
 * Retrieve a custom property set on @self.
 *
 * Returns: A string associated with the custom property, @name, or %NULL 
 * if the property has not been set.
 */
gchar *
jana_component_get_custom_prop (JanaComponent *self, const gchar *name)
{
	return JANA_COMPONENT_GET_INTERFACE (self)->
		get_custom_prop (self, name);
}

/**
 * jana_component_set_custom_prop:
 * @self: A #JanaComponent
 * @name: The key name of the property
 * @value: The value of the property
 *
 * Set a property on @self. If the property has been set previously, the value 
 * will be overwritten. Implementations of #JanaComponent may require that 
 * property names conform to a particular specification.
 *
 * Returns: %TRUE if the property was set successfully, %FALSE otherwise.
 */
gboolean
jana_component_set_custom_prop (JanaComponent *self, const gchar *name,
				const gchar *value)
{
	return JANA_COMPONENT_GET_INTERFACE (self)->
		set_custom_prop (self, name, value);
}

/**
 * jana_component_props_list_free:
 * @props: A property list returned by jana_component_get_custom_props_list()
 *
 * Frees a #JanaComponent property list, returned by 
 * jana_component_get_custom_props_list().
 */
void
jana_component_props_list_free (GList *props)
{
	while (props) {
		gchar **prop = (gchar **)props->data;
		g_free (prop[0]);
		g_free (prop[1]);
		g_free (prop);
		props = g_list_delete_link (props, props);
	}
}

