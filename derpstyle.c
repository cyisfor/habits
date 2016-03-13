#include <gtk/gtk.h>
#include <assert.h>

int main(int argc, char *argv[])
{
  int i;
  gtk_init(&argc,&argv);
  //GtkBuilder* b = gtk_builder_new_from_file("derp.glade.xml");
  GtkWidget* top = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  GtkListStore* items = gtk_list_store_new(2,
										   G_TYPE_STRING,
										   G_TYPE_STRING);

										   
  GtkWidget* view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(items));
  gtk_tree_view_insert_column_with_attributes
	(GTK_TREE_VIEW(view),
	 0,
	 "Herp",
	 gtk_cell_renderer_text_new(),
	 "text",0,
	 NULL);
  gtk_tree_view_insert_column_with_attributes
	(GTK_TREE_VIEW(view),
	 1,
	 "Derp",
	 gtk_cell_renderer_text_new(),
	 "text",1,
	 NULL);
  gtk_container_add(GTK_CONTAINER(top),view);
  GtkCssProvider* prov = gtk_css_provider_new();
  gtk_css_provider_load_from_path
	(prov,
	"derp.css",
	 NULL);
  gtk_style_context_add_provider
	(gtk_widget_get_style_context(view),
	 GTK_STYLE_PROVIDER(prov),
	 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  GtkTreeIter iter;
  gtk_tree_model_get_iter_first(GTK_TREE_MODEL(items),&iter);

  for(i=0;i<3;++i) {
	gtk_list_store_insert_with_values
	  (items,&iter,0,
	   0, "Row",
	   1, "Row",
	   -1);
  }
  gtk_widget_show_all(top);
  gtk_main();
  return 0;
}
