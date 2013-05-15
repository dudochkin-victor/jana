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


#ifndef JANA_TIME_H
#define JANA_TIME_H

#include <glib-object.h>

#define JANA_TYPE_TIME			(jana_time_get_type ())
#define JANA_TIME(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj),\
					 JANA_TYPE_TIME, JanaTime))
#define JANA_IS_TIME(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
					 JANA_TYPE_TIME))
#define JANA_TIME_GET_INTERFACE(inst)	(G_TYPE_INSTANCE_GET_INTERFACE ((inst),\
					 JANA_TYPE_TIME, JanaTimeInterface))

#define JANA_TYPE_DURATION		(jana_duration_get_type ())

typedef struct _JanaDuration JanaDuration;

/**
 * JanaTime:
 *
 * The #JanaTime struct contains only private data.
 */
typedef struct _JanaTime JanaTime; /* dummy object */
typedef struct _JanaTimeInterface JanaTimeInterface;

struct _JanaTimeInterface {
	GTypeInterface parent;

	guint8  (*get_seconds)	(JanaTime *self);
	guint8  (*get_minutes)	(JanaTime *self);
	guint8  (*get_hours)	(JanaTime *self);

	guint8  (*get_day)	(JanaTime *self);
	guint8  (*get_month)	(JanaTime *self);
	guint16 (*get_year)	(JanaTime *self);

	gboolean (*get_isdate)	(JanaTime *self);
	gboolean (*get_daylight)(JanaTime *self);

	gchar * (*get_tzname)	(JanaTime *self);
	glong   (*get_offset)	(JanaTime *self);

	void (*set_seconds)	(JanaTime *self, gint seconds);
	void (*set_minutes)	(JanaTime *self, gint minutes);
	void (*set_hours)	(JanaTime *self, gint hours);

	void (*set_day)		(JanaTime *self, gint day);
	void (*set_month)	(JanaTime *self, gint month);
	void (*set_year)	(JanaTime *self, gint year);

	void (*set_isdate)	(JanaTime *self, gboolean isdate);

	void (*set_tzname)	(JanaTime *self, const gchar *tzname);
	void (*set_offset)	(JanaTime *self, glong offset);
	
	JanaTime * (*duplicate)	(JanaTime *self);
};

/**
 * JanaDuration:
 * @start: The start of the duration
 * @end: The end of the duration
 *
 * This struct specifies a time period.
 **/
struct _JanaDuration {
	JanaTime *start;
	JanaTime *end;
};

GType jana_time_get_type (void);
GType jana_duration_get_type (void);

guint8 jana_time_get_seconds 	(JanaTime *self);
guint8 jana_time_get_minutes 	(JanaTime *self);
guint8 jana_time_get_hours 	(JanaTime *self);

guint8  jana_time_get_day 	(JanaTime *self);
guint8  jana_time_get_month 	(JanaTime *self);
guint16 jana_time_get_year 	(JanaTime *self);

gboolean jana_time_get_isdate	(JanaTime *self);
gboolean jana_time_get_daylight	(JanaTime *self);

gchar * jana_time_get_tzname 	(JanaTime *self);
glong   jana_time_get_offset 	(JanaTime *self);

void jana_time_set_seconds 	(JanaTime *self, gint seconds);
void jana_time_set_minutes 	(JanaTime *self, gint minutes);
void jana_time_set_hours 	(JanaTime *self, gint hours);

void jana_time_set_day 		(JanaTime *self, gint day);
void jana_time_set_month 	(JanaTime *self, gint month);
void jana_time_set_year 	(JanaTime *self, gint year);

void jana_time_set_isdate	(JanaTime *self, gboolean isdate);

void jana_time_set_tzname 	(JanaTime *self, const gchar *tzname);
void jana_time_set_offset 	(JanaTime *self, glong offset);

JanaTime * jana_time_duplicate	(JanaTime *self);


JanaDuration *	jana_duration_new	(JanaTime *start, JanaTime *end);
JanaDuration *	jana_duration_copy	(JanaDuration *duration);
void		jana_duration_set_start	(JanaDuration *self, JanaTime *start);
void		jana_duration_set_end	(JanaDuration *self, JanaTime *end);
gboolean	jana_duration_valid	(JanaDuration *self);
void		jana_duration_free	(JanaDuration *self);

#endif /*JANA_TIME_H*/

