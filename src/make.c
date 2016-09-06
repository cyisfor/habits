void run(const char* dest, const char* exe) {
	int pid = fork();
	if(pid == -1
