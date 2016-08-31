#include "db.h"
#include "checkup_glade.h"

int int main(int argc, char *argv[])
{
	puts("\x1b]0;checkup\a");
	fflush(stdout);
	GtkBuilder* b = gtk_builder_new_from_string(checkup_glade,
																							checkup_glade_length);
	GtkWidget* top = GTK_WIDGET(gtk_builder_get_object(b, "top"));
	return 0;
}
