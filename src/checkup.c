#include "db.h"
#include "checkup.glade.h"
#include "litlen.h"

#include <glib.h>
#include <gtk/gtk.h>
#include <libnotify/notify.h>
#include <assert.h>
#include <error.h>


enum Column {
	EHUNNO,
	NAME,
	ELAPSED,
	DISABLED,
	DANGER,
	IDENT,
	BACKGROUND
};

static void color_for(GdkRGBA* dest, float ratio) {
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
COLOR(background, 0.9, 0.9, 0.9, 1.0);

int main(int argc, char *argv[])
{
	puts("\x1b]0;checkup\a");
	fflush(stdout);
	gtk_init(&argc,&argv);
	db_init();
	
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
		GValue label, ident;
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
		while(db_next_pending(&habit)) {
			gchar* elapsed = NULL;
			interval_stringify(&elapsed, habit.elapsed);
			if(habit.has_performed) {
				color_for(&thingy, (habit.elapsed - habit.frequency) /
									habit.frequency);
			}
			
			if(has_row) {
				gtk_list_store_set(GTK_LIST_STORE(items),
													 &row,
													 IDENT, habit.ident,
													 ELAPSED, elapsed,
													 DISABLED, FALSE,
													 NAME, habit.description
													 -1);
				has_row = gtk_tree_model_iter_next(items,&row);
			} else {
				gtk_list_store_insert_with_values(GTK_LIST_STORE(items),
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
			return G_SOURCE_CONTINUE;
		}
		// take off expired items at the end
		while(has_row) {
			has_row = gtk_list_store_remove(GTK_LIST_STORE(items), &row);
		}
	}

	int complete_row(GtkTreeModel *model,
										GtkTreePath *path,
										GtkTreeIter *iter,
										gpointer data) {
		GValue ident;
		gtk_tree_model_get_value(model, iter, EHUNNO, &ident);
		assert(G_VALUE_HOLDS_INT64(&ident));
		db_perform(g_value_get_int64(&ident));
		return TRUE;
	}

	void on_didit() {
		gtk_tree_model_foreach(items, complete_row, NULL);
		update_intervals(NULL);
	}
	void disabled_toggled(gchar* spath) {
		GtkTreePath* path = gtk_tree_path_new_from_string(spath);
		GtkTreeIter iter;
		assert(TRUE==gtk_tree_model_get_iter(items, &iter, path));
		gtk_tree_path_free(path);
		GValue disabledv;
		gtk_tree_model_get_value(items,
														 &iter,
														 DISABLED,
														 &disabledv);
		assert(G_VALUE_HOLDS_BOOLEAN(&disabledv) == TRUE);
		gboolean disabled = g_value_get_boolean(&disabledv);
		gtk_list_store_insert_with_values(GTK_LIST_STORE(items),
															&iter,
															DISABLED,
															disabled == FALSE ? TRUE : FALSE,
															-1);
		GValue ident;
		gtk_tree_model_get_value(items,
														 &iter,
														 IDENT,
														 &ident);
		assert(G_VALUE_HOLDS_INT64(&ident));
		db_set_enabled(g_value_get_int64(&ident), disabled == FALSE);
	}
	guint handle, poker;
	void start() {
		handle = g_timeout_add_seconds(1, update_intervals, NULL);
		poker = g_timeout_add_seconds(600, poke_me, NULL);
	}
	void stop() {
		g_source_remove(handle);
		g_source_remove(poker);
		handle = -1;
		poker = -1;
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

	setup_new();
	
	gtk_main();
	db_done();
	return 0;
}
