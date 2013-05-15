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
 * SECTION:jana-time
 * @short_description: A time representation interface
 *
 * #JanaTime is the interface for representing a time. It has functions to set 
 * all the components of a basic calendar time, including date and timezone.
 */

#include "jana-time.h"
#include "jana-utils.h"

static void
jana_time_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

GType
jana_time_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (JanaTimeInterface),
			jana_time_base_init,   /* base_init */
			NULL,

		};
		type = g_type_register_static (G_TYPE_INTERFACE,
			"JanaTime", &info, 0);
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}
	return type;
}

/**
 * jana_time_get_seconds:
 * @self: A #JanaTime
 *
 * Get the seconds component of the time. See jana_time_set_seconds().
 *
 * Returns: The seconds component of the time.
 */
guint8
jana_time_get_seconds (JanaTime *self)
{
	return JANA_TIME_GET_INTERFACE (self)->get_seconds (self);
}

/**
 * jana_time_get_minutes:
 * @self: A #JanaTime
 *
 * Get the minutes component of the time. See jana_time_set_minutes().
 *
 * Returns: The minutes component of the time.
 */
guint8
jana_time_get_minutes (JanaTime *self)
{
	return JANA_TIME_GET_INTERFACE (self)->get_minutes (self);
}

/**
 * jana_time_get_hours:
 * @self: A #JanaTime
 *
 * Get the hours component of the time. See jana_time_set_hours().
 *
 * Returns: The hours component of the time.
 */
guint8
jana_time_get_hours (JanaTime *self)
{
	return JANA_TIME_GET_INTERFACE (self)->get_hours (self);
}

/**
 * jana_time_get_day:
 * @self: A #JanaTime
 *
 * Get the day component of the time. See jana_time_set_day().
 *
 * Returns: The day component of the time.
 */
guint8
jana_time_get_day (JanaTime *self)
{
	return JANA_TIME_GET_INTERFACE (self)->get_day (self);
}

/**
 * jana_time_get_month:
 * @self: A #JanaTime
 *
 * Get the month component of the time. See jana_time_set_month().
 *
 * Returns: The month component of the time.
 */
guint8
jana_time_get_month (JanaTime *self)
{
	return JANA_TIME_GET_INTERFACE (self)->get_month (self);
}

/**
 * jana_time_get_year:
 * @self: A #JanaTime
 *
 * Get the year component of the time. See jana_time_set_year().
 *
 * Returns: The year component of the time.
 */
guint16
jana_time_get_year (JanaTime *self)
{
	return JANA_TIME_GET_INTERFACE (self)->get_year (self);
}

/**
 * jana_time_get_isdate:
 * @self: A #JanaTime
 *
 * Determines whether the time has been set as a date-only time. See 
 * jana_time_set_isdate().
 *
 * Returns: %TRUE if the time should be considered as a date, %FALSE otherwise.
 */
gboolean
jana_time_get_isdate (JanaTime *self)
{
	return JANA_TIME_GET_INTERFACE (self)->get_isdate (self);
}

/**
 * jana_time_get_daylight:
 * @self: A #JanaTime
 *
 * Determines whether the time is in daylight savings or not. See 
 * jana_time_set_daylight().
 *
 * Returns: %TRUE if the time is in daylight savings, %FALSE otherwise.
 */
gboolean
jana_time_get_daylight (JanaTime *self)
{
	return JANA_TIME_GET_INTERFACE (self)->get_daylight (self);
}

/**
 * jana_time_get_tzname:
 * @self: A #JanaTime
 *
 * Retrieves the timezone of the time. See jana_time_set_tzname().
 *
 * Returns: The timezone name, e.g. "GMT/BST", "EST", etc.
 */
gchar *
jana_time_get_tzname (JanaTime *self)
{
	return JANA_TIME_GET_INTERFACE (self)->get_tzname (self);
}

/**
 * jana_time_get_offset:
 * @self: A #JanaTime
 *
 * Gets the UTC (Universal Time Co-ordinated) offset of the time, in seconds. 
 * See jana_time_set_offset().
 *
 * Returns: The offset, in seconds, over UTC.
 */
