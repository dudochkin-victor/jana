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
 * SECTION:jana-utils
 * @short_description: A set of utility functions
 *
 * jana-utils contains a collection of utility functions related to, and 
 * using the interfaces specified in libjana.
 */

#include <string.h>
#include <locale.h>
#include <langinfo.h>
#include <time.h>
#include <math.h>
#include "jana-utils.h"

static const guint8 days_in_month[] =
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static gboolean locale_set = FALSE;

/**
 * jana_utils_time_is_leap_year:
 * @year: A year
 *
 * Determines if @year is a leap year.
 *
 * Returns: %TRUE if @year is a leap year, %FALSE otherwise.
 */
gboolean
jana_utils_time_is_leap_year (guint16 year)
{
	/* See Wikipedia 'leap years':
	 *	if year modulo 400 is 0 then leap
	 *	 else if year modulo 100 is 0 then no_leap
	 *	 else if year modulo 4 is 0 then leap
	 *	 else no_leap
 	 */
 	if ((!(year % 400)) || ((year % 100) && (!(year % 4))))
 		return TRUE;
 	else
 		return FALSE;
}

/**
 * jana_utils_time_days_in_month:
 * @year: A year
 * @month: A month
 *
 * Determines the amount of days in the given month and year.
 *
 * Returns: The amount of days in the given month and year.
 */
guint8
jana_utils_time_days_in_month (guint16 year, guint8 month)
{
	if ((month == 2) && (jana_utils_time_is_leap_year (year)))
		return days_in_month[month-1] + 1;
	else
		return days_in_month[month-1];
}

/**
 * jana_utils_time_day_of_week:
 * @time: A #JanaTime
 *
 * Determines the day of the week that @time occurs on.
 *
 * Returns: The day of the week that @time occurs on.
 */
GDateWeekday
jana_utils_time_day_of_week (JanaTime *time)
{
	GDateWeekday day;
	GDate *date;
	
	/* TODO: Write function to do this directly on a JanaTime */
	date = jana_utils_time_to_gdate (time);
	day = g_date_get_weekday (date);
	g_date_free (date);
	
	return day;
}

/**
 * jana_utils_time_day_of_year:
 * @time: A #JanaTime
 *
 * Determines what day of the year @time is, where 1st January is the first 
 * day of the year.
 *
 * Returns: The day of the year that @time is.
 */
guint
jana_utils_time_day_of_year (JanaTime *time)
{
	guint month, day;
	
	day = 0;
	
	for (month = 1; month < jana_time_get_month (time); month++)
		day += jana_utils_time_days_in_month (
			jana_time_get_year (time), month);
	
	day += jana_time_get_day (time);
	
	return day;
}

/**
 * jana_utils_time_week_of_year:
 * @time: A #JanaTime
 * @week_starts_monday: %TRUE if the week begins on Monday, %FALSE if it 
 * begins on Sunday
 *
 * Retrieves what week of the year @time occurs within.
 *
 * Returns: The week of the year @time occurs within.
 */
guint
jana_utils_time_week_of_year (JanaTime *time, gboolean week_starts_monday)
{
	guint week;
	GDate *date;
	
	/* TODO: Write function to do this directly on a JanaTime */
	date = jana_utils_time_to_gdate (time);
	if (week_starts_monday)
		week = g_date_get_monday_week_of_year (date);
	else
		week = g_date_get_sunday_week_of_year (date);
	g_date_free (date);
	
	return week;
}

/**
 * jana_utils_time_set_start_of_week:
 * @start: A #JanaTime
 *
 * Moves @start backwards to the start of the week (Monday), if it isn't 
 * already.
 *
 * Returns: The day of the week @start was on, before modification.
 */
GDateWeekday
jana_utils_time_set_start_of_week (JanaTime *start)
{
	GDateWeekday day = jana_utils_time_day_of_week (start);
	if (day)
		jana_time_set_day (start,
			jana_time_get_day (start) - (day - 1));
	
	return day;
}

/**
 * jana_utils_time_set_end_of_week:
 * @end: A #JanaTime
 *
 * Moves @end forwards to the end of the week (Sunday), if it isn't already.
 *
 * Returns: The day of the week @end was on, before modification.
 */
GDateWeekday
jana_utils_time_set_end_of_week (JanaTime *end)
{
	GDateWeekday day = jana_utils_time_day_of_week (end);
	if (day)
		jana_time_set_day (end,
			jana_time_get_day (end) + (7 - day));

	return day;
}

/**
 * jana_utils_time_to_gdate:
 * @time: A #JanaTime
 *
 * Converts a #JanaTime to a #GDate.
 *
 * Returns: A newly allocated #GDate, with fields set from @time.
 */
GDate *
jana_utils_time_to_gdate (JanaTime *time)
{
	GDate *date = g_date_new ();
	g_date_set_dmy (date, jana_time_get_day (time),
		jana_time_get_month (time),
		jana_time_get_year (time));

	return date;
}

/**
 * jana_utils_time_to_tm:
 * @time: #JanaTime implementation
 *
 * Converts @time to a broken-down time structure, usable by libc time
 * functions. tm_gmtoff and tm_zone components of this structure are not 
 * filled in, however, they can be completed using jana_time_get_offset() and 
 * jana_time_get_tzname().
 *
 * Returns: A broken-down time structure
 */
