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
 * SECTION:jana-note
 * @short_description: A note/memo/journal component interface
 * @see_also: #JanaComponent
 *
 * #JanaNote is the interface for components that store miscellaneus textual 
 * information.
 */

#include "jana-note.h"

static void
jana_note_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

GType
jana_note_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (JanaNoteInterface),
			jana_note_base_init,   /* base_init */
			NULL,

		};
		type = g_type_register_static (G_TYPE_INTERFACE,
			"JanaNote", &info, 0);
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}
	return type;
}

/**
 * jana_note_get_author:
 * @note: A #JanaNote
 *
 * Retrieves the author of the note.
 *
 * Returns: A newly allocated string containing the author of the note,
 * or %NULL.
 */
gchar *
jana_note_get_author (JanaNote *note)
{
	return JANA_NOTE_GET_INTERFACE (note)->get_author (note);
}

/**
 * jana_note_get_recipient:
 * @note: A #JanaNote
 *
 * Retrieves the recipient of the note.
 *
 * Returns: A newly allocated string containing the recipient of the note,
 * or %NULL.
 */
gchar *
jana_note_get_recipient (JanaNote *note)
{
	return JANA_NOTE_GET_INTERFACE (note)->get_recipient (note);
}

/**
 * jana_note_get_body:
 * @note: A #JanaNote
 *
 * Retrieves the note body.
 *
 * Returns: A newly allocated string containing the body of the note, or %NULL.
 */
gchar *
jana_note_get_body (JanaNote *note)
{
	return JANA_NOTE_GET_INTERFACE (note)->get_body (note);
}

/**
 * jana_note_get_creation_time:
 * @note: A #JanaNote
 *
 * Retrieves the creation time of the note.
 *
 * Returns: A #JanaTime filled with the creation time of the note.
 */
JanaTime *
jana_note_get_creation_time (JanaNote *note)
{
	return JANA_NOTE_GET_INTERFACE (note)->get_creation_time (note);
}

/**
 * jana_note_get_modified_time:
 * @note: A #JanaNote
 *
 * Retrieves the time of the last modification to this note.
 *
 * Returns: A #JanaTime filled with the last modification time of the note.
 */
JanaTime *
jana_note_get_modified_time (JanaNote *note)
{
	return JANA_NOTE_GET_INTERFACE (note)->get_modified_time (note);
}

/**
 * jana_note_set_author:
 * @note: A #JanaNote
 * @author: The author of the note, or %NULL
 *
 * Sets the author of the note.
 */
void
jana_note_set_author (JanaNote *note, const gchar *author)
{
	JANA_NOTE_GET_INTERFACE (note)->set_author (note, author);
}

/**
 * jana_note_set_recipient:
 * @note: A #JanaNote
 * @recipient: The recipient of the note, or %NULL
 *
 * Sets the recipient of the note.
 */
void
jana_note_set_recipient (JanaNote *note, const gchar *recipient)
{
	JANA_NOTE_GET_INTERFACE (note)->set_recipient (note, recipient);
}

/**
 * jana_note_set_body:
 * @note: A #JanaNote
 * @body: The note body
 *
 * Sets the body of the note.
 */
void
jana_note_set_body (JanaNote *note, const gchar *body)
{
	JANA_NOTE_GET_INTERFACE (note)->set_body (note, body);
}

/**
 * jana_note_set_creation_time:
 * @note: A #JanaNote
 * @time: A #JanaTime
 *
 * Sets the creation time of the note. The creation time is automatically set 
 * to the current local time when created, but this allows it to be overridden. 
 * This can be useful to back-date notes, for example, or to set a particular 
 * timezone on a note.
 */
void
jana_note_set_creation_time (JanaNote *note, JanaTime *time)
{
	JANA_NOTE_GET_INTERFACE (note)->set_creation_time (note, time);
}
