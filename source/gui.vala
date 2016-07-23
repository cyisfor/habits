using Gtk;

[CCode (cname="gui_run")]
void run() {
	var b = new Builder();
	b.add_from_string(glade, glade_size);
	var top = b.get_object("top") as Window;
	var items = b.get_object("items") as Window;
	var selection = b.get_object("selection") as Window;
	Gtk.main();
}