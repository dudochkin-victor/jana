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


#ifndef JANA_UTILS_H
#define JANA_UTILS_H

#include <time.h>
#include <glib.h>
#include <libjana/jana-event.h>
#include <libjana/jana-note.h>
#include <libjana/jana-task.h>
#include <libjana/jana-time.h>

gboolean jana_utils_time_is_leap_year (guint16 year);

guint8 jana_utils_time_days_in_month (guint16 year, guint8 month);

GDateWeekday jana_utils_time_day_of_week (JanaTime *time);

guint jana_utils_time_day_of_year (JanaTime *time);

GDateWeekday jana_utils_time_set_start_of_week (JanaTime *start);

GDateWeekday jana_utils_time_set_end_of_week (JanaTime *end);

guint jana_utils_time_week_of_year (JanaTime *time,
				    gboolean week_starts_monday);

GDate * jana_utils_time_to_gdate (JanaTime *time);

gchar * jana_utils_strftime (JanaTime *time, const gchar *format);

gint jana_utils_time_compare (JanaTime *time1, JanaTime *time2,
			      gboolean date_only);

JanaTime * jana_utils_time_copy (JanaTime *source, JanaTime *dest);

void jana_utils_time_diff (JanaTime *t1, JanaTime *t2, gint *year, gint *month,
			   gint *day, gint *hours, gint *minutes,
			   glong *seconds);

void jana_utils_time_adjust (JanaTime *time, gint year, gint month, gint day,
			     gint hours, gint minutes, gint seconds);

JanaTime * jana_utils_time_now (JanaTime *time);

gboolean jana_utils_duration_contains (JanaDuration *duration, JanaTime *time);

JanaEvent * jana_utils_event_copy (JanaEvent *source, JanaEvent *dest);

JanaNote * jana_utils_note_copy (JanaNote *source, JanaNote *dest);

JanaTask * jana_utils_task_copy (JanaTask *source, JanaTask *dest);

GList * jana_utils_event_get_instances (JanaEvent *event, JanaTime *range_start,
					JanaTime *range_end, glong offset);

void jana_utils_component_insert_category (JanaComponent *component,
					   const gchar *category,
					   gint position);

gboolean jana_utils_component_remove_category (JanaComponent *component,
					       const gchar *category);

gboolean jana_utils_component_has_category (JanaComponent *component,
					    const gchar *category);

void jana_utils_instance_list_free (GList *instances);

gchar * jana_utils_get_local_tzname ();

struct tm jana_utils_time_to_tm (JanaTime *time);

gchar * jana_utils_recurrence_to_string (JanaRecurrence *recur,
					 JanaTime *start);

gboolean jana_utils_recurrence_diff (JanaRecurrence *r1, JanaRecurrence *r2);

const gchar * jana_utils_ab_day (guint day);

gdouble jana_utils_time_daylight_hours (gdouble latitude, guint day_of_year);

#endif /* JANA_UTILS_H */

