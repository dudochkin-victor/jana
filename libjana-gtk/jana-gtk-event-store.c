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


#include <string.h>
#include <gtk/gtk.h>
#include <libjana/jana-store-view.h>
#include <libjana/jana-component.h>
#include <libjana/jana-event.h>
#include <libjana/jana-utils.h>
#include "jana-gtk-event-store.h"

G_DEFINE_TYPE (JanaGtkEventStore, jana_gtk_event_store, GTK_TYPE_LIST_STORE)

#define EVENT_STORE_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), JANA_GTK_TYPE_EVENT_STORE, JanaGtkEventStorePrivate))

typedef struct _JanaGtkEventStorePrivate JanaGtkEventStorePrivate;

struct _JanaGtkEventStorePrivate
{
	GHashTable *events_hash;
	gboolean split;

	JanaStoreView *view;
	glong offset;
};

enum {
	PROP_VIEW = 1,
	PROP_OFFSET,
};

static void
event_store_added_cb (JanaStoreView *view, GList *components,
		      JanaGtkEventStore *store)
{
	JanaTime *range_start, *range_end;
	
	JanaGtkEventStorePrivate *priv = EVENT_STORE_PRIVATE (store);

	jana_store_view_get_range (priv->view,
		&range_start, &range_end);

	for (; components; components = components->next) {
		gchar *uid, *summary, *description, *location, **categories;
		JanaTime *start, *end;
		JanaRecurrence *recur;
		JanaEvent *event;
		GList *instance, *instances, *iter_list = NULL;
		gint days, inst_days;
		glong seconds;
		
		if (jana_component_get_component_type (JANA_COMPONENT (
		    components->data)) != JANA_COMPONENT_EVENT) continue;
		event = JANA_EVENT (components->data);
		
		uid = jana_component_get_uid (JANA_COMPONENT (event));
		summary = jana_event_get_summary (event);
		description = jana_event_get_description (event);
		location = jana_event_get_location (event);
		categories = jana_event_get_categories (event);
		start = jana_event_get_start (event);
		end = jana_event_get_end (event);
		recur = jana_event_get_recurrence (event);
		
		/* See how many days are between the start and the end so we
		 * can set the first_instance and last_instance parameters 
		 * correctly.
		 */
		jana_utils_time_diff (start, end, NULL, NULL, &days,
			NULL, NULL, &seconds);
		
		if (days && (seconds == 0)) days--;
		inst_days = 0;
		instances = jana_utils_event_get_instances (event,
			range_start, range_end, priv->offset);
		for (instance = instances; instance; instance = instance->next){
			JanaDuration *duration = (JanaDuration *)instance->data;
			GtkTreeIter *iter = g_slice_new (GtkTreeIter);
			iter_list = g_list_append (iter_list, iter);
			gboolean first, last;
			
			if (inst_days == 0) first = TRUE;
			else first = FALSE;
			if (inst_days == days) {
				last = TRUE;
				inst_days -= (days + 1);
			} else
				last = FALSE;
			
			gtk_list_store_insert_with_values (
				GTK_LIST_STORE (store), iter, 0,
				JANA_GTK_EVENT_STORE_COL_UID, uid,
				JANA_GTK_EVENT_STORE_COL_SUMMARY, summary,
				JANA_GTK_EVENT_STORE_COL_DESCRIPTION,
					description,
				JANA_GTK_EVENT_STORE_COL_LOCATION, location,
				JANA_GTK_EVENT_STORE_COL_CATEGORIES, categories,
				JANA_GTK_EVENT_STORE_COL_START, duration->start,
				JANA_GTK_EVENT_STORE_COL_END, duration->end,
				JANA_GTK_EVENT_STORE_COL_FIRST_INSTANCE, first,
				JANA_GTK_EVENT_STORE_COL_LAST_INSTANCE, last,
				JANA_GTK_EVENT_STORE_COL_HAS_RECURRENCES,
					jana_event_has_recurrence (event),
				JANA_GTK_EVENT_STORE_COL_HAS_ALARM,
					jana_event_has_alarm (event),
				JANA_GTK_EVENT_STORE_COL_RECUR_TYPE, recur,
				-1);
			
			inst_days ++;
		}
		g_hash_table_insert (priv->events_hash,
			(gpointer)uid, iter_list);
		jana_utils_instance_list_free (instances);
		
		/* Don't free uid, it'll be freed by the hash table */
		/*g_free (uid);*/
		g_free (summary);
		g_free (description);
		g_free (location);
		g_strfreev (categories);
		g_object_unref (start);
		g_object_unref (end);
		jana_recurrence_free (recur);
	}

	if (range_start) g_object_unref (range_start);
	if (range_end) g_object_unref (range_end);
}

