void db_set_enabled(long ident, bool enabled);

bool db_next_pending(habit* self);

void db_perform(sqlite_int64 ident);

void db_done(void);

void db_init(void);
