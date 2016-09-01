#include "new_habit.h"
#include "db.h"
#include "readable_interval.h"

#include "new_habit.glade.h"

#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h> // strtol
#include <assert.h>

#define NAMES X(top) X(importance) X(description) X(frequency) X(freqadj) X(readable_frequency) X(create_habit)

struct Stuff {
	#define X(name) GtkWidget* name;
	NAMES
	#undef X
	guint updating_importance;
	GtkAdjustment* freqadjderp;
	GtkAdjustment* importancederp;
	gboolean typing;
} stuff = {}; 

double a,b;
const gdouble c = 1.2; // > 1
gdouble starting_point;
static void calc_constants(void) {
/*
	y = c^(a*x) + b;
	bottom = c^(a*0) + b;
	b = bottom - 1;
	y = c^(a*x) + bottom - 1;
	top = c^(a*1.0) + bottom - 1;
	c^(a*1.0) = top - bottom + 1;
	a * 1.0 = log_c(top - bottom + 1);
	a = log(top - bottom + 1) / log_c;

	reversed:
	x = log(y-b) / a
*/
	const int bottom = 300;
	const int top = 86400 * 30 + 1;
	a = log(top - bottom + 1) / log(c);
	b = bottom - 1;
	starting_point = log(86400-b)/a;
}

static gdouble stretch_along(gdouble spot) {
	return (pow(c, a * spot) + b);
}

static void update_readable_frequency(void) {
	char* err = NULL;
	long frequency = strtol(gtk_entry_get_text(GTK_ENTRY(stuff.frequency)),
													&err, 10);
	assert(err == NULL || *err == '\0');

	gtk_label_set_text(GTK_LABEL(stuff.readable_frequency),
										 readable_interval(frequency));
}

static gboolean adjust_frequency(void* udata) {
	stuff.updating_importance = 0;
	gdouble spot = stretch_along(gtk_adjustment_get_value(stuff.freqadjderp));
	static char micros[0x1000];
	ssize_t amt = snprintf(micros,0x1000,"%ld",(long)spot);
	gtk_entry_set_text(GTK_ENTRY(stuff.frequency), micros);
	update_readable_frequency();
	return G_SOURCE_REMOVE;
}

static gboolean do_create(GtkButton* btn, void* udata) {
	char* err = NULL;
	long frequency = strtol(gtk_entry_get_text(GTK_ENTRY(stuff.frequency)),
													&err, 10);
	assert(err == NULL || *err == '\0');
	if(frequency == 0) frequency = 600;
	double importance = gtk_adjustment_get_value(stuff.importancederp);
	gboolean created =
		db_create_habit(gtk_entry_get_text(GTK_ENTRY(stuff.description)),
									gtk_entry_get_text_length(GTK_ENTRY(stuff.description)),
									importance,frequency);

	GtkWidget* dialog = gtk_message_dialog_new
		(NULL,
		 0,
		 GTK_MESSAGE_INFO,
		 GTK_BUTTONS_OK,
		 "Habit was %s: %s!",
		 created ?
		 "created" : "updated",
		 gtk_entry_get_text(GTK_ENTRY(stuff.description)));
	gtk_widget_show_all(dialog);
	gtk_widget_hide(stuff.top);
}

static gboolean
prepare_adjust_frequency (GtkRange     *range,
													 GtkScrollType scroll,
													 gdouble       value,
													 gpointer      user_data) {
	if(stuff.updating_importance) return FALSE;
	stuff.updating_importance =
		g_timeout_add(100,adjust_frequency,NULL);
}

static gboolean update_text_freq(void* udata) {
	update_readable_frequency();
	stuff.typing = 0;
	return G_SOURCE_REMOVE;
}

static void on_text_freq(GtkEditable* thing, gpointer udata) {
	if(stuff.typing) {
		g_source_remove(stuff.typing);
	}
	stuff.typing = g_timeout_add(500,update_text_freq,NULL);
}

void new_habit_init(void) {
	calc_constants();
	GtkBuilder* b = gtk_builder_new_from_string(new_habit_glade,
																							new_habit_glade_length);

#define X(name) \
	stuff.name = GTK_WIDGET(gtk_builder_get_object(b, #name));
	NAMES
	#undef X

		stuff.freqadjderp = gtk_range_get_adjustment(GTK_RANGE(stuff.freqadj));
	stuff.importancederp = gtk_range_get_adjustment
		(GTK_RANGE(stuff.importance));
	g_signal_connect(stuff.create_habit, "clicked",
									 G_CALLBACK(do_create), NULL);
	g_signal_connect(stuff.freqadj, "change-value",
									 G_CALLBACK(prepare_adjust_frequency), NULL);
	g_signal_connect(stuff.frequency, "changed",
									 G_CALLBACK(on_text_freq),NULL);
	g_signal_connect(stuff.top, "delete-event",G_CALLBACK(gtk_widget_hide_on_delete),NULL);
}


void new_habit_show(void) {
	gtk_entry_set_text(GTK_ENTRY(stuff.frequency),"86400");
	update_readable_frequency();
	gtk_entry_set_text(GTK_ENTRY(stuff.description),"");
	gtk_adjustment_set_value(stuff.importancederp, 0.5);
	gtk_adjustment_set_value(stuff.freqadjderp, starting_point);
	gtk_widget_show_all(stuff.top);
}