static void
event_store_modified_cb (JanaStoreView *view, GList *components,
			 JanaGtkEventStore *store)
{
	JanaTime *range_start, *range_end;
	JanaGtkEventStorePrivate *priv = EVENT_STORE_PRIVATE (store);

	jana_store_view_get_range (priv->view,
		&range_start, &range_end);

	for (; components; components = components->next) {
		GList *instance, *instances, *iter_list;
		gchar *uid, *summary, *description, *location, **categories;
		gpointer orig_uid, orig_list;
		JanaTime *start, *end;
		JanaRecurrence *recur;
		gint days, inst_days;
		gint iter_count = 0;
		GtkTreeIter *iter;
		JanaEvent *event;
		glong seconds;
		
		if (jana_component_get_component_type (components->data) !=
		    JANA_COMPONENT_EVENT) continue;
		event = JANA_EVENT (components->data);

		uid = jana_component_get_uid (JANA_COMPONENT (event));

		if (!g_hash_table_lookup_extended (
		     priv->events_hash, uid, &orig_uid, &orig_list)) {
			g_free (uid);
			continue;
		}
		
		iter_list = (GList *)orig_list;
		
		summary = jana_event_get_summary (event);
		description = jana_event_get_description (event);
		location = jana_event_get_location (event);
		categories = jana_event_get_categories (event);
		start = jana_event_get_start (event);
		end = jana_event_get_end (event);
		recur = jana_event_get_recurrence (event);
		
		jana_utils_time_diff (start, end, NULL, NULL, &days,
			NULL, NULL, &seconds);
		
		if (days && (seconds == 0)) days--;
		inst_days = 0;
		/* Go through instances, naively changing ones that already
		 * exist, trimming if there are too many and adding if there
		 * aren't enough. As we can't know the nature of the change,
		 * there's nothing better we can do here.
		 */
		instances = jana_utils_event_get_instances (event,
			range_start, range_end, priv->offset);
		for (instance = instances; instance; instance = instance->next){
			JanaDuration *duration = (JanaDuration *)instance->data;
			gboolean first, last;
			
			if (inst_days == 0) first = TRUE;
			else first = FALSE;
			if (inst_days == days) {
				last = TRUE;
				inst_days -= (days + 1);
			} else
				last = FALSE;

			iter = (GtkTreeIter *)g_list_nth_data (
				iter_list, iter_count);
			
			if (iter) {
				/* Change row */
				gtk_list_store_set (
				    GTK_LIST_STORE (store), iter,
				    JANA_GTK_EVENT_STORE_COL_UID, uid,
				    JANA_GTK_EVENT_STORE_COL_SUMMARY,
					summary,
				    JANA_GTK_EVENT_STORE_COL_DESCRIPTION,
					description,
				    JANA_GTK_EVENT_STORE_COL_LOCATION,
					location,
				    JANA_GTK_EVENT_STORE_COL_CATEGORIES,
					categories,
				    JANA_GTK_EVENT_STORE_COL_START,
					duration->start,
				    JANA_GTK_EVENT_STORE_COL_END,
					duration->end,
				    JANA_GTK_EVENT_STORE_COL_FIRST_INSTANCE,
					first,
				    JANA_GTK_EVENT_STORE_COL_LAST_INSTANCE,
					last,
				    JANA_GTK_EVENT_STORE_COL_HAS_RECURRENCES,
					jana_event_has_recurrence (event),
				    JANA_GTK_EVENT_STORE_COL_HAS_ALARM,
					jana_event_has_alarm (event),
				    JANA_GTK_EVENT_STORE_COL_RECUR_TYPE,
					recur, -1);
			} else {
				/* Add new row */
				iter = g_slice_new (GtkTreeIter);
				gtk_list_store_insert_with_values (
				    GTK_LIST_STORE (store), iter, 0,
				    JANA_GTK_EVENT_STORE_COL_UID, uid,
				    JANA_GTK_EVENT_STORE_COL_SUMMARY,
					summary,
				    JANA_GTK_EVENT_STORE_COL_DESCRIPTION,
					description,
				    JANA_GTK_EVENT_STORE_COL_LOCATION,
					location,
				    JANA_GTK_EVENT_STORE_COL_CATEGORIES,
					categories,
				    JANA_GTK_EVENT_STORE_COL_START,
					duration->start,
				    JANA_GTK_EVENT_STORE_COL_END,
					duration->end,
				    JANA_GTK_EVENT_STORE_COL_FIRST_INSTANCE,
					first,
				    JANA_GTK_EVENT_STORE_COL_LAST_INSTANCE,
					last,
				    JANA_GTK_EVENT_STORE_COL_HAS_RECURRENCES,
					jana_event_has_recurrence (event),
				    JANA_GTK_EVENT_STORE_COL_HAS_ALARM,
					jana_event_has_alarm (event),
				    JANA_GTK_EVENT_STORE_COL_RECUR_TYPE,
					recur, -1);
				iter_list = g_list_append (iter_list, iter);
			}
			iter_count ++;
			inst_days ++;
		}
		/* Trim off instances if there are too many */
		while ((instance = g_list_nth (iter_list, iter_count))) {
			iter = (GtkTreeIter *)instance->data;
			gtk_list_store_remove (
				GTK_LIST_STORE (store), iter);
			g_slice_free (GtkTreeIter, iter);
			iter_list = g_list_delete_link (iter_list, instance);
		}
		jana_utils_instance_list_free (instances);
		
		g_hash_table_steal (priv->events_hash, orig_uid);
		g_free (orig_uid);
		g_hash_table_insert (priv->events_hash, uid, iter_list);
		
		g_free (summary);
		g_free (description);
		g_free (location);
		g_strfreev (categories);
		g_object_unref (start);
		g_object_unref (end);
		jana_recurrence_free (recur);
	}

	if (range_start) g_object_unref (range_start);
	if (range_end) g_object_unref (range_end);
}

