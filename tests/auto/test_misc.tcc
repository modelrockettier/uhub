#include <uhub.h>

EXO_TEST(is_num_0,  { return is_num('0'); });
EXO_TEST(is_num_1,  { return is_num('1'); });
EXO_TEST(is_num_2,  { return is_num('2'); });
EXO_TEST(is_num_3,  { return is_num('3'); });
EXO_TEST(is_num_4,  { return is_num('4'); });
EXO_TEST(is_num_5,  { return is_num('5'); });
EXO_TEST(is_num_6,  { return is_num('6'); });
EXO_TEST(is_num_7,  { return is_num('7'); });
EXO_TEST(is_num_8,  { return is_num('8'); });
EXO_TEST(is_num_9,  { return is_num('9'); });
EXO_TEST(is_num_10, { return !is_num('/'); });
EXO_TEST(is_num_11, { return !is_num(':'); });
EXO_TEST(is_num_12, { return !is_num(' '); });
EXO_TEST(is_num_13, { return !is_num('\t'); });
EXO_TEST(is_num_14, { return !is_num('\r'); });
EXO_TEST(is_num_15, { return !is_num('\n'); });
EXO_TEST(is_num_16, { return !is_num('\v'); });
EXO_TEST(is_num_17, { return !is_num('\0'); });
EXO_TEST(is_num_18, { return !is_num('\x07'); });
EXO_TEST(is_num_19, { return !is_num('\x7f'); });
EXO_TEST(is_num_20, { return !is_num('U'); });
EXO_TEST(is_num_21, { return !is_num('l'); });
EXO_TEST(is_num_22, { return !is_num('-'); });

EXO_TEST(is_space_1,  { return is_space(' '); });
EXO_TEST(is_space_2,  { return !is_space('\t'); });
EXO_TEST(is_space_3,  { return !is_space('\r'); });
EXO_TEST(is_space_4,  { return !is_space('\n'); });
EXO_TEST(is_space_5,  { return !is_space('\v'); });
EXO_TEST(is_space_6,  { return !is_space('\0'); });
EXO_TEST(is_space_7,  { return !is_space('\x07'); });
EXO_TEST(is_space_8,  { return !is_space('\x7f'); });
EXO_TEST(is_space_9,  { return !is_space('5'); });
EXO_TEST(is_space_10, { return !is_space('U'); });
EXO_TEST(is_space_11, { return !is_space('l'); });
EXO_TEST(is_space_12, { return !is_space('!'); });

EXO_TEST(is_white_space_1,  { return is_white_space(' '); });
EXO_TEST(is_white_space_2,  { return is_white_space('\t'); });
EXO_TEST(is_white_space_3,  { return is_white_space('\r'); });
EXO_TEST(is_white_space_4,  { return !is_white_space('\n'); });
EXO_TEST(is_white_space_5,  { return !is_white_space('\v'); });
EXO_TEST(is_white_space_6,  { return !is_white_space('\0'); });
EXO_TEST(is_white_space_7,  { return !is_white_space('\x07'); });
EXO_TEST(is_white_space_8,  { return !is_white_space('\x7f'); });
EXO_TEST(is_white_space_9,  { return !is_white_space('5'); });
EXO_TEST(is_white_space_10, { return !is_white_space('U'); });
EXO_TEST(is_white_space_11, { return !is_white_space('l'); });
EXO_TEST(is_white_space_12, { return !is_white_space('!'); });

EXO_TEST(is_printable_1,  { return is_printable(' '); });
EXO_TEST(is_printable_2,  { return is_printable('\t'); });
EXO_TEST(is_printable_3,  { return is_printable('\r'); });
EXO_TEST(is_printable_4,  { return is_printable('\n'); });
EXO_TEST(is_printable_5,  { return !is_printable('\v'); });
EXO_TEST(is_printable_6,  { return !is_printable('\0'); });
EXO_TEST(is_printable_7,  { return !is_printable('\x07'); });
EXO_TEST(is_printable_8,  { return !is_printable('\x7f'); });
EXO_TEST(is_printable_9,  { return is_printable('5'); });
EXO_TEST(is_printable_10, { return is_printable('U'); });
EXO_TEST(is_printable_11, { return is_printable('l'); });
EXO_TEST(is_printable_12, { return is_printable('!'); });

