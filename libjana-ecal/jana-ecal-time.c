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
 * SECTION:jana-ecal-time
 * @short_description: An implementation of #JanaTime using libecal
 *
 * #JanaEcalTime is an implementation of #JanaTime that provides a 
 * wrapper over #icaltimetype, using libecal.
 */

#define HANDLE_LIBICAL_MEMORY 1

#include "jana-ecal-time.h"
#include <libjana/jana-time.h>
#include <string.h>

static void time_interface_init (gpointer g_iface, gpointer iface_data);

static guint8 time_get_seconds 	(JanaTime *self);
static guint8 time_get_minutes 	(JanaTime *self);
static guint8 time_get_hours 	(JanaTime *self);

static guint8  time_get_day 	(JanaTime *self);
static guint8  time_get_month 	(JanaTime *self);
static guint16 time_get_year 	(JanaTime *self);

static gboolean time_get_isdate	(JanaTime *self);
static gboolean time_get_daylight(JanaTime *self);

static gchar * time_get_tzname 	(JanaTime *self);
static glong  time_get_offset 	(JanaTime *self);

static void time_set_seconds 	(JanaTime *self, gint seconds);
static void time_set_minutes 	(JanaTime *self, gint minutes);
static void time_set_hours 	(JanaTime *self, gint hours);

static void time_set_day 	(JanaTime *self, gint day);
static void time_set_month 	(JanaTime *self, gint month);
static void time_set_year 	(JanaTime *self, gint year);

static void time_set_isdate	(JanaTime *self, gboolean isdate);

static void time_set_tzname 	(JanaTime *self, const gchar *tzname);
static void time_set_offset 	(JanaTime *self, glong offset);

static JanaTime * time_duplicate(JanaTime *self);

G_DEFINE_TYPE_WITH_CODE (JanaEcalTime, 
                        jana_ecal_time, 
                        G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE (JANA_TYPE_TIME,
                                               time_interface_init));

#define TIME_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), JANA_ECAL_TYPE_TIME, JanaEcalTimePrivate))

typedef struct _JanaEcalTimePrivate JanaEcalTimePrivate;

struct _JanaEcalTimePrivate
{
	icaltimetype *time;
	
	gboolean year_set;
	gboolean month_set;
	gboolean day_set;
};

enum {
	PROP_ICALTIME = 1,
};

