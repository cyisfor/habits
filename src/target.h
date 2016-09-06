#include <sys/stat.h>
#include <stdbool.h>

typedef struct {
	const char* path;
	struct stat info;
	bool updated;
} target;
