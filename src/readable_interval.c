char* readable_interval(long seconds) {
	char buf[0x1000];
	ssize_t offset = 0;
	gdouble spot = seconds;
	gboolean started = FALSE;
	if(spot >= 86400 * 30) {
		int months = (int)(spot / (86400 * 30));
		spot = spot - months * 86400 * 30;
		offset += snprintf(buf+offset,0x1000-offset,
											 "%d month%s",months,months > 1 ? "s" : "");
		started = TRUE;
	}
	if(spot >= 86400) {
		int days = (int)(spot / 86400);
		spot = spot - days * 86400;
		if(started == FALSE)
			buf[offset++] = ' ';
		started = TRUE;
		offset += snprintf(buf+offset,0x1000-offset,
											 "%d day%s",days,days > 1 ? "s" : "");
	}
	if(spot >= 3600) {
		int hours = (int)(spot / 3600);
		spot = spot - hours * 3600;
		if(started == FALSE)
			buf[offset++] = ' ';
		started = TRUE;
		offset += snprintf(buf+offset,0x1000-offset,
											 "%d hour%s",hours,hours > 1 ? "s" : "");
	}
	if(spot >= 60) {
		int minutes = (int)(spot / 60);
		spot = spot - minutes * 60;
		if(started == FALSE)
			buf[offset++] = ' ';
		started = TRUE;
		offset += snprintf(buf+offset,0x1000-offset,"%d minute%s",
											 minutes,minutes > 1 ? "s" : "");
	}
	if(spot >= 1) {
		int seconds = (int)spot;
		// spot = spot - seconds;
		if(started == FALSE)
			buf[offset++] = ' ';
		//started = TRUE;
		offset += snprintf(buf+offset,0x1000-offset,"%d second%s",
											 seconds,seconds > 1 ? "s" : "");
	}
	buf[offset+1] = '\0';

