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
 * SECTION:jana-ecal-utils
 * @short_description: A set of utility functions for libjana-ecal
 *
 * jana-ecal-utils contains a set of libjana-ecal related utility functions.
 */

#define HANDLE_LIBICAL_MEMORY 1

#include <string.h>
#include <libical/icaltime.h>
#include <libjana/jana-utils.h>
#include <libecal/libecal.h>
#include <libjana-ecal/jana-ecal-time.h>
#include <gconf/gconf-client.h>
#include "jana-ecal-utils.h"

/**
 * jana_ecal_utils_time_now:
 * @location: A full timezone name
 *
 * Creates a new #JanaEcalTime with the current time of the given location.
 *
 * Returns: A new #JanaEcalTime, cast as a #JanaTime.
 */
JanaTime *
jana_ecal_utils_time_now (const gchar *location)
{
	icaltimetype ical_time;
	const icaltimezone *zone;
	
	zone = (const icaltimezone *)icaltimezone_get_builtin_timezone (
		location);
	ical_time = icaltime_current_time_with_zone (zone);
	icaltime_set_timezone (&ical_time, zone);

	return jana_ecal_time_new_from_icaltime (&ical_time);
}

/**
 * jana_ecal_utils_time_now_local:
 *
 * Creates a new #JanaEcalTime with the current time of the guessed location. 
 * See jana_ecal_utils_guess_location().
 *
 * Returns: A new #JanaEcalTime, cast as a #JanaTime.
 */
JanaTime *
jana_ecal_utils_time_now_local ()
{
	gchar *location = jana_ecal_utils_guess_location ();
	JanaTime *time = jana_ecal_utils_time_now (location);
	g_free (location);
	return time;
}

/**
 * jana_ecal_utils_time_today:
 * @location: A full timezone name
 *
 * Creates a new #JanaEcalTime with the current date of the given location.
 *
 * Returns: A new #JanaEcalTime, cast as a #JanaTime.
 */
JanaTime *
jana_ecal_utils_time_today (const gchar *location)
{
	JanaTime *time;
	icaltimetype ical_time;
	const icaltimezone *zone;
	
	zone = (const icaltimezone *)icaltimezone_get_builtin_timezone (
		location);
	ical_time = icaltime_current_time_with_zone (zone);
	icaltime_set_timezone (&ical_time, zone);

	time = jana_ecal_time_new_from_icaltime (&ical_time);
	jana_time_set_isdate (time, TRUE);

	return time;
}

/**
 * jana_ecal_utils_guess_location:
 *
 * Tries to guess the location by checking for common system settings and 
 * files, and finally falling back on the first location that matches the
 * current system timezone. See also #JANA_ECAL_LOCATION_KEY.
 *
 * Returns: A newly allocated string with the guessed location.
 */
