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
 * SECTION:jana-ecal-component
 * @short_description: An implementation of #JanaComponent using libecal
 *
 * #JanaEcalComponent is an implementation of #JanaComponent that provides a 
 * wrapper over #ECalComponent, using libecal.
 */

#define HANDLE_LIBICAL_MEMORY 1

#include "jana-ecal-component.h"
#include "jana-ecal-time.h"
#include <libjana/jana-utils.h>
#include <string.h>

static void component_interface_init (gpointer g_iface, gpointer iface_data);

static JanaComponentType	component_get_component_type
							(JanaComponent *self);
static gboolean			component_is_fully_represented
							(JanaComponent *self);
static gchar *			component_get_uid	(JanaComponent *self);
static const gchar *		component_peek_uid	(JanaComponent *self);
static gchar **			component_get_categories(JanaComponent *self);
static void			component_set_categories(JanaComponent *self,
						const gchar **categories);

static gboolean		component_supports_custom_props	(JanaComponent *self);
static GList *		component_get_custom_props_list	(JanaComponent *self);
static gchar *		component_get_custom_prop	(JanaComponent *self,
							 const gchar *name);
static gboolean		component_set_custom_prop	(JanaComponent *self,
							 const gchar *name,
							 const gchar *value);

G_DEFINE_TYPE_WITH_CODE (JanaEcalComponent, 
                        jana_ecal_component, 
                        G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE (JANA_TYPE_COMPONENT,
                                               component_interface_init));

#define COMPONENT_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
			  JANA_ECAL_TYPE_COMPONENT, JanaEcalComponentPrivate))


typedef struct _JanaEcalComponentPrivate JanaEcalComponentPrivate;

struct _JanaEcalComponentPrivate
{
	ECalComponent *comp;
};

enum {
	PROP_ECALCOMP = 1,
};

