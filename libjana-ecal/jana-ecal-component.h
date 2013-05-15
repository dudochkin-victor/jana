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


#ifndef JANA_ECAL_COMPONENT_H
#define JANA_ECAL_COMPONENT_H

#include <glib-object.h>
#include <libecal/libecal.h>
#include <libjana/jana-time.h>
#include <libjana/jana-component.h>

#define JANA_ECAL_TYPE_COMPONENT	(jana_ecal_component_get_type ())
#define JANA_ECAL_COMPONENT(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
					 JANA_ECAL_TYPE_COMPONENT, \
					 JanaEcalComponent))
#define JANA_ECAL_COMPONENT_CLASS(vtable)	(G_TYPE_CHECK_CLASS_CAST \
						 ((vtable), \
						 JANA_ECAL_TYPE_COMPONENT, \
						 JanaEcalComponentClass))
#define JANA_ECAL_IS_COMPONENT(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
					 JANA_ECAL_TYPE_COMPONENT))
#define JANA_ECAL_IS_COMPONENT_CLASS(vtable)	(G_TYPE_CHECK_CLASS_TYPE \
						 ((vtable), \
						 JANA_ECAL_TYPE_COMPONENT))
#define JANA_ECAL_COMPONENT_GET_CLASS(inst)	(G_TYPE_INSTANCE_GET_CLASS \
						 ((inst), \
						 JANA_ECAL_TYPE_COMPONENT, \
						 JanaEcalComponentClass))


typedef struct _JanaEcalComponent JanaEcalComponent;
typedef struct _JanaEcalComponentClass JanaEcalComponentClass;

/**
 * JanaEcalComponent:
 *
 * The #JanaEcalComponent struct contains only private data.
 */
struct _JanaEcalComponent {
	GObject parent;
};

struct _JanaEcalComponentClass {
	GObjectClass parent;
};

GType jana_ecal_component_get_type (void);

JanaComponent *jana_ecal_component_new_from_ecalcomp (ECalComponent *component);

gchar * jana_ecal_component_get_summary (JanaEcalComponent *self);
gchar * jana_ecal_component_get_description (JanaEcalComponent *self);
gchar * jana_ecal_component_get_location (JanaEcalComponent *self);
gchar * jana_ecal_component_get_recurrence_id (JanaEcalComponent *self);

JanaTime * jana_ecal_component_get_start (JanaEcalComponent *self);
JanaTime * jana_ecal_component_get_end (JanaEcalComponent *self);

void jana_ecal_component_set_summary (JanaEcalComponent *self,
				      const gchar *summary);

void jana_ecal_component_set_description (JanaEcalComponent *self,
					  const gchar *description);

void jana_ecal_component_set_location (JanaEcalComponent *self,
				       const gchar *location);

void jana_ecal_component_set_start (JanaEcalComponent *self, JanaTime *start);
void jana_ecal_component_set_end (JanaEcalComponent *self, JanaTime *end);

#endif /* JANA_ECAL_COMPONENT_H */
