#include "db.h"
#include "litlen.h"
#include "base.sql.h"
#include "pending.sql.h"

#include <sqlite3.h>
#include <error.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h> // NULL
#include <time.h> // clock_*

sqlite3_stmt *set_enabled_stmt = NULL,
	*next_pending_stmt = NULL,
	*perform_stmt = NULL;

sqlite3* db = NULL;

static sqlite3_int64 clock_now() {
	struct timespec now;
	clock_gettime(CLOCK_REALTIME_COARSE,&now);
	return now.tv_sec * 1000L + now.tv_nsec / 1000000;
}

static void check(int res) {
	if(res == SQLITE_DONE || res == SQLITE_ROW || res == SQLITE_OK) return;
	error(0,sqlite3_errcode(db),"%d(%s): %s",
				sqlite3_errcode(db),
				sqlite3_errstr(res),
				sqlite3_errmsg(db));
	abort();
}

void db_create_habit(const char* desc, ssize_t desclen,
										 double importance, sqlite3_int64 frequency) {
	check(sqlite3_bind_text(create_find_stmt, 1, desc, desclen, NULL));
	int res = sqlite3_step(create_find_stmt);
	if(res == SQLITE_ROW) {
		sqlite3_int64 ident = sqlite3_column_int64(create_find_stmt, 0);
		check(sqlite3_bind_double(create_update_stmt, 1, importance));
		check(sqlite3_bind_int64(create_update_stmt, 2, frequency));
		check(sqlite3_bind_int64(create_update_stmt, 3, ident));
		res = sqlite3_step(create_update_stmt);
		check(sqlite3_reset(create_update_stmt));
		check(res);
		return;
	}
	check(res);
	check(sqlite3_bind_double(create_insert_stmt, 1, importance));
	check(sqlite3_bind_int64(create_insert_stmt, 2, frequency));
	check(sqlite3_bind_text(create_insert_stmt, 3, desc, desclen, NULL));
	res = sqlite3_step(create_insert_stmt);
	check(sqlite3_reset(create_insert_stmt));
	check(res);
}	

void db_set_enabled(long ident, bool enabled) {
	check(sqlite3_bind_int(set_enabled_stmt, 1, ident));
	check(sqlite3_bind_int(set_enabled_stmt, 2, enabled ? 1 : 0));
	int res = sqlite3_step(set_enabled_stmt);
	check(sqlite3_reset(set_enabled_stmt));
	check(res);
}

bool db_next_pending(struct db_habit* self) {
	int res = sqlite3_step(next_pending_stmt);
	if(res == SQLITE_DONE) {
		sqlite3_reset(next_pending_stmt);
		return false;
	}
	check(res);
	assert(res == SQLITE_ROW);
	self->ident = sqlite3_column_int64(next_pending_stmt, 0);
	self->description = sqlite3_column_text(next_pending_stmt, 1);
	self->frequency = sqlite3_column_double(next_pending_stmt, 2);
	self->has_performed = sqlite3_column_int(next_pending_stmt, 3) == 0;
	if(self->has_performed) {
		self->elapsed = clock_now() - sqlite3_column_int64(next_pending_stmt, 4);
	}
	return true;
}

void db_perform(sqlite_int64 ident) {
	sqlite3_bind_int64(perform_stmt,1,ident);
	sqlite3_bind_int64(perform_stmt,2,clock_now());
	int res = sqlite3_step(perform_stmt);
	sqlite3_reset(perform_stmt);
	check(res);
}

void db_done(void) {
	sqlite3_finalize(set_enabled_stmt);
	sqlite3_finalize(next_pending_stmt);
	sqlite3_finalize(perform_stmt);
	check(sqlite3_close(db));
}

void sqlite_millinow(sqlite3_context* ctx, int narg, sqlite3_value** args) {
	sqlite3_result_int64(ctx, clock_now());
}

void db_init(void) {
	assert(0==sqlite3_open("habits.sqlite",&db));
	assert(db!= NULL);
	char* errmsg = NULL;
	int res = sqlite3_exec(db, base_sql, NULL, NULL, &errmsg);
	if(res != 0) {
		error(1,res,errmsg);
	}

	sqlite3_create_function(db, "millinow", 0, SQLITE_UTF8, NULL,
													sqlite_millinow, NULL, NULL);
	
	#define PREPARE(what,sql) \
		check(sqlite3_prepare_v2(db, LITLEN(sql), &what, NULL));
	PREPARE(set_enabled_stmt, "UPDATE habits SET enabled = ?2 WHERE id = ?1");
	PREPARE(next_pending_stmt,pending_sql);
	// XXX: must set elapsed to custom `now` - last_performed for query ordering
	PREPARE(perform_stmt, "UPDATE habits SET last_performed = ?2 WHERE id = ?1");
}
