#include "path.h"

#include "string_array.h"
#include "target_array.h"
#include "myassert/assert.h"
#include "template/apply.h"

#include <unistd.h> // fork
#include <error.h>
#include <sys/wait.h> // waitpid
#include <string.h> // strlen, memcpy
#include <fcntl.h> // open
#include <stdio.h> // rename...
#include <stdlib.h> // setenv



string_array cflags;
string_array ldflags;

#define PKGCONFIG_LINE 0x1000

struct pkgconfig {
	// just to hold for cflags usage
	char cflags[PKGCONFIG_LINE];
	char libs[PKGCONFIG_LINE];
} pkgconfig;

void pkg_config(const char* names) {
	void doit(bool use_cflags) {
		int io[2];
		pipe(io);
		int pid = fork();
		assert(pid >= 0);
		if(pid == 0) {
			dup2(io[1],0);
			close(io[1]);
			execlp("pkg-config",cflags ? "--cflags" : "--libs",names,NULL);
		}
		close(io[1]);
		char* buf = use_cflags ? pkgconfig.cflags : pkgconfig.libs;
		ssize_t amt = read(io[0],buf,0x100);
		for(;;) {
			char* token = strtok(buf," ");
			if(token == NULL) break;
			if(use_cflags)
				string_PUSH(cflags,token);
			else
				string_PUSH(ldflags,token);
		}
		// now buf is chopped up, and cflags isn't pointing to stack data
		// but static data instead, that won't disappear.
	}
	doit(true);
	doit(false);
}

void string_free(const char* s) {}

