#ifndef UPDATE_H
#define UPDATE_H

#include <gtk/gtk.h>
#include <cairo.h>

struct update_info {
	GtkStatusIcon* icon;
	GtkTreeModel* items;
	// MUST wrap this in a GtkTreeModelSort since the db
	// assumes the store is in the order it provides
	GtkTreeSortable* sortable_items;
	GtkWindow* top;
	cairo_surface_t* surface;
	guint updater;
	gint last_ratio;
};

void update_init(struct update_info* this);
int update_intervals(void* udata);
void update_start(struct update_info* this);
void update_stop(struct update_info* this);


enum Column {
	NAME,
	ELAPSED,
	DISABLED,
	DANGER,
	IDENT,
	BACKGROUND,
	ELAPSED_ORDER
};

#endif /* UPDATE_H */
