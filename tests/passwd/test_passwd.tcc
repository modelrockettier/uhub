#include <uhub.h>
#include <sqlite3.h>

extern const char* dbfile;
extern sqlite3* db;
extern struct linked_list* rows;
extern char* output;

int run_passwd(const char* args);
int get_rows(const char* sql);
int free_rows();
int in_output(const char* str);
int output_lines();
int is_starting_word(const char* word);


EXO_TEST(init_db, {
	/* Remove the users database if it's left over from a previous run */
	unlink(dbfile);
	return 1;
});

EXO_TEST(create_db, {
	return !run_passwd("create");
});

EXO_TEST(open_db_1, {
	int rc = sqlite3_open_v2(dbfile, &db, SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READONLY, NULL);
	return rc == SQLITE_OK;
});

EXO_TEST(table_test_1, {
	return get_rows("SELECT name FROM sqlite_master WHERE type = 'table';") &&
		list_size(rows) == 2;
});

EXO_TEST(table_test_2, {
	struct row_data* r1 = list_get_first(rows);
	struct row_data* r2 = list_get_next(rows);
	return
		r1->count == 1 &&
		r2->count == 1 &&
		str_match(r1->values[0], "name") &&
		str_match(r2->values[0], "users");
});

EXO_TEST(table_test_3, {
	return get_rows("SELECT * FROM users;") &&
		list_size(rows) == 0;
});

EXO_TEST(add_user_1, {
	return !run_passwd("add nick1 pass1");
});

EXO_TEST(get_users_1, {
	return get_rows("SELECT nickname,password,credentials,created,activity FROM users;") &&
		list_size(rows) == 2;
});

EXO_TEST(table_test_5, {
	struct row_data* cols = list_get_first(rows);
	return
		cols->count == 5 &&
		str_match(cols->values[0], "nickname") &&
		str_match(cols->values[1], "password") &&
		str_match(cols->values[2], "credentials") &&
		str_match(cols->values[3], "created") &&
		str_match(cols->values[4], "activity");
});

EXO_TEST(check_user_1, {
	struct row_data* user1 = list_get_next(rows);
	return
		user1->count == 5 &&
		/* nickname */
		str_match(user1->values[0], "nick1") &&
		/* password */
		str_match(user1->values[1], "pass1") &&
		/* credentials */
		str_match(user1->values[2], "user") &&
		/* created (just check if non-null) */
		user1->values[3] && strlen(user1->values[3]) > 0;
});

EXO_TEST(add_user_2, {
	return !run_passwd("add nick2 pass2 admin");
});

EXO_TEST(get_users_2, {
	return get_rows("SELECT nickname,password,credentials,created,activity FROM users;") &&
		list_size(rows) == 3;
});

EXO_TEST(check_user_2_1, {
	list_get_first(rows); /* ignore the column names */
	struct row_data* user1 = list_get_next(rows);
	return
		user1->count == 5 &&
		/* nickname */
		str_match(user1->values[0], "nick1") &&
		/* password */
		str_match(user1->values[1], "pass1") &&
		/* credentials */
		str_match(user1->values[2], "user") &&
		/* created (just check if non-null) */
		user1->values[3] && strlen(user1->values[3]) > 0;
});

EXO_TEST(check_user_2_2, {
	struct row_data* user2 = list_get_next(rows);
	return
		user2->count == 5 &&
		/* nickname */
		str_match(user2->values[0], "nick2") &&
		/* password */
		str_match(user2->values[1], "pass2") &&
		/* credentials */
		str_match(user2->values[2], "admin") &&
		/* created (just check if non-null) */
		user2->values[3] && strlen(user2->values[3]) > 0;
});


EXO_TEST(list_1, {
	return !run_passwd("list");
});

EXO_TEST(list_2, {
	return
		output_lines() > 1 &&
		in_output("CRED") &&
		in_output("NICK") &&
		in_output("LAST-LOGIN");
});

EXO_TEST(list_3, {
	return
		in_output("nick1") &&
		!in_output("pass1") &&
		in_output("user") &&
		in_output("Never");
});

EXO_TEST(list_4, {
	return
		in_output("nick2") &&
		!in_output("pass2") &&
		in_output("admin");
});