static void
jana_ecal_time_get_property (GObject *object, guint property_id,
			     GValue *value, GParamSpec *pspec)
{
	JanaEcalTimePrivate *priv = TIME_PRIVATE (object);

	switch (property_id) {
	    case PROP_ICALTIME :
		g_value_set_pointer (value, priv->time);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_ecal_time_set_property (GObject *object, guint property_id,
                             const GValue *value, GParamSpec *pspec)
{
	icaltimetype *time;
	JanaEcalTimePrivate *priv = TIME_PRIVATE (object);

	switch (property_id) {
	    case PROP_ICALTIME :
		if ((time = g_value_get_pointer (value))) {
			priv->time = g_slice_new (icaltimetype);
			g_memmove (priv->time, time, sizeof (icaltimetype));

			if ((!icaltime_is_null_time (*priv->time)) &&
			    icaltime_is_valid_time (*priv->time)) {
				priv->year_set = TRUE;
				priv->month_set = TRUE;
				priv->day_set = TRUE;
			}
		} else {
			priv->time = g_slice_new0 (icaltimetype);
			*priv->time = icaltime_null_time ();
		}
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_ecal_time_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (jana_ecal_time_parent_class)->dispose)
		G_OBJECT_CLASS (jana_ecal_time_parent_class)->dispose (object);
}

static void
jana_ecal_time_finalize (GObject *object)
{
	JanaEcalTimePrivate *priv = TIME_PRIVATE (object);

	if (priv->time) {
		g_slice_free (icaltimetype, priv->time);
		priv->time = NULL;
	}

	G_OBJECT_CLASS (jana_ecal_time_parent_class)->finalize (object);
}

static void
time_interface_init (gpointer g_iface, gpointer iface_data)
{
	JanaTimeInterface *iface = (JanaTimeInterface *)g_iface;
	
	iface->get_seconds = time_get_seconds;
	iface->get_minutes = time_get_minutes;
	iface->get_hours = time_get_hours;

	iface->get_day = time_get_day;
	iface->get_month = time_get_month;
	iface->get_year = time_get_year;

	iface->get_isdate = time_get_isdate;
	iface->get_daylight = time_get_daylight;

	iface->get_tzname = time_get_tzname;
	iface->get_offset = time_get_offset;

	iface->set_seconds = time_set_seconds;
	iface->set_minutes = time_set_minutes;
	iface->set_hours = time_set_hours;

	iface->set_day = time_set_day;
	iface->set_month = time_set_month;
	iface->set_year = time_set_year;

	iface->set_isdate = time_set_isdate;

	iface->set_tzname = time_set_tzname;
	iface->set_offset = time_set_offset;
	
	iface->duplicate = time_duplicate;
}

static void
jana_ecal_time_class_init (JanaEcalTimeClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (JanaEcalTimePrivate));

	object_class->get_property = jana_ecal_time_get_property;
	object_class->set_property = jana_ecal_time_set_property;
	object_class->dispose = jana_ecal_time_dispose;
	object_class->finalize = jana_ecal_time_finalize;

	g_object_class_install_property (
		object_class,
		PROP_ICALTIME,
		g_param_spec_pointer (
			"icaltime",
			"struct icaltimetype *",
			"The icaltimetype represented by this JanaTime "
			"object.",
			G_PARAM_READABLE | G_PARAM_WRITABLE |
			G_PARAM_CONSTRUCT_ONLY));
}

static void
jana_ecal_time_init (JanaEcalTime *self)
{
	/*JanaEcalTimePrivate *priv = TIME_PRIVATE (self);*/
}

/**
 * jana_ecal_time_new:
 *
 * Creates a new #JanaEcalTime.
 *
 * Returns: A new #JanaEcalTime, cast as a #JanaTime.
 */
JanaTime *
jana_ecal_time_new ()
{
	return JANA_TIME (g_object_new (JANA_ECAL_TYPE_TIME, NULL));
}

/**
 * jana_ecal_time_new_from_icaltime:
 * @time: An #icaltimetype
 *
 * Creates a new #JanaEcalTime using the given #icaltimetype.
 *
 * Returns: A new #JanaEcalTime, cast as a #JanaTime.
 */
JanaTime *
jana_ecal_time_new_from_icaltime (const icaltimetype *time)
{
	return JANA_TIME (g_object_new (JANA_ECAL_TYPE_TIME,
		"icaltime", time, NULL));
}

/**
 * jana_ecal_time_new_from_ecaltime:
 * @time: An #ECalComponentDateTime
 *
 * Creates a new #JanaEcalTime using the given #ECalComponentDateTime
 *
 * Returns: A new #JanaEcalTime, cast as a #JanaTime
 */
JanaTime *
jana_ecal_time_new_from_ecaltime (ECalComponentDateTime *dt)
{
	if (dt->value) {
		if (dt->tzid) {
			icaltime_set_timezone (dt->value,
				icaltimezone_get_builtin_timezone_from_tzid (
					dt->tzid));
		}
		return jana_ecal_time_new_from_icaltime (dt->value);
	} else
		return NULL;
}

static guint8
time_get_seconds (JanaTime *self)
{
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);

	return (guint8)priv->time->second;
}

static guint8
time_get_minutes (JanaTime *self)
{
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);

	return (guint8)priv->time->minute;
}

static guint8
time_get_hours (JanaTime *self)
{
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);

	return (guint8)priv->time->hour;
}

static guint8
time_get_day (JanaTime *self)
{
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);

	return (guint8)priv->time->day;
}

static guint8
time_get_month (JanaTime *self)
{
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);

	return (guint8)priv->time->month;
}

static guint16
time_get_year (JanaTime *self)
{
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);

	return (guint16)priv->time->year;
}

static gboolean
time_get_isdate (JanaTime *self)
{
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);

	return priv->time->is_date;
}

static gboolean
time_get_daylight (JanaTime *self)
{
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);

	return priv->time->is_daylight;
}

static gchar *
time_get_tzname (JanaTime *self)
{
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);
	char *zone = icaltimezone_get_tznames (
		(icaltimezone *)priv->time->zone);
	
	return zone ? g_strdup (zone) : g_strdup ("UTC");
}

