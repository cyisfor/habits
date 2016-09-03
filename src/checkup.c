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
	double r,g,b;
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
	notify_init("checkup");
	
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
	GET(search_start);
	GET(search);
	GET(search_done);
	GET(mainbox);
	GET(searchbox);

	gtk_window_stick(GTK_WINDOW(top));
	g_signal_connect(top,"destroy",gtk_main_quit, NULL);

	struct poke_info poke_info = {
		items: items,
	};
	struct update_info update_info = {
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
		update_intervals(update_info);
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

	void start() {
		update_start(update_info);
		poke_start(poke_info);
	}
	void stop() {
		update_stop(update_info);
		poke_stop(poke_info);
	}

	void on_update_toggled() {
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(update)) == TRUE) {
			start();
		} else {
			stop();
		}
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(update), TRUE);
	update_intervals(update_info);
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


	guint searcher = 0;
	
	gboolean search_for_stuff(void* udata) {
		db_search(gtk_entry_get_text(GTK_ENTRY(search)),
							gtk_entry_get_text_length(GTK_ENTRY(search)));
		searcher = 0;
		return G_SOURCE_REMOVE;
	}
	
	void search_later() {
		if(searcher != 0) g_source_remove(searcher);
		searcher = g_timeout_add(100,search_for_stuff,NULL);
	}

	gulong search_changer = 0;
	
	void switch_to_search() {
		gtk_widget_hide(mainbox);
		gtk_widget_show_all(searchbox);
		if(search_changer == 0)
			search_changer = g_signal_connect(search,"changed",search_later,NULL);
		search_for_stuff(NULL);
	}

	void switch_to_main() {
		if(searcher) g_source_remove(searcher);
		if(search_changer) {
			g_signal_handler_disconnect(search, search_changer);
			search_changer = 0;
		}
		gtk_widget_hide(searchbox);
		gtk_widget_show_all(mainbox);
		db_stop_searching();
		update_intervals(NULL);
	}
	gtk_widget_hide(searchbox);
	g_signal_connect(search_start,"clicked",switch_to_search,NULL);
	g_signal_connect(search_done,"clicked",switch_to_main,NULL);
	gtk_main();
	db_done();
	return 0;
}