struct tm
jana_utils_time_to_tm (JanaTime *time)
{
	struct tm broken;
	
	memset (&broken, 0, sizeof (struct tm));
	broken.tm_sec = jana_time_get_seconds (time);
	broken.tm_min = jana_time_get_minutes (time);
	broken.tm_hour = jana_time_get_hours (time);
	broken.tm_mday = jana_time_get_day (time);
	broken.tm_mon = jana_time_get_month (time) - 1;
	broken.tm_year = jana_time_get_year (time) - 1900;
	
	/* Normalise time (fills in details such as day of week, etc.) */
	mktime (&broken);
	
	/* The time was already adjusted for DST, un-adjust */
	/* FIXME: Is DST always one hour? Cursory investigation suggests yes */
	if (broken.tm_isdst > 0) broken.tm_hour -= 1;
	
	return broken;
}

/**
 * jana_utils_strftime:
 * @time: #JanaTime implementation
 * @format: Time formatting string
 *
 * Create a formatted string of @time, using strftime() and @format.
 *
 * Returns: A newly allocated string, representing @time using @format, or
 * %NULL on failure.
 */
gchar *
jana_utils_strftime (JanaTime *time, const gchar *format)
{
	gchar *string;
	gint size;
	struct tm brokentime = jana_utils_time_to_tm (time);
	
	if ((size = strftime (NULL, G_MAXINT, format, &brokentime))) {
		string = g_malloc (size + 1);
		strftime (string, size + 1, format, &brokentime);
		return string;
	} else return NULL;
}

/**
 * jana_utils_time_compare:
 * @time1: A #JanaTime
 * @time2: A #JanaTime to compare with
 * @date_only: Whether to only consider date fields
 *
 * Compares @time1 to @time2, taking into account the offset field.
 *
 * Returns: -1 if @time1 occurs before @time2, 0 if @time1 is equal to @time2, 
 * and 1 if @time1 is after @time2.
 */
gint
jana_utils_time_compare (JanaTime *time1, JanaTime *time2, gboolean date_only)
{
	JanaTime *time2_corrected;
	gint time1_part;
	guint8 seconds, minutes, hours, day, month;
	guint16 year;
	gboolean corrected = FALSE;
	
	/* Get time2 relative to time1 */
	if (jana_time_get_offset (time1) == jana_time_get_offset (time2))
		time2_corrected = time2;
	else {
		time2_corrected = jana_time_duplicate (time2);
		jana_time_set_offset (time2_corrected,
			jana_time_get_offset (time1));
		corrected = TRUE;
	}
	year = jana_time_get_year (time2_corrected);
	month = jana_time_get_month (time2_corrected);
	day = jana_time_get_day (time2_corrected);
	if (!jana_time_get_isdate (time2_corrected)) {
		hours = jana_time_get_hours (time2_corrected);
		minutes = jana_time_get_minutes (time2_corrected);
		seconds = jana_time_get_seconds (time2_corrected);
	} else {
		hours = 0; minutes = 0; seconds = 0;
	}
	if (corrected) g_object_unref (time2_corrected);
	
	/* Check years */
	time1_part = jana_time_get_year (time1);
	if (time1_part < year)
		return -1;
	else if (time1_part > year)
		return 1;
	
	/* Check months */
	time1_part = jana_time_get_month (time1);
	if (time1_part < month)
		return -1;
	else if (time1_part > month)
		return 1;
	
	/* Check days */
	time1_part = jana_time_get_day (time1);
	if (time1_part < day)
		return -1;
	else if (time1_part > day)
		return 1;
	
	if (date_only) return 0;
	if (jana_time_get_isdate (time1)) {
		if (hours || minutes || seconds)
			return -1;
		else
			return 0;
	}
	
	/* Check hours */
	time1_part = jana_time_get_hours (time1);
	if (time1_part < hours)
		return -1;
	else if (time1_part > hours)
		return 1;
		
	/* Check minutes */
	time1_part = jana_time_get_minutes (time1);
	if (time1_part < minutes)
		return -1;
	else if (time1_part > minutes)
		return 1;
	
	/* Check seconds */
	time1_part = jana_time_get_seconds (time1);
	if (time1_part < seconds)
		return -1;
	else if (time1_part > seconds)
		return 1;
	
	return 0;
}

/**
 * jana_utils_time_copy:
 * @source: A #JanaTime to copy from
 * @dest: A #JanaTime to copy to
 *
 * Copies the contents of @source to @dest. This can be useful when you want 
 * to duplicate a time without creating a new #JanaTime, or when you need 
 * to convert a time between different implementations of #JanaTime. Fields 
 * are set in such an order as to not cause normalisation to alter the time.
 *
 * Returns: @dest.
 */
JanaTime *
jana_utils_time_copy (JanaTime *source, JanaTime *dest)
{
	gchar *tzname;
	
	tzname = jana_time_get_tzname (source);
	jana_time_set_tzname (dest, tzname);
	g_free (tzname);
	
	jana_time_set_offset (dest, jana_time_get_offset (source));
	
	jana_time_set_year (dest, jana_time_get_year (source));
	jana_time_set_month (dest, jana_time_get_month (source));
	jana_time_set_day (dest, jana_time_get_day (source));

	jana_time_set_hours (dest, jana_time_get_hours (source));
	jana_time_set_minutes (dest, jana_time_get_minutes (source));
	jana_time_set_seconds (dest, jana_time_get_seconds (source));

	jana_time_set_isdate (dest, jana_time_get_isdate (source));
	
	return dest;
}

