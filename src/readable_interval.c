#include "readable_interval.h"

#include <stdio.h>
#include <stdbool.h>

const char* readable_interval(long seconds, bool shorten) {
	static char buf[0x1000];
	
	ssize_t offset = 0;
	double spot = seconds;
	bool started = false;
#define REDUCE(unit, name, letter)																			\
	if(spot >= (unit)) {																									\
		int name ## _s = (int)(spot / (unit));															\
		spot = spot - name ## _s * (unit);																	\
		if(shorten) {																												\
			offset += snprintf(buf+offset,0x1000-offset,"%d" letter,name ## _s);	\
		} else {																														\
			if(started == true) {																							\
				buf[offset++] = ' ';																						\
			} else {																													\
				started = true;																									\
			}																																	\
			if(months == 1) {																									\
				memcpy(buf+offset,LITLEN("1 " #name));													\
				buf += sizeof("1 " #name)-1;																		\
			} else {																													\
				offset += snprintf(buf+offset,0x1000-offset,										\
													 "%d " #name "s", name ## _s);								\
			}																																	\
		}																																		\
	}
	REDUCE(86400*30,month,"m");
	REDUCE(86400,day,"d");
	REDUCE(3600,hour,"h");
	REDUCE(60,minute,"m");
	REDUCE(1,second,"s");
	buf[offset+1] = '\0';

	return buf;
}