EXO_TEST(itoa_1, { return str_match(uhub_itoa(0), "0"); });
EXO_TEST(itoa_2, { return str_match(uhub_itoa(1), "1"); });
EXO_TEST(itoa_3, { return str_match(uhub_itoa(-1), "-1"); });
EXO_TEST(itoa_4, { return str_match(uhub_itoa(255), "255"); });
EXO_TEST(itoa_5, { return str_match(uhub_itoa(-3), "-3"); });
EXO_TEST(itoa_6, { return str_match(uhub_itoa(-2147483648), "-2147483648"); });
EXO_TEST(itoa_7, { return str_match(uhub_itoa(2147483647), "2147483647"); });
EXO_TEST(itoa_8, { return str_match(uhub_itoa(-65536), "-65536"); });

EXO_TEST(ulltoa_1,  { return str_match(uhub_ulltoa(0ULL), "0"); });
EXO_TEST(ulltoa_2,  { return str_match(uhub_ulltoa(1ULL), "1"); });
EXO_TEST(ulltoa_3,  { return str_match(uhub_ulltoa(255ULL), "255"); });
EXO_TEST(ulltoa_4,  { return str_match(uhub_ulltoa(2147483647ULL), "2147483647"); });
EXO_TEST(ulltoa_5,  { return str_match(uhub_ulltoa(2147483648ULL), "2147483648"); });
EXO_TEST(ulltoa_6,  { return str_match(uhub_ulltoa(4294967295ULL), "4294967295"); });
EXO_TEST(ulltoa_7,  { return str_match(uhub_ulltoa(4294967296ULL), "4294967296"); });
EXO_TEST(ulltoa_8,  { return str_match(uhub_ulltoa(9223372036854775807ULL), "9223372036854775807"); });
EXO_TEST(ulltoa_9,  { return str_match(uhub_ulltoa(9223372036854775808ULL), "9223372036854775808"); });
EXO_TEST(ulltoa_10, { return str_match(uhub_ulltoa(18446744073709551615ULL), "18446744073709551615"); });

EXO_TEST(string_to_bool_1_1, { int b = 0; return string_to_boolean("1", &b) && b; });
EXO_TEST(string_to_bool_1_2, { int b = 1; return string_to_boolean("0", &b) && !b; });
EXO_TEST(string_to_bool_2_1, { int b = 0; return string_to_boolean("on", &b) && b; });
EXO_TEST(string_to_bool_2_2, { int b = 1; return string_to_boolean("off", &b) && !b; });
EXO_TEST(string_to_bool_3_1, { int b = 0; return string_to_boolean("yes", &b) && b; });
EXO_TEST(string_to_bool_3_2, { int b = 1; return string_to_boolean("no", &b) && !b; });
EXO_TEST(string_to_bool_4_1, { int b = 0; return string_to_boolean("true", &b) && b; });
EXO_TEST(string_to_bool_4_2, { int b = 1; return string_to_boolean("false", &b) && !b; });
EXO_TEST(string_to_bool_5_1, { int b = 0; return string_to_boolean("TRUE", &b) && b; });
EXO_TEST(string_to_bool_5_2, { int b = 1; return string_to_boolean("FALSE", &b) && !b; });
EXO_TEST(string_to_bool_6_1, { int b = 0; return string_to_boolean("On", &b) && b; });
EXO_TEST(string_to_bool_6_2, { int b = 1; return string_to_boolean("oFf", &b) && !b; });
EXO_TEST(string_to_bool_7_1, { int b = 2; return !string_to_boolean(NULL, &b) && b == 2; });
EXO_TEST(string_to_bool_7_2, { int b = 2; return !string_to_boolean("", &b) && b == 2; });
EXO_TEST(string_to_bool_7_3, { int b = 2; return !string_to_boolean("1", NULL) && b == 2; });
EXO_TEST(string_to_bool_8_1, { int b = 2; return !string_to_boolean("3", &b) && b == 2; });
EXO_TEST(string_to_bool_8_2, { int b = 2; return !string_to_boolean("so", &b) && b == 2; });
EXO_TEST(string_to_bool_8_3, { int b = 2; return !string_to_boolean("yea", &b) && b == 2; });
EXO_TEST(string_to_bool_8_4, { int b = 2; return !string_to_boolean("sure", &b) && b == 2; });
EXO_TEST(string_to_bool_8_5, { int b = 2; return !string_to_boolean("maybe", &b) && b == 2; });
EXO_TEST(string_to_bool_8_6, { int b = 2; return !string_to_boolean("unsure", &b) && b == 2; });
EXO_TEST(string_to_bool_9_1, { int b = 2; return !string_to_boolean("00", &b) && b == 2; });
EXO_TEST(string_to_bool_9_2, { int b = 2; return !string_to_boolean("01", &b) && b == 2; });
EXO_TEST(string_to_bool_9_3, { int b = 2; return !string_to_boolean("onn", &b) && b == 2; });
EXO_TEST(string_to_bool_9_4, { int b = 2; return !string_to_boolean("offf", &b) && b == 2; });
EXO_TEST(string_to_bool_9_5, { int b = 2; return !string_to_boolean("falsee", &b) && b == 2; });
EXO_TEST(string_to_bool_9_6, { int b = 2; return !string_to_boolean("nope", &b) && b == 2; });

