#include <glib.h>
#define GDK_VERSION_MIN_REQUIRED (G_ENCODE_VERSION(3,12))
#include "update.h"

#include "readable_interval.h"

#include "db.h"
#include "define_this.h"

#include <glib.h> // G_SOURCE_CONTINUE etc
#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h> // M_PI


static void color_for(GdkRGBA* dest, double ratio) {
	double r,g,b;
	b = 0;
	if(ratio >= 1) {
		g = 0;
		r = 1;
	} else if(ratio <= -1) {
		g = 1;
		r = 0;
	} else {
		g = -0.5 * (ratio - 1);
		r = 0.5 * (ratio + 1);
	}
	dest->red = r;
	dest->green = g;
	dest->blue = b;
}

#define COLOR(name, r,g,b,a) GdkRGBA name = { .red = r, .green = g, .blue = b, .alpha = a };

COLOR(black,0,0,0,0);
COLOR(grey,0.95,0.95,0.95,1.0);
COLOR(white,1,1,1,1);

int update_intervals(gpointer udata) {
	DEFINE_THIS(struct update_info);
	GtkTreeIter row;
	bool has_row = gtk_tree_model_get_iter_first(this->items, &row);
	struct db_habit habit;
	GdkRGBA thingy;
	bool odd = false;
	double max_ratio = -1;
	while(db_next(&habit)) {
		const char* elapsed = "never";
		if(habit.has_performed) {
			elapsed = readable_interval(habit.elapsed / 1000, true);
			double ratio = (habit.elapsed - habit.frequency) /
				(double)habit.frequency;
			if(ratio > max_ratio) {
				max_ratio = ratio;
			}
			color_for(&thingy, ratio);
		}

		if(has_row == TRUE) {
			gtk_list_store_set(GTK_LIST_STORE(this->items),
												 &row,
												 IDENT, habit.ident,
												 ELAPSED, elapsed,
												 DISABLED, habit.enabled ? FALSE : TRUE,
												 NAME, habit.description,
												 -1);
		} else {
			GdkRGBA* background;
			if(odd) {
				background = &grey;
			} else {
				background = &white;
			}
			odd = !odd;
			gtk_list_store_insert_with_values
				(GTK_LIST_STORE(this->items),
				 &row,
				 -1,
				 IDENT, habit.ident,
				 DISABLED, FALSE,
				 ELAPSED, elapsed,
				 BACKGROUND, background,
				 NAME, habit.description,
				 -1);
		}
		if(habit.has_performed) {
			gtk_list_store_set(GTK_LIST_STORE(this->items),
												 &row,
												 DANGER, &thingy,
												 -1);
		}
		has_row = gtk_tree_model_iter_next(this->items,&row);
	}

	color_for(&thingy,max_ratio);
	cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
																												64,64);
	cairo_t* cairo = cairo_create(surface);
	cairo_set_source_rgb(cairo,
											 thingy.red,
											 thingy.green,
											 thingy.blue);
	cairo_arc(cairo,32,32,32,0,2*M_PI);
	cairo_fill(cairo);
	cairo_clip(cairo);
	cairo_destroy(cairo);
	GdkPixbuf* icon = gdk_pixbuf_get_from_surface(surface,
																								0,0,
																								64,64);
	cairo_surface_destroy(surface);
	gtk_window_set_icon(this->top,icon); 
	gtk_window_set_default_icon(icon);
	gtk_status_icon_set_from_pixbuf(this->icon, icon);

	// XXX: g_unref(icon) ?

	// take off expired this->items at the end
	if(has_row) {
		//has_row = gtk_tree_model_iter_next(this->items, &row);
		while(has_row) {
			has_row = gtk_list_store_remove(GTK_LIST_STORE(this->items), &row);
		}
	}
	return G_SOURCE_CONTINUE;
}

void update_start(struct update_info* this) {
	if(this->updater==0)
		this->updater = g_timeout_add_seconds(1, update_intervals, this);
}

void update_stop(struct update_info* this) {
	if(this->updater!=0) {
		g_source_remove(this->updater);
		this->updater = 0;
	}
}