static gint
days_between (gint year1, gint year2) {
	gint reverse = 0, year, days;
	
	if (year1 > year2) {
		reverse = year1;
		year1 = year2;
		year2 = reverse;
	}
	
	for (days = 0, year = year1; year < year2; year++) {
		if (jana_utils_time_is_leap_year (year)) days += 366;
		else days += 365;
	}
	
	return reverse ? -days : days;
}

/**
 * jana_utils_time_diff:
 * @t1: A #JanaTime
 * @t2: A #JanaTime
 * @year: Return location for difference in years, or %NULL
 * @month: Return location for difference in months, or %NULL
 * @day: Return location for difference in days, or %NULL
 * @hours: Return location for difference in hours, or %NULL
 * @minutes: Return location for difference in minutes, or %NULL
 * @seconds: Return location for difference in seconds, or %NULL
 *
 * Gets the difference between two times. Times are compared so that if @t2 is
 * later than @t1, the difference is positive. If a parameter is ommitted, the
 * result will be accumulated in the next nearest parameter. For example, 
 * if there were a years difference between @t1 and @t2, but the @year
 * parameter was ommitted, the @month parameter will accumulate 12 months. 
 * This function takes into account the relative time offsets of @t1 and @t2, 
 * so no prior conversion is required.
 */
void
jana_utils_time_diff (JanaTime *t1, JanaTime *t2, gint *year, gint *month,
		      gint *day, gint *hours, gint *minutes, glong *seconds)
{
	gboolean corrected = FALSE;
	
	if (year) *year = 0;
	if (month) *month = 0;
	if (day) *day = 0;
	if (hours) *hours = 0;
	if (minutes) *minutes = 0;
	if (seconds) *seconds = 0;
	
	if (jana_time_get_offset (t1) != jana_time_get_offset (t2)) {
		t2 = jana_time_duplicate (t2);
		jana_time_set_offset (t2, jana_time_get_offset (t1));
		corrected = TRUE;
	}
	
	/* FIXME: Consider leap seconds? */
	if (year) *year = jana_time_get_year (t2) - jana_time_get_year (t1);
	else if (month) *month = (jana_time_get_year (t2) -
		jana_time_get_year (t1)) * 12;
	else {
		gint days = days_between (
			jana_time_get_year (t1), jana_time_get_year (t2));
		if (day) *day = days;
		else if (hours) *hours = days * 24;
		else if (minutes) *minutes = days * 24 * 60;
		else if (seconds) *seconds = days * 24 * 60 * 60;
	}
	
	if (month) {
		*month += jana_time_get_month (t2) - jana_time_get_month (t1);
	} else {
		/* Count days in the month difference */
		gint year, month, days;
		year = jana_time_get_year (t1);
		for (days = 0, month = jana_time_get_month (t1);
		     month < jana_time_get_month (t2);
		     month = (month % 12) + 1) {
			days += jana_utils_time_days_in_month (year, month);
			if (month == 12) year++;
		}
		
		if (day) *day += days;
		else if (hours) *hours += days * 24;
		else if (minutes) *minutes += days * 24 * 60;
		else if (seconds) *seconds += days * 24 * 60 * 60;
	}
	
	if (day) *day += jana_time_get_day (t2) - jana_time_get_day (t1);
	else if (hours) *hours += (jana_time_get_day (t2) -
		jana_time_get_day (t1)) * 24;
	else if (minutes) *minutes += (jana_time_get_day (t2) -
		jana_time_get_day (t1)) * 24 * 60;
	else if (seconds) *seconds += (jana_time_get_day (t2) -
		jana_time_get_day (t1)) * 24 * 60 * 60;
	
	if (hours) *hours += jana_time_get_hours (t2) - jana_time_get_hours (t1);
	else if (minutes) *minutes += (jana_time_get_hours (t2) -
		jana_time_get_hours (t1)) * 60;
	else if (seconds) *seconds += (jana_time_get_hours (t2) -
		jana_time_get_hours (t1)) * 60 * 60;

	if (minutes) *minutes += jana_time_get_minutes (t2) -
		jana_time_get_minutes (t1);
	else if (seconds) *seconds += (jana_time_get_minutes (t2) -
		jana_time_get_minutes (t1)) * 60;

	if (seconds) *seconds += jana_time_get_seconds (t2) -
		jana_time_get_seconds (t1);

	/* FIXME: Normalise? */
	
	if (corrected) g_object_unref (t2);
}

/**
 * jana_utils_time_adjust:
 * @time: A #JanaTime
 * @year: Years to add to @time
 * @month: Months to add to @time
 * @day: Days to add to @time
 * @hours: Hours to add to @time
 * @minutes: Minutes to add to @time
 * @seconds: Seconds to add to @time
 *
 * A convenience function to adjust a time by a specified amount on each field. 
 * The time will be adjusted in the order seconds, minutes, hours, days, months 
 * and years. It is important to take into account how a time may be normalised 
 * when adjusting. This function can be used in conjunction with 
 * jana_utils_time_diff() to adjust one time to be equal to another, while 
 * taking time offsets into account.
 */
void
jana_utils_time_adjust (JanaTime *time, gint year, gint month, gint day,
			gint hours, gint minutes, gint seconds)
{
	if (seconds) jana_time_set_seconds (
		time, jana_time_get_seconds (time) + seconds);
	if (minutes) jana_time_set_minutes (
		time, jana_time_get_minutes (time) + minutes);
	if (hours) jana_time_set_hours (
		time, jana_time_get_hours (time) + hours);
	if (day) jana_time_set_day (
		time, jana_time_get_day (time) + day);
	if (month) jana_time_set_month (
		time, jana_time_get_month (time) + month);
	if (year) jana_time_set_year (
		time, jana_time_get_year (time) + year);
}