static int check_strip(char const* in, const char* expect)
{
	char buf[512];
	char const* result;

	if (!in)
	{
		result = strip_white_space(NULL);
	}
	else
	{
		strlcpy(buf, in, sizeof(buf));
		result = strip_white_space(buf);
	}

	if (!result)
		return !expect;

	return str_match(result, expect);
}

#define check_unstripped(s) check_strip(s, s)

EXO_TEST(strip_white_space_1, { return check_strip("hello", "hello"); });
EXO_TEST(strip_white_space_2_1, { return check_strip(" hello", "hello"); });
EXO_TEST(strip_white_space_2_2, { return check_strip("  hello", "hello"); });
EXO_TEST(strip_white_space_2_3, { return check_strip("   hello", "hello"); });
EXO_TEST(strip_white_space_3_1, { return check_strip("hello ", "hello"); });
EXO_TEST(strip_white_space_3_2, { return check_strip("hello  ", "hello"); });
EXO_TEST(strip_white_space_3_3, { return check_strip("hello   ", "hello"); });
EXO_TEST(strip_white_space_4_1, { return check_strip(" hello ", "hello"); });
EXO_TEST(strip_white_space_4_2, { return check_strip("  hello ", "hello"); });
EXO_TEST(strip_white_space_4_3, { return check_strip(" hello     ", "hello"); });
EXO_TEST(strip_white_space_5_1, { return check_strip("\thello", "hello"); });
EXO_TEST(strip_white_space_5_2, { return check_strip("\t\thello\t", "hello"); });
EXO_TEST(strip_white_space_5_3, { return check_strip("\rhello", "hello"); });
EXO_TEST(strip_white_space_5_4, { return check_strip("\rhello\r\r", "hello"); });
EXO_TEST(strip_white_space_6_1, { return check_strip("\t hello \t", "hello"); });
EXO_TEST(strip_white_space_6_2, { return check_strip("\thello\r", "hello"); });
EXO_TEST(strip_white_space_6_3, { return check_strip(" hello\r", "hello"); });
EXO_TEST(strip_white_space_6_4, { return check_strip(" \r\t\rhello", "hello"); });
EXO_TEST(strip_white_space_6_5, { return check_strip("\t\thello  \r ", "hello"); });
EXO_TEST(strip_white_space_7_1, { return check_unstripped("\nhello"); });
EXO_TEST(strip_white_space_7_2, { return check_unstripped("hello\n"); });
EXO_TEST(strip_white_space_7_3, { return check_unstripped("\vhello\v\v"); });
EXO_TEST(strip_white_space_7_4, { return check_unstripped("hello\x07"); });
EXO_TEST(strip_white_space_7_5, { return check_unstripped("hello\x7f"); });
EXO_TEST(strip_white_space_7_6, { return check_unstripped("\x7fhello"); });
EXO_TEST(strip_white_space_7_7, { return check_unstripped("5"); });
EXO_TEST(strip_white_space_7_8, { return check_unstripped("!"); });
EXO_TEST(strip_white_space_8_1, { return check_unstripped("hello world"); });
EXO_TEST(strip_white_space_8_2, { return check_strip("\t\thello world \r ", "hello world"); });
EXO_TEST(strip_white_space_8_3, { return check_strip("\t\thello \t world \r ", "hello \t world"); });
EXO_TEST(strip_white_space_9_1, { return check_unstripped(""); });
EXO_TEST(strip_white_space_9_2, { return check_strip(NULL, ""); });

