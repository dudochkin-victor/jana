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
 * SECTION:jana-ecal-task
 * @short_description: An implementation of #JanaTask using libecal
 *
 * #JanaEcalTask is an implementation of #JanaTask that provides a 
 * wrapper over #ECalComponent and its journal-related functions, using libecal.
 */

#define HANDLE_LIBICAL_MEMORY 1

#include "jana-ecal-task.h"
#include "jana-ecal-component.h"
#include "jana-ecal-time.h"
#include "jana-ecal-utils.h"
#include <libjana/jana-utils.h>

static void task_interface_init (gpointer g_iface, gpointer iface_data);
static void component_interface_init (gpointer g_iface, gpointer iface_data);

static JanaComponentType	component_get_component_type
							(JanaComponent *self);
static gboolean			component_is_fully_represented
							(JanaComponent *self);

static gchar *		task_get_summary	(JanaTask *self);
static gchar *		task_get_description	(JanaTask *self);
static gboolean		task_get_completed	(JanaTask *self);
static JanaTime	*	task_get_due_date	(JanaTask *self);
static gint		task_get_priority	(JanaTask *self);

static void		task_set_summary	(JanaTask *self,
						 const gchar *summary);
static void		task_set_description	(JanaTask *self,
						 const gchar *description);
static void		task_set_completed	(JanaTask *self,
						 gboolean completed);
static void		task_set_due_date	(JanaTask *self,
						 JanaTime *time);
static void		task_set_priority	(JanaTask *self,
						 gint priority);

G_DEFINE_TYPE_WITH_CODE (JanaEcalTask, 
			 jana_ecal_task, 
			 JANA_ECAL_TYPE_COMPONENT,
			 G_IMPLEMENT_INTERFACE (JANA_TYPE_COMPONENT,
						component_interface_init)
			 G_IMPLEMENT_INTERFACE (JANA_TYPE_TASK,
						task_interface_init));

static void
component_interface_init (gpointer g_iface, gpointer iface_data)
{
	JanaComponentInterface *iface = (JanaComponentInterface *)g_iface;
	
	iface->get_component_type = component_get_component_type;
	iface->is_fully_represented = component_is_fully_represented;
}

static void
task_interface_init (gpointer g_iface, gpointer iface_data)
{
	JanaTaskInterface *iface = (JanaTaskInterface *)g_iface;
	
	iface->get_summary = task_get_summary;
	iface->get_description = task_get_description;
	iface->get_completed = task_get_completed;
	iface->get_due_date = task_get_due_date;
	iface->get_priority = task_get_priority;

	iface->set_summary = task_set_summary;
	iface->set_description = task_set_description;
	iface->set_completed = task_set_completed;
	iface->set_due_date = task_set_due_date;
	iface->set_priority = task_set_priority;
}

static void
jana_ecal_task_class_init (JanaEcalTaskClass *klass)
{
	/* Not overriding any object methods or adding private data */
}

static void
jana_ecal_task_init (JanaEcalTask *self)
{
	/* No initialisation required */
}

/**
 * jana_ecal_task_new:
 *
 * Creates a new #JanaEcalTask.
 *
 * Returns: A new #JanaEcalTask, cast as a #JanaTask.
 */
JanaTask *
jana_ecal_task_new ()
{
	ECalComponent *comp = e_cal_component_new ();
	JanaTask *task;

	e_cal_component_set_icalcomponent (comp,
		icalcomponent_new (ICAL_VTODO_COMPONENT));
	
	task = JANA_TASK (g_object_new (JANA_ECAL_TYPE_TASK,
		"ecalcomp", comp, NULL));
	
	return task;
}

/**
 * jana_ecal_task_new_from_ecalcomp:
 * @event: An #ECalComponent
 *
 * Creates a new #JanaEcalTask based on the given #ECalComponent. The type of 
 * the given #ECalComponent must be %E_CAL_COMPONENT_JOURNAL. See 
 * e_cal_component_get_vtype().
 *
 * Returns: A new #JanaEcalTask that wraps the given #ECalComponent, cast as 
 * a #JanaTask.
 */
