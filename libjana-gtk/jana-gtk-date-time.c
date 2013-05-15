
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_GLADE
#include <gladeui/glade.h>
#endif

#include <stdlib.h>
#include <string.h>
#include "jana-gtk-date-time.h"
#include <libjana/jana-utils.h>

G_DEFINE_TYPE (JanaGtkDateTime, jana_gtk_date_time, GTK_TYPE_VBOX)

#define DATE_TIME_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), JANA_GTK_TYPE_DATE_TIME, \
	JanaGtkDateTimePrivate))

typedef struct _JanaGtkDateTimePrivate JanaGtkDateTimePrivate;

typedef enum {
	JANA_GTK_DATE_TIME_DMY,
	JANA_GTK_DATE_TIME_MDY,
	JANA_GTK_DATE_TIME_YMD,
} JanaGtkDateTimeEntryFormat;

struct _JanaGtkDateTimePrivate {
	JanaTime *time;

	GtkWidget *label;
	GtkWidget *edit_vbox;
	GtkWidget *time_header;
	GtkWidget *time_table;

	GtkWidget *hour_entry;
	GtkWidget *minute_entry;
	
	GtkWidget *day_entry;
	GtkWidget *month_entry;
	GtkWidget *year_entry;
	
	gboolean editable;
	JanaGtkDateTimeEntryFormat format;
	
	gint timeout;
	gint fast_timeout;
	gint repeat;
};

enum {
	PROP_TIME = 1,
	PROP_EDITABLE,
	PROP_ENTRY_FORMAT,
	PROP_TIMEOUT,
	PROP_FAST_TIMEOUT,
	PROP_REPEAT,
};

enum {
	CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void refresh (JanaGtkDateTime *self);


static void
adjust_entry (GtkEntry *entry, gint direction)
{
	gint min, max;
	gchar *new_text;
	const gchar *text = gtk_entry_get_text (entry);
	gint value = atoi (text);
	
	min = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry), "min"));
	max = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry), "max"));
	
	value += direction;
	if (value > max) value = min;
	if (value < min) value = max;
	
	new_text = g_strdup_printf ("%02d", value);
	gtk_entry_set_text (entry, new_text);
	g_free (new_text);
}

static void
activate_button (GtkButton *button)
{
	GtkEntry *entry = GTK_ENTRY (g_object_get_data (G_OBJECT (button),
		"entry"));
	gint direction = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
		"direction"));

	adjust_entry (entry, direction);
}

static gboolean
button_repeat_cb (GtkButton *button)
{
	activate_button (button);
	return TRUE;
}

static gboolean
button_start_fast_repeat_cb (GtkButton *button)
{
	guint timeout;
	JanaGtkDateTime *self = (JanaGtkDateTime *)g_object_get_data (
		G_OBJECT (button), "self");
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (self);
	
	activate_button (button);

	timeout = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (button),
		"source"));
	g_source_remove (timeout);
	timeout = g_timeout_add (500 / priv->repeat, (GSourceFunc)
		button_repeat_cb, button);
	g_object_set_data (G_OBJECT (button), "source",
		GUINT_TO_POINTER (timeout));

	g_object_set_data (G_OBJECT (button), "fast-source",
		GUINT_TO_POINTER (0));

	return FALSE;
}

static gboolean
button_start_repeat_cb (GtkButton *button)
{
	guint timeout;
	JanaGtkDateTime *self = (JanaGtkDateTime *)g_object_get_data (
		G_OBJECT (button), "self");
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (self);
	
	activate_button (button);

	timeout = g_timeout_add (1000 / priv->repeat, (GSourceFunc)
		button_repeat_cb, button);
	g_object_set_data (G_OBJECT (button), "source",
		GUINT_TO_POINTER (timeout));

	timeout = g_timeout_add (priv->fast_timeout, (GSourceFunc)
		button_start_fast_repeat_cb, button);
	g_object_set_data (G_OBJECT (button), "fast-source",
		GUINT_TO_POINTER (timeout));
	
	return FALSE;
}

static void
button_pressed_cb (GtkButton *button, JanaGtkDateTime *self)
{
	guint timeout;
	activate_button (button);
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (self);
	
	timeout = g_timeout_add (priv->timeout, (GSourceFunc)
		button_start_repeat_cb, button);
	g_object_set_data (G_OBJECT (button), "source",
		GUINT_TO_POINTER (timeout));
}

