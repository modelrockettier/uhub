#include <uhub.h>
#include <sqlite3.h>

extern const char* dbfile;
extern sqlite3* db;

int free_rows();


EXO_TEST(close_db_1, {
	return sqlite3_close(db) == SQLITE_OK;
});

EXO_TEST(remove_test_db, {
	return !unlink(dbfile);
});

EXO_TEST(destroy, {
	sqlite3_close(db);
	free_rows();
	list_destroy(rows);
	hub_free(output);
	return 1;
});

EXO_TEST(exit_log, {
	hub_log_shutdown();
	return 1;
});
