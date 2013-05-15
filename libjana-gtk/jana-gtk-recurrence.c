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


#include "jana-gtk-recurrence.h"
#include <libjana/jana.h>
#include "jana-gtk-date-time.h"

G_DEFINE_TYPE (JanaGtkRecurrence, jana_gtk_recurrence, GTK_TYPE_VBOX)

#define RECURRENCE_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), JANA_GTK_TYPE_RECURRENCE, \
	JanaGtkRecurrencePrivate))

typedef struct _JanaGtkRecurrencePrivate JanaGtkRecurrencePrivate;

struct _JanaGtkRecurrencePrivate
{
	JanaRecurrence *recur;
	JanaTime *time;
	gboolean editable;
	
	GtkWidget *edit_vbox;
	GtkWidget *type_combo;
	GtkWidget *interval_dec_button;
	GtkWidget *interval_inc_button;
	GtkWidget *week_hbox;
	GtkToggleButton *week_buttons[7];
	GtkWidget *month_hbox;
	GtkToggleButton *byday_button;
	GtkToggleButton *bydate_button;
	GtkWidget *end_hbox;
	GtkWidget *end_button;
	GtkWidget *end_label;
	GtkWidget *preview_frame;
	GtkWidget *preview_textview;
};

enum {
	PROP_RECUR = 1,
	PROP_TIME,
	PROP_EDITABLE,
};

static void
fill_type_combo (JanaGtkRecurrence *self)
{
	gchar *recur_string;
	
	JanaGtkRecurrencePrivate *priv = RECURRENCE_PRIVATE (self);
	JanaRecurrence *recur = jana_recurrence_new ();
	
	if (priv->recur) recur->interval = priv->recur->interval;
	
	/* Daily */
	recur_string = jana_utils_recurrence_to_string (recur, NULL);
	gtk_combo_box_insert_text (GTK_COMBO_BOX (priv->type_combo), 1,
		recur_string);
	gtk_combo_box_remove_text (GTK_COMBO_BOX (priv->type_combo), 2);
	g_free (recur_string);

	/* Weekly */
	recur->type = JANA_RECURRENCE_WEEKLY;
	recur_string = jana_utils_recurrence_to_string (recur, NULL);
	gtk_combo_box_insert_text (GTK_COMBO_BOX (priv->type_combo), 2,
		recur_string);
	gtk_combo_box_remove_text (GTK_COMBO_BOX (priv->type_combo), 3);
	g_free (recur_string);

	/* Monthly */
	recur->type = JANA_RECURRENCE_MONTHLY;
	recur_string = jana_utils_recurrence_to_string (recur, NULL);
	gtk_combo_box_insert_text (GTK_COMBO_BOX (priv->type_combo), 3,
		recur_string);
	gtk_combo_box_remove_text (GTK_COMBO_BOX (priv->type_combo), 4);
	g_free (recur_string);

	/* Yearly */
	recur->type = JANA_RECURRENCE_YEARLY;
	recur_string = jana_utils_recurrence_to_string (recur, NULL);
	gtk_combo_box_insert_text (GTK_COMBO_BOX (priv->type_combo), 4,
		recur_string);
	gtk_combo_box_remove_text (GTK_COMBO_BOX (priv->type_combo), 5);
	g_free (recur_string);
	
	jana_recurrence_free (recur);
}

