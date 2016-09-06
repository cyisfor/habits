#include "db.h"
#include "litlen.h"
#include "base_sql.h"
#include "pending_sql.h"
#include "searching_sql.h"
#include <sqlite3.h>
#include <error.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h> // NULL
#include <time.h> // clock_*
#include <unistd.h> // chdir

sqlite3_stmt *begin_stmt = NULL,
	*commit_stmt = NULL;

sqlite3_stmt *set_enabled_stmt = NULL,
	*next_pending_stmt = NULL,
	*next_searching_stmt = NULL,
	*perform_stmt = NULL,
	*history_stmt = NULL;

sqlite3_stmt *create_find_stmt = NULL,
	*create_update_stmt = NULL,
	*create_insert_stmt = NULL;

sqlite3* db = NULL;

static sqlite3_int64 clock_now() {
	struct timespec now;
	clock_gettime(CLOCK_REALTIME_COARSE,&now);
	return now.tv_sec * 1000L + now.tv_nsec / 1000000;
}

static void check(const char* file, int line, int res) {
	if(res == SQLITE_DONE || res == SQLITE_ROW || res == SQLITE_OK) return;
	error(0,res,"%s:%d %d(%s)",
				file,line,
				sqlite3_errcode(db),
				sqlite3_errstr(res));
	abort();
}
#define CHECK(a) check(__FILE__, __LINE__, (a))


void begin() {
	CHECK(sqlite3_step(begin_stmt));
	sqlite3_reset(begin_stmt);
}

void commit() {
	CHECK(sqlite3_step(commit_stmt));
	sqlite3_reset(commit_stmt);
}

bool db_create_habit(const char* desc, ssize_t desclen,
										 double importance, sqlite3_int64 frequency) {
	begin();
	CHECK(sqlite3_bind_text(create_find_stmt, 1, desc, desclen, NULL));
	int res = sqlite3_step(create_find_stmt);
	if(res == SQLITE_ROW) {
		sqlite3_int64 ident = sqlite3_column_int64(create_find_stmt, 0);
		sqlite3_reset(create_find_stmt);
		CHECK(sqlite3_bind_double(create_update_stmt, 1, importance));
		CHECK(sqlite3_bind_int64(create_update_stmt, 2, frequency));
		CHECK(sqlite3_bind_int64(create_update_stmt, 3, ident));
		res = sqlite3_step(create_update_stmt);
		CHECK(sqlite3_reset(create_update_stmt));
		CHECK(res);
		commit();
		return false;
	}
	sqlite3_reset(create_find_stmt);
	CHECK(res);
	CHECK(sqlite3_bind_double(create_insert_stmt, 1, importance));
	CHECK(sqlite3_bind_int64(create_insert_stmt, 2, frequency));
	CHECK(sqlite3_bind_text(create_insert_stmt, 3, desc, desclen, NULL));
	res = sqlite3_step(create_insert_stmt);
	CHECK(sqlite3_reset(create_insert_stmt));
	CHECK(res);
	commit();
	return true;
}	

void db_set_enabled(long ident, bool enabled) {
	CHECK(sqlite3_bind_int(set_enabled_stmt, 1, ident));
	CHECK(sqlite3_bind_int(set_enabled_stmt, 2, enabled ? 1 : 0));
	int res = sqlite3_step(set_enabled_stmt);
	CHECK(sqlite3_reset(set_enabled_stmt));
	CHECK(res);
}

bool searching = false;
const char* search = NULL;

void db_stop_searching(void) {
	searching = false;
	sqlite3_reset(next_searching_stmt);
}

void db_search(const char* query, ssize_t querylen) {
	sqlite3_reset(next_searching_stmt);
	CHECK(sqlite3_bind_text(next_searching_stmt, 1, query, querylen, NULL));
	searching = true;
	sqlite3_reset(next_pending_stmt);
}

