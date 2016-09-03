#include <gtk/gtk.h>

struct poke_info {
	GtkTreeModel* items;
	guint poker;
};

int poke_me(gpointer udata);