EXO_TEST(base32_valid_1,  { return is_valid_base32_char('A'); });
EXO_TEST(base32_valid_2,  { return is_valid_base32_char('B'); });
EXO_TEST(base32_valid_3,  { return is_valid_base32_char('C'); });
EXO_TEST(base32_valid_4,  { return is_valid_base32_char('D'); });
EXO_TEST(base32_valid_5,  { return is_valid_base32_char('E'); });
EXO_TEST(base32_valid_6,  { return is_valid_base32_char('F'); });
EXO_TEST(base32_valid_7,  { return is_valid_base32_char('G'); });
EXO_TEST(base32_valid_8,  { return is_valid_base32_char('H'); });
EXO_TEST(base32_valid_9,  { return is_valid_base32_char('I'); });
EXO_TEST(base32_valid_10, { return is_valid_base32_char('J'); });
EXO_TEST(base32_valid_11, { return is_valid_base32_char('K'); });
EXO_TEST(base32_valid_12, { return is_valid_base32_char('L'); });
EXO_TEST(base32_valid_13, { return is_valid_base32_char('M'); });
EXO_TEST(base32_valid_14, { return is_valid_base32_char('N'); });
EXO_TEST(base32_valid_15, { return is_valid_base32_char('O'); });
EXO_TEST(base32_valid_16, { return is_valid_base32_char('P'); });
EXO_TEST(base32_valid_17, { return is_valid_base32_char('Q'); });
EXO_TEST(base32_valid_18, { return is_valid_base32_char('R'); });
EXO_TEST(base32_valid_19, { return is_valid_base32_char('S'); });
EXO_TEST(base32_valid_20, { return is_valid_base32_char('T'); });
EXO_TEST(base32_valid_21, { return is_valid_base32_char('U'); });
EXO_TEST(base32_valid_22, { return is_valid_base32_char('V'); });
EXO_TEST(base32_valid_23, { return is_valid_base32_char('W'); });
EXO_TEST(base32_valid_24, { return is_valid_base32_char('X'); });
EXO_TEST(base32_valid_25, { return is_valid_base32_char('Y'); });
EXO_TEST(base32_valid_26, { return is_valid_base32_char('Z'); });
EXO_TEST(base32_valid_27, { return is_valid_base32_char('2'); });
EXO_TEST(base32_valid_28, { return is_valid_base32_char('3'); });
EXO_TEST(base32_valid_29, { return is_valid_base32_char('4'); });
EXO_TEST(base32_valid_30, { return is_valid_base32_char('5'); });
EXO_TEST(base32_valid_31, { return is_valid_base32_char('6'); });
EXO_TEST(base32_valid_32, { return is_valid_base32_char('7'); });