glong
jana_time_get_offset (JanaTime *self)
{
	return JANA_TIME_GET_INTERFACE (self)->get_offset (self);
}

/**
 * jana_time_set_seconds:
 * @self: A #JanaTime
 * @seconds: The seconds component to set
 *
 * Sets the seconds component of the time. If the seconds parameter is invalid,
 * the time will be normalised. For example, a seconds parameter of -1 would 
 * cause the minutes to be decreased by 1 and the seconds to be set to 59. If 
 * normalisation causes the time to cross the daylight savings boundary of the 
 * set timezone, the time will be adjusted accordingly.
 */
void
jana_time_set_seconds (JanaTime *self, gint seconds)
{
	JANA_TIME_GET_INTERFACE (self)->set_seconds (self, seconds);
}

/**
 * jana_time_set_minutes:
 * @self: A #JanaTime
 * @minutes: The minutes component to set
 *
 * Sets the minutes component of the time. If the minutes parameter is invalid,
 * the time will be normalised. For example, a minutes parameter of -1 would 
 * cause the hours to be decreased by 1 and the minutes to be set to 59. If 
 * normalisation causes the time to cross the daylight savings boundary of the 
 * set timezone, the time will be adjusted accordingly.
 */
void
jana_time_set_minutes (JanaTime *self, gint minutes)
{
	JANA_TIME_GET_INTERFACE (self)->set_minutes (self, minutes);
}

/**
 * jana_time_set_hours:
 * @self: A #JanaTime
 * @hours: The hours component to set
 *
 * Sets the hours component of the time. If the hours parameter is invalid,
 * the time will be normalised. For example, an hours parameter of -1 would 
 * cause the date to be decreased by 1 day and the hours to be set to 23. If 
 * normalisation causes the time to cross the daylight savings boundary of the 
 * set timezone, the time will be adjusted accordingly.
 */
void
jana_time_set_hours (JanaTime *self, gint hours)
{
	JANA_TIME_GET_INTERFACE (self)->set_hours (self, hours);
}

/**
 * jana_time_set_day:
 * @self: A #JanaTime
 * @day: The day component to set
 *
 * Sets the day component of the time. If the day parameter is invalid,
 * the time will be normalised. For example, a day parameter of 0 would 
 * cause the date to be set to the last day of the previous month. If 
 * normalisation causes the time to cross the daylight savings boundary of the 
 * set timezone, the time will be adjusted accordingly.
 */
void
jana_time_set_day (JanaTime *self, gint day)
{
	JANA_TIME_GET_INTERFACE (self)->set_day (self, day);
}

/**
 * jana_time_set_month:
 * @self: A #JanaTime
 * @month: The month component to set
 *
 * Sets the month component of the time. If the day parameter is invalid,
 * the time will be normalised. For example, a month parameter of 0 would 
 * cause the date to be set to the last month of the previous year. If 
 * normalisation causes the time to cross the daylight savings boundary of the 
 * set timezone, the time will be adjusted accordingly.
 */
void
jana_time_set_month (JanaTime *self, gint month)
{
	JANA_TIME_GET_INTERFACE (self)->set_month (self, month);
}

/**
 * jana_time_set_year:
 * @self: A #JanaTime
 * @year: The year component to set
 *
 * Sets the year component of the time. The time may be normalised in the 
 * situation that the year is adjusted from a leap year to a non-leap year and 
 * the month and day was set to the 29th of February. In this situation, the 
 * month and day would be adjusted to the 1st of March.
 */
void
jana_time_set_year (JanaTime *self, gint year)
{
	JANA_TIME_GET_INTERFACE (self)->set_year (self, year);
}

/**
 * jana_time_set_isdate:
 * @self: A #JanaTime
 * @isdate: Whether the time should be considered only as a date
 *
 * Sets whether the time should be considered only as a date. The seconds, 
 * minutes and hours components of the time are not guaranteed to be 
 * preserved when @isdate is %TRUE.
 */
void
jana_time_set_isdate (JanaTime *self, gboolean isdate)
{
	JANA_TIME_GET_INTERFACE (self)->set_isdate (self, isdate);
}

