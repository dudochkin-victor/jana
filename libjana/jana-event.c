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
 * SECTION:jana-event
 * @short_description: An event component interface
 * @see_also: #JanaComponent
 *
 * #JanaEvent is the interface for components that store information on 
 * calendar events. It has functions for basic event description, as well as 
 * recurrences, exceptions and alarms.
 */

#include <string.h>
#include "jana-event.h"

static void
jana_event_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

GType
jana_event_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (JanaEventInterface),
			jana_event_base_init,   /* base_init */
			NULL,

		};
		type = g_type_register_static (G_TYPE_INTERFACE,
			"JanaEvent", &info, 0);
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}
	return type;
}

GType
jana_recurrence_get_type (void)
{
	static GType our_type = 0;

	if (!our_type)
		our_type = g_boxed_type_register_static ("JanaRecurrence",
			(GBoxedCopyFunc) jana_recurrence_copy,
			(GBoxedFreeFunc) jana_recurrence_free);

	return our_type;
}

/**
 * jana_recurrence_new:
 *
 * Creates a new #JanaRecurrence. This will be an infinite, daily recurrence, 
 * with an interval of 1. All other fields will be left %NULL or %FALSE.
 *
 * Returns: A newly allocated #JanaRecurrence, to be freed with 
 * jana_recurrence_free().
 */
JanaRecurrence *
jana_recurrence_new ()
{
	JanaRecurrence *recur = g_slice_new0 (JanaRecurrence);
	/* JANA_RECURRENCE_DAILY is already zero, but set in case it changes */
	recur->type = JANA_RECURRENCE_DAILY;
	recur->interval = 1;
	return recur;
}

/**
 * jana_recurrence_copy:
 * @recurrence: A #JanaRecurrence
 *
 * Creates a copy of a #JanaRecurrence.
 *
 * Returns: A newly allocated copy of @recurrence, to be freed with 
 * jana_recurrence_free().
 */
JanaRecurrence *
jana_recurrence_copy (JanaRecurrence *recurrence)
{
	JanaRecurrence *recurrence_copy;
	
	if (!recurrence) return NULL;
	
	recurrence_copy = jana_recurrence_new ();
	g_memmove (recurrence_copy, recurrence, sizeof (JanaRecurrence));
	recurrence_copy->end = recurrence->end ?
		jana_time_duplicate (recurrence->end) : NULL;
	
	return recurrence_copy;
}

/**
 * jana_recurrence_free:
 * @recurrence: A #JanaRecurrence
 *
 * Frees the memory associated with @recurrence. If @recurrence is %NULL, 
 * does nothing.
 */
void
jana_recurrence_free (JanaRecurrence *recurrence)
{
	if (!recurrence) return;
	
	if (recurrence->end) g_object_unref (recurrence->end);
	g_slice_free (JanaRecurrence, recurrence);
}

/**
 * jana_exceptions_free:
 * @exceptions: A list of exceptions, as returned by 
 * jana_event_get_exceptions ()
 *
 * Frees a list of exceptions. See jana_event_get_exceptions().
 */
void
jana_exceptions_free (GList *exceptions)
{
	GList *e;
	for (e = exceptions; e; e = e->next)
		g_object_unref (G_OBJECT (e->data));
	g_list_free (exceptions);
}

/**
 * jana_event_get_summary:
 * @self: A #JanaEvent
 *
 * Retrieves the summary associated with the event.
 *
 * Returns: A newly allocated string, containing the summary.
 * See jana_event_set_summary().
 */
gchar *
jana_event_get_summary (JanaEvent *self)
{
	return JANA_EVENT_GET_INTERFACE (self)->get_summary (self);
}

/**
 * jana_event_get_description:
 * @self: A #JanaEvent
 *
 * Retrieves the description associated with the event.
 *
 * Returns: A newly allocated string, containing the description. See 
 * jana_event_set_description().
 */
gchar *
jana_event_get_description (JanaEvent *self)
{
	return JANA_EVENT_GET_INTERFACE (self)->get_description (self);
}

/**
 * jana_event_get_location:
 * @self: A #JanaEvent
 *
 * Retrieves the location associated with the event. Note that this is just 
 * a field designated for a user-specified location, and as such, is not related
 * to time-zones and has no particular format. See jana_event_set_location().
 *
 * Returns: A newly allocated string, containing the location.
 */
gchar *
jana_event_get_location (JanaEvent *self)
{
	return JANA_EVENT_GET_INTERFACE (self)->get_location (self);
}