static void
button_released_cb (GtkButton *button, JanaGtkDateTime *self)
{
	guint timeout;
	
	timeout = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (button),
		"source"));
	if (timeout) {
		g_source_remove (timeout);
		g_object_set_data (G_OBJECT (button), "source",
			GUINT_TO_POINTER (0));
	}

	timeout = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (button),
		"fast-source"));
	if (timeout) {
		g_source_remove (timeout);
		g_object_set_data (G_OBJECT (button), "fast-source",
			GUINT_TO_POINTER (0));
	}
}

void
insert_text_cb (GtkEditable *entry, gchar *new_text, gint new_text_length,
		gint *position, JanaGtkDateTime *self)
{
	gint i;
	gchar *c;

	for (i = 0, c = new_text; c;
	     c = g_utf8_find_next_char (c, (new_text_length != -1) ?
	     (new_text + new_text_length) : NULL)) {
		if (*c < '0' || *c > '9') {
			g_signal_stop_emission (entry, g_signal_lookup (
				"insert-text", GTK_TYPE_EDITABLE), 0);
			gdk_beep ();
			break;
		}
	}
}

static void
minute_entry_changed_cb (GtkEntry *entry, JanaGtkDateTime *self)
{
	gint value;
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (self);
	gchar *text = g_strstrip (g_strdup (gtk_entry_get_text (entry)));

	if ((!text) || (!priv->time) || (strcmp (text, "") == 0)) {
		g_free (text);
		return;
	}
	
	value = atoi (text);
	g_free (text);
	if (value != jana_time_get_minutes (priv->time)) {
		jana_time_set_minutes (priv->time, value);
		g_signal_emit (self, signals[CHANGED], 0);
		refresh (self);
	}
}

static void
hour_entry_changed_cb (GtkEntry *entry, JanaGtkDateTime *self)
{
	gint value;
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (self);
	gchar *text = g_strstrip (g_strdup (gtk_entry_get_text (entry)));

	if ((!text) || (!priv->time) || (strcmp (text, "") == 0)) {
		g_free (text);
		return;
	}
	
	value = atoi (text);
	g_free (text);
	if (value != jana_time_get_hours (priv->time)) {
		jana_time_set_hours (priv->time, value);
		g_signal_emit (self, signals[CHANGED], 0);
		refresh (self);
	}
}

static void
day_entry_changed_cb (GtkEntry *entry, JanaGtkDateTime *self)
{
	gint value;
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (self);
	gchar *text = g_strstrip (g_strdup (gtk_entry_get_text (entry)));

	if ((!text) || (!priv->time) || (strcmp (text, "") == 0)) {
		g_free (text);
		return;
	}
	
	value = atoi (text);
	g_free (text);
	if (value != jana_time_get_day (priv->time)) {
		jana_time_set_day (priv->time, value);
		g_signal_emit (self, signals[CHANGED], 0);
		refresh (self);
	}
}

static void
month_entry_changed_cb (GtkEntry *entry, JanaGtkDateTime *self)
{
	gint value;
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (self);
	gchar *text = g_strstrip (g_strdup (gtk_entry_get_text (entry)));

	if ((!text) || (!priv->time) || (strcmp (text, "") == 0)) {
		g_free (text);
		return;
	}
	
	value = atoi (text);
	g_free (text);
	
	if (value != jana_time_get_month (priv->time)) {
		jana_time_set_month (priv->time, value);
		
		/* Update max range of day entry */
		g_object_set_data (G_OBJECT (priv->day_entry), "max",
			GINT_TO_POINTER ((gint)jana_utils_time_days_in_month (
				jana_time_get_year (priv->time),
				jana_time_get_month (priv->time))));
		
		g_signal_emit (self, signals[CHANGED], 0);
		refresh (self);
	}
}

static void
year_entry_changed_cb (GtkEntry *entry, JanaGtkDateTime *self)
{
	gint value;
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (self);
	gchar *text = g_strstrip (g_strdup (gtk_entry_get_text (entry)));

	if ((!text) || (!priv->time) || (strcmp (text, "") == 0)) {
		g_free (text);
		return;
	}
	
	value = atoi (text);
	g_free (text);
	
	if (value != jana_time_get_year (priv->time)) {
		jana_time_set_year (priv->time, value);
		
		/* Update max range of day entry */
		g_object_set_data (G_OBJECT (priv->day_entry), "max",
			GINT_TO_POINTER ((gint)jana_utils_time_days_in_month (
				jana_time_get_year (priv->time),
				jana_time_get_month (priv->time))));
		
		g_signal_emit (self, signals[CHANGED], 0);
		refresh (self);
	}
}

