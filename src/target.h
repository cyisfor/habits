#include <sys/stat.h>
#include <stdbool.h>

typedef struct target {
	const char* path;
	struct stat info;
	bool updated;
} *target;
