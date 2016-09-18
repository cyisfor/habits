#include "search.h"
#include "define_this.h"
#include "db.h"

#include <gtk/gtk.h>
#include <assert.h>
	
gboolean search_for_stuff(void* udata) {
	DEFINE_THIS(struct search_info);
	db_search(gtk_entry_get_text(this->entry),
						gtk_entry_get_text_length(this->entry));
	this->searcher = 0;
	return G_SOURCE_REMOVE;
}

static void search_later(GtkEntry* e, void* udata) {
	DEFINE_THIS(struct search_info);
	if(this->searcher != 0) g_source_remove(this->searcher);
	this->searcher = g_timeout_add(100,search_for_stuff,this);
}

static void toggle_search(GtkButton* btn, void* udata) {
	DEFINE_THIS(struct search_info);
	if(this->shown == TRUE) {
		this->shown = FALSE;
		if(this->searcher) g_source_remove(this->searcher);
		if(this->entry_changer) {
			g_signal_handler_disconnect(this->entry, this->entry_changer);
			this->entry_changer = 0;
		}
		gtk_widget_hide(GTK_WIDGET(this->entry));
		//gtk_widget_show_all(this->mainbox);
		db_stop_searching();
		gtk_image_set_from_icon_name(this->image, "gtk-find",
																 GTK_ICON_SIZE_BUTTON);
		update_intervals(this->update);
	} else {
		this->shown = TRUE;
		gtk_widget_show(GTK_WIDGET(this->entry));
		if(this->entry_changer == 0)
			this->entry_changer =
				g_signal_connect(this->entry,"changed",G_CALLBACK(search_later),this);
		search_for_stuff(this);
		gtk_image_set_from_icon_name(this->image, "gtk-close",
																 GTK_ICON_SIZE_BUTTON);
		gtk_entry_grab_focus_without_selecting(this->entry);
		gtk_editable_set_position(GTK_EDITABLE(this->entry),-1);
	}
}

void search_init(struct search_info* this) {
	this->shown = FALSE;
	assert(this->update != NULL);
	g_signal_connect(this->start,"clicked",G_CALLBACK(toggle_search),this);
}
