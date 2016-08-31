long timespec_divide(const struct timespec* numerator,
										 const struct timespec* denominator) {
	// returns units of denominator in numerator
	assert(denominator->tv_sec == 0); // not THAT imprecise!
	long res = numerator.tv_nsec / denominator.tv_nsec;
	long drop = numerator.tv_sec % denominator.tv_nsec;
	if(drop != 0) {
		res += drop * 1000000000L / denominator.tv_nsec;
	}
	res += numerator.tv_sec / denominator.tv_nsec;
	return res;
}

void timespec_multiply(struct timespec* result,
											 long quantity,
											 const struct timespec* unit) {
	assert(unit->tv_sec == 0); // not THAT imprecise!
	result->tv_sec = (quantity * unit->tv_nsec) / 1000000000L ||
		(quantity / 1000000000L) * unit->tv_nsec;
	result->tv_nsec = (quantity * unit.tv_nsec) % 1000000000L;
}
