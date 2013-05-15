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


#include <string.h>
#include <glib.h>
#include <libjana/jana.h>
#include <libjana-ecal/jana-ecal.h>

/* Test if basic event functions work for JanaEcalEvent:
 * Create a new event, set the summary, description, start, end and categories,
 * then read them back to verify they were set correctly.
 * Returns 0 on success and 1 on error.
 */
int
main (int argc, char **argv)
{
	int error_code = 0;
	gchar *event_summary, *event_description, *event_location;
	const gchar *summary = "Event summary";
	const gchar *description = "Event description";
	const gchar *location = "Event location";

	JanaEvent *event;
	JanaTime *start, *end;
	JanaRecurrence *recur, *recur_n;
	
	g_type_init ();
	
	event = jana_ecal_event_new ();
	
	jana_event_set_summary (event, summary);
	jana_event_set_description (event, description);
	jana_event_set_location (event, location);
	
	start = jana_ecal_time_new ();
	jana_time_set_seconds (start, 0);
	jana_time_set_minutes (start, 0);
	jana_time_set_hours (start, 0);

	jana_time_set_day (start, 1);
	jana_time_set_month (start, 1);
	jana_time_set_year (start, 2007);
	
	/*jana_time_set_tzname (start, "GMT/BST");*/
	jana_ecal_time_set_location (JANA_ECAL_TIME (start), "Europe/London");

	jana_event_set_start (event, start);
	g_object_unref (start);

	end = jana_ecal_time_new ();
	jana_time_set_seconds (end, 0);
	jana_time_set_minutes (end, 0);
	jana_time_set_hours (end, 2);

	jana_time_set_day (end, 1);
	jana_time_set_month (end, 1);
	jana_time_set_year (end, 2007);
	
	/*jana_time_set_tzname (start, "GMT/BST");*/
	jana_ecal_time_set_location (JANA_ECAL_TIME (end), "Europe/London");
	
	jana_event_set_end (event, end);
	
	jana_time_set_year (end, jana_time_get_year (end) + 1);
	
	recur = jana_recurrence_new ();
	recur->type = JANA_RECURRENCE_WEEKLY;
	recur->interval = 2;
	recur->week_days[1] = TRUE;
	recur->week_days[6] = TRUE;
	recur->end = end;
	
	jana_event_set_recurrence (event, recur);

	/* Read back the values, check they're correct */
	start = jana_event_get_start (event);
	end = jana_event_get_end (event);
	
	event_summary = jana_event_get_summary (event);
	event_description = jana_event_get_description (event);
	event_location = jana_event_get_location (event);
	
	recur_n = jana_event_get_recurrence (event);
	
	/*g_debug ("\n%s\n%s\n%s\n%02d/%02d/%04d %02d:%02d:%02d %s\n"
		"%02d/%02d/%04d %02d:%02d:%02d %s",
		event_summary,
		event_description,
		event_location,
		jana_time_get_day (start),
		jana_time_get_month (start),
		jana_time_get_year (start),
		jana_time_get_hours (start),
		jana_time_get_minutes (start),
		jana_time_get_seconds (start),
		jana_time_get_tzname (start),
		jana_time_get_day (end),
		jana_time_get_month (end),
		jana_time_get_year (end),
		jana_time_get_hours (end),
		jana_time_get_minutes (end),
		jana_time_get_seconds (end),
		jana_time_get_tzname (end));*/

	if ((strcmp (summary, event_summary) != 0) ||
	    (strcmp (description, event_description) != 0) ||
	    (strcmp (location, event_location) != 0) ||
	    (jana_time_get_seconds (start) != 0) ||
	    (jana_time_get_minutes (start) != 0) ||
	    (jana_time_get_hours (start) != 0) ||
	    (jana_time_get_day (start) != 1) ||
	    (jana_time_get_month (start) != 1) ||
	    (jana_time_get_year (start) != 2007) ||
	    (jana_time_get_seconds (end) != 0) ||
	    (jana_time_get_minutes (end) != 0) ||
	    (jana_time_get_hours (end) != 2) ||
	    (jana_time_get_day (end) != 1) ||
	    (jana_time_get_month (end) != 1) ||
	    (jana_time_get_year (end) != 2007) ||
	    (jana_utils_recurrence_diff (recur, recur_n))) {
		g_warning ("Error");
		error_code = 1;
	}
	
	g_free (event_summary);
	g_free (event_description);
	g_free (event_location);
	g_object_unref (start);
	g_object_unref (end);
	g_object_unref (event);

	if (error_code == 0)
		g_message ("Success");

	return error_code;
}

