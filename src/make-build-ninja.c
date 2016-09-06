

int main(int argc, char *argv[])
{
	if(program("make")
	generate("gen/checkup.glade.h","checkup.glade.xml");
	object_src = "src/checkup/";
	object_obj = "obj/checkup/";
	object_flags = "-Isrc";
	for(i=0;i<NUM(checkups);++i) {
		object(checkups[i]);
	}

	newline();
	
	object_src = "src/";
	object_obj = "obj/";
	object_flags = "";

	template("querying.template.sql","sql/pending.sql",
					 "",
					 "enabled AND ( ( NOT has_performed ) OR (frequency / 1.5 < elapsed))");
	template("querying.template.sql","sql/searching.sql",
					 ", enabled",
					 "description LIKE ?1");
	object("db"); newline();
	object("readable_interval"); 
	
	
	
			
	return 0;
}
