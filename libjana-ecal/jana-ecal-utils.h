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


#ifndef JANA_ECAL_UTILS_H
#define JANA_ECAL_UTILS_H

#include <time.h>
#include <glib.h>
#include <libjana/jana-time.h>

/**
 * JANA_ECAL_LOCATION_KEY:
 *
 * The GConf key path used to store the ecal timezone location. This will be 
 * checked by jana_ecal_utils_guess_location() before any other method. For 
 * convenience, this path is the same path Evolution uses to store its 
 * ecal timezone location.
 */
#define JANA_ECAL_LOCATION_KEY "/apps/evolution/calendar/display/timezone"

/**
 * JANA_ECAL_LOCATION_KEY:
 *
 * The GConf key used to determine whether to use the system timezone or to
 * use the #JANA_ECAL_LOCATION_KEY.
 */
#define JANA_ECAL_SYSTEM_TIMEZONE_KEY "/apps/evolution/calendar/display/use_system_timezone"

/**
 * JANA_ECAL_LOCATION_KEY_DIR:
 * @see_also: #JANA_ECAL_LOCATION_KEY
 *
 * The parent directory of the GConf key used to store the ecal timezone
 * location. If #JANA_ECAL_LOCATION_KEY exists, this GConf directory can be 
 * monitored to detect when it has changed. See also #JANA_ECAL_LOCATION_KEY.
 */
#define JANA_ECAL_LOCATION_KEY_DIR "/apps/evolution/calendar/display"

JanaTime * jana_ecal_utils_time_now (const gchar *location);
JanaTime * jana_ecal_utils_time_now_local ();
JanaTime * jana_ecal_utils_time_today (const gchar *location);
gchar * jana_ecal_utils_guess_location ();
gchar ** jana_ecal_utils_get_locations ();

#endif