/**
 * jana_utils_time_now:
 * @time: A #JanaTime to overwrite
 *
 * Retrieves the current time, with the timezone and offset set.
 *
 * Returns: @time.
 */
JanaTime *
jana_utils_time_now (JanaTime *jtime)
{
	time_t now_t = time (NULL);
	struct tm *now = localtime (&now_t);
	gchar *tzname = jana_utils_get_local_tzname ();
	
	jana_time_set_tzname (jtime, tzname);
	jana_time_set_offset (jtime, now->tm_gmtoff);
	g_free (tzname);
	
	jana_time_set_seconds (jtime, now->tm_sec);
	jana_time_set_minutes (jtime, now->tm_min);
	jana_time_set_hours (jtime, now->tm_hour);
	
	jana_time_set_day (jtime, now->tm_mday);
	jana_time_set_month (jtime, now->tm_mon + 1);
	jana_time_set_year (jtime, now->tm_year + 1900);
	
	return jtime;
}

/**
 * jana_utils_duration_contains:
 * @duration: A #JanaDuration
 * @time: A #JanaTime
 *
 * Determines whether @time occurs within @duration.
 *
 * Returns: %TRUE if @time occurs within @duration, %FALSE otherwise.
 */
gboolean
jana_utils_duration_contains (JanaDuration *duration, JanaTime *time)
{
	if ((jana_utils_time_compare (time, duration->start, FALSE) >= 0) &&
	    (jana_utils_time_compare (time, duration->end, FALSE) < 0))
		return TRUE;
	else
		return FALSE;
}

static void
copy_component_fields (JanaComponent *source, JanaComponent *dest)
{
	gchar **string_list;

	string_list = jana_component_get_categories (source);
	jana_component_set_categories (dest, (const gchar **)string_list);
	g_strfreev (string_list);
	
	/* FIXME: Also copy custom component properties? */
}

/**
 * jana_utils_event_copy:
 * @source: A #JanaEvent to copy from
 * @dest: A #JanaEvent to copy to
 *
 * Copies the contents of @source into @dest. This function copies all fields
 * of @source over those of @dest, taking into account whether alarms, 
 * recurrences and custom properties are supported.
 *
 * Returns: @dest.
 */
JanaEvent *
jana_utils_event_copy (JanaEvent *source, JanaEvent *dest)
{
	gchar *string;
	JanaTime *time;

	string = jana_event_get_summary (source);
	jana_event_set_summary (dest, string);
	g_free (string);
	
	string = jana_event_get_description (source);
	jana_event_set_description (dest, string);
	g_free (string);
	
	string = jana_event_get_location (source);
	jana_event_set_location (dest, string);
	g_free (string);
	
	time = jana_event_get_start (source);
	jana_event_set_start (dest, time);
	g_object_unref (time);

	time = jana_event_get_end (source);
	jana_event_set_end (dest, time);
	g_object_unref (time);
	
	copy_component_fields ((JanaComponent *)source, (JanaComponent *)dest);
	
	if (jana_event_supports_alarm (dest) &&
	    jana_event_supports_alarm (source)) {
		if (jana_event_has_alarm (source)) {
			time = jana_event_get_alarm_time (source);
			jana_event_set_alarm (dest, time);
			g_object_unref (time);
		} else
			jana_event_set_alarm (dest, NULL);
	}

	if (jana_event_supports_recurrence (dest) &&
	    jana_event_supports_recurrence (source)) {
		if (jana_event_has_recurrence (source)) {
			JanaRecurrence *recur =
				jana_event_get_recurrence (source);
			jana_event_set_recurrence (dest, recur);
			jana_recurrence_free (recur);
		} else
			jana_event_set_recurrence (dest, NULL);
	}

	if (jana_event_supports_exceptions (dest) &&
	    jana_event_supports_exceptions (source)) {
		if (jana_event_has_exceptions (source)) {
			GList *exceptions = jana_event_get_exceptions (source);
			jana_event_set_exceptions (dest, exceptions);
			jana_exceptions_free (exceptions);
		} else
			jana_event_set_exceptions (dest, NULL);
	}
	
	return dest;
}

/**
 * jana_utils_note_copy:
 * @source: A #JanaNote to copy from
 * @dest: A #JanaNote to copy to
 *
 * Copies the contents of @source into @dest. This function copies all fields
 * of @source over those of @dest.
 *
 * Returns: @dest.
 */
JanaNote *
jana_utils_note_copy (JanaNote *source, JanaNote *dest)
{
	gchar *string;
	JanaTime *time;

	string = jana_note_get_author (source);
	jana_note_set_author (dest, string);
	g_free (string);
	
	string = jana_note_get_recipient (source);
	jana_note_set_recipient (dest, string);
	g_free (string);
	
	string = jana_note_get_body (source);
	jana_note_set_body (dest, string);
	g_free (string);
	
	time = jana_note_get_creation_time (source);
	jana_note_set_creation_time (dest, time);
	g_object_unref (time);
	
	copy_component_fields ((JanaComponent *)source, (JanaComponent *)dest);

	return dest;
}

/**
 * jana_utils_task_copy:
 * @source: A #JanaTask to copy from
 * @dest: A #JanaTask to copy to
 *
 * Copies the contents of @source into @dest. This function copies all fields
 * of @source over those of @dest.
 *
 * Returns: @dest.
 */
