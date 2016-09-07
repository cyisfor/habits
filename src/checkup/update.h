#ifndef UPDATE_H
#define UPDATE_H

#include <gtk/gtk.h>

struct update_info {
	GtkTreeModel* items;
	GtkWindow* top;
	guint updater;
	gboolean alarmed;
};

int update_intervals(void* udata);
void update_start(struct update_info* this);
void update_stop(struct update_info* this);


enum Column {
	NAME,
	ELAPSED,
	DISABLED,
	DANGER,
	IDENT,
	BACKGROUND
};

#endif /* UPDATE_H */
