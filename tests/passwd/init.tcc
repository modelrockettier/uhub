#include <uhub.h>
#include <sqlite3.h>

#if defined(LOWLEVEL_DEBUG)
#define TEST_LOG_LEVEL log_plugin
#elif defined(DEBUG)
#define TEST_LOG_LEVEL log_debug
#else
#define TEST_LOG_LEVEL log_info
#endif

#ifndef TEST_LOG_FILE
#define TEST_LOG_FILE "passwd-test.log"
#endif

/* Returns 0 = strings are different, 1 (or other non-zero) = strings match */
static int str_match(char const* a, char const* b)
{
	int same;

	if (!a && !b)
		return 1;

	if (!a || !b)
		same = 0;
	else
		same = !strcmp(a, b);

#ifdef DEBUG_TESTS
	if (!same)
	{
		printf("str_match fail: ");

		if (!a)
			printf("a=<null>, ");
		else
			printf("a='%s', ", a);

		if (!b)
			printf("b=<null>\n");
		else
			printf("b='%s'\n", b);
	}
#endif
	return same;
}

EXO_TEST(init_log, {
	hub_log_initialize(TEST_LOG_FILE, 0);
	return 1;
});

EXO_TEST(set_log_verbosity, {
	hub_set_log_verbosity(TEST_LOG_LEVEL);
	return 1;
});

EXO_TEST(get_log_verbosity, {
	int level = hub_get_log_verbosity();
	LOG_INFO("Log level is set to: %d (%s)", level, hub_log_verbosity_to_string(level));
	return level == TEST_LOG_LEVEL;
});

EXO_TEST(check_str_match, {
	return str_match("Hello", "Hello");
});


const char* dbfile = "test_users.db";
sqlite3* db;
struct linked_list* rows;
#define OUTPUT_SIZE 8192
char *output;

int run_passwd(const char* args);
int get_rows(const char* sql);
int free_rows();
int in_output(const char* str);
int output_lines();
int is_starting_word(const char* word);
char const* passwd_path = NULL;


int run_passwd(const char* args)
{
	static char cmd[1024];
	FILE *out;
	size_t bytes_read;

	uhub_assert(passwd_path);
	output[0] = '\0';

	snprintf(cmd, sizeof(cmd), "%s %s %s", passwd_path, dbfile, args);

	out = popen(cmd, "r");
	uhub_assert(out != NULL);

	bytes_read = fread(output, 1, OUTPUT_SIZE - 1, out);

	uhub_assert(bytes_read < OUTPUT_SIZE);
	output[bytes_read] = '\0';

	int rc = pclose(out);
#ifdef DEBUG_TESTS
	if (rc != 0)
	{
		LOG_INFO("run_passwd: \"%s\" returned %d", cmd, rc);
	}
#endif

	return rc;
}

struct row_data {
	int count;
	char* values[];
};

static void free_row_cb(void* ptr)
{
	if (!ptr)
		return;

	int i;
	struct row_data* rd = ptr;

	for (i = 0; i < rd->count; i++)
		hub_free(rd->values[i]);

	hub_free(rd);
}

int free_rows()
{
	list_clear(rows, free_row_cb);
	return 1;
}

static int get_rows_cb(void* ptr, int argc, char **argv, char **colName)
{
	int rc;

	/* Add the column names to the first list element */
	if (list_size(rows) < 1 && colName != NULL)
	{
		rc = get_rows_cb(ptr, argc, colName, NULL);
		if (rc)
			return rc;
	}

	struct row_data* rd = hub_malloc(sizeof(struct row_data) + (sizeof(char*) * argc));
	if (!rd)
		return -1;

	rd->count = argc;

	int i;
	for (i = 0; i < argc; i++)
	{
		if (argv[i])
			rd->values[i] = hub_strdup(argv[i]);
	}

	list_append(rows, rd);
	return 0;
}

/*
 * Note: rows must already be initialized and if successful, the first node
 * in rows will be the column names
 */
int get_rows(const char* sql)
{
	free_rows();
	int rc = sqlite3_exec(db, sql, get_rows_cb, NULL, NULL);
	return rc == SQLITE_OK;
}

int in_output(const char* str)
{
	return (strstr(output, str) != NULL);
}

int output_lines()
{
	int count = 1;
	char* p = strchr(output, '\n');

	while (p)
	{
		p = strchr(p + 1, '\n');
		if (p)
			count++;
	}

	return count;
}

/* Look for a word at the start of a line in the output (ignoring spaces) */
int is_starting_word(const char* word)
{
	char* p = output - 1;
	char* tmp;

	for (;;)
	{
		p = strstr(p + 1, word);
		if (!p)
			break;

		/* Check if the word is followed by whitespace */
		tmp = p + strlen(word);
		switch (*tmp)
		{
			case '\0':
			case '\t':
			case '\r':
			case '\n':
			case ' ':
				break;

			default:
				continue;
		}

		tmp = p - 1;
		while (tmp > output && (*tmp == ' ' || *tmp == '\t'))
			tmp--;

		if (tmp <= output || *tmp == '\n')
			return 1;
	}

#ifdef DEBUG_TESTS
	printf("Could not find starting word \"%s\" in output:\n%s\n", word, output);
#endif
	return 0;
}

EXO_TEST(setup, {
	db = NULL;
	rows = list_create();
	output = hub_malloc(OUTPUT_SIZE);
	if (output)
		output[0] = '\0';
	return output != NULL;
});

const char* PASSWD_PATHS[] = {
#ifdef WIN32
	".\\uhub-passwd",
#else
	"./uhub-passwd",
#endif
	"uhub-passwd"
};

EXO_TEST(run_passwd, {
	size_t i;
	for (i = 0; i < ARRAY_SIZE(PASSWD_PATHS); i++)
	{
		passwd_path = PASSWD_PATHS[i];
		if (run_passwd("help") == 0)
			return 1;
	}

	LOG_FATAL("Could not run uhub-passwd");
	fprintf(stderr, "FATAL: Could not run uhub-passwd\n");

	exit(2);
});
