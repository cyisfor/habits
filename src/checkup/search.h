#include "update.h"

struct search_info {
	GtkEntry* entry;
	GtkImage* image;
	GtkWidget* start;
	struct update_info* update;
	guint entry_changer;
	guint searcher;
	gboolean shown;
};

void search_init(struct search_info* this);