static void
refresh (JanaGtkRecurrence *self)
{
	gint i;
	gchar *recur_text;
	GtkTextBuffer *buffer;
	
	JanaGtkRecurrencePrivate *priv = RECURRENCE_PRIVATE (self);
	
	if ((!priv->recur) || (!priv->time))
		gtk_widget_hide (priv->end_hbox);
	else
		gtk_widget_show (priv->end_hbox);
	
	/* Update type selector combo box */
	fill_type_combo	(self);
	
	/* Update preview */
	recur_text = jana_utils_recurrence_to_string (priv->recur, priv->time);
	buffer = gtk_text_view_get_buffer (
		GTK_TEXT_VIEW (priv->preview_textview));
	gtk_text_buffer_set_text (buffer, recur_text, -1);
	
	/* Show/hide the appropriate editing widgets and set type combo */
	gtk_widget_hide (priv->week_hbox);
	gtk_widget_hide (priv->month_hbox);
	if (priv->recur) {
		if (priv->recur->end) {
			gchar *time_string = jana_utils_strftime (
				priv->recur->end, "%d/%m/%Y");
			gtk_label_set_text (GTK_LABEL (priv->end_label),
				time_string);
			g_free (time_string);
		} else {
			gtk_label_set_text (GTK_LABEL (priv->end_label),
				"Forever");
		}
		
		if (priv->recur->interval <= 1)
			gtk_widget_set_sensitive (
				priv->interval_dec_button, FALSE);
		else
			gtk_widget_set_sensitive (
				priv->interval_dec_button, TRUE);
		gtk_widget_set_sensitive (priv->interval_inc_button, TRUE);
		
		switch (priv->recur->type) {
		    case JANA_RECURRENCE_DAILY :
			gtk_combo_box_set_active (
				GTK_COMBO_BOX (priv->type_combo), 1);
			break;
		    case JANA_RECURRENCE_WEEKLY :
			for (i = 0; i < 7; i++) {
				gtk_toggle_button_set_active (
					priv->week_buttons[i],
					priv->recur->week_days[i]);
			}
			gtk_widget_show (priv->week_hbox);
			gtk_combo_box_set_active (
				GTK_COMBO_BOX (priv->type_combo), 2);
			break;
		    case JANA_RECURRENCE_MONTHLY :
			if (priv->recur->by_date)
				gtk_toggle_button_set_active (
					priv->bydate_button, TRUE);
			else
				gtk_toggle_button_set_active (
					priv->byday_button, TRUE);
			gtk_widget_show (priv->month_hbox);
			gtk_combo_box_set_active (
				GTK_COMBO_BOX (priv->type_combo), 3);
			break;
		    case JANA_RECURRENCE_YEARLY :
			gtk_combo_box_set_active (
				GTK_COMBO_BOX (priv->type_combo), 4);
			break;
		}
	} else {
		gtk_combo_box_set_active (GTK_COMBO_BOX (priv->type_combo), 0);
		gtk_widget_set_sensitive (priv->interval_dec_button, FALSE);
		gtk_widget_set_sensitive (priv->interval_inc_button, FALSE);
	}
}

