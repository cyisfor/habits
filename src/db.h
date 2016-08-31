#include <sqlite3.h>
#include <stdbool.h>


struct habit {
	sqlite3_int64 ident;
	sqlite3_int64 elapsed;
	const char* description;
	long frequency;
	bool has_performed;
};

void db_set_enabled(long ident, bool enabled);

bool db_next_pending(struct habit* self);

void db_perform(sqlite_int64 ident);

void db_done(void);

void db_init(void);

