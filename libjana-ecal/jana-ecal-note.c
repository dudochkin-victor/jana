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
 * SECTION:jana-ecal-note
 * @short_description: An implementation of #JanaNote using libecal
 *
 * #JanaEcalNote is an implementation of #JanaNote that provides a 
 * wrapper over #ECalComponent and its journal-related functions, using libecal.
 */

#define HANDLE_LIBICAL_MEMORY 1

#include "jana-ecal-note.h"
#include "jana-ecal-component.h"
#include "jana-ecal-time.h"
#include "jana-ecal-utils.h"
#include <libjana/jana-utils.h>

static void note_interface_init (gpointer g_iface, gpointer iface_data);
static void component_interface_init (gpointer g_iface, gpointer iface_data);

static JanaComponentType	component_get_component_type
							(JanaComponent *self);
static gboolean			component_is_fully_represented
							(JanaComponent *self);

static gchar *	note_get_author		(JanaNote *self);
static gchar *	note_get_recipient	(JanaNote *self);
static gchar *	note_get_body		(JanaNote *self);

static JanaTime *	note_get_creation_time	(JanaNote *self);
static JanaTime *	note_get_modified_time	(JanaNote *self);

static void	note_set_author    (JanaNote *self, const gchar *author);
static void	note_set_recipient (JanaNote *self, const gchar *recipient);
static void	note_set_body      (JanaNote *self, const gchar *body);
static void	note_set_creation_time (JanaNote *note, JanaTime *time);

G_DEFINE_TYPE_WITH_CODE (JanaEcalNote, 
			 jana_ecal_note, 
			 JANA_ECAL_TYPE_COMPONENT,
			 G_IMPLEMENT_INTERFACE (JANA_TYPE_COMPONENT,
						component_interface_init)
			 G_IMPLEMENT_INTERFACE (JANA_TYPE_NOTE,
						note_interface_init));

static void
component_interface_init (gpointer g_iface, gpointer iface_data)
{
	JanaComponentInterface *iface = (JanaComponentInterface *)g_iface;
	
	iface->get_component_type = component_get_component_type;
	iface->is_fully_represented = component_is_fully_represented;
}

static void
note_interface_init (gpointer g_iface, gpointer iface_data)
{
	JanaNoteInterface *iface = (JanaNoteInterface *)g_iface;
	
	iface->get_author = note_get_author;
	iface->get_recipient = note_get_recipient;
	iface->get_body = note_get_body;
	
	iface->get_creation_time = note_get_creation_time;
	iface->get_modified_time = note_get_modified_time;
	
	iface->set_author = note_set_author;
	iface->set_recipient = note_set_recipient;
	iface->set_body = note_set_body;
	iface->set_creation_time = note_set_creation_time;
}

static void
jana_ecal_note_class_init (JanaEcalNoteClass *klass)
{
	/* Not overriding any object methods or adding private data */
}

static void
jana_ecal_note_init (JanaEcalNote *self)
{
	/* No initialisation required */
}

/**
 * jana_ecal_note_new:
 *
 * Creates a new #JanaEcalNote.
 *
 * Returns: A new #JanaEcalNote, cast as a #JanaNote.
 */
JanaNote *
jana_ecal_note_new ()
{
	ECalComponent *comp = e_cal_component_new ();
	JanaTime *now;
	JanaNote *note;

	e_cal_component_set_icalcomponent (comp,
		icalcomponent_new (ICAL_VJOURNAL_COMPONENT));
	
	note = JANA_NOTE (g_object_new (JANA_ECAL_TYPE_NOTE,
		"ecalcomp", comp, NULL));
	
	/* Set creation time */
	now = jana_ecal_utils_time_now_local ();
	note_set_creation_time (note, now);
	g_object_unref (now);

	return note;
}

/**
 * jana_ecal_note_new_from_ecalcomp:
 * @event: An #ECalComponent
 *
 * Creates a new #JanaEcalNote based on the given #ECalComponent. The type of 
 * the given #ECalComponent must be %E_CAL_COMPONENT_JOURNAL. See 
 * e_cal_component_get_vtype().
 *
 * Returns: A new #JanaEcalNote that wraps the given #ECalComponent, cast as 
 * a #JanaNote.
 */