static void
jana_gtk_recurrence_get_property (GObject *object, guint property_id,
				  GValue *value, GParamSpec *pspec)
{
	JanaGtkRecurrencePrivate *priv = RECURRENCE_PRIVATE (object);
	
	switch (property_id) {
	    case PROP_RECUR :
		g_value_set_boxed (value, priv->recur);
		break;
	    case PROP_TIME :
		g_value_set_object (value, priv->time);
		break;
	    case PROP_EDITABLE :
		g_value_set_boolean (value, priv->editable);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_recurrence_set_property (GObject *object, guint property_id,
				  const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	    case PROP_RECUR :
		jana_gtk_recurrence_set_recur (JANA_GTK_RECURRENCE (object),
			g_value_get_boxed (value));
		break;
	    case PROP_TIME :
		jana_gtk_recurrence_set_time (JANA_GTK_RECURRENCE (object),
			JANA_TIME (g_value_get_object (value)));
		break;
	    case PROP_EDITABLE :
		jana_gtk_recurrence_set_editable (JANA_GTK_RECURRENCE (object),
			g_value_get_boolean (value));
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_recurrence_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (jana_gtk_recurrence_parent_class)->dispose)
		G_OBJECT_CLASS (jana_gtk_recurrence_parent_class)->
			dispose (object);
}

static void
jana_gtk_recurrence_finalize (GObject *object)
{
	G_OBJECT_CLASS (jana_gtk_recurrence_parent_class)->finalize (object);
}

static void
jana_gtk_recurrence_style_set (GtkWidget *widget, GtkStyle *previous_style)
{
	JanaGtkRecurrencePrivate *priv = RECURRENCE_PRIVATE (widget);
	
	gtk_widget_modify_base (priv->preview_textview, GTK_STATE_NORMAL,
		&widget->style->bg[GTK_STATE_NORMAL]);
	
	GTK_WIDGET_CLASS (jana_gtk_recurrence_parent_class)->
		style_set (widget, previous_style);
}

static void
jana_gtk_recurrence_class_init (JanaGtkRecurrenceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	g_type_class_add_private (klass, sizeof (JanaGtkRecurrencePrivate));

	object_class->get_property = jana_gtk_recurrence_get_property;
	object_class->set_property = jana_gtk_recurrence_set_property;
	object_class->dispose = jana_gtk_recurrence_dispose;
	object_class->finalize = jana_gtk_recurrence_finalize;
	
	widget_class->style_set = jana_gtk_recurrence_style_set;

	g_object_class_install_property (
		object_class,
		PROP_RECUR,
		g_param_spec_boxed (
			"recur",
			"Recurrence",
			"The JanaRecurrence represented by this widget.",
			JANA_TYPE_RECURRENCE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_TIME,
		g_param_spec_object (
			"time",
			"Time",
			"The JanaTime the recurrence starts on.",
			G_TYPE_OBJECT,
			G_PARAM_READWRITE));
	
	g_object_class_install_property (
		object_class,
		PROP_EDITABLE,
		g_param_spec_boolean (
			"editable",
			"Editable",
			"Whether the recurrence can be edited or not.",
			FALSE,
			G_PARAM_READWRITE));
}

static void
interval_inc_clicked_cb (GtkButton *button, JanaGtkRecurrence *self)
{
	JanaGtkRecurrencePrivate *priv = RECURRENCE_PRIVATE (self);
	
	if (priv->recur) priv->recur->interval++;
	refresh (self);
}

static void
interval_dec_clicked_cb (GtkButton *button, JanaGtkRecurrence *self)
{
	JanaGtkRecurrencePrivate *priv = RECURRENCE_PRIVATE (self);
	
	if (priv->recur && (priv->recur->interval > 1)) priv->recur->interval--;
	refresh (self);
}

static void
type_combo_changed_cb (GtkComboBox *combo, JanaGtkRecurrence *self)
{
	gint active;
	JanaGtkRecurrencePrivate *priv = RECURRENCE_PRIVATE (self);
	
	active = gtk_combo_box_get_active (combo);
	if (active > 0) {
		JanaRecurrenceType type;
		switch (active) {
		    case 1 :
			type = JANA_RECURRENCE_DAILY;
			break;
		    case 2 :
			type = JANA_RECURRENCE_WEEKLY;
			break;
		    case 3 :
			type = JANA_RECURRENCE_MONTHLY;
			break;
		    case 4 :
			type = JANA_RECURRENCE_YEARLY;
			break;
		    default :
			return;
		}
		if (!priv->recur) {
			priv->recur = jana_recurrence_new ();
			priv->recur->type = type;
			refresh (self);
		} else if (priv->recur->type != type) {
			priv->recur->type = type;
			refresh (self);
		}
	} else if (active == 0) {
		if (priv->recur) {
			jana_recurrence_free (priv->recur);
			priv->recur = NULL;
			refresh (self);
		}
	}
}

#define day_toggle_cb(x) \
static void \
day##x##_toggled_cb (GtkToggleButton *toggle, JanaGtkRecurrence *self) \
{ \
	gboolean active; \
	JanaGtkRecurrencePrivate *priv = RECURRENCE_PRIVATE (self); \
	if ((active = gtk_toggle_button_get_active (toggle)) != \
	    priv->recur->week_days[x]) { \
		priv->recur->week_days[x] = active; \
		refresh (self); \
	} \
}
day_toggle_cb(0)
day_toggle_cb(1)
day_toggle_cb(2)
day_toggle_cb(3)
day_toggle_cb(4)
day_toggle_cb(5)
day_toggle_cb(6)

static void
bydate_toggled_cb (GtkToggleButton *toggle, JanaGtkRecurrence *self)
{
	JanaGtkRecurrencePrivate *priv = RECURRENCE_PRIVATE (self);
	if (priv->recur->by_date != gtk_toggle_button_get_active (toggle)) {
		priv->recur->by_date = !priv->recur->by_date;
		refresh (self);
	}
}

static void
forever_toggled_cb (GtkToggleButton *toggle, JanaGtkDateTime *datetime)
{
	gtk_widget_set_sensitive (GTK_WIDGET (datetime),
		!gtk_toggle_button_get_active (toggle));
}

static void
close_clicked_cb (GtkButton *button, JanaGtkDateTime *datetime)
{
	GtkWidget *window;
	JanaGtkRecurrence *self;
	JanaGtkRecurrencePrivate *priv;
	
	window = gtk_widget_get_toplevel (GTK_WIDGET (button));
	self = (JanaGtkRecurrence *)
		g_object_get_data (G_OBJECT (window), "self");
	priv = RECURRENCE_PRIVATE (self);
	
	if (GTK_WIDGET_SENSITIVE (datetime)) {
		JanaTime *time = jana_gtk_date_time_get_time (datetime);
		if (jana_utils_time_compare (time, priv->time, TRUE) > 0) {
			if (priv->recur->end) g_object_unref (priv->recur->end);
			priv->recur->end = time;
		} else {
			g_object_unref (time);
		}
	} else if (priv->recur->end) {
		g_object_unref (priv->recur->end);
		priv->recur->end = NULL;
	}
	
	refresh (self);
	gtk_widget_destroy (window);
}

static void
end_clicked_cb (GtkButton *button, JanaGtkRecurrence *self)
{
	GtkWidget *window, *datetime, *check, *toplevel, *vbox, *close, *align;
	JanaGtkRecurrencePrivate *priv = RECURRENCE_PRIVATE (self);
	JanaTime *time;
	
	/* Time-editing dialog */
	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	if (GTK_WIDGET_TOPLEVEL (toplevel))
		gtk_window_set_transient_for (GTK_WINDOW (window),
			GTK_WINDOW (toplevel));
	gtk_window_set_modal (GTK_WINDOW (window), TRUE);
	gtk_window_set_type_hint (GTK_WINDOW (window),
		GDK_WINDOW_TYPE_HINT_DIALOG);
	
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
	check = gtk_check_button_new_with_mnemonic ("Repeats _forever");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check),
		priv->recur->end ? FALSE : TRUE);
	time = priv->recur->end ?
		jana_time_duplicate (priv->recur->end) :
		jana_time_duplicate (priv->time);
	jana_time_set_isdate (time, TRUE);
	datetime = jana_gtk_date_time_new (time);
	jana_gtk_date_time_set_editable (JANA_GTK_DATE_TIME (datetime), TRUE);
	gtk_widget_set_sensitive (datetime, priv->recur->end ? TRUE : FALSE);
	align = gtk_alignment_new (1.0, 0.5, 0.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 6, 0, 0, 0);
	close = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	gtk_container_add (GTK_CONTAINER (align), close);
	
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), datetime, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (window), vbox);
	
	g_object_set_data (G_OBJECT (window), "self", self);
	g_signal_connect (check, "toggled",
		G_CALLBACK (forever_toggled_cb), datetime);
	g_signal_connect (close, "clicked",
		G_CALLBACK (close_clicked_cb), datetime);
	
	gtk_widget_show_all (window);
}

