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


#ifndef JANA_ECAL_EVENT_H
#define JANA_ECAL_EVENT_H

#include <glib-object.h>
#include <libecal/libecal.h>
#include <libjana/jana-component.h>
#include <libjana/jana-event.h>

#define JANA_ECAL_TYPE_EVENT		(jana_ecal_event_get_type ())
#define JANA_ECAL_EVENT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
					 JANA_ECAL_TYPE_EVENT, JanaEcalEvent))
#define JANA_ECAL_EVENT_CLASS(vtable)	(G_TYPE_CHECK_CLASS_CAST ((vtable), \
					 JANA_ECAL_TYPE_EVENT, \
					 JanaEcalEventClass))
#define JANA_ECAL_IS_EVENT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
					 JANA_ECAL_TYPE_EVENT))
#define JANA_ECAL_IS_EVENT_CLASS(vtable)(G_TYPE_CHECK_CLASS_TYPE ((vtable), \
					 JANA_ECAL_TYPE_EVENT))
#define JANA_ECAL_EVENT_GET_CLASS(inst)	(G_TYPE_INSTANCE_GET_CLASS ((inst), \
					 JANA_ECAL_TYPE_EVENT, \
					 JanaEcalEventClass))


typedef struct _JanaEcalEvent JanaEcalEvent;
typedef struct _JanaEcalEventClass JanaEcalEventClass;

/**
 * JanaEcalEvent:
 *
 * The #JanaEcalEvent struct contains only private data.
 */
struct _JanaEcalEvent {
	GObject parent;
};

struct _JanaEcalEventClass {
	GObjectClass parent;
};

GType jana_ecal_event_get_type (void);

JanaEvent *jana_ecal_event_new ();
JanaEvent *jana_ecal_event_new_from_ecalcomp (ECalComponent *event);

#endif /* JANA_ECAL_EVENT_H */

