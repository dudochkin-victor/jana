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


#ifndef JANA_ECAL_TIME_H
#define JANA_ECAL_TIME_H

#include <glib-object.h>
#include <libical/ical.h>
#include <libjana/jana-time.h>
#include <libecal/libecal.h>

#define JANA_ECAL_TYPE_TIME		(jana_ecal_time_get_type ())
#define JANA_ECAL_TIME(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
					 JANA_ECAL_TYPE_TIME, JanaEcalTime))
#define JANA_ECAL_TIME_CLASS(vtable)	(G_TYPE_CHECK_CLASS_CAST ((vtable), \
					 JANA_ECAL_TYPE_TIME,JanaEcalTimeClass))
#define JANA_ECAL_IS_TIME(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
					 JANA_ECAL_TYPE_TIME))
#define JANA_ECAL_IS_TIME_CLASS(vtable)	(G_TYPE_CHECK_CLASS_TYPE ((vtable), \
					 JANA_ECAL_TYPE_TIME))
#define JANA_ECAL_TIME_GET_CLASS(inst)	(G_TYPE_INSTANCE_GET_CLASS ((inst), \
					 JANA_ECAL_TYPE_TIME,JanaEcalTimeClass))


typedef struct _JanaEcalTime JanaEcalTime;
typedef struct _JanaEcalTimeClass JanaEcalTimeClass;

/**
 * JanaEcalTime:
 *
 * The #JanaEcalTime struct contains only private data.
 */
struct _JanaEcalTime {
	GObject parent;
};

struct _JanaEcalTimeClass {
	GObjectClass parent;
};

GType jana_ecal_time_get_type (void);

JanaTime *jana_ecal_time_new ();
JanaTime *jana_ecal_time_new_from_icaltime (const icaltimetype *time);
JanaTime *jana_ecal_time_new_from_ecaltime (ECalComponentDateTime *dt);

void jana_ecal_time_set_location (JanaEcalTime *self, const gchar *location);

const gchar *jana_ecal_time_get_location (JanaEcalTime *self);
time_t jana_ecal_time_to_time_t (JanaEcalTime *self);

#endif /* JANA_ECAL_TIME_H */