void init_flags(void) {
	cflags.length = 5;
	cflags.items = NULL;
	string_array_done_pushing(&cflags);
	cflags.items[0] = "-g";
	cflags.items[1] = "-O2";
	cflags.items[2] = "-fdiagnostics-color=always";
	cflags.items[3] = "-Isrc";
	cflags.items[4] = "-Igen";

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

void show(const char* title, char**args) {
	printf("%s: ",title);
	bool first = true;
	while(*args) {
		if(first) first = false;
		else putchar(' ');
		fputs(*args,stdout);
		++args;
	}
	putchar('\n');
}

void build_program(const char* dest, target_array objects) {
	if(spawn()) return;

	int nobj =
		objects.length
		+ cflags.length
		+ ldflags.length
		+ 3; // don't forget +1 for the trailing NULL

	char** args = malloc(sizeof(char**)*nobj);
	args[0] = getenv("CC");
	if(args[0] == NULL) args[0] = "cc";
	int i=0, j = 0;
	for(j=0;j<cflags.length;++j) {
		args[++i] = strdup(cflags.items[j]);
	}
	args[++i] = "-o";
	args[++i] = strdup(dest);
	for(j=0;j<objects.length;++j) {
		args[++i] = strdup(objects.items[j]->path);
	}
	for(j=0;j<ldflags.length;++j) {
		args[++i] = strdup(ldflags.items[j]);
	}
	assert(i == nobj - 1);
	args[nobj] = NULL;
	show("program",args);
	execvp(args[0],(char**)args);
}

// MUST use a malloc'd path for every target_alloc...
target target_alloc(char* path) {
	target self = malloc(sizeof(struct target));
	self->path = path;
	self->updated = (0 != stat(self->path,&self->info));
	return self;
}

void target_free(target target) {
	free((char*)target->path);
	free(target);
}

bool left_is_older(struct stat left, struct stat right) {
	if(left.st_mtime < right.st_mtime) return true;
	if(left.st_mtime == right.st_mtime)
		if(left.st_mtim.tv_nsec < right.st_mtim.tv_nsec) return true;
	return false;
}

target depends(target dest, target source) {
	// only call this when you can respond to ->updated by building
	if(!dest->updated) {
		if(left_is_older(dest->info, source->info)) {
			// dest->build()
			printf("oldered %s %s\n",dest->path,source->path);
			dest->updated = true;
		}
	} 
	return dest;
}

/* only return a target when it has been COMPLETELY built and updated */

target program(const char* name, target_array objects) {
	target target = target_alloc(build_path("bin",name));
	int i;
	for(i=0;i<objects.length;++i) {
		if(depends(target,objects.items[i])->updated) {
			build_program(target->path, objects);
			break;
		}
	}
	return target;
}

void build_object(const char* target, const char* source) {
	if(spawn()) return;

	int nobj = cflags.length+5;
	char** args = malloc(sizeof(char**)*nobj);

	args[0] = getenv("CC");
	if(args[0] == NULL) args[0] = "cc";
	int i = 0;
	for(i=0;i<cflags.length;++i) {
		args[i+1] = strdup(cflags.items[i]);
	}
	args[++i] = "-c";
	args[++i] = "-o";
	args[++i] = strdup(target);
	args[++i] = strdup(source);
	assert(i == nobj - 1);
	args[nobj] = NULL;
	show("object",args);
	execvp(args[0],(char**)args);
}

const char* object_obj = "obj";
const char* object_src = "src";

target object1(const char* name) {
	struct target source = {
		.path = build_path(object_src,add_ext(name,"c"))
	};
	target self = target_alloc(build_path(object_obj,add_ext(name,"o")));
	
	if(depends(self,&source)->updated) {
		build_object(self->path, source.path);
	}
	return self;
}

target object(const char* name, ...) {
	struct target source = {
		.path = build_path(object_src,add_ext(name,"c"))
	};
	target self = target_alloc(build_path(object_obj,add_ext(name,"o")));

	assert(0==stat(source.path,&source.info));
	if(depends(self,&source)->updated) {
		build_object(self->path, source.path);
	} else {
		va_list args;
		va_start(args, name);
		for(;;) {
			target header = va_arg(args,target);
			if(header == NULL) break;
			if(depends(self,header)->updated) {
				build_object(self->path, source.path);
				break;
			}
		}
	}
	free((char*)source.path); // ehhh
	return self;
}

/* update: no ownership, no freeing anything, way too messy
	 use mark/rewind allocation if needed. */

char* temp_for(const char* path) {
	ssize_t len = strlen(path);
	char* ret = malloc(len+6);
	memcpy(ret,path,len);
	memcpy(ret+len,".temp",6);
	return ret;
}

void do_generate(const char* exe, const char* target, const char* source) {
	assert(target != NULL); // but source can be NULL
	char* temp = temp_for(target);
	int pid = fork();
	if(pid == 0) {
		int fd = open(temp,O_WRONLY|O_CREAT|O_TRUNC,0644);
		assert(fd >= 0);
		dup2(fd, 1);
		close(fd);
		printf("generate %s from %s via %s\n",target,source,exe);
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

target resource_exe = NULL;

void generate_resource(const char* name,
											 const char* target,
											 const char* source) {
	setenv("name",name, 1);
	do_generate(resource_exe->path,target,source);
}

target resource(const char* name, const char* source) {
	target self = target_alloc(build_path("gen",add_ext(name,"h")));
	struct target starget = {
		.path = source
	};
	assert(0==stat(source,&starget.info));
	if(depends(self,resource_exe)->updated || depends(self,&starget)->updated) {
		generate_resource(name, self->path, source);
	}
	return self;
}

target generate(const char* dest, target program) {
	target self = target_alloc(strdup(dest));
	if(depends(self,program)->updated) {
		do_generate(program->path, dest, NULL);
	}
	return self;
}

target template(const char* dest, const char* source, ...) {
	char* temp = temp_for(dest);
	target self = target_alloc(build_path("gen",dest));
	struct target starget = {
		.path = source,
	};
	assert(0==stat(source,&starget.info));
	if(depends(self,&starget)->updated) {
		va_list args;
		va_start(args, source);
		apply_template(open(temp,O_WRONLY|O_CREAT|O_TRUNC,0644),
									 open(source,O_RDONLY),args);
		va_end(args);
		rename(temp,dest);
	}
	return self;
}

int main(int argc, char *argv[])
{
	init_flags();
	struct SH {
		target source;
		target header;
	};

	struct SH sa, ta;

	mkdir(object_obj,0755);
	mkdir("gen",0755);
	
	object_src = "gen";
	sa.source = template("string_array.c",
											 "src/array.template.c",
											 "ELEMENT_TYPE", "string",
											 "HEADER", "#include \"target.h\"",
											 NULL);
	sa.header = template("string_array.h",
											 "src/array.template.h",
											 "ELEMENT_TYPE", "string",
											 "HEADER", "typedef const char* string;",
											 NULL);
	target string_array = object("string_array",sa.header,NULL);

	ta.source = template("target_array.c",
											 "src/array.template.c",
											 "ELEMENT_TYPE", "target",
											 NULL);
	ta.header = template("target_array.h",
											 "src/array.template.h",
											 "ELEMENT_TYPE", "target",
											 "INCLUDES", "#include \"target.h\"",
											 NULL);

	target target_array_herpderp = object("target_array",ta.header,NULL);

	object_src = "src";

	target myassert = object("myassert",NULL);
	target path = object("path",NULL);

	target_array o = {};

	target_PUSH(o, object("make",sa.header,ta.header,NULL));
	target_PUSH(o, object1("apply_template"));
	target_PUSH(o, string_array);
	target_PUSH(o, target_array_herpderp);
	target_PUSH(o, myassert);
	target_PUSH(o, path);

	target make = program("make",o);
	if(make->updated) {
		assert(getenv("retryderp")==NULL);
		setenv("retryderp","1",1);
		execvp(argv[0],argv);
	}
	target_array_clear(&o);


#define PACK "./data_to_header_string"
	object_src = PACK;
	target_PUSH(o, object("make_specialescapes",NULL));
	target e = program("make_specialescapes", o);
	target_array_clear(&o);
	target special_escapes = generate(PACK"/specialescapes.c", e);
	target_free(e);

	target_PUSH(o, object("main",special_escapes,NULL));
	resource_exe = program("pack",o);
	target_array_clear(&o);

	target base_sql = resource("base_sql","sql/base.sql");
	target pending_sql = resource("pending_sql","sql/pending.sql");
	target searching_sql = resource("searching_sql","sql/searching.sql");

	object_src = "src";

	target_PUSH(o, myassert);
	target_PUSH(o, object("db", base_sql, pending_sql, searching_sql, NULL));
	target_PUSH(o, object("readable_interval",NULL));
	target_PUSH(o, path);

	struct {
		ssize_t cflags;
		ssize_t ldflags;
	} savepos = {
		cflags.length,
		ldflags.length;
	};
	pkg_config("gtk+-3.0");
	pkg_config("libnotify");
	
	object_obj = build_path("obj","checkup");
	mkdir(object_obj,0755);
	object_src = "src/checkup";

	struct {
		target new_habit;
		target checkup;
	} glade = {
		resource("new_habit_glade","new_habit.glade.xml"),
		resource("checkup_glade","checkup.glade.xml")
	};

	target_PUSH(o, object("main",glade.checkup,NULL));
	target_PUSH(o, object1("poke"));
	target_PUSH(o, object1("disabled"));
	target_PUSH(o, object1("prettify"));
	target_PUSH(o, object1("update"));
	target_PUSH(o, object("new_habit", glade.new_habit, NULL));

	target checkup = program("checkup",o);
	target_array_clear(&o);

	return 0;
}
