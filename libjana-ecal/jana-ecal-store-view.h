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

 
#ifndef JANA_ECAL_STORE_VIEW_H
#define JANA_ECAL_STORE_VIEW_H

#include <glib-object.h>
#include <libecal/libecal.h>
#include <libjana/jana-store-view.h>
#include <libjana-ecal/jana-ecal-store.h>

#define JANA_ECAL_TYPE_STORE_VIEW	(jana_ecal_store_view_get_type ())
#define JANA_ECAL_STORE_VIEW(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
					 JANA_ECAL_TYPE_STORE_VIEW, \
					 JanaEcalStoreView))
#define JANA_ECAL_STORE_VIEW_CLASS(vtable)	(G_TYPE_CHECK_CLASS_CAST \
						 ((vtable), \
						 JANA_ECAL_TYPE_STORE_VIEW, \
						 JanaEcalStoreViewClass))
#define JANA_ECAL_IS_STORE_VIEW(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
					 JANA_ECAL_TYPE_STORE_VIEW))
#define JANA_ECAL_IS_STORE_VIEW_CLASS(vtable)	(G_TYPE_CHECK_CLASS_TYPE \
						 ((vtable), \
						 JANA_ECAL_TYPE_STORE_VIEW))
#define JANA_ECAL_STORE_VIEW_GET_CLASS(inst)	(G_TYPE_INSTANCE_GET_CLASS \
						 ((inst), \
						 JANA_ECAL_TYPE_STORE_VIEW, \
						 JanaEcalStoreViewClass))


typedef struct _JanaEcalStoreView JanaEcalStoreView;
typedef struct _JanaEcalStoreViewClass JanaEcalStoreViewClass;

/**
 * JanaEcalStoreView:
 *
 * The #JanaEcalStoreView struct contains only private data.
 */
struct _JanaEcalStoreView {
	GObject parent;
};

struct _JanaEcalStoreViewClass {
	GObjectClass parent;
};

GType jana_ecal_store_view_get_type (void);

JanaStoreView *jana_ecal_store_view_new (JanaEcalStore *store);

#endif /* JANA_ECAL_STORE_VIEW_H */

