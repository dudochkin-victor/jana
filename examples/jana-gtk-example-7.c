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


#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <gtk/gtk.h>
#include <libjana-ecal/jana-ecal.h>
#include <libjana-gtk/jana-gtk.h>

static GtkWidget *map;
static gchar *location;
gboolean first = TRUE;

static gboolean
set_time (GtkWidget *clock)
{
	JanaTime *time;

	time = jana_ecal_utils_time_now (location);
	jana_gtk_clock_set_time (JANA_GTK_CLOCK (clock), time);
	if (first || (jana_time_get_seconds (time) == 0)) {
		/* Update every minute */
		jana_gtk_world_map_set_time (JANA_GTK_WORLD_MAP (map), time);
		first = FALSE;
	}
	g_object_unref (time);

	return TRUE;
}

static void
clicked_cb (JanaGtkWorldMap *map, GdkEventButton *event)
{
	gdouble lat, lon;
	
	jana_gtk_world_map_get_latlon (map, event->x, event->y, &lat, &lon);
	g_message ("Map clicked at latitude, longitude: %lg, %lg", lat, lon);
}

int
main (int argc, char **argv)
{
	guint id;
	GtkWidget *window, *align, *clock;
	
	gtk_init (&argc, &argv);
	
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	map = jana_gtk_world_map_new ();
	jana_gtk_world_map_add_marker (JANA_GTK_WORLD_MAP (map),
		jana_gtk_world_map_marker_pixbuf_new(
			gdk_pixbuf_new_from_file (
				PKGDATADIR "/flag-uk.png", NULL)),
		51.75, -2.25);
	
	clock = jana_gtk_clock_new ();
	jana_gtk_clock_set_show_seconds (JANA_GTK_CLOCK (clock), TRUE);
	jana_gtk_clock_set_draw_shadow (JANA_GTK_CLOCK (clock), TRUE);
	align = gtk_alignment_new (1.0, 1.0, 0.3, 0.3);
	gtk_container_add (GTK_CONTAINER (align), clock);
	gtk_container_add (GTK_CONTAINER (map), align);
	
	gtk_container_add (GTK_CONTAINER (window), map);
	
	gtk_window_set_default_size (GTK_WINDOW (window), 480, 320);
	gtk_widget_show_all (window);
	
	g_signal_connect (map, "clicked",
		G_CALLBACK (clicked_cb), NULL);
	g_signal_connect (window, "delete-event",
		G_CALLBACK (gtk_main_quit), NULL);

	location = jana_ecal_utils_guess_location ();
	id = g_timeout_add (1000, (GSourceFunc)set_time, clock);
	set_time (clock);

	gtk_main ();

	g_source_remove (id);
	g_free (location);

	return 0;
}