EXO_TEST(base32_invalid_1,  { return !is_valid_base32_char('a'); });
EXO_TEST(base32_invalid_2,  { return !is_valid_base32_char('b'); });
EXO_TEST(base32_invalid_3,  { return !is_valid_base32_char('c'); });
EXO_TEST(base32_invalid_4,  { return !is_valid_base32_char('d'); });
EXO_TEST(base32_invalid_5,  { return !is_valid_base32_char('e'); });
EXO_TEST(base32_invalid_6,  { return !is_valid_base32_char('f'); });
EXO_TEST(base32_invalid_7,  { return !is_valid_base32_char('g'); });
EXO_TEST(base32_invalid_8,  { return !is_valid_base32_char('h'); });
EXO_TEST(base32_invalid_9,  { return !is_valid_base32_char('i'); });
EXO_TEST(base32_invalid_10, { return !is_valid_base32_char('j'); });
EXO_TEST(base32_invalid_11, { return !is_valid_base32_char('k'); });
EXO_TEST(base32_invalid_12, { return !is_valid_base32_char('l'); });
EXO_TEST(base32_invalid_13, { return !is_valid_base32_char('m'); });
EXO_TEST(base32_invalid_14, { return !is_valid_base32_char('n'); });
EXO_TEST(base32_invalid_15, { return !is_valid_base32_char('o'); });
EXO_TEST(base32_invalid_16, { return !is_valid_base32_char('p'); });
EXO_TEST(base32_invalid_17, { return !is_valid_base32_char('q'); });
EXO_TEST(base32_invalid_18, { return !is_valid_base32_char('r'); });
EXO_TEST(base32_invalid_19, { return !is_valid_base32_char('s'); });
EXO_TEST(base32_invalid_20, { return !is_valid_base32_char('t'); });
EXO_TEST(base32_invalid_21, { return !is_valid_base32_char('u'); });
EXO_TEST(base32_invalid_22, { return !is_valid_base32_char('v'); });
EXO_TEST(base32_invalid_23, { return !is_valid_base32_char('w'); });
EXO_TEST(base32_invalid_24, { return !is_valid_base32_char('x'); });
EXO_TEST(base32_invalid_25, { return !is_valid_base32_char('y'); });
EXO_TEST(base32_invalid_26, { return !is_valid_base32_char('z'); });
EXO_TEST(base32_invalid_27, { return !is_valid_base32_char('0'); });
EXO_TEST(base32_invalid_28, { return !is_valid_base32_char('1'); });
EXO_TEST(base32_invalid_29, { return !is_valid_base32_char('8'); });
EXO_TEST(base32_invalid_30, { return !is_valid_base32_char('9'); });
EXO_TEST(base32_invalid_31, { return !is_valid_base32_char('@'); });

EXO_TEST(utf8_valid_1, { return is_valid_utf8("abcdefghijklmnopqrstuvwxyz"); });
EXO_TEST(utf8_valid_2, { return is_valid_utf8("ABCDEFGHIJKLMNOPQRSTUVWXYZ"); });
EXO_TEST(utf8_valid_3, { return is_valid_utf8("0123456789"); });

static const char test_utf_seq_1[] = { 0x65, 0x00 }; // valid
static const char test_utf_seq_2[] = { 0xD8, 0x00 }; // invalid
static const char test_utf_seq_3[] = { 0x24, 0x00 }; // valid
static const char test_utf_seq_4[] = { 0xC2, 0x24, 0x00}; // invalid
static const char test_utf_seq_5[] = { 0xC2, 0xA2, 0x00}; // valid
static const char test_utf_seq_6[] = { 0xE2, 0x82, 0xAC, 0x00}; // valid
static const char test_utf_seq_7[] = { 0xC2, 0x32, 0x00}; // invalid
static const char test_utf_seq_8[] = { 0xE2, 0x82, 0x32, 0x00}; // invalid
static const char test_utf_seq_9[] = { 0xE2, 0x32, 0x82, 0x00}; // invalid
static const char test_utf_seq_10[] = { 0xF0, 0x9F, 0x98, 0x81, 0x00}; // valid

EXO_TEST(utf8_valid_4, { return is_valid_utf8(test_utf_seq_1); });
EXO_TEST(utf8_valid_5, { return !is_valid_utf8(test_utf_seq_2); });
EXO_TEST(utf8_valid_6, { return is_valid_utf8(test_utf_seq_3); });
EXO_TEST(utf8_valid_7, { return !is_valid_utf8(test_utf_seq_4); });
EXO_TEST(utf8_valid_8, { return is_valid_utf8(test_utf_seq_5); });
EXO_TEST(utf8_valid_9, { return is_valid_utf8(test_utf_seq_6); });
EXO_TEST(utf8_valid_10, { return !is_valid_utf8(test_utf_seq_7); });
EXO_TEST(utf8_valid_11, { return !is_valid_utf8(test_utf_seq_8); });
EXO_TEST(utf8_valid_12, { return !is_valid_utf8(test_utf_seq_9); });
EXO_TEST(utf8_valid_13, { return is_valid_utf8(test_utf_seq_10); });

