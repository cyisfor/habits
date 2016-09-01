#include "db.h"
#include "new_habit.h"
#include "readable_interval.h"

#include "checkup.glade.h"
#include "litlen.h"

#include <glib.h>
#include <gtk/gtk.h>
#include <libnotify/notify.h>
#include <assert.h>
#include <error.h>


enum Column {
	NAME,
	ELAPSED,
	DISABLED,
	DANGER,
	IDENT,
	BACKGROUND
};

static void color_for(GdkRGBA* dest, double ratio) {
	int r,g,b;
	b = 0;
	if(ratio >= 1) {
		g = 0;
		r = 1;
	} else if(ratio <= -1) {
		g = 1;
		r = 0;
	} else {
		g = -0.5 * (ratio - 1);
		r = 0.5 * (ratio + 1);
	}
	dest->red = r;
	dest->green = g;
	dest->blue = b;
}

#define COLOR(name, r,g,b,a) GdkRGBA name = { .red = r, .green = g, .blue = b, .alpha = a };

COLOR(black,0,0,0,0);
COLOR(grey,0.95,0.95,0.95,1.0);
COLOR(white,1,1,1,1);

int main(int argc, char *argv[])
{
	puts("\x1b]0;checkup\a");
	fflush(stdout);
	gtk_init(&argc,&argv);
	db_init();
	new_habit_init();

	GtkBuilder* b = gtk_builder_new_from_string(checkup_glade,
																							checkup_glade_length);
	#define GETFULL(type,conv,name) \
		type* name = conv(gtk_builder_get_object(b, #name))
	#define GET(name) GETFULL(GtkWidget, GTK_WIDGET, name)
	GET(top);
	GETFULL(GtkTreeModel,GTK_TREE_MODEL,items);
	GETFULL(GtkTreeSelection,GTK_TREE_SELECTION,selection);
	GET(didit);
	GETFULL(GtkCellRendererToggle,GTK_CELL_RENDERER_TOGGLE,disabled);
	GET(update);
	GET(view);
	GET(open_new);

	gtk_window_stick(GTK_WINDOW(top));
	g_signal_connect(top,"destroy",gtk_main_quit, NULL);

	void on_notify_closed(NotifyNotification* n, gpointer udata) {
		gint reason;
		g_object_get(n,
								 "closed-reason",
								 &reason,
								 NULL);
		if(reason == 2) {
			gtk_window_present(GTK_WINDOW(top));
			gtk_widget_grab_focus(top);
		}
	}

	int poke_me(void* udata) {
		GtkTreeIter iter;
		if(FALSE == gtk_tree_model_get_iter_first(items, &iter))
			return G_SOURCE_CONTINUE;
		GValue label = {},
			ident = {};
		gtk_tree_model_get_value(items,
														 &iter,
														 0,
														 &label);
		gtk_tree_model_get_value(items,
														 &iter,
														 1,
														 &ident);
		NotifyNotification* n = notify_notification_new
			("",
			 g_value_get_string(&label),
			 "task-due");
		g_signal_connect(n, "closed", G_CALLBACK(on_notify_closed), NULL);
		gtk_widget_show_all(GTK_WIDGET(n));
		return G_SOURCE_CONTINUE;
	}

	void interval_stringify(gchar** res, sqlite3_int64 interval) {
		*res = g_strdup_printf("%ld",interval);
	}


	int update_intervals(void* udata) {
		GtkTreeIter row;
		bool has_row = gtk_tree_model_get_iter_first(items, &row);
		struct db_habit habit;
		GdkRGBA thingy;
		bool odd = false;
		while(db_next_pending(&habit)) {
			const char* elapsed = "never";
			if(habit.has_performed) {
				elapsed = readable_interval(habit.elapsed / 1000, true);
				color_for(&thingy, (habit.elapsed - habit.frequency) /
									(double)habit.frequency);
			}

			if(has_row == TRUE) {
				gtk_list_store_set(GTK_LIST_STORE(items),
													 &row,
													 IDENT, habit.ident,
													 ELAPSED, elapsed,
													 DISABLED, FALSE,
													 NAME, habit.description,
													 -1);
			} else {
				GdkRGBA* background;
				if(odd) {
					background = &grey;
				} else {
					background = &white;
				}
				odd = !odd;
				gtk_list_store_insert_with_values
					(GTK_LIST_STORE(items),
					 &row,
					 -1,
					 IDENT, habit.ident,
					 DISABLED, FALSE,
					 ELAPSED, elapsed,
					 BACKGROUND, background,
					 NAME, habit.description,
					 -1);
			}
			if(habit.has_performed) {
				gtk_list_store_set(GTK_LIST_STORE(items),
													 &row,
													 DANGER, &thingy,
													 -1);
			}
			has_row = gtk_tree_model_iter_next(items,&row);
		}
		// take off expired items at the end
		if(has_row) {
			//has_row = gtk_tree_model_iter_next(items, &row);
			while(has_row) {
				has_row = gtk_list_store_remove(GTK_LIST_STORE(items), &row);
			}
		}
		return G_SOURCE_CONTINUE;
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
		update_intervals(NULL);
	}

	g_signal_connect(didit, "clicked",G_CALLBACK(on_didit), NULL);
	
	void
		disabled_toggled (GtkCellRendererToggle *cell_renderer,
											gchar                 *spath,
											gpointer               user_data) {
		GtkTreePath* path = gtk_tree_path_new_from_string(spath);
		GtkTreeIter iter;
		assert(TRUE==gtk_tree_model_get_iter(items, &iter, path));
		gtk_tree_path_free(path);
		GValue disabledv = {};
		gtk_tree_model_get_value(items,
														 &iter,
														 DISABLED,
														 &disabledv);
		assert(G_VALUE_HOLDS_BOOLEAN(&disabledv) == TRUE);
		gboolean disabled = g_value_get_boolean(&disabledv)
			 == FALSE ? TRUE : FALSE; // toggle it
		gtk_list_store_set(GTK_LIST_STORE(items),
															&iter,
															DISABLED,
															disabled,
															-1);
		GValue ident = {};
		gtk_tree_model_get_value(items,
														 &iter,
														 IDENT,
														 &ident);
		assert(G_VALUE_HOLDS_INT64(&ident));
		db_set_enabled(g_value_get_int64(&ident), disabled == FALSE);
	}
	g_signal_connect(disabled,"toggled",G_CALLBACK(disabled_toggled),NULL);

	guint updater = 0,
		poker = 0;

	void start() {
		printf("start %d %d\n",updater,poker);
		if(updater==0)
			updater = g_timeout_add_seconds(1, update_intervals, NULL);
		if(poker==0)
			poker = g_timeout_add_seconds(600, poke_me, NULL);
	}
	void stop() {
		printf("stop %d %d\n",updater,poker);
		if(updater) g_source_remove(updater);
		if(poker) g_source_remove(poker);
		updater = 0;
		poker = 0;
	}

	void on_update_toggled() {
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(update)) == TRUE) {
			start();
		} else {
			stop();
		}
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(update), TRUE);
	update_intervals(NULL);
	start();
	g_signal_connect(update, "toggled", on_update_toggled, NULL);

	GtkCssProvider* css = gtk_css_provider_new();
	GError* error = NULL;
	gboolean ok =
		gtk_css_provider_load_from_data(css,
																	LITLEN("* { "
																	"font-size: 14pt; "
																	"font-weight: bold; "
																	"}"),
																	&error);
	assert(ok == TRUE);
	gtk_style_context_add_provider(gtk_widget_get_style_context(top),
																 GTK_STYLE_PROVIDER(css),
																 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_widget_show_all(top);

	new_habit_init();
	g_signal_connect(open_new,"clicked",G_CALLBACK(new_habit_show),NULL);

	gtk_main();
	db_done();
	return 0;
}
