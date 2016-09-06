#include "update.h"

struct search_info {
	GtkEntry* entry;
	GtkWidget* box;
	GtkWidget* mainbox;
	GtkWidget* start;
	GtkWidget* done;
	struct update_info* update;
	guint entry_changer;
	guint searcher;
};

void search_init(struct search_info* this);
