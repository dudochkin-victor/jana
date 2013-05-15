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


#ifndef JANA_NOTE_H
#define JANA_NOTE_H

#include <glib-object.h>
#include <libjana/jana-time.h>

#define JANA_TYPE_NOTE		(jana_note_get_type ())
#define JANA_NOTE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj),\
				 JANA_TYPE_NOTE, JanaNote))
#define JANA_IS_NOTE(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
				 JANA_TYPE_NOTE))
#define JANA_NOTE_GET_INTERFACE(inst)	(G_TYPE_INSTANCE_GET_INTERFACE ((inst),\
					 JANA_TYPE_NOTE, JanaNoteInterface))

/**
 * JanaNote:
 *
 * The #JanaNote struct contains only private data.
 */
typedef struct _JanaNote JanaNote; /* Dummy object */
typedef struct _JanaNoteInterface JanaNoteInterface;

struct _JanaNoteInterface {
	GTypeInterface parent;
	
	/* Getter functions */
	gchar *		(*get_author)		(JanaNote *self);
	gchar *		(*get_recipient)	(JanaNote *self);
	gchar *		(*get_body)		(JanaNote *self);
	JanaTime *	(*get_creation_time)	(JanaNote *self);
	JanaTime *	(*get_modified_time)	(JanaNote *self);
	
	/* Setter functions */
	void	(*set_author)	(JanaNote *self, const gchar *author);
	void	(*set_recipient)(JanaNote *self, const gchar *recipient);
	void	(*set_body)	(JanaNote *self, const gchar *body);
	void	(*set_creation_time)	(JanaNote *self, JanaTime *time);
};

GType jana_note_get_type (void);

gchar *jana_note_get_author (JanaNote *note);

gchar *jana_note_get_recipient (JanaNote *note);

gchar *jana_note_get_body (JanaNote *note);

JanaTime *jana_note_get_creation_time (JanaNote *note);

JanaTime *jana_note_get_modified_time (JanaNote *note);

void jana_note_set_author (JanaNote *note, const gchar *author);

void jana_note_set_recipient (JanaNote *note, const gchar *recipient);

void jana_note_set_body (JanaNote *note, const gchar *body);

void jana_note_set_creation_time (JanaNote *note, JanaTime *time);

#endif /* JANA_NOTE_H */

