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


#include <glib.h>
#include <glib/gstdio.h>
#include <libical/icaltimezone.h>
#include <libical/icaltime.h>
#include <libjana/jana-time.h>
#include <libjana/jana-event.h>
#include <libjana/jana-store.h>
#include <libjana/jana-store-view.h>
#include <libjana/jana-utils.h>
#include <libjana-ecal/jana-ecal-time.h>
#include <libjana-ecal/jana-ecal-event.h>
#include <libjana-ecal/jana-ecal-store.h>
#include <libjana-ecal/jana-ecal-store-view.h>

/* To build:
 * gcc -o test-jana-ecal-store-view test-jana-ecal-store-view.c ../libjana/jana-utils.c ../libjana/jana-event.c ../libjana/jana-component.c ../libjana/jana-time.c ../libjana/jana-store.c ../libjana/jana-store-view.c ../libjana-ecal/jana-ecal-component.c ../libjana-ecal/jana-ecal-event.c ../libjana-ecal/jana-ecal-store.c ../libjana-ecal/jana-ecal-store-view.c ../libjana-ecal/jana-ecal-time.c `pkg-config --cflags --libs glib-2.0 libecal-1.2 gobject-2.0` -I../ -g -Wall -DHAVE_CID_TYPE
 */

/* Test if basic functions work for JanaEcalStore and JanaEcalStoreView:
 * Open up the system calendar and add three events on differing dates. Then
 * open a view on this calendar, initially narrowed to see just one date, then
 * widen to view all events. Finally, close and delete the calendar.
 *
 * Test runs for 10 seconds, or until it is successful, whichever occurs
 * first. If the test does not complete within 10 seconds, it will count as a
 * failure, however, it may not necessarily have failed. This test should
 * really never take more than a couple of seconds, however.
 *
 * Note that this test can pass when really it should fail... Should check
 * event UIDs to verify results properly.
 *
 * Returns 0 on success, 1 on libjana error and 2 on error removing calendar.
 */

static GMainLoop *main_loop;
static int error_code;
static gboolean first_run = TRUE;
static gboolean add_success = FALSE;
static JanaTime *range_start, *range_end;
static JanaStoreView *store_view;

static void
added_cb (JanaStoreView *store_view, GList *components, gpointer user_data)
{
	guint length = g_list_length (components);
	
	if ((!first_run) && (length == 2)) add_success = TRUE;
	
	for (; components; components = components->next) {
		/*GList *p, *props;
		JanaEvent *event = JANA_EVENT (components->data);

		props = jana_component_get_custom_props_list (
			JANA_COMPONENT (event));
		for (p = props; p; p = p->next) {
			gchar **prop_pair = (gchar **)p->data;
			g_debug ("%s = %s", prop_pair[0], prop_pair[1]);
		}
		jana_component_props_list_free (props);*/
	}

	if (first_run && (length == 1)) {
		/* Successful, widen the view */
		first_run = FALSE;
		jana_time_set_day (range_end,
			jana_time_get_day (range_end) + 2);
		jana_time_set_day (range_start,
			jana_time_get_day (range_start) - 2);
		jana_store_view_set_range (store_view,
			range_start, range_end);
	}
}

static void
modified_cb (JanaStoreView *store_view, GList *components, gpointer user_data)
{
	guint length = g_list_length (components);
	
	if ((!first_run) && (length == 1) && (add_success)) {
		error_code = 0;
		g_main_loop_quit (main_loop);
	}

	for (; components; components = components->next) {
		JanaEvent *event = JANA_EVENT (components->data);

		gchar *summary = jana_event_get_summary (event);
		g_free (summary);
	}
}

static void
removed_cb (JanaStoreView *store_view, GList *uids, gpointer user_data)
{
	for (; uids; uids = uids->next) {
	}
}

static void
opened_cb (JanaStore *store, gpointer user_data)
{
	JanaEvent *event;
	icaltimetype ical_time;
	const icaltimezone *zone;
	JanaTime *start, *end;
	
	event = jana_ecal_event_new ();

	jana_event_set_summary (event, "libjana event 1");
	jana_event_set_description (event, "A description");
	jana_event_set_location (event, "A location");
	
	zone = (const icaltimezone *)icaltimezone_get_builtin_timezone (
		"Europe/London");
	ical_time = icaltime_current_time_with_zone (zone);
	ical_time.zone = zone;

	range_start = jana_ecal_time_new_from_icaltime (&ical_time);
	range_end = jana_ecal_time_new_from_icaltime (&ical_time);
	start = jana_ecal_time_new_from_icaltime (&ical_time);
	end = jana_ecal_time_new_from_icaltime (&ical_time);
	
	jana_time_set_hours (end, jana_time_get_hours (end) + 1);

	jana_event_set_start (event, start);
	jana_event_set_end (event, end);

	jana_time_set_day (range_end, jana_time_get_day (range_end) + 1);
	jana_time_set_day (range_start, jana_time_get_day (range_start) - 1);

	store_view = jana_store_get_view (store);
	jana_store_view_set_range (store_view, range_start, range_end);

	g_signal_connect (G_OBJECT (store_view), "added",
		G_CALLBACK (added_cb), NULL);
	g_signal_connect (G_OBJECT (store_view), "modified",
		G_CALLBACK (modified_cb), NULL);
	g_signal_connect (G_OBJECT (store_view), "removed",
		G_CALLBACK (removed_cb), NULL);
	
	jana_store_view_start (store_view);
	
	jana_store_add_component (store, JANA_COMPONENT (event));
	
	jana_event_set_summary (event, "libjana event 2");
	jana_time_set_day (start, jana_time_get_day (start) - 2);
	jana_time_set_day (end, jana_time_get_day (end) - 2);
	jana_event_set_start (event, start);
	jana_event_set_end (event, end);
	jana_store_add_component (store, JANA_COMPONENT (event));

	jana_event_set_summary (event, "libjana event 3");
	jana_time_set_day (start, jana_time_get_day (start) + 4);
	jana_time_set_day (end, jana_time_get_day (end) + 4);
	jana_event_set_start (event, start);
	jana_event_set_end (event, end);
	jana_store_add_component (store, JANA_COMPONENT (event));

	g_object_unref (event);
	g_object_unref (start);
	g_object_unref (end);
}

static gboolean
timeout_cb (gpointer user_data)
{
	g_main_loop_quit (main_loop);
	
	return FALSE;
}

int
main (int argc, char **argv)
{
	JanaStore *store;
	gchar *uri;
	ECal *ecal;
	GError *error = NULL;
	
	error_code = 1;
	
	g_type_init ();
	
	uri = g_strdup_printf ("file://%s%slibjana-test",
		g_get_tmp_dir (), G_DIR_SEPARATOR_S);
	store = jana_ecal_store_new_from_uri (uri, JANA_COMPONENT_EVENT);
	g_free (uri);
	
	g_signal_connect (G_OBJECT (store), "opened",
		G_CALLBACK (opened_cb), NULL);
	jana_store_open (store);
	
	g_object_get (store, "ecal", &ecal, NULL);
	
	g_timeout_add (10000, (GSourceFunc)timeout_cb, NULL);
	
	main_loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (main_loop);
	
	if (!e_cal_remove (ecal, &error)) {
		g_warning ("Error removing calendar: %s", error->message);
		g_error_free (error);
		error_code = 2;
	}
	
	if (error_code != 0) g_warning ("Error");
	else g_message ("Success");
	
	g_object_unref (range_start);
	g_object_unref (range_end);
	g_object_unref (store_view);
	g_object_unref (store);
	
	return error_code;
}