static void
jana_gtk_recurrence_init (JanaGtkRecurrence *self)
{
	GtkWidget *label, *hbox, *sub_hbox, *arrow, *button;
	GtkSizeGroup *size_group;

	JanaGtkRecurrencePrivate *priv = RECURRENCE_PRIVATE (self);
	
	gtk_box_set_spacing (GTK_BOX (self), 6);
	
	/* Create preview text-view */
	priv->preview_frame = gtk_frame_new (NULL);
	priv->preview_textview = gtk_text_view_new ();
	gtk_text_view_set_editable (GTK_TEXT_VIEW (
		priv->preview_textview), FALSE);
	gtk_text_view_set_cursor_visible (
		GTK_TEXT_VIEW (priv->preview_textview), FALSE);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->preview_textview),
		GTK_WRAP_WORD_CHAR);
	gtk_container_add (GTK_CONTAINER (priv->preview_frame),
		priv->preview_textview);
	gtk_box_pack_end (GTK_BOX (self), priv->preview_frame, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type (
		GTK_FRAME (priv->preview_frame), GTK_SHADOW_NONE);
	gtk_widget_show_all (priv->preview_frame);
	
	priv->edit_vbox = gtk_vbox_new (FALSE, 6);
	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	
	/* 'Repeats' (recurrence type) combo box */
	hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new ("Repeats:");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_size_group_add_widget (size_group, label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
	priv->type_combo = gtk_combo_box_new_text ();
	gtk_combo_box_append_text (GTK_COMBO_BOX (priv->type_combo), "None");
	gtk_combo_box_append_text (GTK_COMBO_BOX (priv->type_combo),
		"Every day");
	gtk_combo_box_append_text (GTK_COMBO_BOX (priv->type_combo),
		"Every week");
	gtk_combo_box_append_text (GTK_COMBO_BOX (priv->type_combo),
		"Every year");
	gtk_combo_box_set_active (GTK_COMBO_BOX (priv->type_combo), 0);
	gtk_box_pack_start (GTK_BOX (hbox), priv->type_combo, TRUE, TRUE, 0);
	g_signal_connect (priv->type_combo, "changed",
		G_CALLBACK (type_combo_changed_cb), self);
	/* Interval buttons */
	sub_hbox = gtk_hbox_new (TRUE, 0);
	priv->interval_dec_button = gtk_button_new ();
	arrow = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (priv->interval_dec_button), arrow);
	gtk_box_pack_start (GTK_BOX (sub_hbox), priv->interval_dec_button,
		FALSE, TRUE, 0);
	priv->interval_inc_button = gtk_button_new ();
	arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (priv->interval_inc_button), arrow);
	gtk_box_pack_start (GTK_BOX (sub_hbox), priv->interval_inc_button,
		FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), sub_hbox, FALSE, TRUE, 0);
	g_signal_connect (priv->interval_inc_button, "clicked",
		G_CALLBACK (interval_inc_clicked_cb), self);
	g_signal_connect (priv->interval_dec_button, "clicked",
		G_CALLBACK (interval_dec_clicked_cb), self);
	
	gtk_widget_show_all (hbox);
	gtk_box_pack_start (GTK_BOX (priv->edit_vbox), hbox, FALSE, TRUE, 0);
	
	/* Weekly recurrence day-chooser */
	priv->week_hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new ("On:");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_size_group_add_widget (size_group, label);
	gtk_box_pack_start (GTK_BOX (priv->week_hbox), label, FALSE, TRUE, 0);
	sub_hbox = gtk_hbox_new (TRUE, 0);
	/* Weekday toggle widgets */
