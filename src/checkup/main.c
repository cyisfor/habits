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

double getnow() {
	struct timespec spec;
	clock_gettime(CLOCK_MONOTONIC,&spec);
	return spec.tv_sec + spec.tv_nsec / 1000000000.0L;
}

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

	GtkBuilder* b = gtk_builder_new_from_string(checkup_glade,
																							checkup_glade_length);
#define GET(name) { GObject* o = gtk_builder_get_object(b, name); assert(o != NULL); o; }
#define GETW(name) GTK_WIDGET(GET(name))
#define DEFW(name) GtkWidget* name = GETW(#name)
	DEFW(top);
	gtk_window_set_icon_name(GTK_WINDOW(top),"gtk-yes");
	gtk_window_set_default_icon_name("gtk-yes");

	GtkStatusIcon* status = GTK_STATUS_ICON(GET("status"));
	GtkMenu* status_menu = GTK_MENU(GET("status_menu"));
	double last_status_clicked = getnow();
	GtkMenuItem* quit_item = GTK_MENU_ITEM(GET("quit"));

	GtkTreeModel* items = GTK_TREE_MODEL(GET("items"));
	GtkTreeSelection* selection = GTK_TREE_SELECTION(GET("selection"));
	DEFW(didit);
	GtkCellRendererToggle* disabled =
		GTK_CELL_RENDERER_TOGGLE(GET("disabled"));
	GtkToggleButton* update = GTK_TOGGLE_BUTTON(GET("update"));
	DEFW(open_new);

	gtk_window_stick(GTK_WINDOW(top));

	struct poke_info poke_info = {
		icon: status,
		items: items
	};
	struct update_info update_info = {
		icon: status,
		items: items,
		top: GTK_WINDOW(top),
	};
	update_init(&update_info);

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
	gboolean hide(GtkWidget* widget, GdkEvent* event, gpointer udata) {
		if(shown) {
			gtk_widget_hide(top);
			shown = false;
		}
		return TRUE;
	}
	double last_focused = getnow();
	bool focused = true;
	void focus(void) {
		puts("focus");
		focused = true;
//		last_focused = getnow();
	}
	void unfocus(void) {
		puts("unfocus");
		focused = false;
		last_focused = getnow();
	}
	g_signal_connect(top,"focus-in-event",G_CALLBACK(focus), NULL);
	g_signal_connect(top,"focus-out-event",G_CALLBACK(unfocus), NULL);
	void show_or_raise(void) {
		double now = getnow();
		printf("focused ? %d %lf\n",focused,now-last_focused);
		if(shown) {
			if(
					/* we lost the focus, and it's been a while
						 since we lost it */
					!focused &&
					(now - last_focused > 5)) {
				puts("lost the focus");
				gtk_window_present(GTK_WINDOW(top));
			} else {
				puts("focused or recently unfocused");
				gtk_widget_hide(top);
				shown = false;
				above = false;
			}
		} else {
			gtk_widget_show(top);
			shown = true;
		}
		last_status_clicked = now;
	}

	g_signal_connect(top,"delete-event",G_CALLBACK(hide), NULL);
	g_signal_connect(status, "activate", G_CALLBACK(show_or_raise), NULL);

	void icon_offer_quit(GtkStatusIcon *status_icon,
											 guint          button,
											 guint          activate_time,
											 gpointer       user_data) {
		gtk_menu_popup(status_menu, NULL, NULL,
									 gtk_status_icon_position_menu, status,
									 button, activate_time);
	}
	g_signal_connect(status, "popup-menu", G_CALLBACK(icon_offer_quit), NULL);

	g_signal_connect(quit_item, "activate",gtk_main_quit,NULL);

	struct new_habit_info* new_habit = new_habit_init();
	g_signal_connect(open_new,"clicked",G_CALLBACK(new_habit_show),new_habit);

	struct search_info search_info = {
		entry: GTK_ENTRY(GET("search")),
		image: GTK_IMAGE(GET("search_image")),
		start: GETW("search_start"),
		update: &update_info
	};
	search_init(&search_info);
	gtk_main();
	db_done();
	return 0;
}