static void
time_normalise (JanaTime *self, int offset)
{
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);

	if (priv->year_set && priv->month_set && priv->day_set) {
		/* Normalise the time and verify daylight settings */
		*priv->time = icaltime_normalize (*priv->time);
		offset -= icaltimezone_get_utc_offset (
			(icaltimezone *)priv->time->zone,
			priv->time, &priv->time->is_daylight);
		if (offset) icaltime_adjust (priv->time, 0, 0, 0, -offset);
	}
}

static glong
time_get_offset (JanaTime *self)
{
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);
	
	return (glong)icaltimezone_get_utc_offset (
		(icaltimezone *)priv->time->zone, priv->time, NULL);
}

static void
time_set_seconds (JanaTime *self, gint seconds)
{
	int offset;
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);
	
	if (priv->time->is_date) return;
	
	offset = icaltimezone_get_utc_offset (
		(icaltimezone *)priv->time->zone, priv->time, NULL);
	priv->time->second = (int)seconds;

	time_normalise (self, offset);
}

static void
time_set_minutes (JanaTime *self, gint minutes)
{
	int offset;
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);
	
	if (priv->time->is_date) return;
	
	offset = icaltimezone_get_utc_offset (
		(icaltimezone *)priv->time->zone, priv->time, NULL);
	priv->time->minute = (int)minutes;

	time_normalise (self, offset);
}

static void
time_set_hours (JanaTime *self, gint hours)
{
	int offset;
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);
	
	if (priv->time->is_date) return;
	
	offset = icaltimezone_get_utc_offset (
		(icaltimezone *)priv->time->zone, priv->time, NULL);
	priv->time->hour = (int)hours;

	time_normalise (self, offset);
}

static void
time_set_day (JanaTime *self, gint day)
{
	int offset;
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);
	
	offset = icaltimezone_get_utc_offset (
		(icaltimezone *)priv->time->zone, priv->time, NULL);
	priv->time->day = (int)day;

	time_normalise (self, offset);
	priv->day_set = TRUE;
}

static void
time_set_month (JanaTime *self, gint month)
{
	int offset;
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);
	
	offset = icaltimezone_get_utc_offset (
		(icaltimezone *)priv->time->zone, priv->time, NULL);
	priv->time->month = (int)month;

	time_normalise (self, offset);
	priv->month_set = TRUE;
}

static void
time_set_year (JanaTime *self, gint year)
{
	int offset;
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);
	
	offset = icaltimezone_get_utc_offset (
		(icaltimezone *)priv->time->zone, priv->time, NULL);
	priv->time->year = (int)year;

	time_normalise (self, offset);
	priv->year_set = TRUE;
}

static void
time_set_isdate	(JanaTime *self, gboolean isdate)
{
	int offset;
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);
	
	offset = icaltimezone_get_utc_offset (
		(icaltimezone *)priv->time->zone, priv->time, NULL);
	priv->time->is_date = isdate ? 1 : 0;
	if (isdate) {
		/* Set non-date fields to zero - required for queries */
		priv->time->hour = 0;
		priv->time->minute = 0;
		priv->time->second = 0;
	}
	time_normalise (self, offset);
}

static void
time_set_tzname (JanaTime *self, const gchar *tzname)
{
	gint i, offset;
	icalarray *builtin;
	gboolean preserve = TRUE;
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);
	const char *current_zone;

	if (!tzname || (strcmp ("UTC", tzname) == 0)) {
		icaltimezone_convert_time (priv->time,
			(icaltimezone *)priv->time->zone,
			icaltimezone_get_utc_timezone ());
		priv->time->zone = icaltimezone_get_utc_timezone ();
		return;
	}
	
	current_zone = icaltimezone_get_tznames ((icaltimezone *)
		priv->time->zone);
	if (current_zone && (strcmp (tzname, current_zone) == 0))
		return;

	offset = icaltimezone_get_utc_offset (
		(icaltimezone *)priv->time->zone, priv->time, NULL);
	
	builtin = icaltimezone_get_builtin_timezones ();
	
