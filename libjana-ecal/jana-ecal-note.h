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


#ifndef JANA_ECAL_NOTE_H
#define JANA_ECAL_NOTE_H

#include <glib-object.h>
#include <libecal/libecal.h>
#include <libjana/jana-component.h>
#include <libjana/jana-note.h>

#define JANA_ECAL_TYPE_NOTE		(jana_ecal_note_get_type ())
#define JANA_ECAL_NOTE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
					 JANA_ECAL_TYPE_NOTE, JanaEcalNote))
#define JANA_ECAL_NOTE_CLASS(vtable)	(G_TYPE_CHECK_CLASS_CAST ((vtable), \
					 JANA_ECAL_TYPE_NOTE, \
					 JanaEcalNoteClass))
#define JANA_ECAL_IS_NOTE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
					 JANA_ECAL_TYPE_NOTE))
#define JANA_ECAL_IS_NOTE_CLASS(vtable)(G_TYPE_CHECK_CLASS_TYPE ((vtable), \
					JANA_ECAL_TYPE_NOTE))
#define JANA_ECAL_NOTE_GET_CLASS(inst)	(G_TYPE_INSTANCE_GET_CLASS ((inst), \
					 JANA_ECAL_TYPE_NOTE, \
					 JanaEcalNoteClass))


typedef struct _JanaEcalNote JanaEcalNote;
typedef struct _JanaEcalNoteClass JanaEcalNoteClass;

/**
 * JanaEcalNote:
 *
 * The #JanaEcalNote struct contains only private data.
 */
struct _JanaEcalNote {
	GObject parent;
};

struct _JanaEcalNoteClass {
	GObjectClass parent;
};

GType jana_ecal_note_get_type (void);

JanaNote *jana_ecal_note_new ();
JanaNote *jana_ecal_note_new_from_ecalcomp (ECalComponent *note);

#endif /* JANA_ECAL_NOTE_H */
