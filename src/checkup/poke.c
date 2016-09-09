#include <glib.h>
#define GDK_VERSION_MIN_REQUIRED (G_ENCODE_VERSION(3,12))

#include "poke.h"
#include "define_this.h"

#include <libnotify/notify.h>
#include <stdlib.h> // random
#include <error.h>


static void on_notify_closed(NotifyNotification* n, gpointer udata) {
	DEFINE_THIS(GtkWindow);
	gint reason;
	g_object_get(n,
							 "closed-reason",
							 &reason,
							 NULL);
	if(reason == 2) {
		gtk_window_present(this);
		gtk_widget_grab_focus(GTK_WIDGET(this));
	}
}

static int poke_me(gpointer udata) {
	DEFINE_THIS(struct poke_info);
	GtkTreeIter iter;
	if(FALSE == gtk_tree_model_get_iter_first(this->items, &iter))
		return G_SOURCE_CONTINUE;
	int cutoff = 2;
	long rando = random();
	for(;;) {
		long derp = rando%cutoff;
		//printf("derp %d %ld %ld\n",cutoff,derp,rando % 1000);
		if(0 != derp) {
			break;
		}
		if(FALSE==gtk_tree_model_iter_next(this->items, &iter)) {
			gtk_tree_model_get_iter_first(this->items,&iter);
			break;
		}
		++cutoff;
	}
	GValue label = {}, ident = {};
	gtk_tree_model_get_value(this->items,
													 &iter,
													 0,
													 &label);
	gtk_tree_model_get_value(this->items,
													 &iter,
													 1,
													 &ident);
	const char* message = g_value_get_string(&label);
	gtk_status_icon_set_tooltip_text(this->icon, message);
	NotifyNotification* n = notify_notification_new
		("",
		 message,
		 "task-due");
	g_signal_connect(n, "closed", G_CALLBACK(on_notify_closed), this);
	GError* err = NULL;
	if(FALSE==notify_notification_show(n, &err)) {
		error(0,0,err->message);
	}
	return G_SOURCE_CONTINUE;
}

void poke_start(struct poke_info* this) {
	if(this->poker==0)
		this->poker = g_timeout_add_seconds(600, poke_me, this);
}

void poke_stop(struct poke_info* this) {
	if(this->poker) {
		g_source_remove(this->poker);
		this->poker = 0;
	}
}