EXO_TEST(search_1, {
	return !run_passwd("list k1");
});

EXO_TEST(search_2, {
	return
		output_lines() > 1 &&
		in_output("CRED") &&
		in_output("NICK") &&
		in_output("LAST-LOGIN");
});

EXO_TEST(search_3, {
	return
		in_output("nick1") &&
		!in_output("pass1") &&
		in_output("user") &&
		in_output("Never");
});

/* Don't look for "admin" since it gets printed when DEBUG_SQL is set */
EXO_TEST(search_4, {
	return
		!in_output("nick2") &&
		!in_output("pass2");
});


EXO_TEST(mod_user_1, {
	return !run_passwd("mod nick1 super");
});

EXO_TEST(get_users_3, {
	return get_rows("SELECT nickname,password,credentials,created,activity FROM users;") &&
		list_size(rows) == 3;
});

EXO_TEST(check_user_3_1, {
	list_get_first(rows); /* ignore the column names */
	struct row_data* user1 = list_get_next(rows);
	return
		user1->count == 5 &&
		/* nickname */
		str_match(user1->values[0], "nick1") &&
		/* password */
		str_match(user1->values[1], "pass1") &&
		/* credentials */
		str_match(user1->values[2], "super") &&
		/* created (just check if non-null) */
		user1->values[3] && strlen(user1->values[3]) > 0;
});

EXO_TEST(check_user_3_2, {
	struct row_data* user2 = list_get_next(rows);
	return
		user2->count == 5 &&
		/* nickname */
		str_match(user2->values[0], "nick2") &&
		/* password */
		str_match(user2->values[1], "pass2") &&
		/* credentials */
		str_match(user2->values[2], "admin") &&
		/* created (just check if non-null) */
		user2->values[3] && strlen(user2->values[3]) > 0;
});

EXO_TEST(pass_user_1, {
	return !run_passwd("pass nick2 newpass");
});

EXO_TEST(get_users_4, {
	return get_rows("SELECT nickname,password,credentials,created,activity FROM users;") &&
		list_size(rows) == 3;
});

EXO_TEST(check_user_4_1, {
	list_get_first(rows); /* ignore the column names */
	struct row_data* user1 = list_get_next(rows);
	return
		user1->count == 5 &&
		/* nickname */
		str_match(user1->values[0], "nick1") &&
		/* password */
		str_match(user1->values[1], "pass1") &&
		/* credentials */
		str_match(user1->values[2], "super") &&
		/* created (just check if non-null) */
		user1->values[3] && strlen(user1->values[3]) > 0;
});

EXO_TEST(check_user_4_2, {
	struct row_data* user2 = list_get_next(rows);
	return
		user2->count == 5 &&
		/* nickname */
		str_match(user2->values[0], "nick2") &&
		/* password */
		str_match(user2->values[1], "newpass") &&
		/* credentials */
		str_match(user2->values[2], "admin") &&
		/* created (just check if non-null) */
		user2->values[3] && strlen(user2->values[3]) > 0;
});

EXO_TEST(del_user_1, {
	return !run_passwd("del nick1");
});

EXO_TEST(get_users_5, {
	return get_rows("SELECT nickname,password,credentials,created,activity FROM users;") &&
		list_size(rows) == 2;
});

EXO_TEST(check_user_5, {
	list_get_first(rows); /* ignore the column names */
	struct row_data* user2 = list_get_next(rows);
	return
		user2->count == 5 &&
		/* nickname */
		str_match(user2->values[0], "nick2") &&
		/* password */
		str_match(user2->values[1], "newpass") &&
		/* credentials */
		str_match(user2->values[2], "admin") &&
		/* created (just check if non-null) */
		user2->values[3] && strlen(user2->values[3]) > 0;
});


EXO_TEST(help_1, {
	return
		/* Just make sure it was able to run the command */
		run_passwd("-h") >= 0 &&
		output_lines() > 1 &&
		is_starting_word("Usage:") &&
		is_starting_word("create") &&
		is_starting_word("add") &&
		is_starting_word("del") &&
		is_starting_word("mod") &&
		is_starting_word("list");
});
