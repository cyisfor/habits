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
#include <assert.h>

static void hsl_to_rgb(GdkRGBA* dest, double h, double s, double li) {
	if(s == 0) {
		// achromatic shortcut
		dest->red = li;
		dest->green = li;
		dest->blue = li;
		return;
	}
	double q = li < 0.5 ? li * (1 + s) : li + s - li * s;
	double p = 2 * li - q;
	// hue to... channel?
	double h2c(double t) {
		if(t < 0) ++t;
		if(t > 1) --t;
		if(6 * t < 1) return p + (q - p) * 6 * t;
		if(2 * t < 1) return q;
		if(3 * t < 2) return p + (q - p) * (2 - 3*t) * 2;
		return p;
	}
	dest->red = h2c(h + 1/3.0);
	dest->green = h2c(h);
	dest->blue = h2c(h - 1/3.0);
}

static void color_for(GdkRGBA* dest, double ratio) {
#define L 0.8
	if(ratio >= 1) {
		dest->green = 0;
		dest->red = L;
	} else if(ratio <= -1) {
		dest->green = L;
		dest->red = 0;
	} else if(ratio < 0) {
		// between green and yellow
		// r=-1, g = 1, r = 0
		// r=0, g = 1, r = 1
		dest->green = L;
		dest->red = (ratio + 1)*L;
	} else {
		// between yellow and red
		// r=0 g = 1, r = 1
		// r=1, g = 0, r = 1
		dest->green = L*(1 - ratio);
		dest->red = L;
	}
	dest->blue = 0;
}

#define COLOR(name, r,g,b,a) GdkRGBA name = { .red = r, .green = g, .blue = b, .alpha = a };

COLOR(black,0,0,0,0);
COLOR(grey,0.95,0.95,0.95,1.0);
COLOR(white,1,1,1,1);

const gint size = 64;
const gint border = 12;

void update_init(struct update_info* this) {
	/* stupid window managers... alt-tab thing stupidly clips off the edges,
		 unless it's small enough. Then it scales it up and THEN clips off the
		 edges -_- so, need a border no matter what.
	*/
	this->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
																						 size + (border<<1), size);
	// don't bother cairo_surface_destroy(surface);
}

int update_intervals(gpointer udata) {
	DEFINE_THIS(struct update_info);
	GtkTreeIter row;
	bool has_row = gtk_tree_model_get_iter_first(this->items, &row);
	struct db_habit habit;
	GdkRGBA thingy;
	bool odd = false;
	double max_ratio = -1;
	bool got_ratio = false;
	while(db_next(&habit)) {
		const char* elapsed = "never";
		if(habit.has_performed) {
			elapsed = readable_interval(habit.elapsed / 1000, true);
			double ratio = (habit.elapsed - habit.frequency) /
				(double)habit.frequency;
			if(got_ratio == false) {
				max_ratio = ratio;
				got_ratio = true;
			} else if (ratio > max_ratio) {
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
	cairo_t* cairo = cairo_create(this->surface);
	cairo_set_source_rgb(cairo,
											 thingy.red,
											 thingy.green,
											 thingy.blue);
	cairo_arc(cairo,size>>1,size>>1,size>>1,0,2*M_PI);
	cairo_fill(cairo);
	cairo_clip(cairo);

	GdkPixbuf* icon =
		gdk_pixbuf_get_from_surface(this->surface,
																0,0,
																size,size);
	gtk_status_icon_set_from_pixbuf(this->icon,icon);

	// now offset it, so we can use the border
	cairo_arc(cairo,(size>>1) + border,size>>1,size>>1,0,2*M_PI);
	cairo_fill(cairo);
	cairo_clip(cairo);

	icon =
		gdk_pixbuf_get_from_surface(this->surface,
																0,0,
																size + (border<<1), size);
	gtk_window_set_icon(this->top,icon);
	gtk_window_set_default_icon(icon);

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
