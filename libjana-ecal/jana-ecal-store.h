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

 
#ifndef JANA_ECAL_STORE_H
#define JANA_ECAL_STORE_H

#include <glib-object.h>
#include <libecal/libecal.h>
#include <libjana/jana-store.h>
#include <libjana-ecal/jana-ecal-store.h>

#define JANA_ECAL_TYPE_STORE	(jana_ecal_store_get_type ())
#define JANA_ECAL_STORE(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
				 JANA_ECAL_TYPE_STORE, JanaEcalStore))
#define JANA_ECAL_STORE_CLASS(vtable)	(G_TYPE_CHECK_CLASS_CAST ((vtable), \
					 JANA_ECAL_TYPE_STORE, \
					 JanaEcalStoreClass))
#define JANA_ECAL_IS_STORE(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
				 JANA_ECAL_TYPE_STORE))
#define JANA_ECAL_IS_STORE_CLASS(vtable)(G_TYPE_CHECK_CLASS_TYPE ((vtable), \
					 JANA_ECAL_TYPE_STORE))
#define JANA_ECAL_STORE_GET_CLASS(inst)	(G_TYPE_INSTANCE_GET_CLASS ((inst), \
					 JANA_ECAL_TYPE_STORE, \
					 JanaEcalStoreClass))


typedef struct _JanaEcalStore JanaEcalStore;
typedef struct _JanaEcalStoreClass JanaEcalStoreClass;

/**
 * JanaEcalStore:
 *
 * The #JanaEcalStore struct contains only private data.
 */
struct _JanaEcalStore {
	GObject parent;
};

struct _JanaEcalStoreClass {
	GObjectClass parent;
};

GType jana_ecal_store_get_type (void);

JanaStore *jana_ecal_store_new 		(JanaComponentType type);
JanaStore *jana_ecal_store_new_from_uri	(const gchar *uri,
					 JanaComponentType type);
const gchar *jana_ecal_store_get_uri	(JanaEcalStore *store);

#endif /* JANA_ECAL_STORE_H */