// Limits of utf-8
static const char test_utf_seq_11[] = { 0x7F, 0x00 }; // valid last 7-bit character
static const char test_utf_seq_12[] = { 0x80, 0x00 }; // invalid truncated string
static const char test_utf_seq_13[] = { 0xBF, 0x00 }; // invalid truncated string
static const char test_utf_seq_14[] = { 0xC0, 0x80, 0x00 }; // invalid out of 2 bytes range
static const char test_utf_seq_15[] = { 0xC1, 0x7F, 0x00 }; // invalid out of 2 bytes range
static const char test_utf_seq_16[] = { 0xC2, 0x00 }; // invalid truncated string
static const char test_utf_seq_17[] = { 0xC2, 0x80, 0x00 }; // valid
static const char test_utf_seq_18[] = { 0xDF, 0xBF, 0x00 }; // valid
static const char test_utf_seq_19[] = { 0xE0, 0x80, 0x80, 0x00 }; // invalid out of 3 bytes range
static const char test_utf_seq_20[] = { 0xE0, 0x9F, 0xBF, 0x00 }; // invalid out of 3 bytes range
static const char test_utf_seq_21[] = { 0xE0, 0x00 }; // invalid truncated string
static const char test_utf_seq_22[] = { 0xE0, 0xA0, 0x00 }; // invalid truncated string
static const char test_utf_seq_23[] = { 0xE0, 0xA0, 0x80, 0x00 }; // valid
static const char test_utf_seq_24[] = { 0xEC, 0x9F, 0xBF, 0x00 }; // valid
static const char test_utf_seq_25[] = { 0xED, 0xA0, 0x80, 0x00 }; // invalid surrogate
static const char test_utf_seq_26[] = { 0xED, 0xBF, 0xBF, 0x00 }; // invalid surrogate
static const char test_utf_seq_27[] = { 0xEF, 0x80, 0x80, 0x00 }; // valid
static const char test_utf_seq_28[] = { 0xEF, 0xBF, 0xBF, 0x00 }; // valid
static const char test_utf_seq_29[] = { 0xF0, 0x80, 0x80, 0x80, 0x00 }; // invalid out of 4 bytes range
static const char test_utf_seq_30[] = { 0xF0, 0x8F, 0xBF, 0xBF, 0x00 }; // invalid out of 4 bytes range
static const char test_utf_seq_31[] = { 0xF0, 0x00 }; // invalid truncated string
static const char test_utf_seq_32[] = { 0xF0, 0x90, 0x00 }; // invalid truncated string
static const char test_utf_seq_33[] = { 0xF0, 0x90, 0x80, 0x00 }; // invalid truncated string
static const char test_utf_seq_34[] = { 0xF0, 0x90, 0x80, 0x80, 0x00 }; // valid
static const char test_utf_seq_35[] = { 0xF4, 0x8F, 0xBF, 0xBF, 0x00 }; // valid
static const char test_utf_seq_36[] = { 0xF4, 0x90, 0x80, 0x80, 0x00 }; // invalid out of 4 bytes range
static const char test_utf_seq_37[] = { 0xFF, 0xBF, 0xBF, 0xBF, 0x00 }; // invalid out of 4 bytes range

