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
#include <libjana/jana.h>
#include <libjana-ecal/jana-ecal.h>

static JanaStoreView *store_view = NULL;

static void
display_event (JanaEvent *event)
{
	gchar *uid, *summary, *location, *description, *recur_string;
	JanaTime *start, *end;
	JanaRecurrence *recur;
	
	uid = jana_component_get_uid (JANA_COMPONENT (event));
	summary = jana_event_get_summary (event);
	location = jana_event_get_location (event);
	description = jana_event_get_description (event);
	start = jana_event_get_start (event);
	end = jana_event_get_end (event);
	recur = jana_event_get_recurrence (event);
	recur_string = jana_utils_recurrence_to_string (recur, start);
	
	g_print ("UID: %s\n"
		"Summary: %s\n"
		"Location: %s\n"
		"Description: %s\n"
		"Starts: %d:%02d %d/%d/%04d\n"
		"End: %d:%02d %d/%d/%04d\n"
		"Recurrence: %s\n\n",
		uid, summary, location, description,
		jana_time_get_hours (start), jana_time_get_minutes (start),
		jana_time_get_day (start), jana_time_get_month (start),
		jana_time_get_year (start),
		jana_time_get_hours (end), jana_time_get_minutes (end),
		jana_time_get_day (end), jana_time_get_month (end),
		jana_time_get_year (end),
		jana_event_has_recurrence (event) ? recur_string : "None");
	
	g_free (uid);
	g_free (summary);
	g_free (location);
	g_free (description);
	g_object_unref (start);
	g_object_unref (end);
	jana_recurrence_free (recur);
	g_free (recur_string);
}

static void
added_cb (JanaStoreView *store_view, GList *components, gpointer user_data)
{
	for (; components; components = components->next) {
		JanaEvent *event = JANA_EVENT (components->data);
		g_message ("Event added:");
		display_event (event);
	}
}

static void
modified_cb (JanaStoreView *store_view, GList *components, gpointer user_data)
{
	for (; components; components = components->next) {
		JanaEvent *event = JANA_EVENT (components->data);
		g_message ("Event modified:");
		display_event (event);
	}
}

static void
removed_cb (JanaStoreView *store_view, GList *uids, gpointer user_data)
{
	for (; uids; uids = uids->next) {
		g_message ("Event removed:");
		g_print ("UID: %s\n", (gchar *)uids->data);
	}
	g_print ("\n");
}

static void
opened_cb (JanaStore *store, gpointer user_data)
{
	store_view = jana_store_get_view (store);
	g_signal_connect (G_OBJECT (store_view), "added",
		G_CALLBACK (added_cb), NULL);
	g_signal_connect (G_OBJECT (store_view), "modified",
		G_CALLBACK (modified_cb), NULL);
	g_signal_connect (G_OBJECT (store_view), "removed",
		G_CALLBACK (removed_cb), NULL);
	jana_store_view_start (store_view);
}

int
main (int argc, char **argv)
{
	GMainLoop *main_loop;
	JanaStore *store;
	
	g_type_init ();
	
	store = jana_ecal_store_new (JANA_COMPONENT_EVENT);
	g_signal_connect (G_OBJECT (store), "opened",
		G_CALLBACK (opened_cb), NULL);
	jana_store_open (store);
	
	main_loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (main_loop);

	if (store_view) g_object_unref (store_view);
	g_object_unref (store);
	
	return 0;
}