JanaNote *
jana_ecal_note_new_from_ecalcomp (ECalComponent *event)
{
	return JANA_NOTE (g_object_new (JANA_ECAL_TYPE_NOTE,
		"ecalcomp", event, NULL));
}

static JanaComponentType
component_get_component_type (JanaComponent *self)
{
	return JANA_COMPONENT_NOTE;
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

static void
set_modification_time (JanaNote *self)
{
	JanaTime *now;
	ECalComponent *comp;
	icaltimetype *icaltime;

	g_object_get (self, "ecalcomp", &comp, NULL);
	
	/* Set modification time */
	now = jana_ecal_utils_time_now_local ();
	g_object_get (now, "icaltime", &icaltime, NULL);
	e_cal_component_set_last_modified (comp, icaltime);
	jana_ecal_component_set_end (JANA_ECAL_COMPONENT (self), now);
	
	g_object_unref (now);
	g_object_unref (comp);
}

static gchar *
note_get_author (JanaNote *self)
{
	return jana_ecal_component_get_summary (JANA_ECAL_COMPONENT (self));
}

static gchar *
note_get_recipient (JanaNote *self)
{
	return jana_ecal_component_get_location (JANA_ECAL_COMPONENT (self));
}

static gchar *
note_get_body (JanaNote *self)
{
	return jana_ecal_component_get_description (JANA_ECAL_COMPONENT (self));
}

static JanaTime *
note_get_creation_time (JanaNote *self)
{
	JanaTime *creation = jana_ecal_component_get_start (
		JANA_ECAL_COMPONENT (self));

	/* Event was probably created by another app and doesn't have dtstart 
	 * set.
	 * FIXME: This will cause problems with store views...
	 */
	if (!creation) {
		icaltimetype *itime;
		ECalComponent *comp;

		g_object_get (self, "ecalcomp", &comp, NULL);
		e_cal_component_get_created (comp, &itime);
		creation = jana_ecal_time_new_from_icaltime (itime);
		if (itime) e_cal_component_free_icaltimetype (itime);
		g_object_unref (comp);
	}
	
	return creation;
}

static JanaTime *
note_get_modified_time (JanaNote *self)
{
	JanaTime *modified = jana_ecal_component_get_end (
		JANA_ECAL_COMPONENT (self));

	/* Event was probably created by another app and doesn't have dtend 
	 * set.
	 * FIXME: This will cause problems with store views...
	 */
	if (!modified) {
		icaltimetype *itime;
		ECalComponent *comp;

		g_object_get (self, "ecalcomp", &comp, NULL);
		e_cal_component_get_last_modified (comp, &itime);
		modified = jana_ecal_time_new_from_icaltime (itime);
		if (itime) e_cal_component_free_icaltimetype (itime);
		g_object_unref (comp);
	}
	
	return modified;
}

static void
note_set_author (JanaNote *self, const gchar *author)
{
	jana_ecal_component_set_summary (JANA_ECAL_COMPONENT (self), author);
	set_modification_time (self);
}

static void
note_set_recipient (JanaNote *self, const gchar *recipient)
{
	jana_ecal_component_set_location (
		JANA_ECAL_COMPONENT (self), recipient);
	set_modification_time (self);
}

static void
note_set_body (JanaNote *self, const gchar *body)
{
	jana_ecal_component_set_description (JANA_ECAL_COMPONENT (self), body);
	set_modification_time (self);
}

static void
note_set_creation_time (JanaNote *note, JanaTime *time)
{
	icaltimetype *icaltime;
	ECalComponent *comp;
	
	if (JANA_ECAL_IS_TIME (time)) {
		time = g_object_ref (time);
	} else {
		time = jana_utils_time_copy (
			time, jana_ecal_time_new ());
	}
	
	g_object_get (note, "ecalcomp", &comp, NULL);
	g_object_get (time, "icaltime", &icaltime, NULL);
	
	e_cal_component_set_created (comp, icaltime);
	jana_ecal_component_set_start (JANA_ECAL_COMPONENT (note), time);
	
	g_object_unref (time);
	g_object_unref (comp);
}