EXO_TEST(utf8_valid_14, { return is_valid_utf8(test_utf_seq_11); });
EXO_TEST(utf8_valid_15, { return !is_valid_utf8(test_utf_seq_12); });
EXO_TEST(utf8_valid_16, { return !is_valid_utf8(test_utf_seq_13); });
EXO_TEST(utf8_valid_17, { return !is_valid_utf8(test_utf_seq_14); });
EXO_TEST(utf8_valid_18, { return !is_valid_utf8(test_utf_seq_15); });
EXO_TEST(utf8_valid_19, { return !is_valid_utf8(test_utf_seq_16); });
EXO_TEST(utf8_valid_20, { return is_valid_utf8(test_utf_seq_17); });
EXO_TEST(utf8_valid_21, { return is_valid_utf8(test_utf_seq_18); });
EXO_TEST(utf8_valid_22, { return !is_valid_utf8(test_utf_seq_19); });
EXO_TEST(utf8_valid_23, { return !is_valid_utf8(test_utf_seq_20); });
EXO_TEST(utf8_valid_24, { return !is_valid_utf8(test_utf_seq_21); });
EXO_TEST(utf8_valid_25, { return !is_valid_utf8(test_utf_seq_22); });
EXO_TEST(utf8_valid_26, { return is_valid_utf8(test_utf_seq_23); });
EXO_TEST(utf8_valid_27, { return is_valid_utf8(test_utf_seq_24); });
EXO_TEST(utf8_valid_28, { return !is_valid_utf8(test_utf_seq_25); });
EXO_TEST(utf8_valid_29, { return !is_valid_utf8(test_utf_seq_26); });
EXO_TEST(utf8_valid_30, { return is_valid_utf8(test_utf_seq_27); });
EXO_TEST(utf8_valid_31, { return is_valid_utf8(test_utf_seq_28); });
EXO_TEST(utf8_valid_32, { return !is_valid_utf8(test_utf_seq_29); });
EXO_TEST(utf8_valid_33, { return !is_valid_utf8(test_utf_seq_30); });
EXO_TEST(utf8_valid_34, { return !is_valid_utf8(test_utf_seq_31); });
EXO_TEST(utf8_valid_35, { return !is_valid_utf8(test_utf_seq_32); });
EXO_TEST(utf8_valid_36, { return !is_valid_utf8(test_utf_seq_33); });
EXO_TEST(utf8_valid_37, { return is_valid_utf8(test_utf_seq_34); });
EXO_TEST(utf8_valid_38, { return is_valid_utf8(test_utf_seq_35); });
EXO_TEST(utf8_valid_39, { return !is_valid_utf8(test_utf_seq_36); });
EXO_TEST(utf8_valid_40, { return !is_valid_utf8(test_utf_seq_37); });

EXO_TEST(uhub_atoi_1,  { return uhub_atoi("0") == 0; });
EXO_TEST(uhub_atoi_2,  { return uhub_atoi("1") == 1; });
EXO_TEST(uhub_atoi_3,  { return uhub_atoi("-1") == -1; });
EXO_TEST(uhub_atoi_4,  { return uhub_atoi("123456789") == 123456789; });
EXO_TEST(uhub_atoi_5,  { return uhub_atoi("-123456789") == -123456789; });
EXO_TEST(uhub_atoi_6,  { return uhub_atoi("123456789012345678901234567890") > 0; });
EXO_TEST(uhub_atoi_7,  { return uhub_atoi("-123456789012345678901234567890") < 0; });
EXO_TEST(uhub_atoi_8,  { return uhub_atoi("") == 0; });
EXO_TEST(uhub_atoi_9,  { return uhub_atoi(" ") == 0; });
EXO_TEST(uhub_atoi_10, { return uhub_atoi("q") == 0; });
EXO_TEST(uhub_atoi_11, { return uhub_atoi("invalid") == 0; });
EXO_TEST(uhub_atoi_12, { return uhub_atoi("!") == 0; });
EXO_TEST(uhub_atoi_13, { return uhub_atoi("-") == 0; });

EXO_TEST(is_number_1,  { int i = 2; return is_number("0", &i) && i == 0; });
EXO_TEST(is_number_2,  { int i = 2; return is_number("1", &i) && i == 1; });
EXO_TEST(is_number_3,  { int i = 2; return is_number("-1", &i) && i == -1; });
EXO_TEST(is_number_4,  { int i = 2; return is_number("123456789", &i) && i == 123456789; });
EXO_TEST(is_number_5,  { int i = 2; return is_number("-123456789", &i) && i == -123456789; });
EXO_TEST(is_number_6,  { int i = 2; return is_number("123456789012345678901234567890", &i) && i > 0; });
EXO_TEST(is_number_7,  { int i = 2; return is_number("-123456789012345678901234567890", &i) && i < 0; });
EXO_TEST(is_number_8,  { int i = 2; return !is_number("q1234", &i) && i == 2; });
EXO_TEST(is_number_9,  { int i = 2; return !is_number("1234q", &i) && i == 2; });
EXO_TEST(is_number_10, { int i = 2; return !is_number("", &i) && i == 2; });
EXO_TEST(is_number_11, { int i = 2; return !is_number(" ", &i) && i == 2; });
EXO_TEST(is_number_12, { int i = 2; return !is_number("q", &i) && i == 2; });
EXO_TEST(is_number_13, { int i = 2; return !is_number("invalid", &i) && i == 2; });
EXO_TEST(is_number_14, { int i = 2; return !is_number("!", &i) && i == 2; });
EXO_TEST(is_number_15, { int i = 0; return !is_number("-", &i) && i == 0; });

