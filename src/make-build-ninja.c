struct target {
	const char* path;
	struct stat info;
	bool updated;
} target;

#define ELEMENT_TYPE target
#include "array.c"
#undef ELEMENT_TYPE

typedef const char* string;
#define ELEMENT_TYPE string
#include "array.c"
#undef ELEMENT_TYPE

string_array cflags;
string_array ldflags;

void init_flags(void) {
	cflags.length = 3;
	cflags.items = NULL;
	string_array_done_pushing(&cflags);
	cflags.items[0] = "-g";
	cflags.items[1] = "-O2";
	cflags.items[2] = "-fdiagnostics-color=always";

	ldflags.length = 0;
	ldflags.items = NULL;
}

bool waitforok(int pid) {
	assert(pid > 0);
	int status = 0;
	assert(pid == waitpid(pid,&status,0));
	if(!WIFEXITED(status) || 0 != WEXITSTATUS(status)) {
		return false;
	}
	return true;
}

bool spawn(void) {
	int pid = fork();
	assert(pid >= 0);
	if(pid != 0) {
		if(!waitforok(pid)) {
			error(23,0,"program failed");
		}
		return true;
	}
	return false;
}

void build_program(const char* dest, target_array objects) {
	if(spawn()) return;

	int nobj =
		objects->length
		+ cflags.length
		+ ldflags.length
		+ 4; // don't forget +1 for the trailing NULL

	const char** args = malloc(sizeof(char**)*nobj);
	args[0] = getenv("CC");
	int i = 0;
	for(i=0;i<cflags.length;++i) {
		args[i+1] = cflags.items[i+1].value;
	}
	args[++i] = "-o";
	args[++i] = program->path;
	for(i=0;i<objects.length;++i) {
		args[++i] = objects.items[i].path;
	}
	for(i=0;i<ldflags.length;++i) {
		args[++i] = ldflags.items[i].value;
	}
	assert(i == nobj - 1);
	args[nobj-1] = NULL;

	execvp(args[0],args);
}

/* only return a target when it has been COMPLETELY built and updated */
target* program(const char* name, ...) {
	target* target = malloc(sizeof(target));
	target->path = build_assured_path("bin",name);
	target->updated = (0 != stat(path,&target->info));
	target_array args;
	va_list_to_targets(&args);
	if(target->updated) {
		build_program(target->path, args);
		return target;
	}
	int i;
	for(i=0;i<args->length;++i) {
		target* dep = args.items[i];
		if(left_is_older(target->info, dep->info)) {
			build_program(target->path, args);
			target->updated = true;
			return target;
		}
	}
	return target;
}

void build_object(const char* target, const char* source) {
	int nobj = cflags.length
		+5;

	const char** args = malloc(sizeof(char**)*nobj);
	args[0] = getenv("CC");
	int i = 0;
	for(i=0;i<cflags.length;++i) {
		args[i+1] = cflags.items[i].value;
	}
	args[++i] = "-c";
	args[++i] = "-o";
	args[++i] = target;
	args[++i] = source;
	assert(i == nobj - 1);
	args[nobj-1] = NULL;

		
	
		
}

const char* object_obj = "obj/";
const char* object_src = "src/";

struct target* object(const char* name, ...) {
	target* target = malloc(sizeof(target));
	target->path = build_path(object_obj,build_ext(name,".o"));
	const char* source = build_path(object_src,name);

	target->updated = (0 != stat(path,&target->info));
	if(target->updated) {
		build_object(target->path,source);
		return target;
	}
	struct stat source_info;
	// manually generate main source before building the object if necessary
	assert(0==stat(source,&source_info));

	if(left_is_older(target->info,source_info)) {
		build_object(target->path, source);
		target->updated = true;
		return target;
	}

	va_list headers;
	va_start(headers, name);
	for(;;) {
		target* header = va_arg(headers,target*);
		if(left_is_older(target->info,header->info)) {
			build_object(target->path, source->path);
			target->updated = true;
			return target;
		}
	}
	assert(target->updated == false);
	return target;
}

void generate_resource(const char* name, const char* target, const char* source) {
	int pid = fork();
	char* temp = temp_for(target);
	if(pid == 0) {
		setenv("name",name,1);
		fd = open(temp,O_WRONLY|O_CREAT|O_TRUNC,0644);
		assert(fd >= 0);
		dup2(fd, 1);
		close(fd);
		execlp("./data_to_header_string/pack","pack",source,NULL);
	}
	if(waitforok(pid)) {
		rename(temp,target);
	} else {
		unlink(temp);
		error(3,0,"generate failed");
	}
	free(temp);
}


struct target* resource(const char* name, target* source) {
	target* target = malloc(sizeof(target));
	target->path = build_path("gen",add_ext(name,".h"));
	target->updated = (0 == stat(target->path,&target->info));
	if(target->updated) {
		generate_resource(name, target->path, source->path);
		return target;
	}
	if(left_is_older(target->info, source->info)) {
		generate_resource(name, target->path, source->path);
		return target;
	}
}

int main(int argc, char *argv[])
{
	assert(getenv("retryderp")==NULL);
	const char* o = object("apply_template");
	if(program("make",).updated) {
		setenv("retryderp","1",1);
		execvp(argv[0],argv);
	}
	generate("gen/checkup.glade.h","checkup.glade.xml");
	object_src = "src/checkup/";
	object_obj = "obj/checkup/";
	string_array_push(cflags);
	cflags.items[cflags.length] = "-Isrc";

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
