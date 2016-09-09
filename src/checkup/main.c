#include "db.h"
#include "new_habit.h"
#include "readable_interval.h"

#include "checkup_glade.h"
#include "litlen.h"

#include <glib.h>
#define GDK_VERSION_MIN_REQUIRED (G_ENCODE_VERSION(3,12))

#include <gtk/gtk.h>

#include <libnotify/notify.h>

#include "poke.h"
#include "update.h"
#include "disabled.h"
#include "search.h"
#include "prettify.h"



#include <assert.h>
#include <error.h>

int main(int argc, char *argv[])
{
	if(getenv("ferrets") == NULL) {
		setenv("ferrets","1",1);
		setenv("name","habits",0);
		execlp("daemonize","daemonize",argv[0],NULL);
		abort();
	}
	puts("\x1b]0;checkup\a");
	fflush(stdout);
	
	gtk_init(&argc,&argv);
	db_init();
	notify_init("checkup");

	GtkStatusIcon* icon = gtk_status_icon_new_from_icon_name("gtk-yes");
	assert(icon);
	gtk_status_icon_set_tooltip_text(icon, "Habits");
	
	GtkBuilder* b = gtk_builder_new_from_string(checkup_glade,
																							checkup_glade_length);
#define GET(name) { GObject* o = gtk_builder_get_object(b, name); assert(o != NULL); o; }
#define GETW(name) GTK_WIDGET(GET(name))
#define DEFW(name) GtkWidget* name = GETW(#name)
	DEFW(top);
	gtk_window_set_icon_name(GTK_WINDOW(top),"gtk-yes");
	gtk_window_set_default_icon_name("gtk-yes");
	GtkTreeModel* items = GTK_TREE_MODEL(GET("items"));
	GtkTreeSelection* selection = GTK_TREE_SELECTION(GET("selection"));
	DEFW(didit);
	GtkCellRendererToggle* disabled =
		GTK_CELL_RENDERER_TOGGLE(GET("disabled"));
	GtkToggleButton* update = GTK_TOGGLE_BUTTON(GET("update"));
	DEFW(open_new);

	gtk_window_stick(GTK_WINDOW(top));

	struct poke_info poke_info = {
		icon: icon,
		items: items
	};
	struct update_info update_info = {
		icon: icon,
		items: items,
		top: GTK_WINDOW(top),
	};

	void interval_stringify(gchar** res, sqlite3_int64 interval) {
		*res = g_strdup_printf("%ld",interval);
	}


	void complete_row(GtkTreeModel *model,
                                GtkTreePath *path,
                                GtkTreeIter *iter,
                                gpointer data) {
		GValue ident = {};
		gtk_tree_model_get_value(model, iter, IDENT, &ident);
		assert(G_VALUE_HOLDS_INT64(&ident));
		db_perform(g_value_get_int64(&ident));
	}

	void on_didit() {
		gtk_tree_selection_selected_foreach(selection, complete_row, NULL);
		gtk_tree_selection_unselect_all(selection);
		update_intervals(&update_info);
	}

	g_signal_connect(didit, "clicked",G_CALLBACK(on_didit), NULL);
	
	g_signal_connect(disabled,"toggled",G_CALLBACK(disabled_toggled), items);

	void start() {
		update_start(&update_info);
		poke_start(&poke_info);
	}
	void stop() {
		update_stop(&update_info);
		poke_stop(&poke_info);
	}

	void on_update_toggled() {
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(update)) == TRUE) {
			start();
		} else {
			stop();
		}
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(update), TRUE);
	update_intervals(&update_info);
	start();
	g_signal_connect(update, "toggled", on_update_toggled, NULL);

	prettify(top);
	bool shown = false;
	void toggle_shown(void) {
		if(shown) {
			gtk_widget_hide(top);
			shown = false;
		} else {
			gtk_widget_show_all(top);
			shown = true;
		}
	}

	g_signal_connect(top,"delete-event",G_CALLBACK(toggle_shown), NULL);
	g_signal_connect(icon, "activate", G_CALLBACK(toggle_shown), NULL);

	struct new_habit_info* new_habit = new_habit_init();
	g_signal_connect(open_new,"clicked",G_CALLBACK(new_habit_show),new_habit);

	struct search_info search_info = {
		entry: GTK_ENTRY(GET("search")),
		box: GETW("searchbox"),
		mainbox: GETW("mainbox"),
		start: GETW("search_start"),
		done: GETW("search_done"),
		update: &update_info
	};
	search_init(&search_info);
	gtk_main();
	db_done();
	return 0;
}
