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
 * SECTION:jana-ecal
 * @short_description: An evolution-data-server based implementation of libjana
 */

#ifndef JANA_ECAL_H
#define JANA_ECAL_H

#ifndef HANDLE_LIBICAL_MEMORY
#define HANDLE_LIBICAL_MEMORY 1
#define JANA_HANDLE_LIBICAL_MEMORY 1
#endif

#include <libjana-ecal/jana-ecal-component.h>
#include <libjana-ecal/jana-ecal-event.h>
#include <libjana-ecal/jana-ecal-store.h>
#include <libjana-ecal/jana-ecal-store-view.h>
#include <libjana-ecal/jana-ecal-time.h>
#include <libjana-ecal/jana-ecal-utils.h>
#include <libjana-ecal/jana-ecal-note.h>
#include <libjana-ecal/jana-ecal-task.h>

#ifdef JANA_HANDLE_LIBICAL_MEMORY
#undef HANDLE_LIBICAL_MEMORY
#endif

#endif

