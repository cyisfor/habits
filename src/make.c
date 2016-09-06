#include "path.h"

#include "string_array.h"
#include "target_array.h"
#include "myassert/assert.h"


#include <unistd.h> // fork
#include <error.h>
#include <sys/wait.h> // waitpid



string_array cflags;
string_array ldflags;

void string_free(const char* s) {}

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
		objects.length
		+ cflags.length
		+ ldflags.length
		+ 4; // don't forget +1 for the trailing NULL

	const char** args = malloc(sizeof(char**)*nobj);
	args[0] = getenv("CC");
	int i = 0;
	for(i=0;i<cflags.length;++i) {
		args[i+1] = cflags.items[i+1];
	}
	args[++i] = "-o";
	args[++i] = dest;
	for(i=0;i<objects.length;++i) {
		args[++i] = objects.items[i].path;
	}
	for(i=0;i<ldflags.length;++i) {
		args[++i] = ldflags.items[i];
	}
	assert(i == nobj - 1);
	args[nobj-1] = NULL;

	execvp(args[0],(char**)args);
}

// MUST use a malloc'd path for every target_alloc...
target* target_alloc(char* path) {
	target* target = malloc(sizeof(target));
	target->path = path;
	target->updated = (0 == stat(target->path,&target->info));
}

void target_free(target* target) {
	free((char*)target->path);
	free(target);
}

bool left_is_older(struct stat left, struct stat right) {
	if(left.st_mtime < right.st_mtime) return true;
	if(left.st_mtime == right.st_mtime)
		if(left.st_mtim.tv_nsec < right.st_mtim.tv_nsec) return true;
	return false;
}

target* depends(target* dest, target* source) {
	// only call this when you can respond to ->updated by building
	if(!dest->updated) {
		if(left_is_older(dest->info, source->info)) {
			// dest->build()
			dest->updated = true;
		}
	}
	return dest;
}

/* only return a target when it has been COMPLETELY built and updated */

target* program(const char* name, target_array objects) {
	target* target = target_alloc(build_path("bin",name));
	int i;
	for(i=0;i<objects.length;++i) {
		if(depends(target,objects.items[i])) {
			build_program(target->path, objects);
		}
	}
	return target;
}

void build_object(const char* target, const char* source) {
	if(spawn()) return;

	int nobj = cflags.length+6;
	const char** args = malloc(sizeof(char**)*nobj);

	args[0] = getenv("CC");
	int i = 0;
	for(i=0;i<cflags.length;++i) {
		args[i+1] = cflags.items[i];
	}
	args[++i] = "-c";
	args[++i] = "-o";
	args[++i] = target;
	args[++i] = source;
	assert(i == nobj - 1);
	args[nobj-1] = NULL;

	execvp(args[0],args,NULL);
}

const char* object_obj = "obj/";
const char* object_src = "src/";

struct target* object(const char* name, ...) {
	target* target = target_alloc(build_path(object_obj,add_ext(name,"o")));
	target source = {
		.path: build_path(object_src,name)
	}
	if(depends(target,&source)) {
		build_object(target->path, source.path);
	} else {
		va_list args;
		va_start(args, name);
		for(;;) {
			target* header = va_arg(args,target*);
			if(header == NULL) break;
			if(depends(target,header)) {
				build_object(target->path, source.path);
				break;
			}
		}
	}
	free(source.path); // ehhh
	return target;
}

/* update: no ownership, no freeing anything, way too messy
	 use mark/rewind allocation if needed. */

void do_generate(const char* exe, const char* target, const char* source) {
	assert(target != NULL); // but source can be NULL
	char* temp = temp_for(target);
	int pid = fork();
	if(pid == 0) {
		setenv("name",name,1);
		fd = open(temp,O_WRONLY|O_CREAT|O_TRUNC,0644);
		assert(fd >= 0);
		dup2(fd, 1);
		close(fd);
		execlp(exe,exe,source,NULL);
	}
	if(waitforok(pid)) {
		rename(temp,target);
	} else {
		unlink(temp);
		error(3,0,"generate failed");
	}
	free(temp);
}

target* resource_exe = NULL;

void generate_resource(const char* name,
											 const char* target,
											 const char* source) {
	setenv("name",name);
	do_generate(resource_exe->path,target,source);
}

