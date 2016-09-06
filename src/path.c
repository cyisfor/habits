#include "path.h"

#include <stdarg.h>


const char* add_ext(const char* name, const char* ext) {
	static char buf[0x100];
	ssize_t len = strlen(name);
	ssize_t elen = strlen(ext);
	assert(len + elen + 2 < 0x100);
	memcpy(buf,name,len);
	buf[len] = '.';
	memcpy(buf+len+1,ext,elen);
	buf[elen] = '\0';
	return buf;
}

char* build_beeg_path(const char* head, ...) {
	char* result = strdup(head);
	ssize_t space = strlen(head);
	ssize_t len = space;
	va_list args;
	va_start(head, args);
	for(;;) {
		const char* component = va_arg(args, const char*);
		if(component == NULL) return result;
		// assure that the directory exists for this component.
		mkdir(result);
		// don't assure that the final component is a directory
		// (since it's probably a file)
		ssize_t clen = strlen(component);
		if(len + clen + 2 >= space) {
			do {
				ssize_t needed = len + clen - space;
				if(needed < 0x100) needed = 0x100;
				// try to keep space at even 0x100 multiples...
				space = ((space + needed) >> 8) <<8;
			} until(len + clen + 2 < space);
			result = realloc(result,space);
		}
		result[len] = '/'; // was \0
		memcpy(result+len+1,component,clen);
		len += clen + 1;
		result[len] = '\0';
	}
}

char* build_path(const char* head, const char* tail) {
	ssize_t hlen = strlen(head);
	ssize_t tlen = strlen(tail);
	char* ret = malloc(hlen+tlen+2);
	memcpy(ret,head,hlen);
	ret[hlen] = '/';
	memcpy(ret+hlen+1,tail,tlen);
	ret[hlen+tlen+1] = '\0';
	return ret;
}
