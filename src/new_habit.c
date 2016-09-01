void setup_new(void) {
	GtkBuilder* b = gtk_builder_new_from_string(new_habit_glade,
																							new_habit_glade_length);
#define GETFULL(type,conv,name)													\
	type* name = conv(gtk_builder_get_object(b, #name))
#define GET(name) GETFULL(GtkWidget, GTK_WIDGET, name)
	GET(top);
	GET(importance);
	GET(description);
	GET(frequency);
	GET(freqadj);
	GET(readable_frequency);
	GET(create_habit);

	g_signal_connect(create_habit, "clicked", do_create)

	GtkBuilder* b = gtk_builder_new_from_