bool db_next(struct db_habit* self) {
	sqlite3_stmt* stmt;
	if(searching) stmt = next_searching_stmt;
	else stmt = next_pending_stmt;
	
	int res = sqlite3_step(stmt);
	if(res == SQLITE_DONE) {
		sqlite3_reset(stmt);
		return false;
	}
	CHECK(res);
	assert(res == SQLITE_ROW);
	self->ident = sqlite3_column_int64(stmt, 0);
	self->description = sqlite3_column_text(stmt, 1);
	self->frequency = sqlite3_column_double(stmt, 2);
	self->has_performed = sqlite3_column_int(stmt, 3) != 0;
	if(self->has_performed) {
		self->elapsed = sqlite3_column_int64(stmt, 4);
	}
	if(searching) {
		self->enabled = sqlite3_column_int(stmt,5) != 0;
	} else {
		self->enabled = true;
	}
	return true;
}

void db_perform(sqlite_int64 ident) {
	begin();
	sqlite3_int64 now = clock_now();
	sqlite3_bind_int64(perform_stmt,1,ident);
	sqlite3_bind_int64(perform_stmt,2,now);
	int res = sqlite3_step(perform_stmt);
	sqlite3_reset(perform_stmt);
	CHECK(res);
	sqlite3_bind_int64(history_stmt,1,ident);
	sqlite3_bind_int64(history_stmt,2,now);
	res = sqlite3_step(history_stmt);
	sqlite3_reset(history_stmt);
	CHECK(res);
	commit();
}

void db_done(void) {
	sqlite3_finalize(set_enabled_stmt);
	sqlite3_finalize(next_searching_stmt);
	sqlite3_finalize(next_pending_stmt);
	sqlite3_finalize(perform_stmt);
	sqlite3_finalize(history_stmt);
	sqlite3_finalize(create_find_stmt);
	sqlite3_finalize(create_update_stmt);
	sqlite3_finalize(create_insert_stmt);
	sqlite3_finalize(begin_stmt);
	sqlite3_finalize(commit_stmt);
	CHECK(sqlite3_close(db));
}

void sqlite_millinow(sqlite3_context* ctx, int narg, sqlite3_value** args) {
	sqlite3_result_int64(ctx, clock_now());
}

void db_init(void) {
	chdir(getenv("HOME"));
	chdir(".local"); // don't bother check if success, just put it in home
	assert(0==sqlite3_open("habits.sqlite",&db));
	chdir("/");
	assert(db!= NULL);
	sqlite3_busy_timeout(db, 10000);
	char* errmsg = NULL;
	int res = sqlite3_exec(db, base_sql, NULL, NULL, &errmsg);
	if(res != 0) {
		error(1,res,errmsg);
	}

	sqlite3_create_function(db, "millinow", 0, SQLITE_UTF8, NULL,
													sqlite_millinow, NULL, NULL);
	
	#define PREPARE(what,sql) \
		CHECK(sqlite3_prepare_v2(db, LITLEN(sql), &what, NULL));
	PREPARE(set_enabled_stmt, "UPDATE habits SET enabled = ?2 WHERE id = ?1");
	PREPARE(next_pending_stmt,pending_sql);
	PREPARE(next_searching_stmt,searching_sql);
	// XXX: must set elapsed to custom `now` - last_performed for query ordering
	PREPARE(perform_stmt, "UPDATE habits SET last_performed = ?2 WHERE id = ?1");
	PREPARE(history_stmt, "INSERT INTO HISTORY (habit,performed) VALUES (?,?)");

	PREPARE(create_find_stmt,"SELECT id FROM habits WHERE description = ?");
	PREPARE(create_update_stmt,"UPDATE habits SET importance = ?, frequency = ? WHERE id = ?");
	PREPARE(create_insert_stmt,"INSERT INTO habits (importance,frequency,description) VALUES (?,?,?)");
	PREPARE(begin_stmt,"BEGIN");
	PREPARE(commit_stmt,"COMMIT");
}
