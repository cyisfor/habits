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

char** va_list_to_array(va_list args) {
	char** ret = NULL;
	size_t space = 0;
	size_t length = 0;
	for(;;) {
		if(length == space) {
			space += 0x100;
			ret = realloc(ret,sizeof(*ret)*0x100);
		}
		char* value = va_arg(args,char*);
		ret[length++] = value;
		if(value == NULL) break;
	}
	return realloc(ret,length);
}

void build_program(const char* dest, char** objects) {
	int nobj = 0;
	va_list temp;
	va_copy(temp,objects);
	for(;;) {
		++nobj;
		d = d->next;
	}
	flag* flag = cflags;
	while(flag) {
		++nobj;
		flag = flag->next;
	}

	flag = ldflags;
	while(flag) {
		++nobj;
		flag = flag->next;
	}

	nobj += 5;
	const char** args = malloc(sizeof(char**)*nobj);
	args[0] = getenv("CC");
	int i = 0;
	flag = cflags;
	while(flag) {
		args[++i] = flag->value;
		flag = flag->next;
	}
	args[++i] = "-c";
	args[++i] = "-o";
	args[++i] = program->path;
	d = program->dependencies;
	while(d) {
		args[++i] = d->path;
		d = d->next;
	}
	flag = ldflags;
	while(flag) {
		args[++i] = flag->value;
		flag = flag->next;
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
	va_list vargs;
	va_start(name,vargs);
	char** args = va_list_to_array(vargs);
	va_end(vargs);
	if(target->updated) {
		build_program(args);
		return target;
	}
	va_list buildargs;
	va_copy(buildargs,args);
	for(;;) {
		target* dep = va_arg(args,target*);
		if(dep == NULL) break;
		if(left_is_older(target->info, dep->info)) {
			build_program(buildargs);
			target->updated = true;
			return target;
		}
	}
	return self;
}

struct target* object(const char* name, ...) {
	va_list args;
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