static int test_format(size_t bytes, const char* expect)
{
	char buf[32];
	(void)format_size(bytes, buf, sizeof(buf));
	return str_match(buf, expect);
}

EXO_TEST(format_size_1,  { return test_format(    0,    "0 B"); });
EXO_TEST(format_size_2,  { return test_format(    1,    "1 B"); });
EXO_TEST(format_size_3,  { return test_format(   21,   "21 B"); });
EXO_TEST(format_size_4,  { return test_format(  325,  "325 B"); });
EXO_TEST(format_size_5,  { return test_format(  999,  "999 B"); });
EXO_TEST(format_size_6,  { return test_format( 1000, "1000 B"); });
EXO_TEST(format_size_7,  { return test_format( 1023, "1023 B"); });

EXO_TEST(format_size_8,  { return test_format( 1024,               "1 KiB"); });
EXO_TEST(format_size_9,  { return test_format( 1024 + 1,           "1 KiB"); });
EXO_TEST(format_size_10, { return test_format( 1130,             "1.1 KiB"); });
EXO_TEST(format_size_11, { return test_format( 2048 - 1,         "1.9 KiB"); });
EXO_TEST(format_size_12, { return test_format( 2048,               "2 KiB"); });
EXO_TEST(format_size_13, { return test_format(10240 - 1,         "9.9 KiB"); });
EXO_TEST(format_size_14, { return test_format(10240,              "10 KiB"); });
EXO_TEST(format_size_15, { return test_format(10240 + 1,          "10 KiB"); });
EXO_TEST(format_size_16, { return test_format(50  << 10,          "50 KiB"); });
EXO_TEST(format_size_17, { return test_format(99  << 10,          "99 KiB"); });
EXO_TEST(format_size_18, { return test_format(100 << 10,         "100 KiB"); });
EXO_TEST(format_size_19, { return test_format(200 << 10,         "200 KiB"); });
EXO_TEST(format_size_20, { return test_format((1000 << 10) - 1,  "999 KiB"); });
EXO_TEST(format_size_21, { return test_format( 1000 << 10,      "1000 KiB"); });
EXO_TEST(format_size_22, { return test_format((1000 << 10) + 1, "1000 KiB"); });

EXO_TEST(format_size_23, { return test_format(1    << 20,    "1 MiB"); });
EXO_TEST(format_size_24, { return test_format(1130 << 10,  "1.1 MiB"); });
EXO_TEST(format_size_25, { return test_format(1536 << 10,  "1.5 MiB"); });
EXO_TEST(format_size_26, { return test_format(8    << 20,    "8 MiB"); });
EXO_TEST(format_size_27, { return test_format(10   << 20,   "10 MiB"); });
EXO_TEST(format_size_28, { return test_format(49   << 20,   "49 MiB"); });
EXO_TEST(format_size_29, { return test_format(100  << 20,  "100 MiB"); });
EXO_TEST(format_size_30, { return test_format(512  << 20,  "512 MiB"); });
EXO_TEST(format_size_31, { return test_format(1000 << 20, "1000 MiB"); });

EXO_TEST(format_size_32, { return test_format((size_t)1 << 30,      "1 GiB"); });
EXO_TEST(format_size_33, { return test_format((size_t)2 << 30,      "2 GiB"); });
EXO_TEST(format_size_34, { return test_format((size_t)3 << 30,      "3 GiB"); });
EXO_TEST(format_size_35, { return test_format((size_t)3584 << 20, "3.5 GiB"); });

#define test_format64(bytes, expect) (sizeof(size_t) < 8 || test_format((size_t)bytes, expect))

EXO_TEST(format_size_36, { return test_format64(1 << 40,       "1 TiB"); });
EXO_TEST(format_size_37, { return test_format64(1 << 50,       "1 PiB"); });
EXO_TEST(format_size_38, { return test_format64(1 << 60,       "1 EiB"); });
EXO_TEST(format_size_39, { return test_format64(9 << 60,       "9 EiB"); });
EXO_TEST(format_size_40, { return test_format64(10140 << 50, "9.9 EiB"); });
EXO_TEST(format_size_41, { return test_format64(15 << 60,     "15 EiB"); });