JanaTask *
jana_utils_task_copy (JanaTask *source, JanaTask *dest)
{
	gchar *string;
	JanaTime *time;
	gint priority;
	gboolean completed;

	string = jana_task_get_summary (source);
	jana_task_set_summary (dest, string);
	g_free (string);
	
	string = jana_task_get_description (source);
	jana_task_set_description (dest, string);
	g_free (string);
	
	completed = jana_task_get_completed (source);
	jana_task_set_completed (dest, completed);
	
	priority = jana_task_get_priority (source);
	jana_task_set_priority (dest, priority);

	time = jana_task_get_due_date (source);
	jana_task_set_due_date (dest, time);
	g_object_unref (time);
	
	copy_component_fields ((JanaComponent *)source, (JanaComponent *)dest);

	return dest;
}

static GList *
jana_utils_event_get_instances_cb (JanaTime *start, JanaTime *end,
				   JanaTime *range_start, JanaTime *range_end,
				   glong offset)
{
	JanaTime *instance_start, *instance_end;
	GList *instances = NULL;
	
	/* TODO: Exception support */
	
	/* Split instances into days */
	instance_start = jana_time_duplicate (start);
	jana_time_set_offset (instance_start, offset);
	instance_end = jana_time_duplicate (instance_start);
	jana_time_set_isdate (instance_end, TRUE);
	jana_time_set_day (instance_end, jana_time_get_day (instance_end) + 1);
	
	end = jana_time_duplicate (end);
	jana_time_set_offset (end, offset);

	while (jana_utils_time_compare (instance_start, end, FALSE) < 0) {
		JanaDuration *instance;
		if (jana_utils_time_compare (instance_end, end, FALSE) > 0) {
			g_object_unref (instance_end);
			instance_end = jana_time_duplicate (end);
		}
		
		if (((!range_end) || (jana_utils_time_compare (
		     instance_start, range_end, FALSE) < 0)) &&
		    ((!range_start) || (jana_utils_time_compare (
		     instance_end, range_start, FALSE) >= 0))) {
			instance = jana_duration_new (
				instance_start, instance_end);
			instances = g_list_append (instances, instance);
		} else if (range_end && (jana_utils_time_compare (
			   instance_start, range_end, FALSE) >= 0))
			break;
		
		jana_time_set_day (instance_start, jana_time_get_day (
			instance_start) + 1);
		jana_time_set_day (instance_end, jana_time_get_day (
			instance_end) + 1);
		jana_time_set_isdate (instance_start, TRUE);
	}
	
	g_object_unref (instance_start);
	g_object_unref (instance_end);
	g_object_unref (end);
	
	return instances;
}

/**
 * jana_utils_event_get_instances:
 * @event: A #JanaEvent
 * @range_start: The start boundary for instances, or %NULL for no boundary
 * @range_end: The end boundary for instances, or %NULL for no boundary
 * @offset: The offset, in seconds, to offset the event by
 *
 * Splits an event across each day it occurs, taking into account @offset, 
 * and returns a list of #JanaDuration's for each day it occurs. The first and 
 * last instances have their start and end adjusted correctly. If the event 
 * doesn't occur over more than one day, the list will contain just one 
 * duration whose start and end match the start and end of the event, adjusted 
 * by @offset. If @range_end is %NULL and @event has an indefinite 
 * recurrence, the recurrence will be ignored. This is to avoid 
 * infinite loops; it is discouraged to call this function without bounds.
 *
 * Returns: A list of #JanaDuration's for each day @event occurs. This list 
 * should be freed with jana_utils_instance_list_free().
 */
