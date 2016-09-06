#include "new_habit.h"
#include "db.h"
#include "readable_interval.h"

#include "new_habit.glade.h"

#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h> // strtol
#include <assert.h>

#define NAMES X(top) X(importance) X(description) X(frequency) X(freqadj) X(readable_frequency) X(create_habit)

struct new_habit_info {
	#define X(name) GtkWidget* name;
	NAMES
	#undef X
	GtkAdjustment* freqadjderp;
	GtkAdjustment* importancederp;
	guint updating_importance;
	guint typing;
};

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
	starting_point = log(86400-b)/a/log(c);
}

static gdouble stretch_along(gdouble spot) {
	return (pow(c, a * spot) + b);
}

static void update_readable_frequency(struct new_habit_info* this) {
	char* err = NULL;
	long frequency = strtol(gtk_entry_get_text(GTK_ENTRY(this->frequency)),
													&err, 10);
	if(err != NULL && *err != '\0') return;

	gtk_label_set_text(GTK_LABEL(this->readable_frequency),
										 readable_interval(frequency, false));
}

static gboolean adjust_frequency(void* udata) {
	DEFINE_THIS(struct new_habit_info);
	this->updating_importance = 0;
	gdouble spot = stretch_along(gtk_adjustment_get_value(this->freqadjderp));
	static char micros[0x1000];
	ssize_t amt = snprintf(micros,0x1000,"%ld",(long)spot);
	gtk_entry_set_text(GTK_ENTRY(this->frequency), micros);
	update_readable_frequency(this);
	return G_SOURCE_REMOVE;
}

static gboolean do_create(GtkButton* btn, void* udata) {
	DEFINE_THIS(struct new_habit_info);
	char* err = NULL;
	long frequency = strtol(gtk_entry_get_text(GTK_ENTRY(this->frequency)),
													&err, 10)
		* 1000; // milliseconds
	assert(err == NULL || *err == '\0');
	if(frequency == 0) frequency = 600000;
	double importance = gtk_adjustment_get_value(this->importancederp);
	gboolean created =
		db_create_habit(gtk_entry_get_text(GTK_ENTRY(this->description)),
									gtk_entry_get_text_length(GTK_ENTRY(this->description)),
									importance,frequency);

	GtkWidget* dialog = gtk_message_dialog_new
		(GTK_WINDOW(this->top),
		 GTK_DIALOG_DESTROY_WITH_PARENT,
		 GTK_MESSAGE_INFO,
		 GTK_BUTTONS_OK,
		 "Habit was %s: %s!",
		 created ?
		 "created" : "updated",
		 gtk_entry_get_text(GTK_ENTRY(this->description)));
	g_signal_connect(dialog,"response",G_CALLBACK(gtk_widget_destroy),NULL);
	gtk_widget_show_all(dialog);
	gtk_widget_hide(this->top);
}

static gboolean
prepare_adjust_frequency (GtkRange     *range,
													 GtkScrollType scroll,
													 gdouble       value,
													 gpointer      udata) {
	DEFINE_THIS(struct new_habit_info);
	if(this->updating_importance) return FALSE;
	this->updating_importance =
		g_timeout_add(100,adjust_frequency,this);
}

static gboolean update_text_freq(void* udata) {
	DEFINE_THIS(struct new_habit_info);
	update_readable_frequency(this);
	this->typing = 0;
	return G_SOURCE_REMOVE;
}

static void on_text_freq(GtkEditable* thing, gpointer udata) {
	if(this->typing) {
		g_source_remove(this->typing);
	}
	this->typing = g_timeout_add(500,update_text_freq,this);
}

struct new_habit_info* new_habit_init(void) {
	calc_constants();
	GtkBuilder* b = gtk_builder_new_from_string(new_habit_glade,
																							new_habit_glade_length);
	struct new_habit_info* this = malloc(sizeof(struct new_habit_info));
	this->typing = 0;
	this->updating_importance = 0;

#define X(name) \
	this->name = GTK_WIDGET(gtk_builder_get_object(b, #name));
	NAMES
	#undef X

	this->freqadjderp = gtk_range_get_adjustment(GTK_RANGE(this->freqadj));
	this->importancederp = gtk_range_get_adjustment
		(GTK_RANGE(this->importance));
	g_signal_connect(this->create_habit, "clicked",
									 G_CALLBACK(do_create), this);
	g_signal_connect(this->freqadj, "change-value",
									 G_CALLBACK(prepare_adjust_frequency), this);
	g_signal_connect(this->frequency, "changed",
									 G_CALLBACK(on_text_freq),this);
	g_signal_connect(this->top, "delete-event",G_CALLBACK(gtk_widget_hide_on_delete),this);
	return this;
}


void new_habit_show(struct new_habit_info* this) {
	gtk_entry_set_text(GTK_ENTRY(this->frequency),"86400");
	update_readable_frequency();
	gtk_entry_set_text(GTK_ENTRY(this->description),"");
	gtk_adjustment_set_value(this->importancederp, 0.5);
	gtk_adjustment_set_value(this->freqadjderp, starting_point);
	gtk_widget_show_all(this->top);
}
