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


#ifndef JANA_EVENT_H
#define JANA_EVENT_H

#include <glib-object.h>
#include <libjana/jana-time.h>
#include <libjana/jana-component.h>

#define JANA_TYPE_EVENT		(jana_event_get_type ())
#define JANA_EVENT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj),\
				 JANA_TYPE_EVENT, JanaEvent))
#define JANA_IS_EVENT(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
				 JANA_TYPE_EVENT))
#define JANA_EVENT_GET_INTERFACE(inst)	(G_TYPE_INSTANCE_GET_INTERFACE ((inst),\
					 JANA_TYPE_EVENT, JanaEventInterface))

#define JANA_TYPE_RECURRENCE	(jana_recurrence_get_type ())

/**
 * JanaEvent:
 *
 * The #JanaEvent struct contains only private data.
 */
typedef struct _JanaEvent JanaEvent; /* Dummy object */
typedef struct _JanaEventInterface JanaEventInterface;

/**
 * JanaRecurrenceType:
 * @JANA_RECURRENCE_DAILY: A daily recurrence
 * @JANA_RECURRENCE_WEEKLY: A weekly recurrence
 * @JANA_RECURRENCE_MONTHLY: A monthly recurrence
 * @JANA_RECURRENCE_YEARLY: A yearly recurrence
 *
 * Enum values for different types of recurrence.
 *
 **/
typedef enum {
	JANA_RECURRENCE_DAILY,
	JANA_RECURRENCE_WEEKLY,
	JANA_RECURRENCE_MONTHLY,
	JANA_RECURRENCE_YEARLY,
} JanaRecurrenceType;

/**
 * JanaRecurrence:
 * @type: The type of recurrence.
 * @interval: The interval of the recurrence, where 1 is 'every', 2 is 'every
 * other', 3 is 'every third', etc.
 * @week_days: For weekly recurrence, this indicates which days the recurrence
 * happens on. The zeroth element in the array corresponds to a Monday and each 
 * day follow sequentially. If @week_days is empty, the starting date of the 
 * recurrence is assumed to be the day to occur on. If @week_days does not 
 * include the starting day but includes other days, the recurrence will 
 * occur on the starting day, then every week on the days specified in 
 * @week_days after the starting day.
 * @by_date: For monthly recurrence, this indicates whether the 
 * recurrence is by day or by date (i.e. First Monday of the month, or 1st of
 * the month). If @by_date is %FALSE, the day of the starting date will be 
 * used to determine which day to occur on. If the starting date is the last 
 * day in that month, the recurrence rule is treated as 'last x-day in month',
 * as not all months have the same amount of week-days.
 * @end: The ending time for the recurrence, or %NULL for indefinite recurrences
 *
 * This struct specifies a particular recurrence.
 **/
typedef struct {
	JanaRecurrenceType	type;
	gint 			interval;
	gboolean		week_days[7];
	gboolean 		by_date;
	JanaTime *		end;
} JanaRecurrence;

struct _JanaEventInterface {
	GTypeInterface parent;
	
	/* Standard parts of all events */
	gchar * 		(*get_summary)		(JanaEvent *self);
	gchar * 		(*get_description)	(JanaEvent *self);
	gchar * 		(*get_location)		(JanaEvent *self);
	JanaTime * 		(*get_start)		(JanaEvent *self);
	JanaTime *		(*get_end)		(JanaEvent *self);
	
	/* Alarm-related */
	gboolean		(*supports_alarm)	(JanaEvent *self);
	gboolean		(*has_alarm)		(JanaEvent *self);
	JanaTime *		(*get_alarm_time)	(JanaEvent *self);
	
	/* Recurrence-related */
	gboolean		(*supports_recurrence)	(JanaEvent *self);
	gboolean		(*has_recurrence)	(JanaEvent *self);
	JanaRecurrence	*	(*get_recurrence)	(JanaEvent *self);
	
	/* Exceptions related */
	gboolean		(*supports_exceptions)	(JanaEvent *self);
	gboolean		(*has_exceptions)	(JanaEvent *self);
	/* A list of JanaTime dates on which the event doesn't occur */
	GList *			(*get_exceptions)	(JanaEvent *self);


	/* Setter functions */
	void	(*set_summary)		(JanaEvent *self, const gchar *summary);
	void	(*set_description)	(JanaEvent *self,
					 const gchar *description);
	void	(*set_location)		(JanaEvent *self,
					 const gchar *location);
	void	(*set_start)		(JanaEvent *self, JanaTime *start);
	void	(*set_end)		(JanaEvent *self, JanaTime *end);

	void	(*set_alarm)		(JanaEvent *self, JanaTime *time);

	void	(*set_recurrence)	(JanaEvent *self,
					 const JanaRecurrence *recurrence);
	
	void	(*set_exceptions)	(JanaEvent *self, GList *exceptions);
};

GType jana_event_get_type (void);
GType jana_recurrence_get_type (void);

JanaRecurrence * jana_recurrence_new ();
JanaRecurrence * jana_recurrence_copy (JanaRecurrence *recurrence);
void jana_recurrence_free (JanaRecurrence *recurrence);

void jana_exceptions_free (GList *exceptions);

gchar * 	jana_event_get_summary		(JanaEvent *self);
gchar * 	jana_event_get_description	(JanaEvent *self);
gchar * 	jana_event_get_location		(JanaEvent *self);
JanaTime * 	jana_event_get_start		(JanaEvent *self);
JanaTime *	jana_event_get_end		(JanaEvent *self);
gchar **	jana_event_get_categories	(JanaEvent *self);

gboolean	jana_event_supports_alarm	(JanaEvent *self);
gboolean	jana_event_has_alarm		(JanaEvent *self);
JanaTime *	jana_event_get_alarm_time	(JanaEvent *self);

gboolean		jana_event_supports_recurrence	(JanaEvent *self);
gboolean		jana_event_has_recurrence	(JanaEvent *self);
JanaRecurrence	*	jana_event_get_recurrence	(JanaEvent *self);

gboolean	jana_event_supports_exceptions	(JanaEvent *self);
gboolean	jana_event_has_exceptions	(JanaEvent *self);
GList *		jana_event_get_exceptions	(JanaEvent *self);

void	jana_event_set_summary		(JanaEvent *self, const gchar *summary);
void	jana_event_set_description	(JanaEvent *self,
					 const gchar *description);
void	jana_event_set_location		(JanaEvent *self,
					 const gchar *location);
void	jana_event_set_start		(JanaEvent *self, JanaTime *start);
void	jana_event_set_end		(JanaEvent *self, JanaTime *end);
void	jana_event_set_categories	(JanaEvent *self,
					 const gchar **categories);

void	jana_event_set_alarm		(JanaEvent *self, JanaTime *time);

void	jana_event_set_recurrence	(JanaEvent *self,
					 const JanaRecurrence *recurrence);

void	jana_event_set_exceptions	(JanaEvent *self, GList *exceptions);

#endif /* JANA_EVENT_H */