GList *
jana_utils_event_get_instances (JanaEvent *event, JanaTime *range_start,
				JanaTime *range_end, glong offset)
{
	JanaTime *start, *end;
	GList *instances = NULL;
	
	if (jana_event_has_recurrence (event)) {
		gint range_year;
		JanaRecurrence *recur = jana_event_get_recurrence (event);
		
		start = jana_event_get_start (event);
		/* All recurrences re-occur yearly, so we can skip to the
		 * same year as the range_start at least.
		 * FIXME: Verify this is ok in the situation of events
		 *        starting on leap years...
		 */
		range_year = jana_time_get_year (range_start);
		if (jana_time_get_year (start) < range_year) {
			jana_time_set_year (start, range_year);
		}
		end = jana_event_get_end (event);

		/* Skip recurrences if an ending bound isn't set, or if the 
		 * interval is invalid
		 */
		if (((!range_end) && (!recur->end)) ||
		    (recur->interval == 0)) {
			instances = jana_utils_event_get_instances_cb (
				start, end, range_start, range_end, offset);
		} else switch (recur->type) {
		    case JANA_RECURRENCE_DAILY :
			while (((!recur->end) || (jana_utils_time_compare (
			        start, recur->end, TRUE) <= 0)) &&
			       ((!range_end) || (jana_utils_time_compare (
				start, range_end, FALSE) < 0))) {
				instances = g_list_concat (instances,
					jana_utils_event_get_instances_cb (
						start, end, range_start,
						range_end, offset));
				
				/* Increment days by the interval size */
				jana_time_set_day (start,
					jana_time_get_day (start) +
					recur->interval);
				jana_time_set_day (end,
					jana_time_get_day (end) +
					recur->interval);
			}
			break;
		    case JANA_RECURRENCE_WEEKLY :
			while (((!recur->end) || (jana_utils_time_compare (
			        start, recur->end, TRUE) <= 0)) &&
			       ((!range_end) || (jana_utils_time_compare (
				start, range_end, FALSE) < 0))) {
				gint i, day_adjust;
				GDateWeekday day;
				instances = g_list_concat (instances,
					jana_utils_event_get_instances_cb (
						start, end, range_start,
						range_end, offset));
				
				day = jana_utils_time_day_of_week (start);
				
				/* Skip weeks depending on interval */
				if (recur->interval > 1) {
					gboolean skip = TRUE;
					/* Check if there are more occurences
					 * left this week first
					 */
					for (i = day; i < 7; i++) {
						if (recur->week_days[i]) {
							skip = FALSE;
							break;
						}
					}
					/* If not, skip the appropriate amount 
					 * of weeks
					 */
					if (skip) {
						jana_time_set_day (start,
							jana_time_get_day (
								start) +
							(7 * (recur->interval -
							      1)));
						jana_time_set_day (end,
							jana_time_get_day (
								end) +
							(7 * (recur->interval -
							      1)));
					}
				}
				
				/* Figure out when the next occurrence is */
				for (i = day % 7,day_adjust = 1; day_adjust < 7;
				     i = (i + 1) % 7, day_adjust ++) {
					if (recur->week_days[i]) break;
				}
				
				jana_utils_time_adjust (start, 0, 0, day_adjust,
					0, 0, 0);
				jana_utils_time_adjust (end, 0, 0, day_adjust,
					0, 0, 0);
			}
			break;
		    case JANA_RECURRENCE_MONTHLY : {
			gint nth_day = ((jana_time_get_day (start) - 1) / 7)+1;
			if ((jana_time_get_day (start) + 7) >
			    jana_utils_time_days_in_month (jana_time_get_year (
			    start), jana_time_get_month (start))) {
				nth_day = -1;
			}
			while (((!recur->end) || (jana_utils_time_compare (
			        start, recur->end, TRUE) <= 0)) &&
			       ((!range_end) || (jana_utils_time_compare (
				start, range_end, FALSE) < 0))) {
				instances = g_list_concat (instances,
					jana_utils_event_get_instances_cb (
						start, end, range_start,
						range_end, offset));
				
				/* Skip months depending on interval */
				if (recur->interval > 1) {
					jana_time_set_month (start,
						jana_time_get_month (start) +
						(recur->interval - 1));
					jana_time_set_month (end,
						jana_time_get_month (end) +
						(recur->interval - 1));
				}
				
				if (recur->by_date) {
					/* Simple case,just add another month */
					jana_time_set_month (start,
						jana_time_get_month (start)+1);
					jana_time_set_month (end,
						jana_time_get_month (end) + 1);
				} else {
					/* Less simple, get to the nth day of
					 * the next month.
					 */
					gint month = jana_time_get_month (
						start);
					gint weeks = 3;
					
					/* Start by adding 3 weeks */
					jana_time_set_day (start,
						jana_time_get_day (start) + 21);
					
					/* Add weeks until we're into the next
					 * month.
					 */
					while (jana_time_get_month (start) ==
					       month) {
						jana_time_set_day (start,
							jana_time_get_day (
								start) + 7);
						weeks ++;
					}
					
					/* Apologies for the indenting that
					 * follows :(
					 */
					
					/* Add weeks until we're onto the right
					 * day.
					 */
					if (nth_day > 0) {
						/* 'nth day of month' style */
						while ((((jana_time_get_day (
						       start) - 1) / 7)+1) !=
						       nth_day) {
							jana_time_set_day (
							    start,
							    jana_time_get_day (
								start) + 7);
							weeks ++;
						}
					} else {
						/* Last day of month style,
						 * skip to the last day of the 
						 * month.
						 */
						while ((jana_time_get_day (
							start) + 7) <
						jana_utils_time_days_in_month (
							jana_time_get_year (
								start),
						jana_time_get_month (start))) {
							jana_time_set_day (
							    start,
							    jana_time_get_day (
								start) + 7);
							weeks ++;
						}
					}
					
					/* Adjust end */
					jana_time_set_day (end,
						jana_time_get_day (end) +
						(weeks * 7));
				}
			}
			break;
		    }
		    case JANA_RECURRENCE_YEARLY :
			while (((!recur->end) || (jana_utils_time_compare (
			        start, recur->end, TRUE) <= 0)) &&
			       ((!range_end) || (jana_utils_time_compare (
				start, range_end, FALSE) < 0))) {
				instances = g_list_concat (instances,
					jana_utils_event_get_instances_cb (
						start, end, range_start,
						range_end, offset));
				
				/* Increment years by the interval size */
				jana_time_set_year (start,
					jana_time_get_year (start) +
					recur->interval);
				jana_time_set_year (end,
					jana_time_get_year (end) +
					recur->interval);
			}
			break;
		}
		
		jana_recurrence_free (recur);
		g_object_unref (start);
		g_object_unref (end);
		
		return instances;
	} else {
		start = jana_event_get_start (event);
		end = jana_event_get_end (event);
		instances = jana_utils_event_get_instances_cb (
			start, end, range_start, range_end, offset);
		
		g_object_unref (start);
		g_object_unref (end);
		
		return instances;
	}
}

/**
 * jana_utils_component_insert_category:
 * @component: A #JanaComponent
 * @category: The category to insert
 * @position: The position to insert at
 *
 * Inserts a category in the category list of the given component, at the 
 * given position. If @position is %-1, the category will be placed at the end 
 * of the list.
 */
