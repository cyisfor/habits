#include <sqlite3.h>
#include <stdbool.h>
#include <stdlib.h> // ssize_t


struct db_habit {
	sqlite3_int64 ident;
	sqlite3_int64 elapsed;
	const char* description;
	long frequency;
	bool has_performed;
};

void db_set_enabled(long ident, bool enabled);

bool db_next_pending(struct db_habit* self);

void db_perform(sqlite_int64 ident);

void db_done(void);

void db_init(void);

bool db_create_habit(const char* desc, ssize_t desclen,
										 double importance, sqlite3_int64 frequency);
