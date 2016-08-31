#include <time.h>
#include <stdio.h>

int main(void) {
	struct timespec res;
	clock_getres(CLOCK_REALTIME, &res);
	printf("resolution: %ld %ld\n",
				 res.tv_sec, res.tv_nsec);
	return 0;
}
