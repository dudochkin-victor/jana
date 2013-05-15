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
 * SECTION:jana-ecal-store-view
 * @short_description: An implementation of #JanaStoreView using libecal
 *
 * #JanaEcalStoreView is an implementation of #JanaStoreView that provides a 
 * wrapper over #ECalView, using libecal.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define HANDLE_LIBICAL_MEMORY 1

#include <string.h>
//#include <libecal/e-cal.h>
//#include <libecal/e-cal-time-util.h>
#include <libjana/jana-utils.h>
#include "jana-ecal-component.h"
#include "jana-ecal-event.h"
#include "jana-ecal-note.h"
#include "jana-ecal-task.h"
#include "jana-ecal-time.h"
#include "jana-ecal-store-view.h"

static void store_view_interface_init (gpointer g_iface, gpointer iface_data);

static void	store_view_get_range	(JanaStoreView *self,
					 JanaTime **start,
					 JanaTime **end);

static void	store_view_set_range	(JanaStoreView *self,
					 JanaTime *start,
					 JanaTime *end);

static JanaStoreViewMatch *store_view_add_match (JanaStoreView *self,
						 JanaStoreViewField field,
						 const gchar *data);

static GList * store_view_get_matches	(JanaStoreView *self);

static void	store_view_remove_match	(JanaStoreView *self,
					 JanaStoreViewMatch *match);

static void	store_view_clear_matches(JanaStoreView *self);

static void	store_view_start	(JanaStoreView *self);

static JanaStore *store_view_get_store	(JanaStoreView *self);

static gboolean	store_view_refresh_query (JanaEcalStoreView *self);

G_DEFINE_TYPE_WITH_CODE (JanaEcalStoreView, 
                        jana_ecal_store_view, 
                        G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE (JANA_TYPE_STORE_VIEW,
                                               store_view_interface_init));

#define STORE_VIEW_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
			  JANA_ECAL_TYPE_STORE_VIEW, JanaEcalStoreViewPrivate))

typedef struct _JanaEcalStoreViewPrivate JanaEcalStoreViewPrivate;

struct _JanaEcalStoreViewPrivate
{
	JanaEcalStore *parent;
	ECalView *query;
	JanaEcalTime *start;
	JanaEcalTime *end;
	GList *matches;
	guint timeout;
	
	GList *current_uids;
	GList *old_uids;
	gboolean started;
	
	guint remove_id;
	guint refresh_id;
};

enum {
	PROP_PARENT = 1,
	PROP_VIEW,
	PROP_START,
	PROP_END,
	PROP_TIMEOUT,
};