JanaTask *
jana_ecal_task_new_from_ecalcomp (ECalComponent *event)
{
	return JANA_TASK (g_object_new (JANA_ECAL_TYPE_TASK,
		"ecalcomp", event, NULL));
}

static JanaComponentType
component_get_component_type (JanaComponent *self)
{
	return JANA_COMPONENT_TASK;
}

static gboolean
component_is_fully_represented (JanaComponent *self)
{
	GSList *desc_list;
	ECalComponent *comp;

	gboolean result = TRUE;

	g_object_get (self, "ecalcomp", &comp, NULL);

	e_cal_component_get_description_list (comp, &desc_list);
	if (g_slist_length (desc_list) > 1) result = FALSE;
	e_cal_component_free_text_list (desc_list);
	
	g_object_unref (comp);

	return result;
}

static gchar *
task_get_summary (JanaTask *self)
{
	return jana_ecal_component_get_summary (JANA_ECAL_COMPONENT (self));
}

static gchar *
task_get_description (JanaTask *self)
{
	return jana_ecal_component_get_description (JANA_ECAL_COMPONENT (self));
}

static gboolean
task_get_completed (JanaTask *self)
{
	ECalComponent *comp;
	icalproperty_status status;

	g_object_get (self, "ecalcomp", &comp, NULL);
	e_cal_component_get_status (comp, &status);
	g_object_unref (comp);

	return (status == ICAL_STATUS_COMPLETED);
}

static JanaTime *
task_get_due_date (JanaTask *self)
{
	ECalComponent *comp;
	ECalComponentDateTime etime;
	JanaTime *time = NULL;

	g_object_get (self, "ecalcomp", &comp, NULL);
	e_cal_component_get_due (comp, &etime);
	if (etime.value)
	{
		time = jana_ecal_time_new_from_ecaltime (&etime);
		e_cal_component_free_datetime (&etime);
	}
	g_object_unref (comp);

	return time;
}

static gint
task_get_priority (JanaTask *self)
{
	ECalComponent *comp;
	gint *priority = NULL;
	gint res;

	g_object_get (self, "ecalcomp", &comp, NULL);
	e_cal_component_get_priority (comp, &priority);
	res = priority ? *priority: 0;
	if (priority)
		e_cal_component_free_priority (priority);
	g_object_unref (comp);

	return res;
}

static void
task_set_summary (JanaTask *self, const gchar *summary)
{
	jana_ecal_component_set_summary (JANA_ECAL_COMPONENT (self), summary);
}

static void
task_set_description (JanaTask *self, const gchar *description)
{
	jana_ecal_component_set_description (
		JANA_ECAL_COMPONENT (self), description);
}

static void
task_set_completed (JanaTask *self,gboolean completed)
{
	ECalComponent *comp;

	g_object_get (self, "ecalcomp", &comp, NULL);
	e_cal_component_set_status (comp, completed ? ICAL_STATUS_COMPLETED : ICAL_STATUS_NONE);
	g_object_unref (comp);
}

static void task_set_due_date (JanaTask *self, JanaTime *time)
{
	ECalComponent *comp;
	icaltimetype *itime;
	ECalComponentDateTime dt;

	g_object_get (time, "icaltime", &itime, NULL);
	dt.value = itime;

	if (icaltime_is_utc (*itime))
		dt.tzid = "UTC";
	else
		dt.tzid = (const char *)icaltimezone_get_tzid (
			(icaltimezone *)itime->zone);

	g_object_get (self, "ecalcomp", &comp, NULL);
	e_cal_component_set_due (comp, &dt);
	g_object_unref (comp);
}

static void task_set_priority (JanaTask *self, gint priority)
{
	ECalComponent *comp;

	g_object_get (self, "ecalcomp", &comp, NULL);
	e_cal_component_set_priority (comp, &priority);
	g_object_unref (comp);
}

