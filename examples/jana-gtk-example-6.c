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


#include <gtk/gtk.h>
#include <libjana-ecal/jana-ecal.h>
#include <libjana-gtk/jana-gtk.h>

static gchar *location;

static gboolean
set_time (JanaGtkClock *clock)
{
	JanaTime *time;

	time = jana_ecal_utils_time_now (location);
	jana_gtk_clock_set_time (JANA_GTK_CLOCK (clock), time);
	g_object_unref (time);

	return TRUE;
}

static void
clicked_cb (JanaGtkClock *clock, GdkEventButton *event)
{
	jana_gtk_clock_set_digital (clock, !jana_gtk_clock_get_digital (clock));
}

int
main (int argc, char **argv)
{
	guint id;
	GtkWidget *window, *clock;
	
	gtk_init (&argc, &argv);
	
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	clock = jana_gtk_clock_new ();
	location = jana_ecal_utils_guess_location ();
	jana_gtk_clock_set_show_seconds (JANA_GTK_CLOCK (clock), TRUE);
	jana_gtk_clock_set_draw_shadow (JANA_GTK_CLOCK (clock), TRUE);
	gtk_container_add (GTK_CONTAINER (window), clock);
	
	gtk_window_set_default_size (GTK_WINDOW (window), 480, 480);
	gtk_widget_show_all (window);
	
	g_signal_connect (clock, "clicked",
		G_CALLBACK (clicked_cb), NULL);
	g_signal_connect (window, "delete-event",
		G_CALLBACK (gtk_main_quit), NULL);
	
	set_time (JANA_GTK_CLOCK (clock));
	id = g_timeout_add (1000, (GSourceFunc)set_time, clock);

	gtk_main ();

	g_source_remove (id);
	g_free (location);

	return 0;
}
