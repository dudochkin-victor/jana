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
 * SECTION:jana-ecal-store
 * @short_description: An implementation of #JanaStore using libecal
 *
 * #JanaEcalStore is an implementation of #JanaStore that provides a 
 * wrapper over #ECal, using libecal.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define HANDLE_LIBICAL_MEMORY 1

#include <libjana/jana-utils.h>
#include <libedataserverui/libedataserverui.h>
#include "jana-ecal-component.h"
#include "jana-ecal-event.h"
#include "jana-ecal-note.h"
#include "jana-ecal-task.h"
#include "jana-ecal-store-view.h"
#include "jana-ecal-store.h"

static gint init_passwords = 0;

static void store_interface_init (gpointer g_iface, gpointer iface_data);

static void		store_open			(JanaStore *self);

static JanaComponent *	 store_get_component		(JanaStore *self,
							 const gchar *uid);

static JanaStoreView *	 store_get_view			(JanaStore *self);

static void	store_add_component	(JanaStore *self, JanaComponent *comp);
static void	store_modify_component	(JanaStore *self, JanaComponent *comp);
static void	store_remove_component	(JanaStore *self, JanaComponent *comp);

static void	store_cal_opened_cb	(ECal *ecal, gint arg1,
					 JanaStore *self);
static gchar *	auth_func_cb 		(ECal       *ecal,
					 const gchar *prompt,
					 const gchar *key,
					 gpointer    user_data);

G_DEFINE_TYPE_WITH_CODE (JanaEcalStore,
                        jana_ecal_store, 
                        G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE (JANA_TYPE_STORE,
                                               store_interface_init));

#define STORE_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
			  JANA_ECAL_TYPE_STORE, JanaEcalStorePrivate))

typedef struct _JanaEcalStorePrivate JanaEcalStorePrivate;

struct _JanaEcalStorePrivate
{
	ECal *ecal;
	JanaComponentType type;
};

enum {
	PROP_ECAL = 1,
	PROP_TYPE,
};

