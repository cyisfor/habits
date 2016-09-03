#include "context_shortcuts.h"

void on_notify_closed(NotifyNotification* n, gpointer udata) {
	gint reason;
	g_object_get(n,
							 "closed-reason",
							 &reason,
							 NULL);
	if(reason == 2) {
		gtk_window_present(GTK_WINDOW(top));
		gtk_widget_grab_focus(top);
	}
}

int poke_me(gpointer udata) {
	GtkTreeIter iter;
	if(FALSE == gtk_tree_model_get_iter_first(items, &iter))
		return G_SOURCE_CONTINUE;
	int cutoff = 2;
	long rando = random();
	for(;;) {
		long derp = rando%cutoff;
		//printf("derp %d %ld %ld\n",cutoff,derp,rando % 1000);
		if(0 != derp) {
			break;
		}
		if(FALSE==gtk_tree_model_iter_next(items, &iter)) {
			gtk_tree_model_get_iter_first(items,&iter);
			break;
		}
		++cutoff;
	}
	GValue label = {},
		ident = {};
		gtk_tree_model_get_value(items,
														 &iter,
														 0,
														 &label);
		gtk_tree_model_get_value(items,
														 &iter,
														 1,
														 &ident);
		NotifyNotification* n = notify_notification_new
			("",
			 g_value_get_string(&label),
			 "task-due");
		g_signal_connect(n, "closed", G_CALLBACK(on_notify_closed), NULL);
		GError* err = NULL;
		if(FALSE==notify_notification_show(n, &err)) {
			error(0,0,err->message);
		}
		return G_SOURCE_CONTINUE;
}