void
jana_utils_component_insert_category (JanaComponent *component,
				      const gchar *category,
				      gint position)
{
	gint i, j, size, length;
	gchar **categories;
	const gchar **new_categories;
	
	categories = jana_component_get_categories (component);
	
	length = categories ? g_strv_length (categories) : 0;
	size = sizeof (gchar *) * (length + 2);
	new_categories = g_slice_alloc0 (size);
	
	for (i = 0, j = 0; i < length; i++, j++) {
		if (i == position) {
			new_categories[j] = category;
			j++;
		}
		new_categories[j] = categories[i];
	}
	if (i == j) new_categories[i] = category;
	
	jana_component_set_categories (component, new_categories);
	
	g_slice_free1 (size, new_categories);
	g_strfreev (categories);
}

/**
 * jana_utils_component_remove_category:
 * @component: A #JanaComponent
 * @category: The category to remove
 *
 * Removes the first occurrence of the given category from the component's 
 * category list.
 *
 * Returns: %TRUE if a category was removed, %FALSE otherwise.
 */
gboolean
jana_utils_component_remove_category (JanaComponent *component,
				      const gchar *category)
{
	gint i, j, size, length;
	gchar **categories;
	const gchar **new_categories;
	gboolean removed = FALSE;

	categories = jana_component_get_categories (component);
	if (!categories) return FALSE;
	
	length = g_strv_length (categories);
	size = sizeof (gchar *) * length;
	new_categories = g_slice_alloc0 (size);
	
	for (i = 0, j = 0; i < g_strv_length (categories); i++, j++) {
		if (strcmp (categories[i], category) == 0) j--;
		else new_categories[j] = categories[i];
	}
	if (i != j) {
		removed = TRUE;
		jana_component_set_categories (component, new_categories);
	}
	
	g_slice_free1 (size, new_categories);
	g_strfreev (categories);
	
	return removed;
}

/**
 * jana_utils_component_has_category:
 * @component: A #JanaComponent
 * @category: The category to search for
 *
 * Checks if the specified component has a particular category set.
 *
 * Returns: %TRUE if that category exists, %FALSE otherwise.
 */
gboolean
jana_utils_component_has_category (JanaComponent *component,
				   const gchar *category)
{
	gint i;
	gchar **categories;
	
	if (!category) return FALSE;
	
	categories = jana_component_get_categories (component);
	if (!categories) return FALSE;
	
	for (i = 0; i < g_strv_length (categories); i++) {
		if (strcmp (categories[i], category) == 0) {
			g_strfreev (categories);
			return TRUE;
		}
	}

	g_strfreev (categories);
	
	return FALSE;
}

/**
 * jana_utils_instance_list_free:
 * @instances: An instance list returned by jana_utils_event_get_instances()
 *
 * Frees an event instance list. See jana_utils_event_get_instances().
 */
void
jana_utils_instance_list_free (GList *instances)
{
	while (instances) {
		jana_duration_free ((JanaDuration *)instances->data);
		instances = g_list_delete_link (instances, instances);
	}
}

/**
 * jana_utils_get_local_tzname:
 *
 * Retrieves the local timezone name using system functions.
 *
 * Returns: A newly allocated string with the local tzname.
 */
gchar *
jana_utils_get_local_tzname ()
{
	static gboolean tzset_called = FALSE;
	
	if (!tzset_called) {
		tzset ();
	}

	return g_strdup_printf ("%s%s%s", tzname[0],
		(tzname[1]&&(tzname[1][0]!='\0')) ? "/" : "",
		(tzname[1]&&(tzname[1][0]!='\0')) ? tzname[1] : "");
}

static const
gchar *number_suffix (gint number)
{
	if ((number == 11) || (number == 13)) return "th";
	else switch (number % 10) {
	    case 1 :
		return "st";
	    case 2 :
		return "nd";
	    case 3 :
		return "rd";
	    default :
		return "th";
	}
}

/**
 * jana_utils_recurrence_to_string:
 * @recur: A #JanaRecurrence
 * @start: The start of the recurrence
 *
 * Creates a localised string that represents @recur in a human-readable 
 * format. If @recur is %NULL, a string representing no recurrence 
 * will be returned. If @start is %NULL, monthly recurrences will not be 
 * fully described.
 *
 * Returns: A newly allocated string with the human-readable recurrence info
 */
