#include "disabled.h"
#include "define_this.h"
#include <assert.h>
#include "update.h"

void
disabled_toggled (GtkCellRendererToggle *cell_renderer,
									gchar                 *spath,
									gpointer               udata) {
	DEFINE_THIS(GtkTreeModel);
	GtkTreePath* path = gtk_tree_path_new_from_string(spath);
	GtkTreeIter iter;
	assert(TRUE==gtk_tree_model_get_iter(this, &iter, path));
	gtk_tree_path_free(path);
	GValue disabledv = {};
	gtk_tree_model_get_value(this,
													 &iter,
													 DISABLED,
													 &disabledv);
	assert(G_VALUE_HOLDS_BOOLEAN(&disabledv) == TRUE);
	gboolean disabled = g_value_get_boolean(&disabledv)
		== FALSE ? TRUE : FALSE; // toggle it
	gtk_list_store_set(GTK_LIST_STORE(this),
										 &iter,
										 DISABLED,
										 disabled,
										 -1);
	GValue ident = {};
	gtk_tree_model_get_value(this,
													 &iter,
													 IDENT,
													 &ident);
	assert(G_VALUE_HOLDS_INT64(&ident));
	db_set_enabled(g_value_get_int64(&ident), disabled == FALSE);
}