static void
jana_ecal_component_get_property (GObject *object, guint property_id,
			      GValue *value, GParamSpec *pspec)
{
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (object);

	switch (property_id) {
	    case PROP_ECALCOMP :
		g_value_set_object (value, priv->comp);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_ecal_component_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (object);

	switch (property_id) {
	    case PROP_ECALCOMP :
		priv->comp = E_CAL_COMPONENT (g_value_dup_object (value));
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_ecal_component_dispose (GObject *object)
{
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (object);

	if (priv->comp) {
		g_object_unref (priv->comp);
		priv->comp = NULL;
	}

	if (G_OBJECT_CLASS (jana_ecal_component_parent_class)->dispose)
		G_OBJECT_CLASS (jana_ecal_component_parent_class)->dispose (object);
}

static void
jana_ecal_component_finalize (GObject *object)
{
	G_OBJECT_CLASS (jana_ecal_component_parent_class)->finalize (object);
}

static void
component_interface_init (gpointer g_iface, gpointer iface_data)
{
	JanaComponentInterface *iface = (JanaComponentInterface *)g_iface;
	
	iface->get_component_type = component_get_component_type;
	iface->is_fully_represented = component_is_fully_represented;
	iface->get_uid = component_get_uid;
	iface->peek_uid = component_peek_uid;
	iface->get_categories = component_get_categories;
	iface->set_categories = component_set_categories;
	
	iface->supports_custom_props = component_supports_custom_props;
	iface->get_custom_props_list = component_get_custom_props_list;
	iface->get_custom_prop = component_get_custom_prop;
	iface->set_custom_prop = component_set_custom_prop;
}

static void
jana_ecal_component_class_init (JanaEcalComponentClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (JanaEcalComponentPrivate));

	object_class->get_property = jana_ecal_component_get_property;
	object_class->set_property = jana_ecal_component_set_property;
	object_class->dispose = jana_ecal_component_dispose;
	object_class->finalize = jana_ecal_component_finalize;

	g_object_class_install_property (
		object_class,
		PROP_ECALCOMP,
		g_param_spec_object (
			"ecalcomp",
			"ECalComponent *",
			"The ECalComponent represented by this JanaComponent "
			"object.",
			E_TYPE_CAL_COMPONENT,
			G_PARAM_READABLE | G_PARAM_WRITABLE |
			G_PARAM_CONSTRUCT_ONLY));
}

static void
jana_ecal_component_init (JanaEcalComponent *self)
{
}

/**
 * jana_ecal_component_new_from_ecalcomp:
 * @component: An #ECalComponent
 *
 * Creates a new #JanaEcalComponent from an #ECalComponent.
 *
 * Returns: A new #JanaEcalComponent that wraps the given #ECalComponent, 
 * cast as a #JanaComponent.
 */
JanaComponent *
jana_ecal_component_new_from_ecalcomp (ECalComponent *component)
{
	return JANA_COMPONENT (g_object_new (JANA_ECAL_TYPE_COMPONENT,
		"ecalcomp", component, NULL));
}

/**
 * jana_ecal_component_get_summary:
 * @self: A #JanaEcalComponent
 *
 * Retrieves the summary from the underlying #ECalComponent. This function 
 * is intended for using only when extending #JanaEcalComponent.
 *
 * Returns: A newly allocated string with the summary from the underlying 
 * #ECalComponent, or %NULL.
 */
gchar *
jana_ecal_component_get_summary (JanaEcalComponent *self)
{
	ECalComponentText summary;
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (self);
	
	e_cal_component_get_summary (priv->comp, &summary);
	
	if (summary.value) return g_strdup (summary.value);
	/*else if (summary.altrep) return g_strdup (summary.altrep);*/
	else return NULL;
}

/**
 * jana_ecal_component_get_description:
 * @self: A #JanaEcalComponent
 *
 * Retrieves the first description from the underlying #ECalComponent. This 
 * function is intended for using only when extending #JanaEcalComponent.
 *
 * Returns: A newly allocated string with the first description from the 
 * underlying #ECalComponent, or %NULL.
 */
gchar *
jana_ecal_component_get_description (JanaEcalComponent *self)
{
	GSList *desc_list;
	ECalComponentText *desc;
	gchar *desc_copy = NULL;
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (self);
	
	/* Note, that only journal components are allowed to have more than one
	 * description. It's ok to just take the first here.
	 */
	e_cal_component_get_description_list (priv->comp, &desc_list);
	if (desc_list) {
		desc = desc_list->data;
		if (desc->value)
			desc_copy = g_strdup (desc->value);
	}
	
	e_cal_component_free_text_list (desc_list);
	
	return desc_copy;
}

/**
 * jana_ecal_component_get_location:
 * @self: A #JanaEcalComponent
 *
 * Retrieves the location from the underlying #ECalComponent. This function 
 * is intended for using only when extending #JanaEcalComponent.
 *
 * Returns: A newly allocated string with the location from the 
 * underlying #ECalComponent, or %NULL.
 */
gchar *
jana_ecal_component_get_location (JanaEcalComponent *self)
{
	const char *location;
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (self);
	
	e_cal_component_get_location (priv->comp, &location);
	
	return location ? g_strdup (location) : NULL;
}

/**
 * jana_ecal_component_set_summary:
 * @self: A #JanaEcalComponent
 * @summary: A UTF-8 string
 *
 * Sets the summary on the underlying #ECalComponent. This function 
 * is intended for using only when extending #JanaEcalComponent.
 */
void
jana_ecal_component_set_summary (JanaEcalComponent *self, const gchar *summary)
{
	ECalComponentText summary_text;
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (self);
	
	summary_text.value = summary;
	summary_text.altrep = NULL;
	
	e_cal_component_set_summary (priv->comp, &summary_text);
}

/**
 * jana_ecal_component_set_description:
 * @self: A #JanaEcalComponent
 * @description: A UTF-8 string
 *
 * Sets the description on the underlying #ECalComponent. This function 
 * is intended for using only when extending #JanaEcalComponent.
 */
void
jana_ecal_component_set_description (JanaEcalComponent *self,
				     const gchar *description)
{
	ECalComponentText desc_text;
	GSList *text_list;
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (self);
	
	desc_text.value = description;
	desc_text.altrep = NULL;
	text_list = g_slist_append (NULL, &desc_text);
	
	e_cal_component_set_description_list (priv->comp, text_list);
	
	g_slist_free (text_list);
}

/**
 * jana_ecal_component_set_location:
 * @self: A #JanaEcalComponent
 * @location: A UTF-8 string
 *
 * Sets the location on the underlying #ECalComponent. This function 
 * is intended for using only when extending #JanaEcalComponent.
 */
void
jana_ecal_component_set_location (JanaEcalComponent *self,
				  const gchar *location)
{
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (self);
	e_cal_component_set_location (priv->comp, location);
}

/**
 * jana_ecal_component_get_start:
 * @self: A #JanaEcalComponent
 *
 * Retrieves the dtstart of the underlying #ECalComponent. This function 
 * is intended for using only when extending #JanaEcalComponent.
 *
 * Returns: A #JanaTime representing the dtstart of the underlying 
 * #ECalComponent, or %NULL.
 */
JanaTime *
jana_ecal_component_get_start (JanaEcalComponent *self)
{
	ECalComponentDateTime dt;
	JanaTime *time;
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (self);

	dt.value = NULL;	
	e_cal_component_get_dtstart (priv->comp, &dt);
	
	if (dt.value) {
		time = jana_ecal_time_new_from_ecaltime (&dt);
		e_cal_component_free_datetime (&dt);
	} else {
		const gchar *location;
		
		/* A NULL time means 'from the beginning of time', so create
		 * a very early time as substitute.
		 */
		time = jana_ecal_time_new ();
		
		e_cal_component_get_location (priv->comp, &location);
		if (location)
			jana_ecal_time_set_location (
				JANA_ECAL_TIME (time), location);
		
		jana_time_set_year (time, -G_MAXINT);
		jana_time_set_month (time, 1);
		jana_time_set_day (time, 1);
		jana_time_set_isdate (time, TRUE);
	}
	
	return time;
}

/**
 * jana_ecal_component_get_end:
 * @self: A #JanaEcalComponent
 *
 * Retrieves the dtend of the underlying #ECalComponent. This function 
 * is intended for using only when extending #JanaEcalComponent.
 *
 * Returns: A #JanaTime representing the dtend of the underlying 
 * #ECalComponent, or %NULL.
 */
JanaTime *
jana_ecal_component_get_end (JanaEcalComponent *self)
{
	ECalComponentDateTime dt;
	JanaTime *time;
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (self);

	dt.value = NULL;	
	e_cal_component_get_dtend (priv->comp, &dt);
	time = jana_ecal_time_new_from_ecaltime (&dt);
	if (dt.value) e_cal_component_free_datetime (&dt);
	return time;
}

/**
 * jana_ecal_component_set_start:
 * @self: A #JanaEcalComponent
 * @start: A #JanaTime
 *
 * Sets the dtstart on the underlying #ECalComponent. This function 
 * is intended for using only when extending #JanaEcalComponent.
 */
void
jana_ecal_component_set_start (JanaEcalComponent *self, JanaTime *start)
{
	JanaEcalTime *time;
	icaltimetype *icaltime;
	ECalComponentDateTime dt;
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (self);
	
	if (JANA_ECAL_IS_TIME (start)) {
		time = g_object_ref (JANA_ECAL_TIME (start));
	} else {
		time = JANA_ECAL_TIME (jana_utils_time_copy (
			start, jana_ecal_time_new ()));
	}
	
	g_object_get (time, "icaltime", &icaltime, NULL);
	
	/* vaue, tzid */
	dt.value = icaltime;
	if (icaltime_is_utc (*icaltime))
		dt.tzid = "UTC";
	else
		dt.tzid = (const char *)icaltimezone_get_tzid (
			(icaltimezone *)icaltime->zone);
	
	e_cal_component_set_dtstart (priv->comp, &dt);
	
	g_object_unref (time);
}

/**
 * jana_ecal_component_set_end:
 * @self: A #JanaEcalComponent
 * @end: A #JanaTime
 *
 * Sets the dtend on the underlying #ECalComponent. This function 
 * is intended for using only when extending #JanaEcalComponent.
 */
void
jana_ecal_component_set_end (JanaEcalComponent *self, JanaTime *end)
{
	JanaEcalTime *time;
	icaltimetype *icaltime;
	ECalComponentDateTime dt;
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (self);
	
	if (JANA_ECAL_IS_TIME (end)) {
		time = g_object_ref (JANA_ECAL_TIME (end));
	} else {
		time = JANA_ECAL_TIME (jana_utils_time_copy (
			end, jana_ecal_time_new ()));
	}
	
	g_object_get (time, "icaltime", &icaltime, NULL);
	
	/* vaue, tzid */
	dt.value = icaltime;
	if (icaltime_is_utc (*icaltime))
		dt.tzid = "UTC";
	else
		dt.tzid = (const char *)icaltimezone_get_tzid (
			(icaltimezone *)icaltime->zone);
	
	e_cal_component_set_dtend (priv->comp, &dt);
	
	g_object_unref (time);
}

static JanaComponentType
component_get_component_type (JanaComponent *self)
{
	/* Sub-classes should override this method */
	return JANA_COMPONENT_NULL;
}

static gboolean
component_is_fully_represented (JanaComponent *self)
{
	/* Sub-classes should override this method */
	return FALSE;
}

static gchar *
component_get_uid (JanaComponent *self)
{
	const char *uid = NULL;
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (self);
	
	e_cal_component_get_uid (priv->comp, &uid);
	
	return uid ? g_strdup (uid) : NULL;
}

static const gchar *
component_peek_uid (JanaComponent *self)
{
	const char *uid = NULL;
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (self);
	
	e_cal_component_get_uid (priv->comp, &uid);
	
	return uid;
}

static gchar **
component_get_categories (JanaComponent *self)
{
	const char *categories;
	gchar **category_list;
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (self);
	
	e_cal_component_get_categories (priv->comp, &categories);

	if (categories) {
		category_list = g_strsplit (categories, ",", 0);
		return category_list;
	} else
		return NULL;
}

static void
component_set_categories (JanaComponent *self, const gchar **categories)
{
	gchar *categories_joined;
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (self);
	
	categories_joined = g_strjoinv (",", (gchar **)categories);
	/* Note: Setting an empty categories list on an event without a
	 *       category set will cause libical to crash.
	 */
	if ((!categories_joined) || (categories_joined[0] == '\0')) {
		const gchar *categories;
		e_cal_component_get_categories (priv->comp, &categories);
		if (categories)
			e_cal_component_set_categories (priv->comp, NULL);
	} else {
		e_cal_component_set_categories (priv->comp, categories_joined);
	}
	g_free (categories_joined);
}

static gboolean
component_supports_custom_props (JanaComponent *self)
{
	return TRUE;
}

static GList *
component_get_custom_props_list (JanaComponent *self)
{
	icalproperty *prop;
	icalcomponent *comp;
	GList *props = NULL;
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (self);
	
	comp = e_cal_component_get_icalcomponent (priv->comp);
	
	for (prop = icalcomponent_get_first_property (comp, ICAL_X_PROPERTY);
	     prop; prop = icalcomponent_get_next_property (
	     comp, ICAL_X_PROPERTY)) {
		gchar **prop_pair = g_new (gchar *, 2);
		gchar *name = g_strdup (icalproperty_get_x_name (prop));
#ifdef LIBICAL_MEMFIXES
		gchar *value = icalproperty_get_value_as_string (prop);
#else
		gchar *value = g_strdup (icalproperty_get_value_as_string (
			prop));
#endif
		prop_pair[0] = name;
		prop_pair[1] = value;
		props = g_list_prepend (props, prop_pair);
	}
	
	return props;
}

static gchar *
component_get_custom_prop (JanaComponent *self, const gchar *name)
{
	icalproperty *prop;
	icalcomponent *comp;
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (self);
	
	comp = e_cal_component_get_icalcomponent (priv->comp);
	
	/* See if the property exists first */
	for (prop = icalcomponent_get_first_property (comp, ICAL_X_PROPERTY);
	     prop; prop = icalcomponent_get_next_property (
	     comp, ICAL_X_PROPERTY)) {
		if (strcmp (icalproperty_get_x_name (prop), name) == 0) {
#ifdef LIBICAL_MEMFIXES
			return icalproperty_get_value_as_string (prop);
#else
			return g_strdup (icalproperty_get_value_as_string (
				prop));
#endif
		}
	}
	
	return NULL;
}

static gboolean
component_set_custom_prop (JanaComponent *self, const gchar *name,
			   const gchar *value)
{
	icalproperty *prop;
	icalcomponent *comp;
	gboolean exists = FALSE;
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (self);
	
	if (strncmp ("X-", name, 2) != 0) return FALSE;
	
	comp = e_cal_component_get_icalcomponent (priv->comp);
	
	/* See if the property exists first */
	for (prop = icalcomponent_get_first_property (comp, ICAL_X_PROPERTY);
	     prop; prop = icalcomponent_get_next_property (
	     comp, ICAL_X_PROPERTY)) {
		if (strcmp (icalproperty_get_x_name (prop), name) == 0) {
			exists = TRUE;
			break;
		}
	}
	
	if (!exists) {
		/* Create a new property */
		prop = icalproperty_new (ICAL_X_PROPERTY);
		icalproperty_set_x_name (prop, name);
	}
	
	icalproperty_set_value (prop, icalvalue_new_from_string (
		ICAL_X_VALUE, value));
	
	if (!exists) {
		icalcomponent_add_property (comp, prop);
		/* FIXME: Check that the property added without errors - no
		 *        idea how to do this :(
		 */
		/*if (icalproperty_isa (prop) != ICAL_X_PROPERTY) {
			icalcomponent_remove_property (comp, prop);
			icalproperty_free (prop);
			return FALSE;
		}*/
	}
	
	return TRUE;
}

/**
 * jana_ecal_component_get_recurrence_id:
 * @self: A #JanaEcalComponent
 *
 * Gets a string representing the recurremce for this component instance.
 *
 * Returns: A newly allocated string for the recurrence.
 */
gchar *
jana_ecal_component_get_recurrence_id (JanaEcalComponent *self)
{
	JanaEcalComponentPrivate *priv = COMPONENT_PRIVATE (self);

	return e_cal_component_get_recurid_as_string (priv->comp);
}