gchar *
jana_ecal_utils_guess_location ()
{
	gint i;
	FILE *file;
	gchar *tzname;
	gchar string[128];
	icaltimetype today;
	icalarray *builtin;
	time_t now_t = time (NULL);
	struct tm *now = localtime (&now_t);

	if (!gconf_client_get_bool (gconf_client_get_default (),
		JANA_ECAL_SYSTEM_TIMEZONE_KEY, NULL))
	{
		/* Check the Evolution timezone key first */
		tzname = gconf_client_get_string (gconf_client_get_default (),
			JANA_ECAL_LOCATION_KEY, NULL);
		if (tzname && icaltimezone_get_builtin_timezone (tzname)) return tzname;
		g_free (tzname);
	}
	
	/* Debian systems have /etc/timezone */
	if ((file = fopen ("/etc/timezone", "r"))) {
		if ((fgets (string, 128, file)) && (string[0] != '\0')) {
			gint c;
			for (c = 0; (string[c] != '\0') &&
			     (string[c] != '\n'); c++);
			string[c] = '\0';
			if (icaltimezone_get_builtin_timezone (string)) {
				fclose (file);
				return g_strdup (string);
			}
		}
		fclose (file);
	}
	
#if GLIB_CHECK_VERSION(2,14,0)
	/* OpenSuSE (and RH?) systems have /etc/sysconfig/clock */
	if ((file = fopen ("/etc/sysconfig/clock", "r"))) {
		GRegex *regex;
		GError *error = NULL;

		regex = g_regex_new ("ZONE *= *\".*\"",
			G_REGEX_OPTIMIZE, 0, &error);
		if (!regex) {
			g_warning ("Failed to create regex: %s",
				error->message);
			g_error_free (error);
		} else while (fgets (string, 128, file)) {
			GMatchInfo *match_info;
			if (g_regex_match (regex, string, 0, &match_info)) {
				gchar *zone;
				gchar *match = g_match_info_fetch (
					match_info, 0);
				
				g_match_info_free (match_info);
				g_regex_unref (regex);
				regex = NULL;
				
				if (match[strlen (match) - 2] == '\n')
					match[strlen (match)-2] = '\0';
				else
					match[strlen (match)-1] = '\0';
				zone = g_strdup (strchr (match, '\"') + 1);
				g_free (match);
				
				if (icaltimezone_get_builtin_timezone (zone)) {
					fclose (file);
					return zone;
				} else {
					g_free (zone);
					break;
				}
			}
			g_match_info_free (match_info);
		}
		if (regex) g_regex_unref (regex);
		fclose (file);
	}
#endif
	
	/* Check to see if the /etc/localtime symlink exists */
	if (g_file_test ("/etc/localtime", G_FILE_TEST_IS_SYMLINK)) {
		gchar *link;
		if ((link = g_file_read_link ("/etc/localtime", NULL))) {
			tzname = g_strrstr (link, "zoneinfo/");
			if (tzname &&
			    icaltimezone_get_builtin_timezone (tzname + 9)) {
				tzname = g_strdup (tzname + 9);
				g_free (link);
				return tzname;
			}
			g_free (link);
		}
	}
	
	/* Fallback to first location that matches libc timezone and the
	 * offset for the current time.
	 */
	today = icaltime_today ();
	tzname = jana_utils_get_local_tzname ();
	if (strcmp (tzname, "UTC") == 0) return tzname;
	
	builtin = icaltimezone_get_builtin_timezones ();
	for (i = 0; i < builtin->num_elements; i++) {
		icaltimezone *zone = (icaltimezone *)icalarray_element_at (
			builtin, i);
		gchar *zone_tz;
		int offset;
		
		offset = icaltimezone_get_utc_offset (zone, &today, NULL);
		zone_tz = icaltimezone_get_tznames (zone);

		if (zone_tz && (strcasecmp (tzname, zone_tz) == 0) &&
		    (offset == now->tm_gmtoff)) {
			g_free (tzname);
			return g_strdup (icaltimezone_get_display_name (zone));
		}
	}
	g_free (tzname);
	
	/* No matching timezone found, fall back to UTC */
	return g_strdup ("UTC");
}

/**
 * jana_ecal_utils_get_locations:
 *
 * Retrieves the built-in list of timezone locations from libecal.
 *
 * Returns: A newly allocated string array, to be freed with g_strfreev().
 */
gchar **
jana_ecal_utils_get_locations ()
{
	gint i, z;
	icalarray *builtin;
	gchar **locations;
	
	builtin = icaltimezone_get_builtin_timezones ();
	locations = g_new0 (gchar *, builtin->num_elements + 1);
	for (i = 0, z = 0; i < builtin->num_elements; i++) {
		icaltimezone *zone = (icaltimezone *)icalarray_element_at (
			builtin, i);

		/* Don't return zones that don't have a tzname */
		if (icaltimezone_get_tznames (zone)) {
			locations[z] = g_strdup (
				icaltimezone_get_display_name (zone));
			z++;
		}
	}
	
	return locations;
}
