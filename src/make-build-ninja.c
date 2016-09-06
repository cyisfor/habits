struct target {
	const char* path;
	struct stat info;
	bool statted;
	bool updated;
	void (*build)(target*);
	struct target* dependencies;
} target;

void resolve(target* target) {
	bool rebuild = false;
	if(target->statted == false) {
		target->exists = (0 == stat(target->path,&target->info));
		if(!target->exists) {
			rebuild = true;
		}
	}
	if(!rebuild) {
		// assume depenencies are already resolved
		target* d = target->dependencies;
		while(d) {
			if(target->info.st_mtime < d->info.st_mtime ||
				 (target->info.st_mtime == d->info.st_mtime &&
					target->info.st_mtim.tv_nsec < d->info.st_mtim.tv_nsec)) {
				rebuild = true;
				break;
			}
		}
	}
	if(!rebuild) return;
	target->build(target);
	target->exists = (0 == stat(target->path,&target->info));
	assert(target->exists);
	target->updated = true;
}

#define ELEMENT_TYPE target
#include "array.c"
#undef ELEMENT_TYPE

typedef const char* string;
#define ELEMENT_TYPE string
#include "array.c"
#undef ELEMENT_TYPE

string_array cflags;
string_array ldflags;

void basicflags(void) {
	string_array_push(cflags,

void build_program(const char* dest, array objects) {
	int nobj =
		objects->length
		+ cflags.length
		+ ldflags.length
		+ 5; // don't forget +1 for the trailing NULL

	const char** args = malloc(sizeof(char**)*nobj);
	args[0] = getenv("CC");
	int i = 0;
	for(i=0;i<cflags.length;++i) {
		args[++i] = cflags.items[i].value;
	}
	args[++i] = "-c";
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
	int pid = fork();
	if(pid == 0)
		execvp(args[0],args);
	assert(pid > 0);
	int status = 0;
	assert(pid == waitpid(pid,&status,0));
	if(!WIFEXITED(status) || 0 != WEXITSTATUS(status)) {
		error(1,0,"program failed");
	}
}

/* only return a target when it has been COMPLETELY built and updated */
target* program(const char* name, ...) {
	target* target = malloc(sizeof(target));
	target->path = build_assured_path("bin",name);
	target->updated = (0 != stat(path,&target->info));
	array args;
	va_list_to_array(&args);
	if(target->updated) {
		build_program(name, args);
		return target;
	}
	int i;
	for(i=0;i<args->length;++i) {
		target* dep = args.items[i];
		if(left_is_older(target->info, dep->info)) {
			build_program(name, args);
			target->updated = true;
			return target;
		}
	}
	return target;
}

struct target* object(const char* name, ...) {
	
	va_start(name,args);
	target* self = malloc(sizeof(target));
	self->path = build_path("obj",name);
	self->statted = false;
	self->build = build_object;
	self.dependencies = source(name); // we always depend on a source of our name
	for(;;)
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
