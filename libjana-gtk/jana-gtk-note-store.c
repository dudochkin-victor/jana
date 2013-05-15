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


#include <libjana/jana.h>
#include "jana-gtk-note-store.h"

G_DEFINE_TYPE (JanaGtkNoteStore, jana_gtk_note_store, GTK_TYPE_LIST_STORE)

#define NOTE_STORE_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), JANA_GTK_TYPE_NOTE_STORE, \
	JanaGtkNoteStorePrivate))

typedef struct _JanaGtkNoteStorePrivate JanaGtkNoteStorePrivate;

struct _JanaGtkNoteStorePrivate
{
	GHashTable *notes_hash;
	JanaStoreView *view;
};

enum {
	PROP_VIEW = 1,
};

static void
jana_gtk_note_store_get_property (GObject *object, guint property_id,
				  GValue *value, GParamSpec *pspec)
{
	JanaGtkNoteStorePrivate *priv = NOTE_STORE_PRIVATE (object);

	switch (property_id) {
	    case PROP_VIEW :
		g_value_set_object (value, priv->view);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_note_store_set_property (GObject *object, guint property_id,
				    const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	    case PROP_VIEW :
		jana_gtk_note_store_set_view (JANA_GTK_NOTE_STORE (object),
			g_value_get_object (value));
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_note_store_dispose (GObject *object)
{
	JanaGtkNoteStorePrivate *priv = NOTE_STORE_PRIVATE (object);

	if (priv->view) {
		g_object_unref (priv->view);
		priv->view = NULL;
	}

	if (G_OBJECT_CLASS (jana_gtk_note_store_parent_class)->dispose)
		G_OBJECT_CLASS (jana_gtk_note_store_parent_class)->dispose (
			object);
}

static void
jana_gtk_note_store_finalize (GObject *object)
{
	JanaGtkNoteStorePrivate *priv = NOTE_STORE_PRIVATE (object);
	
	g_hash_table_destroy (priv->notes_hash);

	G_OBJECT_CLASS (jana_gtk_note_store_parent_class)->finalize (object);
}

static void
jana_gtk_note_store_class_init (JanaGtkNoteStoreClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	g_type_class_add_private (klass, sizeof (JanaGtkNoteStorePrivate));
	
	object_class->get_property = jana_gtk_note_store_get_property;
	object_class->set_property = jana_gtk_note_store_set_property;
	object_class->dispose = jana_gtk_note_store_dispose;
	object_class->finalize = jana_gtk_note_store_finalize;

	g_object_class_install_property (
		object_class,
		PROP_VIEW,
		g_param_spec_object (
			"view",
			"JanaStoreView *",
			"The JanaStoreView monitored.",
			G_TYPE_OBJECT,
			G_PARAM_READWRITE));
}

static void
note_store_free_iter (gpointer data)
{
	g_slice_free (GtkTreeIter, data);
}

static gint
jana_gtk_note_store_compare (GtkTreeModel *model, GtkTreeIter *a,
			     GtkTreeIter *b, gpointer user_data)
{
	JanaTime *created1, *created2;

	JanaGtkNoteStore *store = JANA_GTK_NOTE_STORE (user_data);
	gint result = 0;
	
	gtk_tree_model_get (GTK_TREE_MODEL (store), a,
		JANA_GTK_NOTE_STORE_COL_CREATED, &created1, -1);
	gtk_tree_model_get (GTK_TREE_MODEL (store), b,
		JANA_GTK_NOTE_STORE_COL_CREATED, &created2, -1);
	
	if (created1 && created2)
		result = jana_utils_time_compare (created1, created2, FALSE);
	
	if (created1) g_object_unref (created1);
	if (created2) g_object_unref (created2);
	
	return result;
}

static void
jana_gtk_note_store_init (JanaGtkNoteStore *self)
{
	JanaGtkNoteStorePrivate *priv = NOTE_STORE_PRIVATE (self);

	priv->view = NULL;
	priv->notes_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
		g_free, note_store_free_iter);
	
	gtk_list_store_set_column_types (GTK_LIST_STORE (self),
		JANA_GTK_NOTE_STORE_COL_LAST,
		(GType []){G_TYPE_STRING,	/* UID */
			   G_TYPE_STRV,		/* CATEGORIES */
			   G_TYPE_STRING,	/* AUTHOR */
			   G_TYPE_STRING,	/* RECIPIENT */
			   G_TYPE_STRING,	/* BODY */
			   G_TYPE_OBJECT,	/* CREATED */
			   G_TYPE_OBJECT,	/* MODIFIED */
		});
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (self),
		JANA_GTK_NOTE_STORE_COL_CREATED, jana_gtk_note_store_compare,
		self, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (self),
		JANA_GTK_NOTE_STORE_COL_CREATED, GTK_SORT_ASCENDING);
}

GtkTreeModel *
jana_gtk_note_store_new (void)
{
	return (GtkTreeModel *)g_object_new (JANA_GTK_TYPE_NOTE_STORE, NULL);
}

GtkTreeModel *
jana_gtk_note_store_new_full (JanaStoreView *view)
{
	return (GtkTreeModel *)g_object_new (JANA_GTK_TYPE_NOTE_STORE, "view", view, NULL);
}

static void
note_store_added_cb (JanaStoreView *view, GList *components,
		     JanaGtkNoteStore *store)
{
	JanaGtkNoteStorePrivate *priv = NOTE_STORE_PRIVATE (store);

	for (; components; components = components->next) {
		gchar *uid, **categories, *author, *recipient, *body;
		JanaTime *creation, *modified;
		JanaNote *note;
		GtkTreeIter *iter;
		
		if (jana_component_get_component_type (JANA_COMPONENT (
		    components->data)) != JANA_COMPONENT_NOTE) {
			g_warning ("Non-note component encountered");
			continue;
		}
		
		note = JANA_NOTE (components->data);
		iter = g_slice_new (GtkTreeIter);
		uid = jana_component_get_uid (JANA_COMPONENT (note));
		categories = jana_component_get_categories (
			JANA_COMPONENT (note));
		author = jana_note_get_author (note);
		recipient = jana_note_get_recipient (note);
		body = jana_note_get_body (note);
		creation = jana_note_get_creation_time (note);
		modified = jana_note_get_modified_time (note);
		
		gtk_list_store_insert_with_values (
			GTK_LIST_STORE (store), iter, 0,
			JANA_GTK_NOTE_STORE_COL_UID, uid,
			JANA_GTK_NOTE_STORE_COL_CATEGORIES, categories,
			JANA_GTK_NOTE_STORE_COL_AUTHOR, author,
			JANA_GTK_NOTE_STORE_COL_RECIPIENT, recipient,
			JANA_GTK_NOTE_STORE_COL_BODY, body,
			JANA_GTK_NOTE_STORE_COL_CREATED, creation,
			JANA_GTK_NOTE_STORE_COL_MODIFIED, modified, -1);
		
		g_hash_table_insert (priv->notes_hash, uid, iter);
		
		g_strfreev (categories);
		g_free (author);
		g_free (recipient);
		g_free (body);
		if (creation) g_object_unref (creation);
		if (modified) g_object_unref (modified);
	}
}

static void
note_store_modified_cb (JanaStoreView *view, GList *components,
			JanaGtkNoteStore *store)
{
	JanaGtkNoteStorePrivate *priv = NOTE_STORE_PRIVATE (store);

	for (; components; components = components->next) {
		gchar *uid, **categories, *author, *recipient, *body;
		JanaTime *creation, *modified;
		JanaNote *note;
		GtkTreeIter *iter;
		
		uid = jana_component_get_uid (
			JANA_COMPONENT (components->data));
		
		if (!(iter = g_hash_table_lookup (priv->notes_hash, uid))) {
			GList *one_comp;
			g_free (uid);
			g_warning ("Encountered unrecognised component, adding "
				"to local store");
			one_comp = g_list_prepend (NULL, components->data);
			note_store_added_cb (view, one_comp, store);
			continue;
		}
		
		note = JANA_NOTE (components->data);
		categories = jana_component_get_categories (
			JANA_COMPONENT (note));
		author = jana_note_get_author (note);
		recipient = jana_note_get_recipient (note);
		body = jana_note_get_body (note);
		creation = jana_note_get_creation_time (note);
		modified = jana_note_get_modified_time (note);
		
		gtk_list_store_set (GTK_LIST_STORE (store), iter,
			JANA_GTK_NOTE_STORE_COL_CATEGORIES, categories,
			JANA_GTK_NOTE_STORE_COL_AUTHOR, author,
			JANA_GTK_NOTE_STORE_COL_RECIPIENT, recipient,
			JANA_GTK_NOTE_STORE_COL_BODY, body,
			JANA_GTK_NOTE_STORE_COL_CREATED, creation,
			JANA_GTK_NOTE_STORE_COL_MODIFIED, modified, -1);

		g_free (uid);
		g_strfreev (categories);
		g_free (author);
		g_free (recipient);
		g_free (body);
		if (creation) g_object_unref (creation);
		if (modified) g_object_unref (modified);
	}
}

static void
note_store_removed_cb (JanaStoreView *view, GList *uids,
		       JanaGtkNoteStore *store)
{
	JanaGtkNoteStorePrivate *priv = NOTE_STORE_PRIVATE (store);

	for (; uids; uids = uids->next) {
		GtkTreeIter *iter;
		const gchar *uid = (const gchar *)uids->data;

		if (!(iter = g_hash_table_lookup (priv->notes_hash, uid))) {
			g_warning ("Unrecognised component removed from view");
			continue;
		}
		
		gtk_list_store_remove (GTK_LIST_STORE (store), iter);
		g_hash_table_remove (priv->notes_hash, uid);
	}
}

void
jana_gtk_note_store_set_view (JanaGtkNoteStore *store, JanaStoreView *view)
{
	JanaGtkNoteStorePrivate *priv = NOTE_STORE_PRIVATE (store);
	
	if (priv->view) {
		g_signal_handlers_disconnect_by_func (
			priv->view, note_store_added_cb, store);
		g_signal_handlers_disconnect_by_func (
			priv->view, note_store_modified_cb, store);
		g_signal_handlers_disconnect_by_func (
			priv->view, note_store_removed_cb, store);
		g_object_unref (priv->view);
		priv->view = NULL;
		g_hash_table_remove_all (priv->notes_hash);
		gtk_list_store_clear (GTK_LIST_STORE (store));
	}
	if (view) {
		priv->view = g_object_ref (view);
		g_signal_connect (priv->view, "added",
			G_CALLBACK (note_store_added_cb), store);
		g_signal_connect (priv->view, "modified",
			G_CALLBACK (note_store_modified_cb), store);
		g_signal_connect (priv->view, "removed",
			G_CALLBACK (note_store_removed_cb), store);
	}
}

JanaStoreView *
jana_gtk_note_store_get_view (JanaGtkNoteStore *store)
{
	JanaGtkNoteStorePrivate *priv = NOTE_STORE_PRIVATE (store);
	
	return priv->view ? g_object_ref (priv->view) : NULL;
}

JanaStore *
jana_gtk_note_store_get_store (JanaGtkNoteStore *self)
{
	JanaGtkNoteStorePrivate *priv = NOTE_STORE_PRIVATE (self);
	
	if (priv->view) {
		return jana_store_view_get_store (priv->view);
	} else return NULL;
}
