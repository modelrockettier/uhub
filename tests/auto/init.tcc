#include <uhub.h>

#if defined(LOWLEVEL_DEBUG)
#define TEST_LOG_LEVEL log_plugin
#elif defined(DEBUG)
#define TEST_LOG_LEVEL log_debug
#else
#define TEST_LOG_LEVEL log_info
#endif

#ifndef TEST_LOG_FILE
#define TEST_LOG_FILE "test.log"
#endif

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

/*
 * Expect result to be 1 of 2 possible strings, don't warn if it matches either
 * Returns 0 = strings are different, 1 (or other non-zero) = result matches
 * one of the expected values
 */
static int str_match2(char const* result, char const* expect, char const* alt)
{
	int same;

	if (!result)
	{
		if (!expect || !alt)
			return 1;

		printf("str_match2 fail: result=<NULL>, expect='%s', alt='%s'\n", expect, alt);
		return 0;
	}

	same =
		(expect && !strcmp(result, expect)) ||
		(alt && !strcmp(result, alt));

#ifdef DEBUG_TESTS
	if (!same)
	{
		printf("str_match2 fail: result='%s', ", result);

		if (!expect)
			printf("expect=<null>, ");
		else
			printf("expect='%s', ", expect);

		if (!alt)
			printf("alt=<null>\n");
		else
			printf("alt='%s'\n", alt);
	}
#endif

	return same;
}

EXO_TEST(check_str_match, {
	return str_match("Hello", "Hello");
});
