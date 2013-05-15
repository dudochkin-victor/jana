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
#include <libical/icaltimezone.h>
#include <libical/icaltime.h>
#include <libjana/jana-time.h>
#include <libjana-ecal/jana-ecal-time.h>

/* To build:
 * gcc -o test-jana-ecal-time test-jana-ecal-time.c ../libjana/jana-time.c ../libjana-ecal/jana-ecal-time.c `pkg-config --cflags --libs glib-2.0 libecal-1.2 gobject-2.0` -I../ -g
 */

/* Test if DST auto-adjust works:
 * This test creates a time object for 2:00 1/1/2007, GMT/BST and changes the
 * month to July. If all goes well, the time should be adjusted forward by
 * an hour and it should be in daylight savings.
 * Returns 0 on success and 1 on error.
 */

int
main (int argc, char **argv)
{
	gint error = 0;
	JanaTime *jtime;
	icaltimetype itime;
	icaltimezone *zone = icaltimezone_get_builtin_timezone (
		"Europe/London");
	
	/* Test DST conversions */
	itime = icaltime_null_time ();
	itime.second = 0;
	itime.minute = 0;
	itime.hour = 2;
	
	itime.day = 1;
	itime.month = 1;
	itime.year = 2007;
	
	itime = icaltime_convert_to_zone (itime, zone);
	
	g_type_init ();
	
	jtime = jana_ecal_time_new_from_icaltime (&itime);
	
	/*g_debug ("%s time: %d/%d/%d, %d:%02d %s",
		jana_time_get_tzname (jtime),
		jana_time_get_day (jtime),
		jana_time_get_month (jtime),
		jana_time_get_year (jtime),
		jana_time_get_hours (jtime),
		jana_time_get_minutes (jtime),
		jana_time_get_daylight (jtime) ? "DST" : "");
	
	g_debug ("Setting time forward to BST");*/
	
	jana_time_set_month (jtime, 7);
	
	if ((jana_time_get_hours (jtime) != 3) ||
	    (!jana_time_get_daylight (jtime)))
		error = 1;

	/*g_debug ("%s time: %d/%d/%d, %d:%02d %s",
		jana_time_get_tzname (jtime),
		jana_time_get_day (jtime),
		jana_time_get_month (jtime),
		jana_time_get_year (jtime),
		jana_time_get_hours (jtime),
		jana_time_get_minutes (jtime),
		jana_time_get_daylight (jtime) ? "DST" : "");*/
	
	g_object_unref (jtime);
	
	if (error)
		g_warning ("Error (%d)", error);
	else
		g_message ("Success");
		
	return error;
}

