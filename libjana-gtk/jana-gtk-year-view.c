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
#include <locale.h>
#include <langinfo.h>
#include "jana-gtk-year-view.h"

G_DEFINE_TYPE (JanaGtkYearView, jana_gtk_year_view, GTK_TYPE_TABLE)

#define YEAR_VIEW_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), JANA_GTK_TYPE_YEAR_VIEW, \
	JanaGtkYearViewPrivate))

typedef struct _JanaGtkYearViewPrivate JanaGtkYearViewPrivate;

struct _JanaGtkYearViewPrivate {
	/* The event box containing the month labels and such */
	GtkWidget *boxes[12];
	
	/* The month labels */
	GtkWidget *month_labels[12];

	/* The labels showing how many events in the month there are */
	GtkWidget *events_labels[12];
	
	/* Amount of events in a particular month */
	guint events[12];
	
	JanaTime *year;
	gint selected_month;

	/* List of JanaGtkEventStore's */
	GList *stores;
	
	JanaTime *highlighted_time;
};

enum {
	PROP_MONTHS_PER_ROW = 1,
	PROP_YEAR,
	PROP_HIGHLIGHTED_TIME,
};

enum {
	SELECTION_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
recount_events (JanaGtkYearView *self)
{
	gint month, year;
	GList *stores;
	JanaGtkYearViewPrivate *priv = YEAR_VIEW_PRIVATE (self);
	
	/* Reset event count */
	for (month = 0; month < 12; month++) priv->events[month] = 0;
	
	year = priv->year ? jana_time_get_year (priv->year) : 0;
	
	/* Iterate through all stores and count events per month */
	for (stores = priv->stores; stores; stores = stores->next) {
		GtkTreeIter iter;
		JanaGtkEventStore *store = (JanaGtkEventStore *)stores->data;
		if (gtk_tree_model_get_iter_first (
		    (GtkTreeModel *)store, &iter)) do {
			JanaTime *start, *old_start;
			
			gtk_tree_model_get ((GtkTreeModel *)store, &iter,
				JANA_GTK_EVENT_STORE_COL_START, &old_start, -1);
			if (!old_start) continue;
			
			/* Adjust for timezones */
			start = jana_time_duplicate (old_start);
			jana_time_set_offset (start,
				jana_time_get_offset (priv->year));
			
			if (jana_time_get_year (start) == year)
				priv->events[jana_time_get_month (start)-1] ++;
			
			g_object_unref (start);
			g_object_unref (old_start);
		} while (gtk_tree_model_iter_next (
			 (GtkTreeModel *)store, &iter));
	}
	
	/* Set labels */
	for (month = 0; month < 12; month++) {
		if (priv->events[month]) {
			gchar *number = g_strdup_printf (
				"%d", priv->events[month]);
			gtk_label_set_text (GTK_LABEL (
				priv->events_labels[month]), number);
			g_free (number);
		} else {
			gtk_label_set_text (GTK_LABEL (
				priv->events_labels[month]), "-");
		}
	}
}

static void
jana_gtk_year_view_get_property (GObject *object, guint property_id,
				 GValue *value, GParamSpec *pspec)
{
	JanaGtkYearViewPrivate *priv = YEAR_VIEW_PRIVATE (object);

	switch (property_id) {
	    case PROP_YEAR :
		if (priv->year) g_value_take_object (
			value, jana_time_duplicate (priv->year));
		break;
	    case PROP_HIGHLIGHTED_TIME :
		if (priv->highlighted_time)
			g_value_take_object (value,
				jana_time_duplicate (priv->highlighted_time));
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_year_view_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
	JanaGtkYearView *year_view = JANA_GTK_YEAR_VIEW (object);

	switch (property_id) {
	    case PROP_YEAR :
		jana_gtk_year_view_set_year (year_view, g_value_get_object (value));
		break;
	    case PROP_MONTHS_PER_ROW :
		jana_gtk_year_view_set_months_per_row (
			JANA_GTK_YEAR_VIEW (object), g_value_get_uint (value));
		break;
	    case PROP_HIGHLIGHTED_TIME :
		jana_gtk_year_view_set_highlighted_time (
			JANA_GTK_YEAR_VIEW (object), g_value_get_object (value));
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
row_deleted_cb (GtkTreeModel *tree_model, GtkTreePath *path,
		JanaGtkYearView *self)
{
	recount_events (self);
}

static void
row_changed_cb (GtkTreeModel *tree_model, GtkTreePath *path,
		GtkTreeIter *iter, JanaGtkYearView *self)
{
	recount_events (self);
}

static void
row_inserted_cb (GtkTreeModel *tree_model, GtkTreePath *path,
		 GtkTreeIter *iter, JanaGtkYearView *self)
{
	/* TODO: Just consider this one row rather than recounting all the
	 *       events.
	 */
	recount_events (self);
}

static void
jana_gtk_year_view_dispose (GObject *object)
{
	gint i;
	
	JanaGtkYearViewPrivate *priv = YEAR_VIEW_PRIVATE (object);

	for (i = 0; i < 12; i++) {
		if (priv->boxes[i]) {
			g_object_unref (priv->boxes[i]);
			priv->boxes[i] = NULL;
		}
	}
	
	while (priv->stores) {
		GObject *store = (GObject *)priv->stores->data;
		g_signal_handlers_disconnect_by_func (
			store, row_inserted_cb, object);
		g_signal_handlers_disconnect_by_func (
			store, row_changed_cb, object);
		g_signal_handlers_disconnect_by_func (
			store, row_deleted_cb, object);
		g_object_unref (store);
		priv->stores = g_list_delete_link (priv->stores, priv->stores);
	}
	
	if (G_OBJECT_CLASS (jana_gtk_year_view_parent_class)->dispose)
		G_OBJECT_CLASS (jana_gtk_year_view_parent_class)->
			dispose (object);
}

static void
jana_gtk_year_view_finalize (GObject *object)
{
	JanaGtkYearViewPrivate *priv = YEAR_VIEW_PRIVATE (object);

	if (priv->highlighted_time) {
		g_object_unref (priv->highlighted_time);
		priv->highlighted_time = NULL;
	}
	
	G_OBJECT_CLASS (jana_gtk_year_view_parent_class)->finalize (object);
}

static void
jana_gtk_year_view_class_init (JanaGtkYearViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (JanaGtkYearViewPrivate));

	object_class->get_property = jana_gtk_year_view_get_property;
	object_class->set_property = jana_gtk_year_view_set_property;
	object_class->dispose = jana_gtk_year_view_dispose;
	object_class->finalize = jana_gtk_year_view_finalize;

	g_object_class_install_property (
		object_class,
		PROP_MONTHS_PER_ROW,
		g_param_spec_uint (
			"months_per_row",
			"Months per row",
			"Months to display per row.",
			1, 12, 3,
			G_PARAM_WRITABLE));

	g_object_class_install_property (
		object_class,
		PROP_YEAR,
		g_param_spec_object (
			"year",
			"Year",
			"Year to count events in.",
			G_TYPE_OBJECT,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_HIGHLIGHTED_TIME,
		g_param_spec_object (
			"highlighted_time",
			"Highlighted time",
			"A time that should be highlighted, "
			"for example, the current time.",
			G_TYPE_OBJECT,
			G_PARAM_READWRITE));

	signals[SELECTION_CHANGED] =
		g_signal_new ("selection_changed",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaGtkYearViewClass,
					 selection_changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__OBJECT,
			G_TYPE_NONE, 1, G_TYPE_INT);
}

static gboolean
button_press_event_cb (GtkWidget *widget, GdkEventButton *event,
		       JanaGtkYearView *self)
{
	gint month;
	
	JanaGtkYearViewPrivate *priv = YEAR_VIEW_PRIVATE (self);
	
	month = GPOINTER_TO_INT (g_object_get_data (
		G_OBJECT (widget), "month"));
	if ((month) && (month != priv->selected_month)) {
		if (priv->selected_month) {
			priv->selected_month --;
			gtk_widget_set_state (priv->boxes[
				priv->selected_month], GTK_STATE_NORMAL);
			gtk_widget_set_state (priv->month_labels[
				priv->selected_month], GTK_STATE_NORMAL);
			gtk_widget_set_state (priv->events_labels[
				priv->selected_month], GTK_STATE_NORMAL);
		}
		priv->selected_month = month;
		month --;
		gtk_widget_set_state (priv->boxes[month],
			GTK_STATE_SELECTED);
		gtk_widget_set_state (priv->month_labels[month],
			GTK_STATE_SELECTED);
		gtk_widget_set_state (priv->events_labels[month],
			GTK_STATE_SELECTED);

		g_signal_emit (self, signals[SELECTION_CHANGED], 0,
			priv->selected_month);
	}
	
	return FALSE;
}

static void
regenerate_labels (JanaGtkYearView *self)
{
	gint month;
	JanaGtkYearViewPrivate *priv = YEAR_VIEW_PRIVATE (self);

	for (month = 0; month < 12; month++) {
		GtkLabel *label = GTK_LABEL (priv->month_labels[month]);
		
		if (priv->highlighted_time && priv->year &&
		    (jana_time_get_month (
		     priv->highlighted_time) == (month + 1)) &&
		    (jana_time_get_year (priv->year) ==
		     jana_time_get_year (priv->highlighted_time))) {
			gchar *markup;
			
			gtk_label_set_use_markup (label, TRUE);
			markup = g_strdup_printf ("<b>%s</b>",
				nl_langinfo (ABMON_1+month));
			gtk_label_set_markup (label, markup);
			g_free (markup);
		} else {
			gtk_label_set_use_markup (label, FALSE);
			gtk_label_set_text (label, nl_langinfo (ABMON_1+month));
		}
	}
}

static void
jana_gtk_year_view_init (JanaGtkYearView *self)
{
	gint month;
	static gboolean locale_set = FALSE;
	JanaGtkYearViewPrivate *priv = YEAR_VIEW_PRIVATE (self);

	if (!locale_set) {
		setlocale (LC_TIME, "");
		locale_set = TRUE;
	}
	
	for (month = 0; month < 12; month++) {
		GtkWidget *vbox;
		
		priv->boxes[month] = g_object_ref (gtk_event_box_new ());
		priv->month_labels[month] = gtk_label_new (NULL);
		priv->events_labels[month] = gtk_label_new ("-");
		priv->events[month] = 0;
		
		g_object_set_data (G_OBJECT (priv->boxes[month]),
			"month", GINT_TO_POINTER (month + 1));
		
		vbox = gtk_vbox_new (FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), priv->month_labels[month],
			TRUE, TRUE, 0);
		gtk_box_pack_end (GTK_BOX (vbox), priv->events_labels[month],
			TRUE, TRUE, 0);
		
		gtk_container_add (GTK_CONTAINER (priv->boxes[month]), vbox);
		gtk_widget_show_all (priv->boxes[month]);

		g_signal_connect (priv->boxes[month], "button-press-event",
			G_CALLBACK (button_press_event_cb), self);
	}
	
	regenerate_labels (self);
}

/**
 * jana_gtk_year_view_new:
 * @months_per_row: Number of months to pack into a row
 * @year: The year to count events in
 *
 * Create a new widget that provides a summary of events over @year.
 *
 * Returns: A new #JanaGtkYearView widget.
 */
GtkWidget *
jana_gtk_year_view_new (guint months_per_row, JanaTime *year)
{
	return GTK_WIDGET (g_object_new (JANA_GTK_TYPE_YEAR_VIEW,
		"months_per_row", months_per_row, "year", year, NULL));
}

/**
 * jana_gtk_year_view_add_store:
 * @self: A #JanaGtkYearView
 * @store: A #JanaGtkEventStore
 *
 * Adds @store to the list of #JanaGtkEventStore objects used in visualising
 * the year.
 */
void
jana_gtk_year_view_add_store (JanaGtkYearView *self, JanaGtkEventStore *store)
{
	JanaGtkYearViewPrivate *priv = YEAR_VIEW_PRIVATE (self);
	
	priv->stores = g_list_prepend (priv->stores, g_object_ref (store));
	
	recount_events (self);
	
	g_signal_connect (store, "row-inserted",
		G_CALLBACK (row_inserted_cb), self);
	g_signal_connect (store, "row-changed",
		G_CALLBACK (row_changed_cb), self);
	g_signal_connect (store, "row-deleted",
		G_CALLBACK (row_deleted_cb), self);
}

/**
 * jana_gtk_year_view_remove_store:
 * @self: A #JanaGtkYearView
 * @store: A #JanaGtkEventStore
 *
 * Removes @store from the list of #JanaGtkEventStore objects used in 
 * visualising the year. If @store has not previously been added with 
 * jana_gtk_year_view_add_store(), nothing happens.
 */
void
jana_gtk_year_view_remove_store (JanaGtkYearView *self,
				 JanaGtkEventStore *store)
{
	GList *link;
	JanaGtkYearViewPrivate *priv = YEAR_VIEW_PRIVATE (self);
	
	if (!(link = g_list_find (priv->stores, store))) return;
	
	g_signal_handlers_disconnect_by_func (store, row_inserted_cb, self);
	g_signal_handlers_disconnect_by_func (store, row_changed_cb, self);
	g_signal_handlers_disconnect_by_func (store, row_deleted_cb, self);
	
	g_object_unref (store);
	
	priv->stores = g_list_delete_link (priv->stores, link);
	
	recount_events (self);
}

/**
 * jana_gtk_year_view_set_year:
 * @self: A #JanaGtkYearView
 * @year: The year to count events in
 *
 * Set the year for @self to count events in.
 */
void
jana_gtk_year_view_set_year (JanaGtkYearView *self, JanaTime *year)
{
	JanaGtkYearViewPrivate *priv = YEAR_VIEW_PRIVATE (self);
	
	if (priv->year) {
		g_object_unref (priv->year);
		priv->year = NULL;
	}
	if (year) {
		priv->year = jana_time_duplicate (year);
		jana_time_set_isdate (priv->year, TRUE);
		jana_time_set_day (priv->year, 1);
		jana_time_set_month (priv->year, 1);
		regenerate_labels (self);
		recount_events (self);
	}
}

/**
 * jana_gtk_year_view_get_year:
 * @self: A #JanaGtkYearView
 *
 * Gets the year @self is counting events in.
 *
 * Returns: The year @self is counting events in.
 */
JanaTime *
jana_gtk_year_view_get_year (JanaGtkYearView *self)
{
	JanaGtkYearViewPrivate *priv = YEAR_VIEW_PRIVATE (self);
	return jana_time_duplicate (priv->year);
}

static void
container_remove_swapped (GtkWidget *widget, gpointer container)
{
	gtk_container_remove (GTK_CONTAINER (container), widget);
}

/**
 * jana_gtk_year_view_set_months_per_row:
 * @self: A #JanaGtkYearView
 * @months_per_row: Months to pack per row, between 1 and 12, inclusive
 *
 * Set the number of months for @self to pack into a row.
 */
void
jana_gtk_year_view_set_months_per_row (JanaGtkYearView *self,
				       guint months_per_row)
{
	gint month, cols;

	JanaGtkYearViewPrivate *priv = YEAR_VIEW_PRIVATE (self);

	g_assert ((months_per_row > 0) && (months_per_row <= 12));
	
	/* Empty container */
	gtk_container_foreach (GTK_CONTAINER (self),
		container_remove_swapped, self);
	
	/* Pack month boxes */
	cols = months_per_row;
	for (month = 0; month < 12; month++) {
		gtk_table_attach (GTK_TABLE (self),
			priv->boxes[month], month % cols,
			(month % cols) + 1, month / cols,
			(month / cols) + 1,
			GTK_EXPAND | GTK_FILL,
			GTK_EXPAND | GTK_FILL, 0, 0);
	}
}

/**
 * jana_gtk_year_view_get_selected_month:
 * @self: A #JanaGtkYearView
 *
 * Gets the currently selected month, between 0 and 12 inclusive. 0 
 * corresponds to there being no event selected.
 *
 * Returns: The selected month, 1 - 12, or 0 if there is no selected month.
 */
gint
jana_gtk_year_view_get_selected_month (JanaGtkYearView *self)
{
	JanaGtkYearViewPrivate *priv = YEAR_VIEW_PRIVATE (self);
	return priv->selected_month;
}

/**
 * jana_gtk_year_view_set_selected_month:
 * @self: A #JanaGtkYearView
 * @month: A month (1 - 12), or 0 to clear the selection
 *
 * Sets or clears the selected month.
 */
void
jana_gtk_year_view_set_selected_month (JanaGtkYearView *self, gint month)
{
	JanaGtkYearViewPrivate *priv = YEAR_VIEW_PRIVATE (self);
	
	g_assert ((month >= 0) && (month <= 12));
	
	if (priv->selected_month == month) return;
	
	if (month) {
		button_press_event_cb (priv->boxes[month-1], NULL, self);
	} else {
		priv->selected_month --;
		gtk_widget_set_state (priv->boxes[
			priv->selected_month], GTK_STATE_NORMAL);
		gtk_widget_set_state (priv->month_labels[
			priv->selected_month], GTK_STATE_NORMAL);
		gtk_widget_set_state (priv->events_labels[
			priv->selected_month], GTK_STATE_NORMAL);
		priv->selected_month = 0;
		g_signal_emit (self, signals[SELECTION_CHANGED], 0, 0);
	}
}

/**
 * jana_gtk_year_view_set_highlighted_time:
 * @self: A #JanaGtkYearView
 * @time: A #JanaTime
 *
 * Sets a time to be highlighted. This can be used to highlight the current
 * time, for example.
 */
void
jana_gtk_year_view_set_highlighted_time (JanaGtkYearView *self,
					 JanaTime *time)
{
	JanaGtkYearViewPrivate *priv = YEAR_VIEW_PRIVATE (self);
	
	if (priv->highlighted_time) {
		g_object_unref (priv->highlighted_time);
		priv->highlighted_time = NULL;
	}
	if (time) {
		priv->highlighted_time = jana_time_duplicate (time);
		jana_time_set_offset (priv->highlighted_time,
			jana_time_get_offset (priv->year));
	}
	
	regenerate_labels (self);
}
