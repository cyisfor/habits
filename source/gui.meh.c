#include "glade.gen.h"

void color_for(float ratio, GDKRGBA* color) {
	float r,g,b;
	b = 0;
	if(ratio >= 1) {
		g = 0;
		r = 1;
	} else if(ratio <= -1) {
		g = 1;
		r = 0;
	} else {
		g = -0.5 * (ratio - 1);
		r = 0.5 * (ratio - 1);
	}
	color->red = r;
	color->green = g;
	color->blue = b;
	// color->alpha = 1;
}

GdkRGBA black = { 0, 0, 0, 1 };
GdkRGBA grey = {0.95, 0.95, 0.95, 1};
GdkRGBA white = {1, 1, 1, 1};

static void toggle_enable(GtkCellRendererToggle* btn, char* path, GtkListStore* items) {
	GtkTreePath* path = gtk_tree_path_new_from_string(path);
	GtkTreeIter iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(items),path,&iter);
	gtk_tree_path_free(path);

	gtk_list_store_set(items,gtk_tree_model_get(

void gui_run(gui ui) {
	GtkBuilder* b = gtk_builder_new_from_string(glade, glade_size);
#define O(type,name) GTK_ ## type(gtk_builder_get_object(b, name));
	ui->top = O(WINDOW,"top");
	ui->items = O(LIST_STORE,"items");
	ui->selection = O(TREE_SELECTION, "selection");
#undef O
	
#define S(what,sig,handle,ptr)										\
	g_signal_connect_swapped(gtk_builder_get_object(b,#name), sig, handle, ptr)
	S(top,"destroy",gtk_main_quit,NULL);
	S(disabled,"toggled",toggle_enable,ui->items);
	S(didit,"clicked",complete_selected,ui->selection);
#undef S

	GtkCssProvider* css = gtk_css_provider_new();
	gtk_css_provider_load_from_data(css,
																	style,
																	style_size,
																	NULL);
	GtkStyleContext* ctx = gtk_window_get_style_context(ui->top);
	gtk_style_context_add_provider(ctx, css,
																 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	gtk_widget_show_all(GTK_WIDGET(ui->top));
	gtk_main();
}
