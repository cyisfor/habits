#include <gtk/gtk.h>

struct poke_info {
	GtkStatusIcon* icon;
	GtkTreeModel* items;
	guint poker;
};

void poke_start(struct poke_info*);
void poke_stop(struct poke_info*);
