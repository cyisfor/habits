#ifndef UPDATE_H
#define UPDATE_H

#include <gtk/gtk.h>
#define GDK_DISABLE_DEPRECATED_WARNINGS
#include <gtk/deprecated/statusicon.h>

struct update_info {
	GtkStatusIcon* icon;
	GtkTreeModel* items;
	GtkWindow* top;
	guint updater;
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