static void
jana_ecal_store_view_get_property (GObject *object, guint property_id,
				   GValue *value, GParamSpec *pspec)
{
	JanaEcalStoreViewPrivate *priv = STORE_VIEW_PRIVATE (object);

	switch (property_id) {
	    case PROP_PARENT :
		g_value_set_object (value, priv->parent);
		break;
	    case PROP_VIEW :
		g_value_set_object (value, priv->query);
		break;
	    case PROP_START :
		g_value_take_object (value, jana_time_duplicate (
			JANA_TIME (priv->start)));
		break;
	    case PROP_END :
		g_value_take_object (value, jana_time_duplicate (
			JANA_TIME (priv->end)));
		break;
	    case PROP_TIMEOUT :
		g_value_set_uint (value, priv->timeout);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_ecal_store_view_set_property (GObject *object, guint property_id,
				   const GValue *value, GParamSpec *pspec)
{
	JanaEcalStoreViewPrivate *priv = STORE_VIEW_PRIVATE (object);

	switch (property_id) {
	    case PROP_PARENT :
		priv->parent = JANA_ECAL_STORE (g_value_dup_object (value));
		store_view_refresh_query (JANA_ECAL_STORE_VIEW (object));
		break;
	    case PROP_START :
		jana_store_view_set_range (JANA_STORE_VIEW (object), 
			JANA_TIME (g_value_get_object (value)),
			JANA_TIME (priv->end));
		break;
	    case PROP_END :
		jana_store_view_set_range (JANA_STORE_VIEW (object), 
			JANA_TIME (priv->start), 
			JANA_TIME (g_value_get_object (value)));
		break;
	    case PROP_TIMEOUT :
		priv->timeout = g_value_get_uint (value);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_ecal_store_view_dispose (GObject *object)
{
	JanaEcalStoreViewPrivate *priv = STORE_VIEW_PRIVATE (object);
	
	if (priv->remove_id) {
		g_source_remove (priv->remove_id);
		priv->remove_id = 0;
	}
	
	if (priv->refresh_id) {
		g_source_remove (priv->refresh_id);
		priv->refresh_id = 0;
	}
	
	if (priv->query) {
		g_object_unref (priv->query);
		priv->query = NULL;
	}
	
	if (priv->parent) {
		g_object_unref (priv->parent);
		priv->parent = NULL;
	}
	
	if (priv->start) {
		g_object_unref (priv->start);
		priv->start = NULL;
	}
	
	if (priv->end) {
		g_object_unref (priv->end);
		priv->end = NULL;
	}

	if (G_OBJECT_CLASS (jana_ecal_store_view_parent_class)->dispose)
		G_OBJECT_CLASS (jana_ecal_store_view_parent_class)->dispose (
			object);
}

static void
jana_ecal_store_view_finalize (GObject *object)
{
	JanaEcalStoreViewPrivate *priv = STORE_VIEW_PRIVATE (object);
	
	while (priv->old_uids) {
		g_free (priv->old_uids->data);
		priv->old_uids = g_list_delete_link (priv->old_uids,
			priv->old_uids);
	}

	while (priv->current_uids) {
		g_free (priv->current_uids->data);
		priv->current_uids = g_list_delete_link (priv->current_uids,
			priv->current_uids);
	}

	G_OBJECT_CLASS (jana_ecal_store_view_parent_class)->finalize (object);
}

static void
store_view_interface_init (gpointer g_iface, gpointer iface_data)
{
	JanaStoreViewInterface *iface = (JanaStoreViewInterface *)g_iface;
	
	iface->get_range = store_view_get_range;
	iface->set_range = store_view_set_range;
	iface->add_match = store_view_add_match;
	iface->get_matches = store_view_get_matches;
	iface->remove_match = store_view_remove_match;
	iface->clear_matches = store_view_clear_matches;

	iface->start = store_view_start;
	
	iface->get_store = store_view_get_store;
}

static void
jana_ecal_store_view_class_init (JanaEcalStoreViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (JanaEcalStoreViewPrivate));

	object_class->get_property = jana_ecal_store_view_get_property;
	object_class->set_property = jana_ecal_store_view_set_property;
	object_class->dispose = jana_ecal_store_view_dispose;
	object_class->finalize = jana_ecal_store_view_finalize;

	g_object_class_install_property (
		object_class,
		PROP_PARENT,
		g_param_spec_object (
			"parent",
			"JanaEcalStore *",
			"The JanaEcalStore this view is filtering.",
			JANA_ECAL_TYPE_STORE,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (
		object_class,
		PROP_VIEW,
		g_param_spec_object (
			"view",
			"ECalView *",
			"The ECalView being used to query the parent ECal.",
			E_TYPE_CAL_VIEW,
			G_PARAM_READABLE));

	g_object_class_install_property (
		object_class,
		PROP_START,
		g_param_spec_object (
			"start",
			"JanaTime *",
			"The start of the range being queried.",
			G_TYPE_OBJECT,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_END,
		g_param_spec_object (
			"end",
			"JanaTime *",
			"The end of the range being queried.",
			G_TYPE_OBJECT,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_TIMEOUT,
		g_param_spec_uint (
			"timeout",
			"guint",
			"The time-out (in milliseconds) before components from "
			"an old query are removed.",
			0, G_MAXUINT, 0,
			G_PARAM_READWRITE));
}

static void
jana_ecal_store_view_init (JanaEcalStoreView *self)
{
	JanaEcalStoreViewPrivate *priv = STORE_VIEW_PRIVATE (self);
	
	/* FIXME: For timeout to really work, need to store event time
	 *        and check if it's still in range when range changes -
	 *        but this is complicated by recurring events, so just
	 *        setting the default timeout to zero for now.
	 */
	priv->timeout = 0;
}

/**
 * jana_ecal_store_view_new:
 * @store: A #JanaEcalStore
 *
 * Creates a new #JanaEcalStoreView on the given store.
 *
 * Returns: A new #JanaEcalStoreView, cast as a #JanaStoreView.
 */
JanaStoreView *
jana_ecal_store_view_new (JanaEcalStore *store)
{
	return JANA_STORE_VIEW (g_object_new (JANA_ECAL_TYPE_STORE_VIEW,
		"parent", store, NULL));
}

static gboolean
store_view_remove_old_cb (JanaEcalStoreView *self)
{
	JanaEcalStoreViewPrivate *priv = STORE_VIEW_PRIVATE (self);
	
	if (priv->old_uids) {
		g_signal_emit_by_name (self, "removed", priv->old_uids);

		while (priv->old_uids) {
			g_free (priv->old_uids->data);
			priv->old_uids = g_list_delete_link (priv->old_uids,
				priv->old_uids);
		}
	}
	
	priv->remove_id = 0;
	
	return FALSE;
}

static JanaComponent *
store_view_jcomp_from_ecomp (ECalComponent *comp)
{
	JanaComponent *jcomp;
	
	switch (e_cal_component_get_vtype (comp)) {
	    case E_CAL_COMPONENT_EVENT :
		jcomp = JANA_COMPONENT (
			jana_ecal_event_new_from_ecalcomp (comp));
		break;
	    case E_CAL_COMPONENT_JOURNAL :
		jcomp = JANA_COMPONENT (
			jana_ecal_note_new_from_ecalcomp (comp));
		break;
	    case E_CAL_COMPONENT_TODO :
		jcomp = JANA_COMPONENT (
			jana_ecal_task_new_from_ecalcomp (comp));
		break;
	    default :
		jcomp = jana_ecal_component_new_from_ecalcomp (comp);
		break;
	}
	
	return jcomp;
}

static void
store_view_objects_added_cb (ECalView *query, GList *objects,
			     JanaStoreView *self)
{
	JanaEcalStoreViewPrivate *priv = STORE_VIEW_PRIVATE (self);

	GList *comps_added = NULL;
	GList *comps_modified = NULL;
	
	for (; objects; objects = objects->next) {
		JanaComponent *jcomp;
		const char *uid = icalcomponent_get_uid (objects->data);
		GList *previous_uid = g_list_find_custom (priv->old_uids,
			uid, (GCompareFunc)strcmp);
		ECalComponent *comp = e_cal_component_new ();
		
		e_cal_component_set_icalcomponent (comp,
			icalcomponent_new_clone (objects->data));

		jcomp = store_view_jcomp_from_ecomp (comp);

		if (previous_uid) {
			g_free (previous_uid->data);
			priv->old_uids = g_list_delete_link (
				priv->old_uids, previous_uid);
			comps_modified = g_list_prepend (comps_modified,
				jcomp);
		} else
			comps_added = g_list_prepend (comps_added, jcomp);
		
		priv->current_uids = g_list_prepend (priv->current_uids,
			g_strdup (uid));
	}
	
	if (comps_added) g_signal_emit_by_name (self, "added", comps_added);
	if (comps_modified) g_signal_emit_by_name (self, "modified",
		comps_modified);
	
	while (comps_added) {
		g_object_unref (G_OBJECT (comps_added->data));
		comps_added = g_list_delete_link (comps_added, comps_added);
	}
	
	while (comps_modified) {
		g_object_unref (G_OBJECT (comps_modified->data));
		comps_modified = g_list_delete_link (
			comps_modified, comps_modified);
	}
}

static void
store_view_objects_modified_cb (ECalView *query, GList *objects,
				JanaStoreView *self)
{
	/*JanaEcalStoreViewPrivate *priv = STORE_VIEW_PRIVATE (self);*/

	GList *comps_modified = NULL;
	
	for (; objects; objects = objects->next) {
		JanaComponent *jcomp;
		ECalComponent *comp = e_cal_component_new ();
		
		e_cal_component_set_icalcomponent (comp,
			icalcomponent_new_clone (objects->data));
		jcomp = store_view_jcomp_from_ecomp (comp);

		comps_modified = g_list_prepend (comps_modified, jcomp);
	}
	
	if (comps_modified) g_signal_emit_by_name (self, "modified",
		comps_modified);
	
	while (comps_modified) {
		g_object_unref (G_OBJECT (comps_modified->data));
		comps_modified = g_list_delete_link (
			comps_modified, comps_modified);
	}
}

static void
store_view_objects_removed_cb (ECalView *query, GList *uids,
			       JanaStoreView *self)
{
	GList *comps_removed = NULL;
	
	for (; uids; uids = uids->next) {
#ifdef HAVE_ECALCOMPONENTID
		ECalComponentId *id = uids->data;
		comps_removed = g_list_prepend (comps_removed, id->uid);
#else
		comps_removed = g_list_prepend (comps_removed, uids->data);
#endif
	}
	
	g_signal_emit_by_name (self, "removed", comps_removed);
	
	g_list_free (comps_removed);
}

static void
store_view_progress_cb (ECalView *query, gchar *message, gint percent,
			JanaStoreView *self)
{
	/* Ignore 100%, we'll get the done signal - we don't want to emit 1.0
	 * twice.
	 */
	if (percent < 100)
		g_signal_emit_by_name (self, "progress", percent);
}

static void
store_view_done_cb (ECalView *query, ECalendarStatus status,
		    JanaStoreView *self)
{
	g_signal_emit_by_name (self, "progress", 100);
}

static gchar *
get_match_string (GList *matches)
{
	GList *m;
	GString *string;
	gchar *match_string;
	
	if (!matches) return NULL;
	
	string = g_string_new ("");
	for (m = matches; m; m = m->next) {
		JanaStoreViewMatch *match;
		const gchar *field;
		
		if (m != matches) {
			g_string_prepend (string, "(or ");
			g_string_append_c (string, ' ');
		}
		
		match = (JanaStoreViewMatch *)m->data;
		switch (match->field) {
		    case JANA_STORE_VIEW_AUTHOR :
		    case JANA_STORE_VIEW_SUMMARY :
			field = "summary";
			break;
		    case JANA_STORE_VIEW_RECIPIENT :
		    case JANA_STORE_VIEW_LOCATION :
			field = "location";
			break;
		    case JANA_STORE_VIEW_BODY :
		    case JANA_STORE_VIEW_DESCRIPTION :
			field = "description";
			break;
		    case JANA_STORE_VIEW_ANYFIELD :
		    default :
			field = "any";
			break;
		}
		
		if (match->field == JANA_STORE_VIEW_CATEGORY) {
			g_string_append_printf (string,
				"(has-categories? \"%s\")", match->data);
		} else {
			g_string_append_printf (string,
				"(contains? \"%s\" \"%s\")",
				field, match->data);
		}
		if (m != matches) g_string_append_c (string, ')');
	}
	
	match_string = string->str;
	g_string_free (string, FALSE);
	
	return match_string;
}

static gboolean
store_view_refresh_query (JanaEcalStoreView *self)
{
	ECal *ecal;
	gchar *query;
	GError *error = NULL;

	JanaEcalStoreViewPrivate *priv = STORE_VIEW_PRIVATE (self);
	
	if (priv->query) {
		g_object_unref (priv->query);
		priv->query = NULL;
	}
	
	priv->old_uids = g_list_concat (priv->old_uids, priv->current_uids);
	priv->current_uids = NULL;
	
	if (priv->start || priv->end || priv->matches) {
		gchar *start, *end;
		
		if (priv->start) {
			icaltimetype *ical_start;
			g_object_get (priv->start, "icaltime",
				&ical_start, NULL);
			start = isodate_from_time_t (
				icaltime_as_timet_with_zone (*ical_start,
					ical_start->zone));
		} else
			start = isodate_from_time_t (0);
		if (priv->end) {
			icaltimetype *ical_end;
			g_object_get (priv->end, "icaltime", &ical_end, NULL);
			end = isodate_from_time_t (icaltime_as_timet_with_zone (
				*ical_end, ical_end->zone));
		} else
			end = isodate_from_time_t (G_MAXLONG);
		
		if (priv->matches) {
			gchar *match = get_match_string (priv->matches);
			query = g_strdup_printf ("(and %s "
				"(occur-in-time-range? "
				"(make-time \"%s\") (make-time \"%s\")))",
				match, start, end);
			g_free (match);
		} else
			query = g_strdup_printf ("(occur-in-time-range? "
				"(make-time \"%s\") (make-time \"%s\"))",
				start, end);

		g_free (start);
		g_free (end);
	} else {
		query = g_strdup ("#t");
	}

	g_object_get (priv->parent, "ecal", &ecal, NULL);
	
	if (!e_cal_get_query (ecal, query, &priv->query, &error)) {
		g_warning ("Failed to retrieve query '%s': %s",
			query, error->message);
		g_error_free (error);
	} else {
		g_signal_connect (priv->query, "objects-added",
			G_CALLBACK (store_view_objects_added_cb), self);
		g_signal_connect (priv->query, "objects-modified",
			G_CALLBACK (store_view_objects_modified_cb), self);
		g_signal_connect (priv->query, "objects-removed",
			G_CALLBACK (store_view_objects_removed_cb), self);
		g_signal_connect (priv->query, "view_progress",
			G_CALLBACK (store_view_progress_cb), self);
		g_signal_connect (priv->query, "view_done",
			G_CALLBACK (store_view_done_cb), self);
		
		if (priv->started) e_cal_view_start (priv->query);
	}
	
	g_free (query);
	g_object_unref (ecal);
	
	/* Remove old components (but possibly wait to see if they don't need
	 * removing, to prevent 'flickering' when resizing the view).
	 */
	if (priv->old_uids) {	
		if (priv->timeout) {
			if (priv->remove_id)
				g_source_remove (priv->remove_id);
			priv->remove_id = g_timeout_add (
				priv->timeout, (GSourceFunc)
				store_view_remove_old_cb, self);
		} else
			store_view_remove_old_cb (self);
	}
	
	priv->refresh_id = 0;
	
	return FALSE;
}

static void
store_view_get_range (JanaStoreView *self,
		      JanaTime **start,
		      JanaTime **end)
{
	/*JanaEcalStoreViewPrivate *priv = STORE_VIEW_PRIVATE (self);*/
	
	g_object_get (self, "start", start, "end", end, NULL);
}

static void
store_view_set_range (JanaStoreView *self,
		      JanaTime *start,
		      JanaTime *end)
{
	JanaEcalStoreViewPrivate *priv = STORE_VIEW_PRIVATE (self);

	if (priv->start) g_object_unref (priv->start);
	if (!start) {
		priv->start = NULL;
	} else {
		if (JANA_ECAL_IS_TIME (start))
			priv->start = JANA_ECAL_TIME (
				jana_time_duplicate (start));
		else
			priv->start = JANA_ECAL_TIME (jana_utils_time_copy (
				start, jana_ecal_time_new ()));
	}

	if (priv->end) g_object_unref (priv->end);
	if (!end) {
		priv->end = NULL;
	} else {
		if (JANA_ECAL_IS_TIME (end))
			priv->end = JANA_ECAL_TIME (
				jana_time_duplicate (end));
		else
			priv->end = JANA_ECAL_TIME (jana_utils_time_copy (
				end, jana_ecal_time_new ()));
	}

	store_view_refresh_query (JANA_ECAL_STORE_VIEW (self));
}

static JanaStoreViewMatch *
store_view_add_match (JanaStoreView *self, JanaStoreViewField field,
		      const gchar *data)
{
	JanaStoreViewMatch *match;
	JanaEcalStoreViewPrivate *priv = STORE_VIEW_PRIVATE (self);
	
	g_assert (data);
	
	match = g_slice_alloc (sizeof (JanaStoreViewMatch));
	match->field = field;
	match->data = g_strdup (data);
	priv->matches = g_list_prepend (priv->matches, match);
	
	if (!priv->refresh_id) priv->refresh_id = g_idle_add (
		(GSourceFunc)store_view_refresh_query, self);

	return match;
}

static GList *
store_view_get_matches (JanaStoreView *self)
{
	JanaEcalStoreViewPrivate *priv = STORE_VIEW_PRIVATE (self);
	
	return g_list_copy (priv->matches);
}

static void
store_view_remove_match (JanaStoreView *self, JanaStoreViewMatch *match)
{
	JanaEcalStoreViewPrivate *priv = STORE_VIEW_PRIVATE (self);
	
	priv->matches = g_list_remove (priv->matches, match);
	g_free (match->data);
	g_slice_free (JanaStoreViewMatch, match);

	if (!priv->refresh_id) priv->refresh_id = g_idle_add (
		(GSourceFunc)store_view_refresh_query, self);
}

static void
store_view_clear_matches (JanaStoreView *self)
{
	JanaEcalStoreViewPrivate *priv = STORE_VIEW_PRIVATE (self);
	
	while (priv->matches) {
		store_view_remove_match (self,
			(JanaStoreViewMatch *)priv->matches->data);
	}
}

static void
store_view_start (JanaStoreView *self)
{
	JanaEcalStoreViewPrivate *priv = STORE_VIEW_PRIVATE (self);

	if (priv->query && (!priv->started)) {
		e_cal_view_start (priv->query);
	}

	priv->started = TRUE;
}

static JanaStore *
store_view_get_store (JanaStoreView *self)
{
	JanaStore *store;
	
	g_object_get (self, "parent", &store, NULL);
	
	return store;
}