/**
 * jana_event_get_start:
 * @self: A #JanaEvent
 *
 * Retrieves the time at which the event starts. See jana_event_set_start().
 *
 * Returns: A referenced #JanaTime for the start of the event.
 */
JanaTime *
jana_event_get_start (JanaEvent *self)
{
	return JANA_EVENT_GET_INTERFACE (self)->get_start (self);
}

/**
 * jana_event_get_end:
 * @self: A #JanaEvent
 *
 * Retrieves the time at which the event ends. See jana_event_set_end().
 *
 * Returns: A referenced #JanaTime for the end of the event.
 */
JanaTime *
jana_event_get_end (JanaEvent *self)
{
	return JANA_EVENT_GET_INTERFACE (self)->get_end (self);
}

/**
 * jana_event_supports_alarm:
 * @self: A #JanaEvent
 *
 * Determines whether the event supports setting an alarm.
 *
 * Returns: %TRUE if the event supports setting an alarm, %FALSE otherwise.
 */
gboolean
jana_event_supports_alarm (JanaEvent *self)
{
	return JANA_EVENT_GET_INTERFACE (self)->supports_alarm (self);
}

/**
 * jana_event_has_alarm:
 * @self: A #JanaEvent
 *
 * Determines whether the event has an alarm set on it.
 *
 * Returns: %TRUE if the event has an alarm set, %FALSE otherwise.
 */
gboolean
jana_event_has_alarm (JanaEvent *self)
{
	return JANA_EVENT_GET_INTERFACE (self)->has_alarm (self);
}

/**
 * jana_event_get_alarm_time:
 * @self: A #JanaEvent
 *
 * Retrieves the alarm time set on the event. See jana_event_set_alarm_time().
 *
 * Returns: A referenced #JanaTime for the alarm set on the event, or %NULL if 
 * there is no alarm set.
 */
JanaTime *
jana_event_get_alarm_time (JanaEvent *self)
{
	return JANA_EVENT_GET_INTERFACE (self)->get_alarm_time (self);
}

/**
 * jana_event_supports_recurrence:
 * @self: A #JanaEvent
 *
 * Determines whether the event supports setting a recurrence.
 *
 * Returns: %TRUE if the event supports setting a recurrence, %FALSE otherwise.
 */
gboolean
jana_event_supports_recurrence (JanaEvent *self)
{
	return JANA_EVENT_GET_INTERFACE (self)->supports_recurrence (self);
}

/**
 * jana_event_has_recurrence:
 * @self: A #JanaEvent
 *
 * Determines whether the event has a set recurrence.
 *
 * Returns: %TRUE if the event has a set recurrence, %FALSE otherwise.
 */
gboolean
jana_event_has_recurrence (JanaEvent *self)
{
	return JANA_EVENT_GET_INTERFACE (self)->has_recurrence (self);
}

/**
 * jana_event_get_recurrence:
 * @self: A #JanaEvent
 *
 * Retrieves the recurrence description set on the event.
 *
 * Returns: A newly allocated #JanaRecurrence, or %NULL if there is no 
 * set recurrence. See jana_event_get_recurrence().
 */
JanaRecurrence	*
jana_event_get_recurrence (JanaEvent *self)
{
	return JANA_EVENT_GET_INTERFACE (self)->get_recurrence (self);
}

/**
 * jana_event_supports_exceptions:
 * @self: A #JanaEvent
 *
 * Determines whether the event supports setting an exceptions list.
 *
 * Returns: %TRUE if the event supports setting exceptions, %FALSE otherwise.
 */
gboolean
jana_event_supports_exceptions (JanaEvent *self)
{
	return JANA_EVENT_GET_INTERFACE (self)->supports_exceptions (self);
}

/**
 * jana_event_has_exceptions:
 * @self: A #JanaEvent
 *
 * Determines whether the event has an exceptions list.
 *
 * Returns: %TRUE if the event has an exceptions list, %FALSE otherwise.
 */
gboolean
jana_event_has_exceptions (JanaEvent *self)
{
	return JANA_EVENT_GET_INTERFACE (self)->has_exceptions (self);
}

/**
 * jana_event_get_exceptions:
 * @self: A #JanaEvent
 *
 * Retrieves the event's exceptions list. See jana_event_set_exceptions().
 *
 * Returns: A list of dates, as #JanaTime, on which the event does not occur, 
 * or %NULL if there is no set exceptions list. This list should be freed with 
 * jana_exceptions_free().
 */