static void
jana_ecal_store_get_property (GObject *object, guint property_id,
			      GValue *value, GParamSpec *pspec)
{
	JanaEcalStorePrivate *priv = STORE_PRIVATE (object);

	switch (property_id) {
	    case PROP_ECAL :
		g_value_set_object (value, priv->ecal);
		break;
	    case PROP_TYPE :
		g_value_set_int (value, priv->type);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_ecal_store_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
	JanaEcalStorePrivate *priv = STORE_PRIVATE (object);

	switch (property_id) {
	    case PROP_ECAL :
		priv->ecal = E_CAL (g_value_dup_object (value));
		/*e_cal_set_auth_func (priv->ecal, auth_func_cb, NULL);
		g_signal_connect (G_OBJECT (priv->ecal), "cal-opened",
			G_CALLBACK (store_cal_opened_cb), object);*/  //DV
		break;
	    case PROP_TYPE :
		priv->type = g_value_get_int (value);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_ecal_store_dispose (GObject *object)
{
	JanaEcalStorePrivate *priv = STORE_PRIVATE (object);

	if (priv->ecal) {
		g_object_unref (priv->ecal);
		priv->ecal = NULL;
	}

	if (G_OBJECT_CLASS (jana_ecal_store_parent_class)->dispose)
		G_OBJECT_CLASS (jana_ecal_store_parent_class)->dispose (object);

	if (g_atomic_int_dec_and_test (&init_passwords))
		e_passwords_shutdown ();
}

static void
jana_ecal_store_finalize (GObject *object)
{
	G_OBJECT_CLASS (jana_ecal_store_parent_class)->finalize (object);
}

static void
store_interface_init (gpointer g_iface, gpointer iface_data)
{
	JanaStoreInterface *iface = (JanaStoreInterface *)g_iface;
	
	iface->open = store_open;
	
	iface->get_component = store_get_component;
	
	iface->get_view = store_get_view;
	
	iface->add_component = store_add_component;
	iface->modify_component = store_modify_component;
	iface->remove_component = store_remove_component;
}

static void
jana_ecal_store_class_init (JanaEcalStoreClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (JanaEcalStorePrivate));

	object_class->get_property = jana_ecal_store_get_property;
	object_class->set_property = jana_ecal_store_set_property;
	object_class->dispose = jana_ecal_store_dispose;
	object_class->finalize = jana_ecal_store_finalize;

	g_object_class_install_property (
		object_class,
		PROP_ECAL,
		g_param_spec_object (
			"ecal",
			"ECal *",
			"The ECal represented by this JanaStore object.",
			E_TYPE_CAL,
			G_PARAM_READABLE | G_PARAM_WRITABLE |
			G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (
		object_class,
		PROP_TYPE,
		g_param_spec_int (
			"type",
			"JanaComponentType",
			"The component type stored in this JanaStore object.",
			JANA_COMPONENT_NULL, JANA_COMPONENT_TASK,
			JANA_COMPONENT_NULL,
			G_PARAM_READABLE | G_PARAM_WRITABLE |
			G_PARAM_CONSTRUCT_ONLY));
}

static void
jana_ecal_store_init (JanaEcalStore *self)
{
	/*JanaEcalStorePrivate *priv = STORE_PRIVATE (self);*/
}

#ifndef HAVE_ECAL_NEW_SYSTEM_MEMOS
/* Taken from eds libecal */
static ECal *
e_cal_new_system_memos (void)
{
	ECal *ecal;
	char *uri;
	char *filename;

	filename = g_build_filename (g_get_home_dir (),
				     ".evolution/memos/local/system",
				     NULL);
	uri = g_filename_to_uri (filename, NULL, NULL);
	g_free (filename);
	/*ecal = e_cal_new_from_uri (uri, E_CAL_SOURCE_TYPE_JOURNAL);*/ //DV
	g_free (uri);
	
	/*return ecal;*/ //DV
	return NULL; //DV
}
#endif

/**
 * jana_ecal_store_new:
 * @type: The type of store to create/open
 *
 * Opens or creates the default evolution-data-server system storage for the 
 * specified type of component.
 *
 * Returns: A #JanaEcalStore that wraps the evolution-data-server system 
 * storage of the specified component type, cast as a #JanaStore.
 */
JanaStore *
jana_ecal_store_new (JanaComponentType type)
{
	ECal *ecal;
	
	switch (type) {
	    /*se JANA_COMPONENT_EVENT :
		ecal = e_cal_new_system_calendar ();
		break;
	    case JANA_COMPONENT_NOTE :
		ecal = e_cal_new_system_memos ();
		break;
	    case JANA_COMPONENT_TASK :
		ecal = e_cal_new_system_tasks ();
		break;*/ //DV
	    default :
		g_warning ("%s called with invalid type", G_STRFUNC);
		return NULL;
	}
	
	g_atomic_int_inc (&init_passwords);
	
	if (g_atomic_int_get(&init_passwords) == 1) {
		e_passwords_init ();
	}
	return JANA_STORE (g_object_new (JANA_ECAL_TYPE_STORE,
		"ecal", ecal, "type", type, NULL));
}

static gchar *
auth_func_cb (ECal       *ecal,
	      const gchar *prompt,
	      const gchar *key,
	      gpointer    user_data)
{
	ESource *source;
	const gchar *auth_domain;
	const gchar *component_name;

	source = e_cal_get_source (ecal);
	/*auth_domain = e_source_get_property (source, "auth-domain");*/ //DV
	component_name = auth_domain ? auth_domain : "Calendar";

	return e_passwords_get_password (component_name, key);
}

static gboolean
store_cal_opened_signal_cb (JanaStore *self)
{
	g_signal_emit_by_name (self, "opened");
	return FALSE;
}

static void
store_cal_opened_cb (ECal *ecal, gint arg1, JanaStore *self)
{
	/* This happens in a thread, so use g_idle_add to get back to the
	 * main thread.
	 */
	g_idle_add ((GSourceFunc)store_cal_opened_signal_cb, self);
}

/**
 * jana_ecal_store_new_from_uri:
 * @uri: The uri to the store
 * @type: The type of store to create/open
 *
 * Opens or creates an evolution-data-server storage at the given uri for the 
 * specified type of component.
 *
 * Returns: A #JanaEcalStore that wraps an evolution-data-server storage of 
 * the specified component type at the given uri, cast as a #JanaStore.
 */
JanaStore *
jana_ecal_store_new_from_uri (const gchar *uri, JanaComponentType type)
{
	ECal *ecal;
	ECalSourceType etype;
	
	switch (type) {
	    case JANA_COMPONENT_EVENT :
		etype = E_CAL_SOURCE_TYPE_EVENT;
		break;
	    case JANA_COMPONENT_NOTE :
		etype = E_CAL_SOURCE_TYPE_JOURNAL;
		break;
	    case JANA_COMPONENT_TASK :
		etype = E_CAL_SOURCE_TYPE_TODO;
		break;
	    default :
		g_warning ("%s called with invalid type", G_STRFUNC);
		return NULL;
	}
	
	/*if ((ecal = e_cal_new_from_uri (uri, etype))) {
		e_cal_set_auth_func (ecal, auth_func_cb, NULL);
		return JANA_STORE (g_object_new (JANA_ECAL_TYPE_STORE,
			"ecal", ecal, "type", type, NULL));
	} else */ //DV
	{
		g_warning ("Could not create ECal in %s", G_STRFUNC);
		return NULL;
	}
}

/**
 * jana_ecal_store_get_uri
 * @store: The store to get the uri of.
 *
 * Returns: The uri of the calendar that this store represents.
 */
const gchar *
jana_ecal_store_get_uri (JanaEcalStore *store)
{
	JanaEcalStorePrivate *priv = STORE_PRIVATE (store);

	g_return_val_if_fail (JANA_ECAL_IS_STORE (store), NULL);

	//return e_cal_get_uri (priv->ecal);
	return NULL;//DV
}

static void
store_open (JanaStore *self)
{
	JanaEcalStorePrivate *priv = STORE_PRIVATE (self);

	e_cal_open_async (priv->ecal, FALSE);
}

/* TODO: Test this function */
static JanaComponent *
store_get_component (JanaStore *self, const gchar *uid)
{
	GList *comps;
	JanaComponent *component;
	GError *error = NULL;
	JanaEcalStorePrivate *priv = STORE_PRIVATE (self);
	
	if (!e_cal_get_objects_for_uid (priv->ecal, uid, &comps, &error)) {
		g_warning ("Unable to retrieve event: %s", error->message);
		g_error_free (error);
		return NULL;
	}
	
	if (!comps) return NULL;
	
	switch (e_cal_component_get_vtype (E_CAL_COMPONENT (comps->data))) {
	    case E_CAL_COMPONENT_EVENT :
		component = JANA_COMPONENT (jana_ecal_event_new_from_ecalcomp (
			E_CAL_COMPONENT (comps->data)));
		break;
	    case E_CAL_COMPONENT_TODO :
	    case E_CAL_COMPONENT_JOURNAL :
		component = JANA_COMPONENT (jana_ecal_note_new_from_ecalcomp (
			E_CAL_COMPONENT (comps->data)));
		break;
	    default:
		component = jana_ecal_component_new_from_ecalcomp (
			E_CAL_COMPONENT (comps->data));
		break;
	}

	while (comps) {
		g_object_unref (comps->data);
		comps = g_list_delete_link (comps, comps);
	}
	
	return component;
}

static JanaStoreView *
store_get_view (JanaStore *self)
{
	return jana_ecal_store_view_new (JANA_ECAL_STORE (self));
}

static JanaEcalComponent *
get_jana_ecal_comp (JanaStore *self, JanaComponent *comp)
{
	/*JanaEcalStorePrivate *priv = STORE_PRIVATE (self);*/

	switch (jana_component_get_component_type (comp)) {
	    case JANA_COMPONENT_EVENT : {
		JanaEcalEvent *event;
		if (!JANA_ECAL_IS_EVENT (comp)) {
			event = JANA_ECAL_EVENT (
				jana_utils_event_copy (JANA_EVENT (comp),
				jana_ecal_event_new ()));
		} else {
			event = g_object_ref (JANA_ECAL_EVENT (
				g_object_ref (comp)));
		}
		
		return JANA_ECAL_COMPONENT (event);
		break;
	    }
	    case JANA_COMPONENT_NOTE : {
		JanaEcalNote *note;
		if (!JANA_ECAL_IS_NOTE (comp)) {
			note = JANA_ECAL_NOTE (
				jana_utils_note_copy (JANA_NOTE (comp),
				jana_ecal_note_new ()));
		} else {
			note = g_object_ref (JANA_ECAL_NOTE (
				g_object_ref (comp)));
		}
		
		return JANA_ECAL_COMPONENT (note);
		break;
	    }
	    case JANA_COMPONENT_TASK : {
		JanaEcalTask *task;
		if (!JANA_ECAL_IS_TASK (comp)) {
			task = JANA_ECAL_TASK (
				jana_utils_task_copy (JANA_TASK (comp),
				jana_ecal_task_new ()));
		} else {
			task = g_object_ref (JANA_ECAL_TASK (
				g_object_ref (comp)));
		}
		
		return JANA_ECAL_COMPONENT (task);
		break;
	    }
	    default :
		g_warning ("%s called with invalid component type", G_STRFUNC);
		return NULL;
	}
}

static void
store_add_component (JanaStore *self, JanaComponent *comp)
{
	JanaEcalComponent *jcomp;
	ECalComponent *ecomp;
	icalcomponent *icalcomp;
	GError *error = NULL;
	char *uid;
	JanaEcalStorePrivate *priv = STORE_PRIVATE (self);

	if (!(jcomp = get_jana_ecal_comp (self, comp))) return;
	
	g_object_get (jcomp, "ecalcomp", &ecomp, NULL);

	/* Reset the UID to avoid collisions when adding the same JanaComponent
	 * object multiple times.
	 */
	uid = e_cal_component_gen_uid ();
	e_cal_component_set_uid (ecomp, uid);
	g_free (uid);
	
	icalcomp = e_cal_component_get_icalcomponent (ecomp);
	if (!e_cal_create_object (priv->ecal, icalcomp,
	     &uid, &error)) {
		g_warning ("Error adding component to store: %s",
			error->message);
	} else {
		/* Note, older versions of eds don't set the uid on object
		 * creation. This is fixed in eds-dbus and svn eds.
		 */
		icalcomponent_set_uid (icalcomp, uid);
	}
	
	g_object_unref (ecomp);
	g_object_unref (jcomp);
}

static void
store_modify_component (JanaStore *self, JanaComponent *comp)
{
	JanaEcalComponent *jcomp;
	ECalComponent *ecomp;
	GError *error = NULL;
	JanaEcalStorePrivate *priv = STORE_PRIVATE (self);

	if (!(jcomp = get_jana_ecal_comp (self, comp))) return;
	
	g_object_get (jcomp, "ecalcomp", &ecomp, NULL);
	if (!e_cal_modify_object (priv->ecal,
	     e_cal_component_get_icalcomponent (ecomp),
	     CALOBJ_MOD_ALL, &error)) {
		g_warning ("Error modifying component: %s",
			error->message);
	}
	
	g_object_unref (ecomp);
	g_object_unref (jcomp);
}

static void
store_remove_component (JanaStore *self, JanaComponent *comp)
{
	gchar *uid;
	GError *error = NULL;
	JanaEcalStorePrivate *priv = STORE_PRIVATE (self);

	uid = jana_component_get_uid (comp);
	
	if (!(e_cal_remove_object (priv->ecal, uid, &error))) {
		g_warning ("Error removing component: %s",
			error->message);
	}
	
	g_free (uid);
}

