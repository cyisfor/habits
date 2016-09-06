#include "prettify.h"

#include "litlen.h"

#include <assert.h>

void prettify(GtkWidget* top) {	
	GtkCssProvider* css = gtk_css_provider_new();
	GError* error = NULL;
	gboolean ok =
		gtk_css_provider_load_from_data(css,
																		LITLEN("* { "
																					 "font-size: 14pt; "
																					 "font-weight: bold; "
																					 "}"),
																		&error);
	assert(ok == TRUE);
	gtk_style_context_add_provider(gtk_widget_get_style_context(top),
																 GTK_STYLE_PROVIDER(css),
																 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}