static void
create_edit_page (JanaGtkDateTime *self)
{
	const gchar *date_separator_text = "/";
	GtkWidget *date_table, *align, *header;
	GtkWidget *time_separator_label, *date_separator_label;
	GtkWidget *hour_up_button, *hour_up, *hour_down_button,
		*hour_down, *minute_up_button, *minute_up,
		*minute_down_button, *minute_down,
		*day_up_button, *day_up, *day_down_button, *day_down,
		*month_up_button, *month_up, *month_down_button, *month_down,
		*year_up_button, *year_up, *year_down_button, *year_down;
	GtkWidget *uleft, *umiddle, *uright,
		*left, *middle, *right,
		*lleft, *lmiddle, *lright;

	/* Taken and modified from Dates. Sorry for the awful quality :( */

	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (self);
	
	priv->edit_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_end (GTK_BOX (self), priv->edit_vbox, FALSE, TRUE, 0);
	gtk_widget_set_no_show_all (priv->edit_vbox, TRUE);
	
	/* Time editing widgets */

	/* Have time take up 2/3 of the space that date does, otherwise it
	 * looks rubbish.
	 */
	align = gtk_alignment_new (0.5, 0.5, 0.66, 1);
	gtk_widget_show (align);
	priv->time_table = gtk_table_new (3, 3, FALSE);
	gtk_widget_show (priv->time_table);
	gtk_table_set_col_spacings (GTK_TABLE (priv->time_table), 6);
	priv->time_header = gtk_label_new ("Time");
	gtk_widget_show (priv->time_header);
	gtk_container_add (GTK_CONTAINER (align), priv->time_table);
	gtk_box_pack_start (GTK_BOX (priv->edit_vbox),
		priv->time_header, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (priv->edit_vbox),
		align, FALSE, TRUE, 0);

	/* Hours / minutes separator */
	time_separator_label = gtk_label_new (":");
	gtk_widget_show (time_separator_label);
	gtk_table_attach (GTK_TABLE (priv->time_table), time_separator_label,
		1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

	/* Minute entry */
	priv->minute_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (priv->minute_entry), 2);
	gtk_entry_set_activates_default (GTK_ENTRY (priv->minute_entry), TRUE);
	gtk_entry_set_width_chars (GTK_ENTRY (priv->minute_entry), 1);
	gtk_widget_show (priv->minute_entry);
	gtk_table_attach (GTK_TABLE (priv->time_table), priv->minute_entry,
		2, 3, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);

	/* Hour increment button + arrow */
	hour_up_button = gtk_button_new ();
	gtk_widget_show (hour_up_button);
	gtk_table_attach (GTK_TABLE (priv->time_table), hour_up_button, 0, 1, 0, 1,
		GTK_FILL, 0, 0, 0);

	hour_up = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_OUT);
	gtk_widget_show (hour_up);
	gtk_container_add (GTK_CONTAINER (hour_up_button), hour_up);

	/* Hour decrement button + arrow */
	hour_down_button = gtk_button_new ();
	gtk_widget_show (hour_down_button);
	gtk_table_attach (GTK_TABLE (priv->time_table), hour_down_button,
		0, 1, 2, 3, GTK_FILL, 0, 0, 0);

	hour_down = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
	gtk_widget_show (hour_down);
	gtk_container_add (GTK_CONTAINER (hour_down_button), hour_down);

	/* Minute increment button + arrow */
	minute_up_button = gtk_button_new ();
	gtk_widget_show (minute_up_button);
	gtk_table_attach (GTK_TABLE (priv->time_table), minute_up_button,
		2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	minute_up = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_OUT);
	gtk_widget_show (minute_up);
	gtk_container_add (GTK_CONTAINER (minute_up_button), minute_up);

	/* Left digit minute decrement button + arrow */
	minute_down_button = gtk_button_new ();
	gtk_widget_show (minute_down_button);
	gtk_table_attach (GTK_TABLE (priv->time_table), minute_down_button,
		2, 3, 2, 3, GTK_FILL, 0, 0, 0);

	minute_down = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
	gtk_widget_show (minute_down);
	gtk_container_add (GTK_CONTAINER (minute_down_button), minute_down);

	/* Hour entry */
	priv->hour_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (priv->hour_entry), 2);
	gtk_entry_set_activates_default (GTK_ENTRY (priv->hour_entry), TRUE);
	gtk_entry_set_width_chars (GTK_ENTRY (priv->hour_entry), 2);
	gtk_widget_show (priv->hour_entry);
	gtk_table_attach (GTK_TABLE (priv->time_table), priv->hour_entry, 0, 1, 1, 2,
		GTK_EXPAND | GTK_FILL, 0, 0, 0);
	
	/* Date editing widgets */
	date_table = gtk_table_new (3, 5, FALSE);
	gtk_widget_show (date_table);
	gtk_table_set_col_spacings (GTK_TABLE (date_table), 6);
	header = gtk_label_new ("Date");
	gtk_widget_show (header);
	gtk_box_pack_start (GTK_BOX (priv->edit_vbox), header, FALSE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (priv->edit_vbox), date_table, FALSE, TRUE, 0);

	/* Separators */
	date_separator_label = gtk_label_new (date_separator_text);
	gtk_widget_show (date_separator_label);
	gtk_table_attach (GTK_TABLE (date_table), date_separator_label,
		1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

	date_separator_label = gtk_label_new (date_separator_text);
	gtk_widget_show (date_separator_label);
	gtk_table_attach (GTK_TABLE (date_table), date_separator_label,
		3, 4, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

	/* Day increment button + arrow */
	day_up_button = gtk_button_new ();
	gtk_widget_show (day_up_button);
	day_up = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_OUT);
	gtk_widget_show (day_up);
	gtk_container_add (GTK_CONTAINER (day_up_button), day_up);

	/* Day decrement button + arrow */
	day_down_button = gtk_button_new ();
	gtk_widget_show (day_down_button);
	day_down = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
	gtk_widget_show (day_down);
	gtk_container_add (GTK_CONTAINER (day_down_button), day_down);

	/* Month increment button + arrow */
	month_up_button = gtk_button_new ();
	gtk_widget_show (month_up_button);
	month_up = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_OUT);
	gtk_widget_show (month_up);
	gtk_container_add (GTK_CONTAINER (month_up_button), month_up);

	/* Month decrement button + arrow */
	month_down_button = gtk_button_new ();
	gtk_widget_show (month_down_button);
	month_down = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
	gtk_widget_show (month_down);
	gtk_container_add (GTK_CONTAINER (month_down_button), month_down);

	/* Year increment button + arrow */
	year_up_button = gtk_button_new ();
	gtk_widget_show (year_up_button);
	year_up = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_OUT);
	gtk_widget_show (year_up);
	gtk_container_add (GTK_CONTAINER (year_up_button), year_up);

	/* Year decrement button + arrow */
	year_down_button = gtk_button_new ();
	gtk_widget_show (year_down_button);
	year_down = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
	gtk_widget_show (year_down);
	gtk_container_add (GTK_CONTAINER (year_down_button), year_down);

	/* Day entry */
	priv->day_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (priv->day_entry), 2);
	gtk_entry_set_activates_default (GTK_ENTRY (priv->day_entry), TRUE);
	gtk_entry_set_width_chars (GTK_ENTRY (priv->day_entry), 2);
	gtk_widget_show (priv->day_entry);

	/* Month entry */
	priv->month_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (priv->month_entry), 2);
	gtk_entry_set_activates_default (GTK_ENTRY (priv->month_entry), TRUE);
	gtk_entry_set_width_chars (GTK_ENTRY (priv->month_entry), 2);
	gtk_widget_show (priv->month_entry);

	/* Year entry */
	priv->year_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (priv->year_entry), 4);
	gtk_entry_set_activates_default (GTK_ENTRY (priv->year_entry), TRUE);
	gtk_entry_set_width_chars (GTK_ENTRY (priv->year_entry), 4);
	gtk_widget_show (priv->year_entry);

	switch (priv->format) {
	    case JANA_GTK_DATE_TIME_MDY :
		left = priv->month_entry;
		middle = priv->day_entry;
		right = priv->year_entry;
		uleft = month_up_button;
		umiddle = day_up_button;
		uright = year_up_button;
		lleft = month_down_button;
		lmiddle = day_down_button;
		lright = year_down_button;
		break;
	    case JANA_GTK_DATE_TIME_YMD :
		left = priv->year_entry;
		middle = priv->month_entry;
		right = priv->day_entry;
		uleft = year_up_button;
		umiddle = month_up_button;
		uright = day_up_button;
		lleft = year_down_button;
		lmiddle = month_down_button;
		lright = day_down_button;
		break;
	    case JANA_GTK_DATE_TIME_DMY :
	    default :
		left = priv->day_entry;
		middle = priv->month_entry;
		right = priv->year_entry;
		uleft = day_up_button;
		umiddle = month_up_button;
		uright = year_up_button;
		lleft = day_down_button;
		lmiddle = month_down_button;
		lright = year_down_button;
		break;
	}

	gtk_table_attach (GTK_TABLE (date_table), uleft, 0, 1, 0, 1,
		GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_table_attach (GTK_TABLE (date_table), umiddle,
		2, 3, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_table_attach (GTK_TABLE (date_table), uright,
		4, 5, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);

	gtk_table_attach (GTK_TABLE (date_table), left, 0, 1, 1, 2,
		GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_table_attach (GTK_TABLE (date_table), middle,
		2, 3, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_table_attach (GTK_TABLE (date_table), right,
		4, 5, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);

	gtk_table_attach (GTK_TABLE (date_table), lleft, 0, 1, 2, 3,
		GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_table_attach (GTK_TABLE (date_table), lmiddle,
		2, 3, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_table_attach (GTK_TABLE (date_table), lright,
		4, 5, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	
	/* Sort out signals */
	g_object_set_data (G_OBJECT (hour_up_button), "self", self);
	g_object_set_data (G_OBJECT (hour_up_button),
		"direction", GINT_TO_POINTER (1));
	g_object_set_data (G_OBJECT (hour_up_button),
		"entry", priv->hour_entry);
	g_object_set_data (G_OBJECT (hour_down_button), "self", self);
	g_object_set_data (G_OBJECT (hour_down_button),
		"direction", GINT_TO_POINTER (-1));
	g_object_set_data (G_OBJECT (hour_down_button),
		"entry", priv->hour_entry);
	g_object_set_data (G_OBJECT (minute_up_button), "self", self);
	g_object_set_data (G_OBJECT (minute_up_button),
		"direction", GINT_TO_POINTER (1));
	g_object_set_data (G_OBJECT (minute_up_button),
		"entry", priv->minute_entry);
	g_object_set_data (G_OBJECT (minute_down_button), "self", self);
	g_object_set_data (G_OBJECT (minute_down_button),
		"direction", GINT_TO_POINTER (-1));
	g_object_set_data (G_OBJECT (minute_down_button),
		"entry", priv->minute_entry);

	g_object_set_data (G_OBJECT (day_up_button), "self", self);
	g_object_set_data (G_OBJECT (day_up_button),
		"direction", GINT_TO_POINTER (1));
	g_object_set_data (G_OBJECT (day_up_button),
		"entry", priv->day_entry);
	g_object_set_data (G_OBJECT (day_down_button), "self", self);
	g_object_set_data (G_OBJECT (day_down_button),
		"direction", GINT_TO_POINTER (-1));
	g_object_set_data (G_OBJECT (day_down_button),
		"entry", priv->day_entry);
	g_object_set_data (G_OBJECT (month_up_button), "self", self);
	g_object_set_data (G_OBJECT (month_up_button),
		"direction", GINT_TO_POINTER (1));
	g_object_set_data (G_OBJECT (month_up_button),
		"entry", priv->month_entry);
	g_object_set_data (G_OBJECT (month_down_button), "self", self);
	g_object_set_data (G_OBJECT (month_down_button),
		"direction", GINT_TO_POINTER (-1));
	g_object_set_data (G_OBJECT (month_down_button),
		"entry", priv->month_entry);
	g_object_set_data (G_OBJECT (year_up_button), "self", self);
	g_object_set_data (G_OBJECT (year_up_button),
		"direction", GINT_TO_POINTER (1));
	g_object_set_data (G_OBJECT (year_up_button),
		"entry", priv->year_entry);
	g_object_set_data (G_OBJECT (year_down_button), "self", self);
	g_object_set_data (G_OBJECT (year_down_button),
		"direction", GINT_TO_POINTER (-1));
	g_object_set_data (G_OBJECT (year_down_button),
		"entry", priv->year_entry);
	
	g_object_set_data (G_OBJECT (priv->hour_entry),
		"max", GINT_TO_POINTER (23));
	g_object_set_data (G_OBJECT (priv->minute_entry),
		"max", GINT_TO_POINTER (59));
	g_object_set_data (G_OBJECT (priv->day_entry),
		"min", GINT_TO_POINTER (1));
	g_object_set_data (G_OBJECT (priv->day_entry),
		"max", GINT_TO_POINTER (31));
	g_object_set_data (G_OBJECT (priv->month_entry),
		"min", GINT_TO_POINTER (1));
	g_object_set_data (G_OBJECT (priv->month_entry),
		"max", GINT_TO_POINTER (12));
	g_object_set_data (G_OBJECT (priv->year_entry),
		"min", GINT_TO_POINTER (1900));
	g_object_set_data (G_OBJECT (priv->year_entry),
		"max", GINT_TO_POINTER (G_MAXUINT16));
	
	g_signal_connect (hour_up_button, "pressed",
		G_CALLBACK (button_pressed_cb), self);
	g_signal_connect (hour_down_button, "pressed",
		G_CALLBACK (button_pressed_cb), self);
	g_signal_connect (minute_up_button, "pressed",
		G_CALLBACK (button_pressed_cb), self);
	g_signal_connect (minute_down_button, "pressed",
		G_CALLBACK (button_pressed_cb), self);
	g_signal_connect (day_up_button, "pressed",
		G_CALLBACK (button_pressed_cb), self);
	g_signal_connect (day_down_button, "pressed",
		G_CALLBACK (button_pressed_cb), self);
	g_signal_connect (month_up_button, "pressed",
		G_CALLBACK (button_pressed_cb), self);
	g_signal_connect (month_down_button, "pressed",
		G_CALLBACK (button_pressed_cb), self);
	g_signal_connect (year_up_button, "pressed",
		G_CALLBACK (button_pressed_cb), self);
	g_signal_connect (year_down_button, "pressed",
		G_CALLBACK (button_pressed_cb), self);

	g_signal_connect (hour_up_button, "released",
		G_CALLBACK (button_released_cb), self);
	g_signal_connect (hour_down_button, "released",
		G_CALLBACK (button_released_cb), self);
	g_signal_connect (minute_up_button, "released",
		G_CALLBACK (button_released_cb), self);
	g_signal_connect (minute_down_button, "released",
		G_CALLBACK (button_released_cb), self);
	g_signal_connect (day_up_button, "released",
		G_CALLBACK (button_released_cb), self);
	g_signal_connect (day_down_button, "released",
		G_CALLBACK (button_released_cb), self);
	g_signal_connect (month_up_button, "released",
		G_CALLBACK (button_released_cb), self);
	g_signal_connect (month_down_button, "released",
		G_CALLBACK (button_released_cb), self);
	g_signal_connect (year_up_button, "released",
		G_CALLBACK (button_released_cb), self);
	g_signal_connect (year_down_button, "released",
		G_CALLBACK (button_released_cb), self);
	
	g_signal_connect (priv->hour_entry, "insert-text",
		G_CALLBACK (insert_text_cb), self);
	g_signal_connect (priv->minute_entry, "insert-text",
		G_CALLBACK (insert_text_cb), self);
	g_signal_connect (priv->day_entry, "insert-text",
		G_CALLBACK (insert_text_cb), self);
	g_signal_connect (priv->month_entry, "insert-text",
		G_CALLBACK (insert_text_cb), self);
	g_signal_connect (priv->year_entry, "insert-text",
		G_CALLBACK (insert_text_cb), self);
	
	g_signal_connect (priv->minute_entry, "changed",
		G_CALLBACK (minute_entry_changed_cb), self);
	g_signal_connect (priv->hour_entry, "changed",
		G_CALLBACK (hour_entry_changed_cb), self);
	g_signal_connect (priv->day_entry, "changed",
		G_CALLBACK (day_entry_changed_cb), self);
	g_signal_connect (priv->month_entry, "changed",
		G_CALLBACK (month_entry_changed_cb), self);
	g_signal_connect (priv->year_entry, "changed",
		G_CALLBACK (year_entry_changed_cb), self);
}

static void
refresh (JanaGtkDateTime *self)
{
	gchar *text;
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (self);
	
	switch (priv->format) {
	    case JANA_GTK_DATE_TIME_MDY :
		if (jana_time_get_isdate (priv->time))
			text = g_strdup_printf ("%02d/%02d/%04d",
				jana_time_get_month (priv->time),
				jana_time_get_day (priv->time),
				jana_time_get_year (priv->time));
		else
			text = g_strdup_printf ("%02d:%02d %02d/%02d/%04d",
				jana_time_get_hours (priv->time),
				jana_time_get_minutes (priv->time),
				jana_time_get_month (priv->time),
				jana_time_get_day (priv->time),
				jana_time_get_year (priv->time));
		break;
	    case JANA_GTK_DATE_TIME_YMD :
		if (jana_time_get_isdate (priv->time))
			text = g_strdup_printf ("%04d/%02d/%02d",
				jana_time_get_year (priv->time),
				jana_time_get_month (priv->time),
				jana_time_get_day (priv->time));
		else
			text = g_strdup_printf ("%02d:%02d %04d/%02d/%02d",
				jana_time_get_hours (priv->time),
				jana_time_get_minutes (priv->time),
				jana_time_get_year (priv->time),
				jana_time_get_month (priv->time),
				jana_time_get_day (priv->time));
		break;
	    case JANA_GTK_DATE_TIME_DMY :
	    default :
		if (jana_time_get_isdate (priv->time))
			text = g_strdup_printf ("%02d/%02d/%04d",
				jana_time_get_day (priv->time),
				jana_time_get_month (priv->time),
				jana_time_get_year (priv->time));
		else
			text = g_strdup_printf ("%02d:%02d %02d/%02d/%04d",
				jana_time_get_hours (priv->time),
				jana_time_get_minutes (priv->time),
				jana_time_get_day (priv->time),
				jana_time_get_month (priv->time),
				jana_time_get_year (priv->time));
		break;
	}
	
	gtk_label_set_text (GTK_LABEL (priv->label), text);
	g_free (text);
	
	if (!priv->hour_entry) return;
	
	if (!jana_time_get_isdate (priv->time)) {
		text = g_strdup_printf (
			"%02d", jana_time_get_hours (priv->time));
		gtk_entry_set_text (GTK_ENTRY (priv->hour_entry), text);
		g_free (text);

		text = g_strdup_printf (
			"%02d", jana_time_get_minutes (priv->time));
		gtk_entry_set_text (GTK_ENTRY (priv->minute_entry), text);
		g_free (text);
		
		gtk_widget_show (priv->time_header);
		gtk_widget_show (priv->time_table);
	} else {
		gtk_widget_hide (priv->time_header);
		gtk_widget_hide (priv->time_table);
	}

	text = g_strdup_printf ("%02d", jana_time_get_day (priv->time));
	gtk_entry_set_text (GTK_ENTRY (priv->day_entry), text);
	g_free (text);

	text = g_strdup_printf ("%02d", jana_time_get_month (priv->time));
	gtk_entry_set_text (GTK_ENTRY (priv->month_entry), text);
	g_free (text);

	text = g_strdup_printf ("%04d", jana_time_get_year (priv->time));
	gtk_entry_set_text (GTK_ENTRY (priv->year_entry), text);
	g_free (text);
}

static void
clear (JanaGtkDateTime *self)
{
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (self);

	gtk_label_set_text (GTK_LABEL (priv->label), "");
	
	if (!priv->hour_entry) return;
	
	gtk_entry_set_text (GTK_ENTRY (priv->hour_entry), "00");
	gtk_entry_set_text (GTK_ENTRY (priv->minute_entry), "00");
	gtk_entry_set_text (GTK_ENTRY (priv->day_entry), "01");
	gtk_entry_set_text (GTK_ENTRY (priv->month_entry), "01");
	/* TODO: Maybe set today's year rather than leaving it? */
	/*gtk_entry_set_text (GTK_ENTRY (priv->year_entry), "");*/
}

static void
jana_gtk_date_time_get_property (GObject *object, guint property_id,
				 GValue *value, GParamSpec *pspec)
{
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (object);
	
	switch (property_id) {
	    case PROP_TIME :
		if (priv->time)
			g_value_take_object (value,
				jana_time_duplicate (priv->time));
		break;
	    case PROP_EDITABLE :
		g_value_set_boolean (value, priv->editable);
		break;
	    case PROP_ENTRY_FORMAT :
		g_value_set_int (value, priv->format);
		break;
	    case PROP_TIMEOUT :
		g_value_set_int (value, priv->timeout);
		break;
	    case PROP_FAST_TIMEOUT :
		g_value_set_int (value, priv->fast_timeout);
		break;
	    case PROP_REPEAT :
		g_value_set_int (value, priv->repeat);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_date_time_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (object);
	
	switch (property_id) {
	    case PROP_TIME :
		jana_gtk_date_time_set_time (JANA_GTK_DATE_TIME (object),
			g_value_get_object (value));
		break;
	    case PROP_EDITABLE :
		jana_gtk_date_time_set_editable (JANA_GTK_DATE_TIME (object),
			g_value_get_boolean (value));
		break;
	    case PROP_ENTRY_FORMAT :
		priv->format = g_value_get_int (value);
		if (priv->time) refresh (JANA_GTK_DATE_TIME (object));
		break;
	    case PROP_TIMEOUT :
		priv->timeout = g_value_get_int (value);
		break;
	    case PROP_FAST_TIMEOUT :
		priv->fast_timeout = g_value_get_int (value);
		break;
	    case PROP_REPEAT :
		priv->repeat = g_value_get_int (value);
		break;
	    default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
jana_gtk_date_time_dispose (GObject *object)
{
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (object);
	
	if (priv->time) {
		g_object_unref (priv->time);
		priv->time = NULL;
	}
	
	if (G_OBJECT_CLASS (jana_gtk_date_time_parent_class)->dispose)
		G_OBJECT_CLASS (jana_gtk_date_time_parent_class)->
			dispose (object);
}

static void
jana_gtk_date_time_finalize (GObject *object)
{
	G_OBJECT_CLASS (jana_gtk_date_time_parent_class)->finalize (object);
}

static GObject *
jana_gtk_date_time_constructor (GType type,
				guint n_properties,
				GObjectConstructParam *properties)
{
	GObjectClass *gobject_class;
	GObject *obj;
  
	gobject_class = G_OBJECT_CLASS (jana_gtk_date_time_parent_class);
	obj = gobject_class->constructor (type, n_properties, properties);
	
	create_edit_page (JANA_GTK_DATE_TIME (obj));
	
	return obj;
}
		

static void
jana_gtk_date_time_class_init (JanaGtkDateTimeClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (JanaGtkDateTimePrivate));

	object_class->constructor = jana_gtk_date_time_constructor;
	object_class->get_property = jana_gtk_date_time_get_property;
	object_class->set_property = jana_gtk_date_time_set_property;
	object_class->dispose = jana_gtk_date_time_dispose;
	object_class->finalize = jana_gtk_date_time_finalize;

	g_object_class_install_property (
		object_class,
		PROP_TIME,
		g_param_spec_object (
			"time",
			"Time",
			"The JanaTime represented by this widget.",
			G_TYPE_OBJECT,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_EDITABLE,
		g_param_spec_boolean (
			"editable",
			"Editable",
			"Whether the JanaTime can be edited or not.",
			FALSE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_ENTRY_FORMAT,
		g_param_spec_int (
			"entry-format",
			"Entry format",
			"The order in which day, month and year are shown.",
			JANA_GTK_DATE_TIME_DMY, JANA_GTK_DATE_TIME_YMD,
			JANA_GTK_DATE_TIME_DMY,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (
		object_class,
		PROP_TIMEOUT,
		g_param_spec_int (
			"timeout",
			"Key-repeat time-out",
			"The amount of milliseconds before key-repeat starts.",
			0, G_MAXINT, 500,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_FAST_TIMEOUT,
		g_param_spec_int (
			"fast-timeout",
			"Secondary key-repeat time-out",
			"The amount of milliseconds after key-repeat starts, "
			"before the key-repeat rate is doubled.",
			0, G_MAXINT, 1000,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_REPEAT,
		g_param_spec_int (
			"repeat",
			"Key-repeat",
			"The amount of times per second the key will be "
			"activated after pressing, after the initial timeout.",
			0, G_MAXINT, 8,
			G_PARAM_READWRITE));

	signals[CHANGED] =
		g_signal_new ("changed",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (JanaGtkDateTimeClass,
					 changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);
}

static void
jana_gtk_date_time_init (JanaGtkDateTime *self)
{
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (self);

	priv->format = JANA_GTK_DATE_TIME_DMY;
	priv->timeout = 500;
	priv->fast_timeout = 1000;
	priv->repeat = 8;

	priv->label = gtk_label_new (NULL);
	gtk_widget_show (priv->label);
	gtk_box_pack_start (GTK_BOX (self), priv->label, FALSE, TRUE, 0);
	
	gtk_widget_set_no_show_all (priv->label, TRUE);
}

GtkWidget*
jana_gtk_date_time_new (JanaTime *time)
{
	/* TODO: Use locale to select correct date format */
	return GTK_WIDGET (g_object_new (JANA_GTK_TYPE_DATE_TIME,
		"entry-format", JANA_GTK_DATE_TIME_DMY, "time", time, NULL));
}

void
jana_gtk_date_time_set_editable (JanaGtkDateTime *self, gboolean editable)
{
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (self);

	priv->editable = editable;
	if (priv->editable) {
		gtk_widget_hide (priv->label);
		gtk_widget_show (priv->edit_vbox);
	} else {
		gtk_widget_hide (priv->edit_vbox);
		gtk_widget_show (priv->label);
	}
}

gboolean
jana_gtk_date_time_get_editable (JanaGtkDateTime *self)
{
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (self);
	
	return priv->editable;
}

JanaTime *
jana_gtk_date_time_get_time (JanaGtkDateTime *self)
{
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (self);
	return priv->time ? jana_time_duplicate (priv->time) : NULL;
}

void
jana_gtk_date_time_set_time (JanaGtkDateTime *self, JanaTime *time)
{
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (self);

	if (priv->time) {
		g_object_unref (priv->time);
		priv->time = NULL;
		clear (self);
	}
	if (time) {
		priv->time = jana_time_duplicate (time);
		refresh (self);
	}
	g_signal_emit (self, signals[CHANGED], 0);
}

#ifdef HAVE_GLADE
void
jana_gtk_date_time_glade_post_create (GladeWidgetAdaptor *adaptor,
				      GObject		 *object,
				      GladeCreateReason   reason)
{
	JanaGtkDateTimePrivate *priv = DATE_TIME_PRIVATE (object);
	gtk_label_set_text (GTK_LABEL (priv->label), "Time display");
}
#endif
