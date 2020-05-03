#include <uhub.h>

static struct cbuffer* buf1;
static struct cbuffer* buf2;
static struct cbuffer* buf3;
static struct tm now;

EXO_TEST(cbuf_create_const_1, {
	buf1 = cbuf_create_const("Hello World!");
	return buf1 != NULL;
});

EXO_TEST(cbuf_create_1, {
	buf2 = cbuf_create(20);
	return buf2 != NULL;
});

EXO_TEST(cbuf_create_2, {
	buf3 = cbuf_create(5);
	return buf3 != NULL;
});

EXO_TEST(cbuf_size_1, { return cbuf_size(buf1) == 12; /* strlen("Hello World!") */ });
EXO_TEST(cbuf_size_2, { return cbuf_size(buf2) == 0; });

EXO_TEST(cbuf_get_1, { return str_match(cbuf_get(buf1), "Hello World!"); });
EXO_TEST(cbuf_get_2, { return str_match(cbuf_get(buf2), ""); });

EXO_TEST(cbuf_append_1, {
	cbuf_append(buf2, "Hello World!");
	return cbuf_size(buf2) == 12 && str_match(cbuf_get(buf2), "Hello World!");
});

EXO_TEST(cbuf_append_2, {
	cbuf_append(buf2, "\n");
	return cbuf_size(buf2) == 13 && str_match(cbuf_get(buf2), "Hello World!\n");
});

EXO_TEST(cbuf_append_bytes_1, {
	char const* p = "Hello World!";
	for (; *p; p++)
		cbuf_append_bytes(buf3, p, 1);

	return
		cbuf_size(buf3) == 12 &&
		str_match(cbuf_get(buf3), "Hello World!");
});

EXO_TEST(cbuf_append_bytes_2, {
	cbuf_append_bytes(buf3, "\n\n\n", 2); /* Cut off 1 newline */
	return cbuf_size(buf3) == 14 && str_match(cbuf_get(buf3), "Hello World!\n\n");
});

EXO_TEST(cbuf_chomp_1, {
	return
		cbuf_chomp(buf2, NULL) &&
		cbuf_size(buf2) == 12 &&
		str_match(cbuf_get(buf2), "Hello World!");
});

EXO_TEST(cbuf_chomp_2, {
	return
		cbuf_chomp(buf3, "\n") &&
		cbuf_size(buf3) == 13 &&
		str_match(cbuf_get(buf3), "Hello World!\n");
});

EXO_TEST(cbuf_chomp_3, {
	return
		cbuf_chomp(buf3, "\n") &&
		cbuf_size(buf3) == 12 &&
		str_match(cbuf_get(buf3), "Hello World!");
});

EXO_TEST(cbuf_chomp_4, {
	return
		!cbuf_chomp(buf3, "\n") &&
		cbuf_size(buf3) == 12 &&
		str_match(cbuf_get(buf3), "Hello World!");
});

EXO_TEST(cbuf_chomp_5, {
	return
		cbuf_chomp(buf3, "!") &&
		cbuf_size(buf3) == 11 &&
		str_match(cbuf_get(buf3), "Hello World");
});

EXO_TEST(cbuf_chomp_6, {
	while (cbuf_chomp(buf3, "!World\n"));
	return
		cbuf_size(buf3) == 6 &&
		str_match(cbuf_get(buf3), "Hello ");
});

EXO_TEST(cbuf_chomp_7, {
	return
		!cbuf_chomp(buf3, "") &&
		cbuf_size(buf3) == 6 &&
		str_match(cbuf_get(buf3), "Hello ");
});

EXO_TEST(cbuf_chomp_8, {
	return
		cbuf_chomp(buf3, NULL) &&
		cbuf_size(buf3) == 5 &&
		str_match(cbuf_get(buf3), "Hello");
});

EXO_TEST(cbuf_clear_1, {
	cbuf_clear(buf3);
	return cbuf_size(buf3) == 0 && str_match(cbuf_get(buf3), "");
});

EXO_TEST(cbuf_append_format_1, {
	cbuf_append_format(buf3, "%d", 123456789);
	return cbuf_size(buf3) == 9 && str_match(cbuf_get(buf3), "123456789");
});

EXO_TEST(cbuf_append_format_2, {
	cbuf_append_format(buf3, " %s", "test");
	return cbuf_size(buf3) == 14 && str_match(cbuf_get(buf3), "123456789 test");
});

EXO_TEST(cbuf_append_format_3, {
	cbuf_append_format(buf3, " %#x", 0xfeedbeef);
	return cbuf_size(buf3) == 25 && str_match(cbuf_get(buf3), "123456789 test 0xfeedbeef");
});

EXO_TEST(cbuf_clear_2, {
	cbuf_clear(buf2);
	return cbuf_size(buf2) == 0 && str_match(cbuf_get(buf2), "");
});

EXO_TEST(cbuf_clear_3, {
	cbuf_clear(buf3);
	return cbuf_size(buf3) == 0 && str_match(cbuf_get(buf3), "");
});

EXO_TEST(populate_time, {
	time_t timestamp = time(NULL);
	return localtime_r(&timestamp, &now) != NULL;
});

EXO_TEST(cbuf_append_format_4, {
	/* printf() equivalent to the strftime() format string below */
	cbuf_append_format(buf2, "[%02d:%02d:%02d]\n", now.tm_hour, now.tm_min, now.tm_sec);
	return cbuf_size(buf2) == 11;
});

EXO_TEST(cbuf_append_strftime_1, {
	/* strftime() equivalent to the printf() format string below */
	cbuf_append_strftime(buf3, "[%H:%M:%S]%n", &now);
	return cbuf_size(buf3) == 11;
});

EXO_TEST(cbuf_append_strftime_2, {
	/* These two buffers should have the same contents */
	return str_match(cbuf_get(buf2), cbuf_get(buf3));
});

EXO_TEST(cbuf_destroy_const_1, { cbuf_destroy(buf1); return 1; });
EXO_TEST(cbuf_destroy_1,       { cbuf_destroy(buf2); return 1; });
EXO_TEST(cbuf_destroy_2,       { cbuf_destroy(buf3); return 1; });
