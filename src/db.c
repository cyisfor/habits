#include "db.h"
#include "pq.h"
#include <error.h>

static void check(int res) {
	if(res == SQLITE_DONE || res == SQLITE_ROW) return;
	error(sqlite3_errcode(pq_db),0,"%d(%s): %s",
				sqlite3_errcode(pq_db),
				sqlite3_errstr(res),
				sqlite3_errmsg(pq_db));
}

void db_set_enabled(long ident, bool enabled) {
	check(sqlite3_bind_int(set_enabled_stmt, 1, ident));
	check(sqlite3_bind_int(set_enabled_stmt, 2, enabled ? 1 : 0));
	int res = sqlite3_step(set_enabled_stmt);
	check(sqlite3_reset(set_enabled_stmt));
	check(res);
}

bool db_next_pending(habit* self) {
	int res = sqlite3_step(next_pending_stmt);
	if(res == SQLITE_DONE) {
		sqlite3_reset(next_pending_stmt);
		return false;
	}
	check(res);
	assert(res == SQLITE_ROW);
	self->id = sqlite3_column_int64(next_pending_stmt, 0);
	self->description = sqlite3_column_text(next_pending_stmt, 1);
	self->frequency = sqlite3_column_double(next_pending_stmt, 2);
	self->elapsed = clock_now() - sqlite3_column_int64(next_pending_stmt, 3);
	return true;
}

void db_perform(sqlite_int64 ident) {
	sqlite3_bind_int64(perform_stmt,1,ident);
	int res = sqlite3_step(perform_stmt);
	sqlite3_reset(perform_stmt);
	check(res);
}

void db_done(void) {
	sqlite3_finalize(set_enabled_stmt);
	sqlite3_finalize(next_pending_stmt);
	sqlite3_finalize();...
	check(sqlite3_close(pq_db));
}

void db_init(void) {
	pq_init();
}