#define day_toggle_widget(x) \
	button = gtk_toggle_button_new_with_label (jana_utils_ab_day (x)); \
	gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE); \
	gtk_box_pack_start (GTK_BOX (sub_hbox), button, TRUE, TRUE, 0); \
	priv->week_buttons[x] = GTK_TOGGLE_BUTTON (button); \
	g_signal_connect (button, "toggled", \
		G_CALLBACK (day##x##_toggled_cb), self)
	day_toggle_widget(0);
	day_toggle_widget(1);
	day_toggle_widget(2);
	day_toggle_widget(3);
	day_toggle_widget(4);
	day_toggle_widget(5);
	day_toggle_widget(6);
	gtk_box_pack_start (GTK_BOX (priv->week_hbox), sub_hbox, TRUE, TRUE, 0);

	gtk_widget_show_all (priv->week_hbox);
	gtk_box_pack_start (GTK_BOX (priv->edit_vbox), priv->week_hbox,
		FALSE, TRUE, 0);
	
	/* Monthly recurrence by date/day chooser */
	priv->month_hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new ("By:");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_size_group_add_widget (size_group, label);
	gtk_box_pack_start (GTK_BOX (priv->month_hbox), label, FALSE, TRUE, 0);
	sub_hbox = gtk_hbox_new (TRUE, 0);
	/* By day */
	priv->byday_button = GTK_TOGGLE_BUTTON (
		gtk_radio_button_new_with_label (NULL, "Day"));
	gtk_toggle_button_set_mode (priv->byday_button, FALSE);
	gtk_box_pack_start (GTK_BOX (sub_hbox), GTK_WIDGET (
		priv->byday_button), FALSE, TRUE, 0);
	/* By date */
	priv->bydate_button = GTK_TOGGLE_BUTTON (
		gtk_radio_button_new_with_label_from_widget (
			GTK_RADIO_BUTTON (priv->byday_button), "Date"));
	gtk_toggle_button_set_mode (priv->bydate_button, FALSE);
	g_signal_connect (priv->bydate_button, "toggled",
		G_CALLBACK (bydate_toggled_cb), self);
	gtk_box_pack_start (GTK_BOX (sub_hbox), GTK_WIDGET (
		priv->bydate_button), FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (priv->month_hbox), sub_hbox,
		FALSE, TRUE, 0);
	
	gtk_widget_show_all (priv->month_hbox);
	gtk_box_pack_start (GTK_BOX (priv->edit_vbox), priv->month_hbox,
		FALSE, TRUE, 0);
	
	/* Until time editor */
	priv->end_hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new ("Until:");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_size_group_add_widget (size_group, label);
	gtk_box_pack_start (GTK_BOX (priv->end_hbox), label, FALSE, TRUE, 0);
	priv->end_button = gtk_button_new ();
	hbox = gtk_hbox_new (FALSE, 6);
	priv->end_label = gtk_label_new (NULL);
	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_IN);
	gtk_box_pack_start (GTK_BOX (hbox), priv->end_label, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), arrow, FALSE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (priv->end_button), hbox);
	g_signal_connect (priv->end_button, "clicked",
		G_CALLBACK (end_clicked_cb), self);
	gtk_box_pack_start (GTK_BOX (priv->end_hbox), priv->end_button,
		FALSE, TRUE, 0);
	
	gtk_widget_show_all (priv->end_hbox);
	gtk_box_pack_start (GTK_BOX (priv->edit_vbox), priv->end_hbox,
		FALSE, TRUE, 0);
	
	gtk_box_pack_start (GTK_BOX (self), priv->edit_vbox, FALSE, TRUE, 0);

	gtk_widget_set_no_show_all (priv->edit_vbox, TRUE);
	gtk_widget_set_no_show_all (priv->preview_frame, TRUE);
	
	refresh (self);
}

