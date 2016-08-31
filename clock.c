void timespec_exact_divide(struct timespec* numerator, struct timespec* denominator) {
	// returns numerator-> divisor / denominator -> modulus
	// assumes divisor is "small" (for clock_getres)
	int denom = denominator->tv_sec * 1000000000 + denominator->tv_nsec;
	long smod = (numerator->tv_sec % denom) * (1000000000 % denom);
	long sdiv = numerator->tv_sec * (1000000000 / denom);
	long nmod = numerator->tv_nsec % denom;
	long ndiv = numerator->tv_nsec / denom;
	numerator->tv_sec = sdiv;
	numerator->tv_nsec = ndiv;
	denominator->tv_sec = smod;
	denominator->tv_nsec = nmod;
}

long timespec_divide(struct timespec* numerator,
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