/**
 * jana_time_set_tzname:
 * @self: A #JanaTime
 * @tzname: The timezone to set
 *
 * Sets the timezone of the time. The time offset will also be adjusted to an 
 * offset that corresponds to the newly set timezone. If there is an offset 
 * for the newly set timezone that is the same as the previously set offset,
 * the offset will remain unchanged. In addition, the time will be adjusted 
 * by the difference between the old offset and the new offset.
 */
void
jana_time_set_tzname (JanaTime *self, const gchar *tzname)
{
	JANA_TIME_GET_INTERFACE (self)->set_tzname (self, tzname);
}

/**
 * jana_time_set_offset:
 * @self: A #JanaTime
 * @offset: The time offset to set
 *
 * Sets the offset of the time. The timezone will also be adjusted to a 
 * zone that corresponds to the newly set offset. If there is a zone 
 * for the newly set offset that is the same as the previously set zone,
 * the zone will remain unchanged. In addition, the time will be adjusted 
 * by the difference between the old offset and the new offset.
 */
void
jana_time_set_offset (JanaTime *self, glong offset)
{
	JANA_TIME_GET_INTERFACE (self)->set_offset (self, offset);
}

/**
 * jana_time_duplicate:
 * @self: A #JanaTime
 *
 * Creates a copy of @self.
 *
 * Returns: A new copy of @self.
 */
JanaTime *
jana_time_duplicate (JanaTime *self)
{
	return JANA_TIME_GET_INTERFACE (self)->duplicate (self);
}

GType
jana_duration_get_type (void)
{
	static GType our_type = 0;

	if (!our_type)
		our_type = g_boxed_type_register_static ("JanaDuration",
			(GBoxedCopyFunc) jana_duration_copy,
			(GBoxedFreeFunc) jana_duration_free);

	return our_type;
}

/**
 * jana_duration_new:
 * @start: The start of the duration, or %NULL
 * @end: The end of the duration, or %NULL
 *
 * Creates a new #JanaDuration.
 *
 * Returns: A newly allocated #JanaDuration, to be freed with 
 * jana_duration_free().
 */
JanaDuration *
jana_duration_new (JanaTime *start, JanaTime *end)
{
	JanaDuration *duration = g_slice_new0 (JanaDuration);
	if (start) duration->start = jana_time_duplicate (start);
	if (end) duration->end = jana_time_duplicate (end);
	return duration;
}

/**
 * jana_duration_copy:
 * @duration: A #JanaDuration
 *
 * Create a copy of a #JanaDuration.
 *
 * Returns: A copy of @duration.
 */
JanaDuration *
jana_duration_copy (JanaDuration *duration)
{
	if (!duration) return NULL;
	
	return jana_duration_new (duration->start, duration->end);
}

/**
 * jana_duration_set_start:
 * @self: A #JanaDuration
 * @start: The start time, or %NULL
 *
 * Sets the start of the duration.
 */
void
jana_duration_set_start (JanaDuration *self, JanaTime *start)
{
	if (self->start) g_object_unref (self->start);
	self->start = start ? jana_time_duplicate (start) : NULL;
}

/**
 * jana_duration_set_end:
 * @self: A #JanaDuration
 * @end: The end time, or %NULL
 *
 * Sets the end of the duration.
 */
void
jana_duration_set_end (JanaDuration *self, JanaTime *end)
{
	if (self->end) g_object_unref (self->end);
	self->end = end ? jana_time_duplicate (end) : NULL;
}

/**
 * jana_duration_valid:
 * @self: A #JanaDuration
 *
 * Determines whether the duration is valid. For the duration to be valid, 
 * the start and end must be set, and the end must occur on or after the start.
 *
 * Returns: %TRUE if the duration is valid, %FALSE otherwise.
 */
gboolean
jana_duration_valid (JanaDuration *self)
{
	if (self && self->start && self->end &&
	    jana_utils_time_compare (self->start, self->end, FALSE) <= 0)
		return TRUE;
	else
		return FALSE;
}

/**
 * jana_duration_free:
 * @self: A #JanaDuration
 *
 * Frees the memory associated with a #JanaDuration.
 */
void
jana_duration_free (JanaDuration *self)
{
	if (!self) return;
	
	g_object_unref (self->start);
	g_object_unref (self->end);
	g_slice_free (JanaDuration, self);
}

