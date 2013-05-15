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
 * SECTION:jana-ecal-event
 * @short_description: An implementation of #JanaEvent using libecal
 *
 * #JanaEcalEvent is an implementation of #JanaEvent that provides a 
 * wrapper over #ECalComponent and its event-related functions, using libecal.
 */

#define HANDLE_LIBICAL_MEMORY 1

#include "jana-ecal-component.h"
#include "jana-ecal-event.h"
#include "jana-ecal-time.h"
#include <libjana/jana-time.h>
#include <libjana/jana-component.h>
#include <libjana/jana-event.h>
#include <libjana/jana-utils.h>
#include <string.h>

static void event_interface_init (gpointer g_iface, gpointer iface_data);
static void component_interface_init (gpointer g_iface, gpointer iface_data);

static JanaComponentType	component_get_component_type
							(JanaComponent *self);
static gboolean			component_is_fully_represented
							(JanaComponent *self);

static gchar * 	event_get_summary	(JanaEvent *self);
static gchar * 	event_get_description	(JanaEvent *self);
static gchar * 	event_get_location	(JanaEvent *self);
static JanaTime * 	event_get_start	(JanaEvent *self);
static JanaTime *	event_get_end	(JanaEvent *self);

static gboolean	event_supports_alarm	(JanaEvent *self);
static gboolean	event_has_alarm		(JanaEvent *self);
static JanaTime *event_get_alarm_time	(JanaEvent *self);

static gboolean		event_supports_recurrence	(JanaEvent *self);
static gboolean		event_has_recurrence		(JanaEvent *self);
static JanaRecurrence *	event_get_recurrence		(JanaEvent *self);

static gboolean	event_supports_exceptions	(JanaEvent *self);
static gboolean	event_has_exceptions	(JanaEvent *self);
static GList *	event_get_exceptions	(JanaEvent *self);

static void	event_set_summary	(JanaEvent *self,
					 const gchar *summary);
static void	event_set_description	(JanaEvent *self,
					 const gchar *description);
static void	event_set_location	(JanaEvent *self,
					 const gchar *location);
static void	event_set_start		(JanaEvent *self, JanaTime *start);
static void	event_set_end		(JanaEvent *self, JanaTime *end);

static void	event_set_alarm		(JanaEvent *self, JanaTime *time);

static void	event_set_recurrence	(JanaEvent *self,
					 const JanaRecurrence *recurrence);

static void	event_set_exceptions	(JanaEvent *self, GList *exceptions);

G_DEFINE_TYPE_WITH_CODE (JanaEcalEvent, 
			 jana_ecal_event, 
			 JANA_ECAL_TYPE_COMPONENT,
			 G_IMPLEMENT_INTERFACE (JANA_TYPE_COMPONENT,
						component_interface_init)
			 G_IMPLEMENT_INTERFACE (JANA_TYPE_EVENT,
						event_interface_init));

#define EVENT_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
			  JANA_ECAL_TYPE_EVENT, JanaEcalEventPrivate))


typedef struct _JanaEcalEventPrivate JanaEcalEventPrivate;

struct _JanaEcalEventPrivate
{
};