gchar *
jana_utils_recurrence_to_string (JanaRecurrence *recur, JanaTime *start)
{
	/* This is gonna be really hard (impossible?) to localise... */
	GString *string;
	gchar *returnval;

	if (!recur) return g_strdup ("None");
	
	if (!locale_set) {
		setlocale (LC_TIME, "");
		locale_set = TRUE;
	}

	string = g_string_new ("Every ");
	switch (recur->interval) {
	    case 2 :
		g_string_append (string, "other ");
		break;
	    case 3 :
		g_string_append (string, "third ");
		break;
	    case 4 :
		g_string_append (string, "fourth ");
		break;
	    case 5 :
		g_string_append (string, "fifth ");
		break;
	    case 6 :
		g_string_append (string, "sixth ");
		break;
	    case 7 :
		g_string_append (string, "seventh ");
		break;
	    case 8 :
		g_string_append (string, "eigth ");
		break;
	    case 9 :
		g_string_append (string, "ninth ");
		break;
	    default :
		if (recur->interval > 9) {
			g_string_append_printf (string, "%d%s ",
				recur->interval,
				number_suffix (recur->interval));
		}
		break;
	}
	
	switch (recur->type) {
	    case JANA_RECURRENCE_DAILY :
		g_string_append (string, "day");
		break;
	    case JANA_RECURRENCE_WEEKLY :
		g_string_append (string, "week");
		break;
	    case JANA_RECURRENCE_MONTHLY :
		g_string_append (string, "month");
		break;
	    case JANA_RECURRENCE_YEARLY :
		g_string_append (string, "year");
		break;
	}
	
	if (recur->type == JANA_RECURRENCE_WEEKLY) {
		gint day, days, count;
		gboolean first = TRUE;
		/* Count the recurring days so we can use 'and' before the last
		 */
		for (day = 0, days = 0; day < 7; day++)
			if (recur->week_days[day]) days++;
		
		if (days) g_string_append (string, ", on");
		
		for (day = 0, count = 0; day < 7; day++) {
			if (recur->week_days[day]) {
				count ++;
				if (!first) {
					if (count == days) g_string_append (
						string, " and");
					else g_string_append (string, ",");
				}
				g_string_append_printf (string, " %s",
					nl_langinfo (DAY_1 + ((day + 1) % 7)));
				first = FALSE;
			}
		}
	} else if ((recur->type == JANA_RECURRENCE_MONTHLY) && (start)) {
		if (recur->by_date) {
			gint day = jana_time_get_day (start);

			g_string_append_printf (string, ", on the %d%s",
				day, number_suffix (day));
		} else {
			gint nth_day = ((jana_time_get_day (start) - 1) / 7)+1;
			GDateWeekday day = jana_utils_time_day_of_week (start);

			g_string_append_printf (string, ", on the %d%s %s",
				nth_day, number_suffix (nth_day),
				nl_langinfo (DAY_1 + (day % 7)));
		}
	}
	
	if (recur->end) {
		g_string_append_printf (string, ", until %d/%d/%04d",
			jana_time_get_day (recur->end),
			jana_time_get_month (recur->end),
			jana_time_get_year (recur->end));
	}
	
	returnval = string->str;
	g_string_free (string, FALSE);
	
	return returnval;
}

/**
 * jana_utils_recurrence_diff:
 * @r1: A #JanaRecurrence
 * @r2: The #JanaRecurrence to compare to
 *
 * Determines whether two recurrence rules are different.
 *
 * Returns: %TRUE if @r1 is different to @r2, %FALSE otherwise.
 */
gboolean
jana_utils_recurrence_diff (JanaRecurrence *r1, JanaRecurrence *r2)
{
	if ((!r1) && (!r2)) return FALSE;
	if ((!r1) || (!r2)) return TRUE;
	
	if ((r1->type == r2->type) && (r1->interval == r2->interval) &&
	    (r1->week_days[0] == r2->week_days[0]) &&
	    (r1->week_days[1] == r2->week_days[1]) &&
	    (r1->week_days[2] == r2->week_days[2]) &&
	    (r1->week_days[3] == r2->week_days[3]) &&
	    (r1->week_days[4] == r2->week_days[4]) &&
	    (r1->week_days[5] == r2->week_days[5]) &&
	    (r1->week_days[6] == r2->week_days[6]) &&
	    (r1->by_date == r2->by_date) &&
	    ((r1->end && r2->end &&
	      (jana_utils_time_compare (r1->end, r2->end, FALSE) == 0)) ||
	     ((!r1->end) && (!r2->end))))
		return FALSE;
	else
		return TRUE;
}

/**
 * jana_utils_ab_day:
 * @day: The day number, where 0 is Monday, 1 is Tuesday, etc.
 *
 * Retrieves the first UTF-8 character of the abbreviated, locale-correct day
 * name, in a null-terminated string. The returned string must not be freed 
 * and will be overwritten by subsequent calls to this function.
 *
 * Returns: A NULL-terminated string containing the 
 * first character of the abbreviated day name.
 */
const gchar *
jana_utils_ab_day (guint day)
{
	static gchar character[5];
	const gchar *abday, *firstchar;

	if (!locale_set) {
		setlocale (LC_TIME, "");
		locale_set = TRUE;
	}
	
	abday = nl_langinfo (DAY_1 + ((day + 1)%7));
	firstchar = g_utf8_next_char (abday);
	memcpy (character, abday, firstchar - abday);
	character[firstchar-abday] = '\0';
	
	return character;
}

/**
 * jana_utils_time_daylight_hours:
 * @latitude: A latitude, in decimal degrees
 * @day_of_year: The day of the year
 *
 * Calculates the amount of hours of daylight one should expect at a particular
 * latitude and time of year.
 *
 * Returns: Hours of daylight
 */
gdouble
jana_utils_time_daylight_hours (gdouble latitude, guint day_of_year)
{
	static gboolean first_call = TRUE;
	static gdouble p[365];
	
	/* See:
	 * http://mathforum.org/library/drmath/view/56478.html
	 * for an explanation of this formula.
	 */

	/* Note that there's a race condition here and this block could be 
	 * entered more than once, but it shouldn't matter.
	 */
	if (first_call) {
		gint i;
		for (i = 0; i < 365; i++) {
			p[i] = asin (0.39795 * cos (0.2163108 + 2 * atan (
				0.9671396 * tan (0.0086 * ((gdouble)i-186)))));
		}
		first_call = FALSE;
	}

	return 24 - 24 / M_PI * acos ((sin (0.8333 * M_PI/180) + sin (
		latitude * M_PI/180) * sin (p[day_of_year])) /
		(cos (latitude * M_PI/180) * cos(p[day_of_year])));
}