struct target* resource(const char* name, const char* source) {
	target* target = target_alloc(build_path("gen",add_ext(name,"h")));
	target starget = {
		.path: source
	};
	assert(0==stat(source,&starget.info));
	if(depends(target,resource_exe) || depends(target,&starget)) {
		generate_resource(name, target->path, source);
	}
	return target;
}

struct target* generate(const char* dest, target* program) {
	target* target = target_alloc(dest);
	if(depends(target,program)) {
		do_generate(program->path, dest, NULL);
	}
	return target;
}

struct target* template(const char* dest, const char* source, ...) {
	char* temp = temp_for(dest);
	target* target = target_alloc(build_path("gen",dest));
	target starget = {
		.path: source,
	};
	assert(0==stat(source,&starget.info));
	if(depends(target,&starget)) {
		va_list args;
		va_start(source, args);
		apply_template(open(temp,O_WRONLY|O_CREAT|O_TRUNC,0644),
									 open(source,O_RDONLY),args);
		va_end(args);
		rename(temp,dest);
	}
	return target;
}

int main(int argc, char *argv[])
{
	assert(getenv("retryderp")==NULL);
	struct SH {
		target* source;
		target* header;
	};

	struct SH sa, ta;

	target_array o;
	target_array_PUSH(o, object("apply_template"));
	template_exe = depends(program("apply_template"),o);
	target_array_clear(&o);

	object_src = "gen/";
	mkdir(object_src);
	sa.source = template("gen/string_array.c",
																 "src/array.template.c",
																 "ELEMENT_TYPE", "const char*",
																 NULL);
	sa.header = template("gen/string_array.h",
																 "src/array.template.h",
																 "ELEMENT_TYPE", "const char*",
																 NULL);
	target* string_array = object("string_array",sa.header,NULL);

	ta.source = template("gen/target_array.c",
																 "src/array.template.c",
																 "ELEMENT_TYPE", "target",
																 NULL);
	ta.header = template("gen/target_array.h",
																 "src/array.template.h",
																 "ELEMENT_TYPE", "target",
																 "INCLUDES", "#include \"target.h\""
																 NULL);

	target* target_array = object("target_array",ta.header,NULL);

	object_src = "src/";

	target_array_PUSH(o, object("make",sa.header,ta.header,NULL));
	target_array_PUSH(o, string_array);
	target_array_PUSH(o, target_array);
	if(program("make",o).updated) {
		setenv("retryderp","1",1);
		execvp(argv[0],argv);
	}
	target_array_clear(&o);


#define PACK "./data_to_header/"
	object_src = PACK;
	target_array_PUSH(o, object("make_specialescapes",NULL));
	target* e = program(PACK"/make_specialescapes", o);
	target_array_clear(o);
	target* special_escapes = generate(PACK"specialescapes.c", e);
	target_free(e);

	target_array_PUSH(o, object("main.c",special_escapes,NULL));
	resource_exe = program("./data_to_header_string/pack",o);
	target_array_clear(&o);

	target* base_sql = resource("base_sql","sql/base.sql");
	target* pending_sql = resource("pending_sql","sql/pending.sql");
	target* searching_sql = resource("searching_sql","sql/searching.sql");

	object_src = "src/";

	target* myassert = object("myassert",NULL);

	target_array_PUSH(o, myassert);
	target_array_PUSH(o, object("db", base_sql, pending_sql, searching_sql, NULL));
	target_array_PUSH(o, object("readable_interval",NULL));
	target_array_PUSH(o, object("path",NULL));

	string_array_PUSH(cflags, "-Isrc");
	object_obj = build_path("obj","checkup");
	object_src = "src/checkup";

	struct {
		target* new_habit;
		target* checkup;
	} glade = {
		resource("new_habit_glade","new_habit.glade.xml"),
		resource("checkup_glade","checkup.glade.xml")
	};

	target_array_PUSH(o, object("main",glade.checkup,NULL));
	target_array_PUSH(o, object1("poke"));
	target_array_PUSH(o, object1("disabled"));
	target_array_PUSH(o, object1("prettify"));
	target_array_PUSH(o, object1("update"));
	target_array_PUSH(o, object("new_habit", glade.new_habit, NULL));

	target* checkup = program("checkup",o);
	target_array_clear(&o);

	return 0;
}
