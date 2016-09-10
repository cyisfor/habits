#ifndef UPDATE_H
#define UPDATE_H

#include <gtk/gtk.h>
#include <cairo.h>

struct update_info {
	GtkStatusIcon* icon;
	GtkTreeModel* items;
	GtkWindow* top;
	cairo_surface_t* surface;
	cairo_surface_t* surfacederp;
	guint updater;
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
	BACKGROUND
};

#endif /* UPDATE_H */