static void
event_store_removed_cb (JanaStoreView *view, GList *uids,
			 JanaGtkEventStore *store)
{
	JanaGtkEventStorePrivate *priv = EVENT_STORE_PRIVATE (store);

	for (; uids; uids = uids->next) {
		const gchar *uid = (const gchar *)uids->data;
		GList *iter_list;
		
		iter_list = (GList *)g_hash_table_lookup (
			priv->events_hash, uid);
		if (!iter_list) continue;
		
		for (; iter_list; iter_list = iter_list->next) {
			GtkTreeIter *iter = (GtkTreeIter *)iter_list->data;
			gtk_list_store_remove (GTK_LIST_STORE (store), iter);
		}
		g_hash_table_remove (priv->events_hash, uid);
	}
}

static void
jana_gtk_event_store_get_property (GObject *object, guint property_id,
				    GValue *value, GParamSpec *pspec)
{
	JanaGtkEventStorePrivate *priv = EVENT_STORE_PRIVATE (object);

	switch (property_id) {
	    case PROP_VIEW :
		g_value_set_object (value, priv->view);
		break;
	    case PROP_OFFSET :
		g_value_set_long (value, priv->offset);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_event_store_set_property (GObject *object, guint property_id,
				    const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	    case PROP_VIEW :
		jana_gtk_event_store_set_view (JANA_GTK_EVENT_STORE (object),
			g_value_get_object (value));
		break;
	    case PROP_OFFSET :
		jana_gtk_event_store_set_offset (JANA_GTK_EVENT_STORE (object),
			g_value_get_long (value));
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_event_store_dispose (GObject *object)
{
	JanaGtkEventStorePrivate *priv = EVENT_STORE_PRIVATE (object);

	if (priv->view) {
		g_object_unref (priv->view);
		priv->view = NULL;
	}

	if (G_OBJECT_CLASS (jana_gtk_event_store_parent_class)->dispose)
		G_OBJECT_CLASS (jana_gtk_event_store_parent_class)->dispose (
			object);
}

static void
jana_gtk_event_store_finalize (GObject *object)
{
	JanaGtkEventStorePrivate *priv = EVENT_STORE_PRIVATE (object);
	
	g_hash_table_destroy (priv->events_hash);

	G_OBJECT_CLASS (jana_gtk_event_store_parent_class)->finalize (object);
}

static void
jana_gtk_event_store_class_init (JanaGtkEventStoreClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	g_type_class_add_private (klass, sizeof (JanaGtkEventStorePrivate));
	
	object_class->get_property = jana_gtk_event_store_get_property;
	object_class->set_property = jana_gtk_event_store_set_property;
	object_class->dispose = jana_gtk_event_store_dispose;
	object_class->finalize = jana_gtk_event_store_finalize;

	g_object_class_install_property (
		object_class,
		PROP_VIEW,
		g_param_spec_object (
			"view",
			"JanaStoreView *",
			"The JanaStoreView monitored.",
			G_TYPE_OBJECT,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_OFFSET,
		g_param_spec_long (
			"offset",
			"glong",
			"The time offset to use when instancing events.",
			-G_MAXLONG, G_MAXLONG, 0,
			G_PARAM_READWRITE));
}

static gint
jana_gtk_event_store_compare (GtkTreeModel *model, GtkTreeIter *a,
			       GtkTreeIter *b, gpointer user_data)
{
	JanaTime *start1, *end1, *start2, *end2;
	gchar *summary1, *summary2;
	gint result = 0;
	JanaGtkEventStore *store = JANA_GTK_EVENT_STORE (user_data);
	
	gtk_tree_model_get (GTK_TREE_MODEL (store), a,
		JANA_GTK_EVENT_STORE_COL_START, &start1,
		JANA_GTK_EVENT_STORE_COL_END, &end1,
		JANA_GTK_EVENT_STORE_COL_SUMMARY, &summary1, -1);
	gtk_tree_model_get (GTK_TREE_MODEL (store), b,
		JANA_GTK_EVENT_STORE_COL_START, &start2,
		JANA_GTK_EVENT_STORE_COL_END, &end2,
		JANA_GTK_EVENT_STORE_COL_SUMMARY, &summary2, -1);
	
	if (start1 && start2) {
		result = jana_utils_time_compare (start1, start2, FALSE);
		if (result == 0) result = jana_utils_time_compare (
			end2, end1, FALSE);
	}
	if (summary1 && summary2)
		if (result == 0) result = strcmp (summary1, summary2);
	
	g_free (summary1);
	g_free (summary2);
	g_object_unref (start1);
	g_object_unref (start2);
	g_object_unref (end1);
	g_object_unref (end2);
	
	return result;
}

void
event_store_free_iter_list (gpointer data)
{
	GList *iter_list = (GList *)data;
	
	while (iter_list) {
		g_slice_free (GtkTreeIter, iter_list->data);
		iter_list = g_list_delete_link (iter_list, iter_list);
	}
}

static void
jana_gtk_event_store_init (JanaGtkEventStore *self)
{
	JanaGtkEventStorePrivate *priv = EVENT_STORE_PRIVATE (self);

	priv->view = NULL;
	priv->events_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
		g_free, event_store_free_iter_list);
	
	gtk_list_store_set_column_types (GTK_LIST_STORE (self),
		JANA_GTK_EVENT_STORE_COL_LAST,
		(GType []){G_TYPE_STRING,	/* UID */
			   G_TYPE_STRING,	/* SUMMARY */
			   G_TYPE_STRING,	/* DESCRIPTION */
			   G_TYPE_STRING,	/* LOCATION */
			   G_TYPE_STRV,		/* CATEGORIES */
			   G_TYPE_OBJECT,	/* START */
			   G_TYPE_OBJECT,	/* END */
			   G_TYPE_BOOLEAN,	/* FIRST_INSTANCE */
			   G_TYPE_BOOLEAN,	/* LAST_INSTANCE */
			   G_TYPE_BOOLEAN,	/* HAS_RECURRENCES */
			   G_TYPE_BOOLEAN,	/* HAS_ALARM */
			   JANA_TYPE_RECURRENCE,/* RECUR_TYPE */
		});
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (self),
		JANA_GTK_EVENT_STORE_COL_START, jana_gtk_event_store_compare,
		self, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (self),
		JANA_GTK_EVENT_STORE_COL_START, GTK_SORT_ASCENDING);
}

GtkTreeModel *
jana_gtk_event_store_new (void)
{
	return (GtkTreeModel *)g_object_new (JANA_GTK_TYPE_EVENT_STORE, NULL);
}

GtkTreeModel *
jana_gtk_event_store_new_full (JanaStoreView *view, glong offset)
{
	return (GtkTreeModel *)g_object_new (JANA_GTK_TYPE_EVENT_STORE,
		"view", view, "offset", offset, NULL);
}

void
jana_gtk_event_store_set_view (JanaGtkEventStore *store, JanaStoreView *view)
{
	JanaGtkEventStorePrivate *priv = EVENT_STORE_PRIVATE (store);
	
	if (priv->view) {
		g_signal_handlers_disconnect_by_func (
			priv->view, event_store_added_cb, store);
		g_signal_handlers_disconnect_by_func (
			priv->view, event_store_modified_cb, store);
		g_signal_handlers_disconnect_by_func (
			priv->view, event_store_removed_cb, store);
		g_object_unref (priv->view);
		priv->view = NULL;
		g_hash_table_remove_all (priv->events_hash);
		gtk_list_store_clear (GTK_LIST_STORE (store));
	}
	if (view) {
		priv->view = g_object_ref (view);
		g_signal_connect (priv->view, "added",
			G_CALLBACK (event_store_added_cb), store);
		g_signal_connect (priv->view, "modified",
			G_CALLBACK (event_store_modified_cb), store);
		g_signal_connect (priv->view, "removed",
			G_CALLBACK (event_store_removed_cb), store);
	}
}

JanaStoreView *
jana_gtk_event_store_get_view (JanaGtkEventStore *store)
{
	JanaGtkEventStorePrivate *priv = EVENT_STORE_PRIVATE (store);
	
	return priv->view ? g_object_ref (priv->view) : NULL;
}

JanaStore *
jana_gtk_event_store_get_store (JanaGtkEventStore *self)
{
	JanaGtkEventStorePrivate *priv = EVENT_STORE_PRIVATE (self);
	
	if (priv->view) {
		return jana_store_view_get_store (priv->view);
	} else return NULL;
}

typedef struct {
	JanaStore *store;
	GList *components;
} ChangeEventsData;

static void
change_events_cb (gpointer key, gpointer value, gpointer user_data)
{
	ChangeEventsData *data = (ChangeEventsData *)user_data;
	const gchar *uid = (const gchar *)key;
	JanaComponent *comp = jana_store_get_component (data->store, uid);
	
	data->components = g_list_prepend (data->components, comp);
}

/**
 * jana_gtk_event_store_set_offset:
 * @self: A #JanaGtkEventStore
 * @offset: Time offset, in seconds
 *
 * Sets the time offset that should be used when splitting events into day 
 * instances.
 */
void
jana_gtk_event_store_set_offset (JanaGtkEventStore *self, glong offset)
{
	JanaGtkEventStorePrivate *priv = EVENT_STORE_PRIVATE (self);
	ChangeEventsData data;
	
	if (priv->offset == offset) return;
	
	priv->offset = offset;
	
	/* Re-instance all events  by firing the modified signal on them */
	data.store = jana_store_view_get_store (priv->view);
	data.components = NULL;

	/* Create a list of all the components */
	g_hash_table_foreach (priv->events_hash, change_events_cb, &data);

	/* Send them to the modified callback */
	event_store_modified_cb (priv->view, data.components, self);

	/* Free components */
	while (data.components) {
		g_object_unref ((GObject *)data.components->data);
		data.components = g_list_delete_link (
			data.components, data.components);
	}
	g_object_unref (data.store);
}