tzname_zone_loop:
	for (i = 0; i < builtin->num_elements; i++) {
		icaltimezone *zone = (icaltimezone *)icalarray_element_at (
			builtin, i);
		int zone_offset;
		gchar *zone_tz;
		
		zone_tz = icaltimezone_get_tznames (zone);
		zone_offset = icaltimezone_get_utc_offset (
			zone, priv->time, NULL);

		if (zone_tz && (strcasecmp (tzname, zone_tz) == 0)) {
			if ((!preserve) || (zone_offset == offset)) {
				icaltimezone_convert_time (priv->time,
					(icaltimezone *)priv->time->zone, zone);
				priv->time->zone = zone;
				return;
			}
		}
	}
	if (preserve) {
		preserve = FALSE;
		goto tzname_zone_loop;
	}
	
	g_warning ("%s: tzname '%s' not set", G_STRFUNC, tzname);
}

static void
time_set_offset (JanaTime *self, glong offset)
{
	gint i;
	gchar *tzname;
	icalarray *builtin;
	gboolean preserve = TRUE;
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);
	
	if (time_get_offset (self) == offset) return;
	
	tzname = icaltimezone_get_tznames ((icaltimezone *)priv->time->zone);
	if (!tzname) tzname = "UTC";

	builtin = icaltimezone_get_builtin_timezones ();
offset_zone_loop:
	for (i = 0; i < builtin->num_elements; i++) {
		icaltimezone *zone = (icaltimezone *)icalarray_element_at (
			builtin, i);
		int zone_offset;
		gchar *zone_tz;
		
		zone_tz = icaltimezone_get_tznames (zone);
		zone_offset = icaltimezone_get_utc_offset (
			zone, priv->time, NULL);

		if (zone_tz && (zone_offset == offset)) {
			if ((!preserve) ||
			    (strcasecmp (tzname, zone_tz) == 0)) {
				icaltimezone_convert_time (priv->time,
					(icaltimezone *)priv->time->zone, zone);
				priv->time->zone = zone;
				return;
			}
		}
	}
	if (preserve) {
		preserve = FALSE;
		goto offset_zone_loop;
	}
	
	g_warning ("%s: Offset '%ld' not set", G_STRFUNC, offset);
}

static JanaTime *
time_duplicate (JanaTime *self) {
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);
	return jana_ecal_time_new_from_icaltime (priv->time);
}

/**
 * jana_ecal_time_set_location:
 * @self: A #JanaEcalTime
 * @location: An iCal/vCalendar timezone string
 *
 * Sets the location of the time, using a timezone string such as 
 * "Europe/London", as opposed to the libc timezone name as used in 
 * jana_time_set_tzname(). The time will still be adjusted for the new 
 * timezone. A %NULL location parameter will be treated as "UTC".
 */
void
jana_ecal_time_set_location (JanaEcalTime *self, const gchar *location)
{
	icaltimezone *zone;
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);
	
	if ((!location) || (strcmp (location, "UTC") == 0))
		zone = icaltimezone_get_utc_timezone ();
	else
		zone = icaltimezone_get_builtin_timezone (location);

	if (zone) {
		icaltimezone *from_zone = priv->time->zone ?
			(icaltimezone *)priv->time->zone :
			icaltimezone_get_utc_timezone ();
		icaltimezone_convert_time (priv->time, from_zone, zone);
		priv->time->zone = zone;		
	}
}

/**
 * jana_ecal_time_get_location:
 * @self: A #JanaEcalTime
 *
 * Retrieves the full timezone name of the given time. See 
 * jana_ecal_time_set_location().
 *
 * Returns: A string containing the full timezone name. This is owned by 
 * libecal and must not be freed.
 */
const gchar *
jana_ecal_time_get_location (JanaEcalTime *self)
{
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);
	const gchar *location =  (const gchar *)icaltimezone_get_location (
		(icaltimezone *)priv->time->zone);
	return location ? location : "UTC";
}

/**
 * jana_ecal_time_to_time_t
 * @self: A #JanaEcalTime
 *
 * Converts a #JanaEcalTime to time_t type.
 *
 * Returns: A time_t representation of the time.
 */
time_t
jana_ecal_time_to_time_t (JanaEcalTime *self)
{
	JanaEcalTimePrivate *priv = TIME_PRIVATE (self);

	return icaltime_as_timet (*priv->time);
}
