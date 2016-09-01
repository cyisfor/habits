#include "new_habit.glade.h"

#include <gtk/gtk.h>
#include <math.h>

#define NAMES X(top) X(importance) X(description) X(frequency) X(freqadj) X(readable_frequency) X(create_habit)

struct Stuff {
	#define X(name) GtkWidget* name;
	NAMES
	#undef X
	guint updating_importance;
	GtkAdjustment* freqadjderp;
	GtkAdjustment* importancederp;
} stuff = {}; 

double a,b;
gdouble starting_point;
void new_habit_init(void) {
/*
	y = exp(a*x) + b;
	bottom = exp(a*0) + b;
	b = bottom - 1;
	y = exp(a*x) + bottom - 1;
	top = exp(a*1.0) + bottom - 1;
	exp(a*1.0) = top - bottom + 1;
	a * 1.0 = log(top - bottom + 1);
	a = log(top - bottom + 1);

	reversed:
	x = log(y-b) / a
*/
	const int bottom = 300;
	const int top = 86400 * 30;
	a = log(top - bottom + 1);
	b = bottom - 1;
	starting_point = log(86400-b)/a;
}

static gdouble stretch_along(gdouble spot) {
	return (exp(a * spot) + b);
}

static void update_readable_frequency(void) {
	char buf[0x1000];
	ssize_t offset = 0;
	gdouble spot = stretch_along(gtk_adjustment_get_value(stuff.freqadjderp));
	if(spot >= 86400 * 30) {
		int months = (int)(spot / (86400 * 30));
		spot = spot - months;
		offset += snprintf(buf+offset,0x1000-offset,
											 "%d month%s",months,months > 1 ? "s" : "");
	}
	if(spot >= 86400) {
		int days = (int)(spot / 86400);
		spot = spot - days;

		offset += snprintf(buf+offset,0x1000-offset,
											 "%d day%s",days,days > 1 ? "s" : "");
	}
	if(spot >= 3600) {
		int hours = (int)(spot / 3600);
		spot = spot - hours;

		offset += snprintf(buf+offset,0x1000-offset,
											 "%d hour%s",hours,hours > 1 ? "s" : "");
	}
	if(spot >= 60) {
		int minutes = (int)(spot / 60);
		spot = spot - minutes;

		offset += snprintf(buf+offset,"%d minute%s",0x1000-offset,
											 minutes,minutes > 1 ? "s" : "");
	}
	if(spot >= 1) {
			int seconds = (int)(spot / (86400 * 30));
		spot = spot - seconds;

		offset += snprintf(buf+offset,"%d second%s",0x1000-offset,
											 seconds,seconds > 1 ? "s" : "");
	}

	gtk_entry_set_text(stuff.readable_frequency,buf);
}

static void adjust_frequency(void* udata) {
	stuff.updating_importance = 0;
	gdouble spot = stretch_along(gtk_adjustment_get_value(stuff.freqadjderp));
	static char micros[0x1000];
	ssize_t amt = snprintf(micros,0x1000,"%ld\n",(long)spot);
	gtk_entry_set_text(stuff.frequency, micros, amt);
	update_readable_frequency();
}

static gboolean do_create(GtkButton* btn, void* udata) {
	char* err = NULL;
	long frequency = strtol(gtk_entry_get_text(stuff.frequency),10,&err);
	if(frequency == 0) frequency = 600;
	double importance = strtod(gtk_adjustment_get_value(stuff.importance));
	
}

static gboolean
prepare_adjust_frequency (GtkRange     *range,
													 GtkScrollType scroll,
													 gdouble       value,
													 gpointer      user_data) {
	if(stuff.updating_importance) return;
	stuff.updating_importance =
		g_timeout_add(G_PRIORITY_DEFAULT, 100,adjust_frequency,NULL);
}

void setup_new(void) {
	GtkBuilder* b = gtk_builder_new_from_string(new_habit_glade,
																							new_habit_glade_length);

#define X(name) \
	stuff.name = GTK_WIDGET(gtk_builder_get_object(b, #name));
	NAMES
	#undef X

		stuff.freqadjderp = gtk_range_get_adjustment(GTK_RANGE(stuff.freqadj));
	stuff.importancederp = gtk_range_get_adjustment
		(GTK_RANGE(stuff.importance));
	g_signal_connect(stuff.create_habit, "clicked", do_create, NULL);
	g_signal_connect(stuff.freqadj, "change-value", prepare_adjust_frequency, NULL);
}


void show_new(void) {
	gtk_entry_set_text(stuff.frequency,"86400");
	update_readable_frequency();
	gtk_entry_set_text(stuff.description,"");
	gtk_adjustment_set_value(stuff.importancederp, 0.5);
	gtk_adjustment_set_value(stuff.freqadjderp, starting_point);
	gtk_widget_show_all(stuff.top);
}
