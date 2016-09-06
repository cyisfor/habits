#include <sys/stat.h>
#include <stdbool.h>

typedef struct {
	char* path;
	struct stat info;
	bool updated;
} target;