GList *
jana_event_get_exceptions (JanaEvent *self)
{
	return JANA_EVENT_GET_INTERFACE (self)->get_exceptions (self);
}

/**
 * jana_event_set_summary:
 * @self: A #JanaEvent
 * @summary: A summary string
 *
 * Sets the event's summary. Ideally, a summary should be a short, one-line 
 * description of the event.
 */
void
jana_event_set_summary (JanaEvent *self, const gchar *summary)
{
	JANA_EVENT_GET_INTERFACE (self)->set_summary (self, summary);
}

/**
 * jana_event_set_description:
 * @self: A #JanaEvent
 * @description: A description string
 *
 * Sets the event's description. A description can elaborate on the event's 
 * summary and include event details that do not fit, or are not appropriate 
 * in other fields.
 */
void
jana_event_set_description (JanaEvent *self, const gchar *description)
{
	JANA_EVENT_GET_INTERFACE (self)->set_description (self, description);
}

/**
 * jana_event_set_location:
 * @self: A #JanaEvent
 * @location: A location string
 *
 * Sets the event's location. Ideally, a location should be a short, one-line 
 * summary of the event's location. A full address would be more suited to the 
 * description field (see jana_event_set_description()).
 */
void
jana_event_set_location (JanaEvent *self, const gchar *location)
{
	JANA_EVENT_GET_INTERFACE (self)->set_location (self, location);
}

/**
 * jana_event_set_start:
 * @self: A #JanaEvent
 * @start: A #JanaTime
 *
 * Sets the event's starting time.
 */
void
jana_event_set_start (JanaEvent *self, JanaTime *start)
{
	JANA_EVENT_GET_INTERFACE (self)->set_start (self, start);
}

/**
 * jana_event_set_end:
 * @self: A #JanaEvent
 * @end: A #JanaTime
 *
 * Sets the event's ending time. This should not occur on or before the 
 * starting time. Setting an ending time before a starting time may cause 
 * unpredictable results, depending on the implementation of #JanaEvent.
 */
void
jana_event_set_end (JanaEvent *self, JanaTime *end)
{
	JANA_EVENT_GET_INTERFACE (self)->set_end (self, end);
}

/**
 * jana_event_set_alarm:
 * @self: A #JanaEvent
 * @time: A #JanaTime, or %NULL
 *
 * Sets or clears the event's alarm time. This function does nothing if the 
 * event does not support alarms. See jana_event_supports_alarm().
 */
void
jana_event_set_alarm (JanaEvent *self, JanaTime *time)
{
	JANA_EVENT_GET_INTERFACE (self)->set_alarm (self, time);
}

/**
 * jana_event_set_recurrence:
 * @self: A #JanaEvent
 * @recurrence: A #JanaRecurrence, or %NULL
 *
 * Sets or clears the event's recurrence rule. This function does nothing if 
 * the event does not support recurrence. See jana_event_supports_recurrence().
 */
void
jana_event_set_recurrence (JanaEvent *self, const JanaRecurrence *recurrence)
{
	JANA_EVENT_GET_INTERFACE (self)->set_recurrence (self, recurrence);
}

/**
 * jana_event_set_exceptions:
 * @self: A #JanaEvent
 * @exceptions: A list of #JanaTime objects.
 *
 * Sets the event's exceptions list. For each #JanaTime in @exceptions, the 
 * event is considered not to occur, even if its start/end or recurrence rule 
 * would dictate otherwise. This function does nothing if the event does not 
 * support exceptions. See jana_event_supports_exceptions().
 */
void
jana_event_set_exceptions (JanaEvent *self, GList *exceptions)
{
	JANA_EVENT_GET_INTERFACE (self)->set_exceptions (self, exceptions);
}

/**
 * jana_event_get_categories:
 * @self: A #JanaEvent
 *
 * See jana_component_get_categories().
 *
 * Deprecated: Use jana_component_get_categories() instead.
 *
 * Returns: A newly allocated, %NULL-terminated array of strings, or %NULL.
 */
gchar **
jana_event_get_categories (JanaEvent *self)
{
	return jana_component_get_categories (JANA_COMPONENT (self));
}

/**
 * jana_event_set_categories:
 * @self: A #JanaEvent
 * @categories: A %NULL-terminated array of strings, or %NULL
 *
 * See jana_component_set_categories().
 *
 * Deprecated: Use jana_component_set_categories() instead.
 */
void
jana_event_set_categories (JanaEvent *self, const gchar **categories)
{
	jana_component_set_categories (JANA_COMPONENT (self), categories);
}