static void
jana_ecal_event_get_property (GObject *object, guint property_id,
			      GValue *value, GParamSpec *pspec)
{
	/*JanaEcalEventPrivate *priv = EVENT_PRIVATE (object);*/

	switch (property_id) {
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_ecal_event_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
	/*JanaEcalEventPrivate *priv = EVENT_PRIVATE (object);*/

	switch (property_id) {
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_ecal_event_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (jana_ecal_event_parent_class)->dispose)
		G_OBJECT_CLASS (jana_ecal_event_parent_class)->dispose (object);
}

static void
jana_ecal_event_finalize (GObject *object)
{
	G_OBJECT_CLASS (jana_ecal_event_parent_class)->finalize (object);
}

static void
component_interface_init (gpointer g_iface, gpointer iface_data)
{
	JanaComponentInterface *iface = (JanaComponentInterface *)g_iface;
	
	iface->get_component_type = component_get_component_type;
	iface->is_fully_represented = component_is_fully_represented;
}

static void
event_interface_init (gpointer g_iface, gpointer iface_data)
{
	JanaEventInterface *iface = (JanaEventInterface *)g_iface;
	
	iface->get_summary = event_get_summary;
	iface->get_description = event_get_description;
	iface->get_location = event_get_location;
	iface->get_start = event_get_start;
	iface->get_end = event_get_end;
	
	iface->supports_alarm = event_supports_alarm;
	iface->has_alarm = event_has_alarm;
	iface->get_alarm_time = event_get_alarm_time;
	
	iface->supports_recurrence = event_supports_recurrence;
	iface->has_recurrence = event_has_recurrence;
	iface->get_recurrence = event_get_recurrence;
	
	iface->supports_exceptions = event_supports_exceptions;
	iface->has_exceptions = event_has_exceptions;
	iface->get_exceptions = event_get_exceptions;
	
	iface->set_summary = event_set_summary;
	iface->set_description = event_set_description;
	iface->set_location = event_set_location;
	iface->set_start = event_set_start;
	iface->set_end = event_set_end;
	iface->set_alarm = event_set_alarm;
	iface->set_recurrence = event_set_recurrence;
	iface->set_exceptions = event_set_exceptions;
}

static void
jana_ecal_event_class_init (JanaEcalEventClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	/* Don't add a zero-size private, causes memory corruption issues */
	/*g_type_class_add_private (klass, sizeof (JanaEcalEventPrivate));*/

	object_class->get_property = jana_ecal_event_get_property;
	object_class->set_property = jana_ecal_event_set_property;
	object_class->dispose = jana_ecal_event_dispose;
	object_class->finalize = jana_ecal_event_finalize;
}

static void
jana_ecal_event_init (JanaEcalEvent *self)
{
}

/**
 * jana_ecal_event_new:
 *
 * Creates a new #JanaEcalEvent.
 *
 * Returns: A new #JanaEcalEvent.
 */
JanaEvent *
jana_ecal_event_new ()
{
	ECalComponent *comp = e_cal_component_new ();
	e_cal_component_set_icalcomponent (comp,
		icalcomponent_new (ICAL_VEVENT_COMPONENT));

	return JANA_EVENT (g_object_new (JANA_ECAL_TYPE_EVENT,
		"ecalcomp", comp, NULL));
}

/**
 * jana_ecal_event_new_from_ecalcomp:
 * @event: An #ECalComponent
 *
 * Creates a new #JanaEcalEvent based on the given #ECalComponent. The type of 
 * the given #ECalComponent must be %E_CAL_COMPONENT_EVENT. See 
 * e_cal_component_get_vtype().
 *
 * Returns: A new #JanaEcalEvent that wraps the given #ECalComponent, cast as 
 * a #JanaEvent.
 */
JanaEvent *
jana_ecal_event_new_from_ecalcomp (ECalComponent *event)
{
	return JANA_EVENT (g_object_new (JANA_ECAL_TYPE_EVENT,
		"ecalcomp", event, NULL));
}

static JanaComponentType
component_get_component_type (JanaComponent *self)
{
	return JANA_COMPONENT_EVENT;
}

static gboolean
component_is_fully_represented (JanaComponent *self)
{
	gboolean result = TRUE;
	ECalComponent *comp;
	g_object_get (self, "ecalcomp", &comp, NULL);

	/* TODO: We actually support slightly less than ECal simple recurrence,
	 * filter this down some more.
	 */
	if (e_cal_component_has_recurrences (comp) &&
	    (!e_cal_component_has_simple_recurrence (comp)))
		result = FALSE;
	
	if (e_cal_component_has_rdates (comp))
		result = FALSE;

	/* TODO: Implement exception dates */
	if (e_cal_component_has_exdates (comp))
		result = FALSE;

	if (e_cal_component_has_exrules (comp))
		result = FALSE;
	
	if (e_cal_component_has_organizer (comp))
		result = FALSE;
	
	if (e_cal_component_has_attendees (comp))
		result = FALSE;
	
	if (e_cal_component_has_attachments (comp))
		result = FALSE;
	
	/* TODO: Implement alarms */
	if (e_cal_component_has_alarms (comp))
		result = FALSE;

	/* TODO: There are probably more parts of ECalComponent/icalcomponent
	 *       that aren't supported. Need to either add more support at the
	 *       JanaEvent level, or do more in-depth checking here. This will
	 *       catch the most important aspects at least.
	 */
	
	g_object_unref (comp);

	return result;
}

static gchar *
event_get_summary (JanaEvent *self)
{
	return jana_ecal_component_get_summary (JANA_ECAL_COMPONENT (self));
}

static gchar *
event_get_description (JanaEvent *self)
{
	return jana_ecal_component_get_description (JANA_ECAL_COMPONENT (self));
}

static gchar *
event_get_location (JanaEvent *self)
{
	return jana_ecal_component_get_location (JANA_ECAL_COMPONENT (self));
}

static JanaTime *
event_get_start (JanaEvent *self)
{
	return jana_ecal_component_get_start (JANA_ECAL_COMPONENT (self));
}

static JanaTime *
event_get_end (JanaEvent *self)
{
	return jana_ecal_component_get_end (JANA_ECAL_COMPONENT (self));
}

static gboolean
event_supports_alarm (JanaEvent *self)
{
	/* TODO: Implement alarms */
	return FALSE;
}

static gboolean
event_has_alarm (JanaEvent *self)
{
	/* TODO: Implement alarms */
	return FALSE;
}

static JanaTime *
event_get_alarm_time (JanaEvent *self)
{
	/* TODO: Implement alarms */
	return NULL;
}

static gboolean
event_supports_recurrence (JanaEvent *self)
{
	return TRUE;
}

static gboolean
event_has_recurrence (JanaEvent *self)
{
	ECalComponent *comp;
	gboolean result;
	
	g_object_get (self, "ecalcomp", &comp, NULL);
	
	result = e_cal_component_has_recurrences (comp);
	g_object_unref (comp);
	
	return result;
}

/* Following function and define copied from libecal */
/* Counts the elements in the by_xxx fields of an icalrecurrencetype */
static int
count_by_xxx (short *field, int max_elements)
{
	int i;

	for (i = 0; i < max_elements; i++)
		if (field[i] == ICAL_RECURRENCE_ARRAY_MAX)
			break;

	return i;
}
#define N_HAS_BY(field) (count_by_xxx (field, sizeof (field)/sizeof (field[0])))

static JanaRecurrence *
event_get_recurrence (JanaEvent *self)
{
	gint i;
	GSList *rrule_list;
	ECalComponent *comp;
	JanaRecurrence *recur;
	struct icalrecurrencetype *r;
	
	if (!event_has_recurrence (self)) return NULL;

	/* Fill in our own recurrence struct */
	g_object_get (self, "ecalcomp", &comp, NULL);
	recur = jana_recurrence_new ();
	e_cal_component_get_rrule_list (comp, &rrule_list);
	r = rrule_list->data;

	recur->interval = r->interval;
	switch (r->freq) {
	    case ICAL_DAILY_RECURRENCE :
		recur->type = JANA_RECURRENCE_DAILY;
		break;
	    case ICAL_WEEKLY_RECURRENCE :
		recur->type = JANA_RECURRENCE_WEEKLY;
		for (i = 0; (i < 8) &&
		     (r->by_day[i] != ICAL_RECURRENCE_ARRAY_MAX); i++) {
			switch (icalrecurrencetype_day_day_of_week (
				r->by_day[i])) {
			    case ICAL_MONDAY_WEEKDAY :
				recur->week_days[0] = TRUE;
				break;
			    case ICAL_TUESDAY_WEEKDAY :
				recur->week_days[1] = TRUE;
				break;
			    case ICAL_WEDNESDAY_WEEKDAY :
				recur->week_days[2] = TRUE;
				break;
			    case ICAL_THURSDAY_WEEKDAY :
				recur->week_days[3] = TRUE;
				break;
			    case ICAL_FRIDAY_WEEKDAY :
				recur->week_days[4] = TRUE;
				break;
			    case ICAL_SATURDAY_WEEKDAY :
				recur->week_days[5] = TRUE;
				break;
			    case ICAL_SUNDAY_WEEKDAY :
				recur->week_days[6] = TRUE;
				break;
			    default :
				break;
			}
		}
		break;
	    case ICAL_MONTHLY_RECURRENCE :
		recur->type = JANA_RECURRENCE_MONTHLY;
		if (N_HAS_BY (r->by_month_day)) {
			/* We don't really support this field at all, we use
			 * the date to determine which day it repeats on.
			 */
			recur->by_date = TRUE;
		}
		break;
	    case ICAL_YEARLY_RECURRENCE :
		recur->type = JANA_RECURRENCE_YEARLY;
		break;
	    default :
		break;
	}
	
	/* TODO: Also check count and calculated until time from it? */
	if (!icaltime_is_null_time (r->until)) {
		recur->end = jana_ecal_time_new_from_icaltime (&r->until);
	}
	
	e_cal_component_free_recur_list (rrule_list);
	g_object_unref (comp);
	
	return recur;
}

static gboolean
event_supports_exceptions (JanaEvent *self)
{
	/* TODO: Implement exceptions */
	return FALSE;
}

static gboolean
event_has_exceptions (JanaEvent *self)
{
	/* TODO: Implement exceptions */
	return FALSE;
}

static GList *
event_get_exceptions (JanaEvent *self)
{
	/* TODO: Implement exceptions */
	return NULL;
}

static void
event_set_summary (JanaEvent *self, const gchar *summary)
{
	jana_ecal_component_set_summary (JANA_ECAL_COMPONENT (self), summary);
}

static void
event_set_description (JanaEvent *self, const gchar *description)
{
	jana_ecal_component_set_description (
		JANA_ECAL_COMPONENT (self), description);
}

static void
event_set_location (JanaEvent *self, const gchar *location)
{
	jana_ecal_component_set_location (JANA_ECAL_COMPONENT (self), location);
}

static void
event_set_start (JanaEvent *self, JanaTime *start)
{
	jana_ecal_component_set_start (JANA_ECAL_COMPONENT (self), start);
}

static void
event_set_end (JanaEvent *self, JanaTime *end)
{
	jana_ecal_component_set_end (JANA_ECAL_COMPONENT (self), end);
}

static void
event_set_alarm (JanaEvent *self, JanaTime *time)
{
	/* TODO: Implement alarms */
}

static void
event_set_recurrence (JanaEvent *self, const JanaRecurrence *recur)
{
	gint i, c;
	ECalComponent *comp;
	GSList *rrule_list;
	struct icalrecurrencetype r;

	g_object_get (self, "ecalcomp", &comp, NULL);
	
	if (!recur) {
		/* Remove recurrence */
		e_cal_component_set_rrule_list (comp, NULL);
		g_object_unref (comp);
		return;
	}
	
	/* Set week start to Monday */
	r.week_start = ICAL_MONDAY_WEEKDAY;
	
	/* Set interval */
	r.interval = recur->interval;
	
	/* Clear by_* fields */
	for (i = 0; i < ICAL_BY_SECOND_SIZE; i++)
		r.by_second[i] = ICAL_RECURRENCE_ARRAY_MAX;
	for (i = 0; i < ICAL_BY_MINUTE_SIZE; i++)
		r.by_minute[i] = ICAL_RECURRENCE_ARRAY_MAX;
	for (i = 0; i < ICAL_BY_HOUR_SIZE; i++)
		r.by_hour[i] = ICAL_RECURRENCE_ARRAY_MAX;
	for (i = 0; i < ICAL_BY_DAY_SIZE; i++)
		r.by_day[i] = ICAL_RECURRENCE_ARRAY_MAX;
	for (i = 0; i < ICAL_BY_MONTHDAY_SIZE; i++)
		r.by_month_day[i] = ICAL_RECURRENCE_ARRAY_MAX;
	for (i = 0; i < ICAL_BY_YEARDAY_SIZE; i++)
		r.by_year_day[i] = ICAL_RECURRENCE_ARRAY_MAX;
	for (i = 0; i < ICAL_BY_WEEKNO_SIZE; i++)
		r.by_week_no[i] = ICAL_RECURRENCE_ARRAY_MAX;
	for (i = 0; i < ICAL_BY_MONTH_SIZE; i++)
		r.by_month[i] = ICAL_RECURRENCE_ARRAY_MAX;
	for (i = 0; i < ICAL_BY_SETPOS_SIZE; i++)
		r.by_set_pos[i] = ICAL_RECURRENCE_ARRAY_MAX;
	
	/* Set type and by_* fields, where appropriate */
	switch (recur->type) {
	    case JANA_RECURRENCE_DAILY :
		r.freq = ICAL_DAILY_RECURRENCE;
		break;
	    case JANA_RECURRENCE_WEEKLY :
		r.freq = ICAL_WEEKLY_RECURRENCE;
		for (i = 0, c = 0; i < 7; i++) {
			if (recur->week_days[i]) {
				switch (i) {
				    case 0 :
					/* Monday */
					r.by_day[c] = ICAL_MONDAY_WEEKDAY;
					break;
				    case 1 :
					/* Tuesday */
					r.by_day[c] = ICAL_TUESDAY_WEEKDAY;
					break;
				    case 2 :
					/* Wednesday */
					r.by_day[c] = ICAL_WEDNESDAY_WEEKDAY;
					break;
				    case 3 :
					/* Thursday */
					r.by_day[c] = ICAL_THURSDAY_WEEKDAY;
					break;
				    case 4 :
					/* Friday */
					r.by_day[c] = ICAL_FRIDAY_WEEKDAY;
					break;
				    case 5 :
					/* Saturday */
					r.by_day[c] = ICAL_SATURDAY_WEEKDAY;
					break;
				    case 6 :
					/* Sunday */
					r.by_day[c] = ICAL_SUNDAY_WEEKDAY;
					break;
				}
				c++;
			}
		}
		break;
	    case JANA_RECURRENCE_MONTHLY :
		r.freq = ICAL_MONTHLY_RECURRENCE;
		if (recur->by_date) {
			JanaTime *start = jana_event_get_start (self);
			r.by_month_day[0] = jana_time_get_day (start);
			g_object_unref (start);
		}
		break;
	    case JANA_RECURRENCE_YEARLY :
		r.freq = ICAL_YEARLY_RECURRENCE;
		break;
	}
	
	/* Set ending time */
	r.count = 0;
	if (recur->end) {
		icaltimetype *time;
		if (JANA_ECAL_IS_TIME (recur->end)) {
			g_object_get (G_OBJECT (recur->end),
				"icaltime", &time, NULL);
			r.until = *time;
		} else {
			JanaTime *ecaltime = jana_ecal_time_new ();
			jana_utils_time_copy (recur->end, ecaltime);
			g_object_get (G_OBJECT (recur->end),
				"icaltime", &time, NULL);
			r.until = *time;
			g_object_unref (ecaltime);
		}
	} else {
		r.until = icaltime_null_time ();
	}
	
	/* Set the recurrence */
	rrule_list = g_slist_append (NULL, &r);
	e_cal_component_set_rrule_list (comp, rrule_list);
	g_slist_free (rrule_list);
	
	g_object_unref (comp);
}

static void
event_set_exceptions (JanaEvent *self, GList *exceptions)
{
	/* TODO: Implement exceptions */
}