GtkWidget *
jana_gtk_recurrence_new (void)
{
	return g_object_new (JANA_GTK_TYPE_RECURRENCE, NULL);
}

void
jana_gtk_recurrence_set_recur (JanaGtkRecurrence *self, JanaRecurrence *recur)
{
	JanaGtkRecurrencePrivate *priv = RECURRENCE_PRIVATE (self);
	
	if (priv->recur) {
		jana_recurrence_free (priv->recur);
		priv->recur = NULL;
	}
	if (recur) priv->recur = jana_recurrence_copy (recur);
	
	refresh (self);
}

JanaRecurrence *
jana_gtk_recurrence_get_recur (JanaGtkRecurrence *self)
{
	JanaGtkRecurrencePrivate *priv = RECURRENCE_PRIVATE (self);
	
	return priv->recur ? jana_recurrence_copy (priv->recur) : NULL;
}

void
jana_gtk_recurrence_set_editable (JanaGtkRecurrence *self, gboolean editable)
{
	JanaGtkRecurrencePrivate *priv = RECURRENCE_PRIVATE (self);
	
	if (editable == priv->editable) return;
	
	priv->editable = editable;
	g_object_set (G_OBJECT (priv->edit_vbox), "visible", editable, NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (priv->preview_frame),
		editable ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
}

gboolean
jana_gtk_recurrence_get_editable (JanaGtkRecurrence *self)
{
	JanaGtkRecurrencePrivate *priv = RECURRENCE_PRIVATE (self);
	
	return priv->editable;
}

void
jana_gtk_recurrence_set_time (JanaGtkRecurrence *self, JanaTime *time)
{
	JanaGtkRecurrencePrivate *priv = RECURRENCE_PRIVATE (self);
	
	if (priv->time) {
		g_object_unref (priv->time);
		priv->time = NULL;
	}
	if (time) priv->time = jana_time_duplicate (time);

	refresh (self);
}

JanaTime *
jana_gtk_recurrence_get_time (JanaGtkRecurrence *self)
{
	JanaGtkRecurrencePrivate *priv = RECURRENCE_PRIVATE (self);

	return priv->time ? jana_time_duplicate (priv->time) : NULL;
}
