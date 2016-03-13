#include <gtk/gtk.h>
#include <assert.h>

#define LITLEN(s) s, sizeof(s)-1
#define BGET(name) (GtkWidget*)gtk_builder_get_object(b,name)
int main(int argc, char *argv[])
{
  int i;
  gtk_init(&argc,&argv);
  GtkBuilder* b = gtk_builder_new_from_file("checkup.glade.xml");
  GtkWidget* view = BGET("view");
  GtkCssProvider* prov = gtk_css_provider_new();
  gtk_css_provider_load_from_data
	(prov,
	 LITLEN("GtkTreeView row:nth-child(even) {\n"
	 "background-color: #FF0000;\n"
	 "}\n"
	 "GtkTreeView row:nth-child(odd) {\n"
	 "background-color #00FFFF;\n"
	 "}\n"),
	 NULL);
  gtk_style_context_add_provider
	(gtk_widget_get_style_context(view),
	 prov,
	 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  GtkWidget* top = BGET("top");

  GtkListStore* items = (GtkListStore*)BGET("items");
  GtkTreeIter iter;
  assert(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(items),&iter)==0);
  for(i=0;i<3;++i) {
	gtk_list_store_insert_with_values
	  (items,&iter,
	   0, "Derp",
	   1, "Herp",
	   2, TRUE,
	   -1);
  }
  gtk_widget_show_all(top);
  gtk_main();
  return 0;
}
