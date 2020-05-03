#include <uhub.h>

static struct hub_user* g_user = 0;
static const char* test_string1 = "IINF AAfoo BBbar CCwhat\n";
static const char* test_string2 = "BMSG AAAB Hello\\sWorld!\n";
static const char* test_string3 = "BINF AAAB IDAN7ZMSLIEBL53OPTM7WXGSTXUS3XOY6KQS5LBGX NIFriend DEstuff SL3 SS0 SF0 VEQuickDC/0.4.17 US6430 SUADC0,TCP4,UDP4 I4127.0.0.1 HO5 HN1 AW\n";
static const char* test_string4 = "BMSG AAAB\n";
static const char* test_string5 = "BMSG AAAB \n";

static struct adc_message* updater1 = NULL;
static struct adc_message* updater2 = NULL;
static const char* update_info1 = "BINF AAAB IDABCDEFGHIJKLMNOPQRSTUVWXYZ1234567ABCDEF NItester SL10 SS12817126127 SF4125 HN3 HR0 HO0 VE++\\s0.698 US104857600 DS81911808 SUTCP4,UDP4 I4127.0.0.1\n";
static const char* update_info2 = "BINF AAAB HN34 SF4126 SS12817526127\n";

static void create_test_user()
{
	if (g_user)
		return;

	g_user = (struct hub_user*) malloc(sizeof(struct hub_user));
	memset(g_user, 0, sizeof(struct hub_user));
	memcpy(g_user->id.nick, "exotic-tester", 13);
	g_user->id.sid = 1;
}

EXO_TEST(adc_message_first, {
	create_test_user();
	return g_user != 0;
});


static int create_msg(const char* string)
{
	struct adc_message* msg = adc_msg_create(string);
	int ok = msg != NULL;
	adc_msg_free(msg);
	return ok;
}

static int verify_msg(const char* string, size_t len)
{
	struct adc_message* msg = adc_msg_parse_verify(g_user, string, len);
	int ok = msg != NULL;
	adc_msg_free(msg);
	return ok;
}

EXO_TEST(adc_message_parse_1,  { return  create_msg("IMSG Hello\\sWorld!"); });
EXO_TEST(adc_message_parse_2,  { return  create_msg(test_string2); });
EXO_TEST(adc_message_parse_3,  { return  verify_msg("BMSG AAAB Hello\\sWorld!", 23); });
EXO_TEST(adc_message_parse_4,  { return !verify_msg("BMSG AAAC Hello\\sWorld!", 23); });
EXO_TEST(adc_message_parse_5,  { return  verify_msg("BMSG AAAB Hello\\sWorld!\n", 24); });
EXO_TEST(adc_message_parse_6,  { return  verify_msg("FMSG AAAB +TCP4 Hello\\sWorld!\n", 30); });
EXO_TEST(adc_message_parse_7,  { return  verify_msg("FMSG AAAB -TCP4 Hello\\sWorld!\n", 30); });
EXO_TEST(adc_message_parse_8,  { return  verify_msg("FMSG AAAB +TCP4+UDP4 Hello\\sWorld!\n", 35); });
EXO_TEST(adc_message_parse_9,  { return  verify_msg("FMSG AAAB +TCP4-UDP4 Hello\\sWorld!\n", 35); });
EXO_TEST(adc_message_parse_10, { return  verify_msg("FMSG AAAB -TCP4-UDP4 Hello\\sWorld!\n", 35); });
EXO_TEST(adc_message_parse_11, { return !verify_msg("FMSG AAAB Hello\\sWorld!\n", 24); });
EXO_TEST(adc_message_parse_12, { return !verify_msg("FMSG AAAB  Hello\\sWorld!\n", 25); });
EXO_TEST(adc_message_parse_13, { return !verify_msg("FMSG AAAB +jalla Hello\\sWorld!\n", 31); });
EXO_TEST(adc_message_parse_14, { return  verify_msg("FMSG AAAB +jall Hello\\sWorld!\n", 30); });
EXO_TEST(adc_message_parse_15, { return  verify_msg("FMSG AAAB +TCP4 Hello\\sWorld!\n", 30); });
EXO_TEST(adc_message_parse_16, { return  verify_msg("BMSG AAAB Hello\\sWorld!\n", 24); });
EXO_TEST(adc_message_parse_17, { return !verify_msg("BMSG aaab Hello\\sWorld!\n", 24); });
EXO_TEST(adc_message_parse_18, { return  verify_msg("DMSG AAAB AAAC Hello\\sthere!\n", 29); });
EXO_TEST(adc_message_parse_19, { return !verify_msg("DMSG AAAC AAAB Hello\\sthere!\n", 29); });
EXO_TEST(adc_message_parse_20, { return  verify_msg("EMSG AAAB AAAC Hello\\sthere!\n", 29); });
EXO_TEST(adc_message_parse_21, { return !verify_msg("EMSG AAAC AAAB Hello\\sthere!\n", 29); });
EXO_TEST(adc_message_parse_22, { return !verify_msg("\n", 0); });
EXO_TEST(adc_message_parse_23, { return !verify_msg("\r\n", 1); });
EXO_TEST(adc_message_parse_24, { return !verify_msg("EMSG AAAC\0AAAB Hello\\sthere!\n", 29); });


EXO_TEST(adc_message_add_arg_1, {
	int ok;
	struct adc_message* msg = adc_msg_create(test_string1);
	adc_msg_add_argument(msg, "XXwtf?");
	ok = str_match(msg->cache, "IINF AAfoo BBbar CCwhat XXwtf?\n");
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_add_arg_2, {
	int ok;
	struct adc_message* msg = adc_msg_create(test_string1);
	adc_msg_add_named_argument(msg, "XX", "wtf?");
	ok = str_match(msg->cache, "IINF AAfoo BBbar CCwhat XXwtf?\n");
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_add_arg_3, {
	int ok;
	struct adc_message* msg = adc_msg_create(test_string1);
	adc_msg_add_named_argument_int(msg, "II", 1234);
	ok = str_match(msg->cache, "IINF AAfoo BBbar CCwhat II1234\n");
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_add_arg_4, {
	int ok;
	struct adc_message* msg = adc_msg_create(test_string1);
	adc_msg_add_named_argument_uint64(msg, "UU", (uint64_t)1234567890123456789);
	ok = str_match(msg->cache, "IINF AAfoo BBbar CCwhat UU1234567890123456789\n");
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_remove_arg_1, {
	int ok;
	struct adc_message* msg = adc_msg_create(test_string1);
	adc_msg_remove_named_argument(msg, "AA");
	ok = str_match(msg->cache, "IINF BBbar CCwhat\n");
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_remove_arg_2, {
	int ok;
	struct adc_message* msg = adc_msg_create(test_string1);
	adc_msg_remove_named_argument(msg, "BB");
	ok = str_match(msg->cache, "IINF AAfoo CCwhat\n");
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_remove_arg_3, {
	int ok;
	struct adc_message* msg = adc_msg_create(test_string1);
	adc_msg_remove_named_argument(msg, "CC");
	ok = str_match(msg->cache, "IINF AAfoo BBbar\n");
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_remove_arg_4, {
	/* this ensures we can remove the last element also */
	int ok;
	struct adc_message* msg = adc_msg_parse_verify(g_user, test_string3, strlen(test_string3));
	adc_msg_remove_named_argument(msg, "AW");
	ok = str_match(msg->cache, "BINF AAAB IDAN7ZMSLIEBL53OPTM7WXGSTXUS3XOY6KQS5LBGX NIFriend DEstuff SL3 SS0 SF0 VEQuickDC/0.4.17 US6430 SUADC0,TCP4,UDP4 I4127.0.0.1 HO5 HN1\n");
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_replace_arg_1, {
	int ok;
	struct adc_message* msg = adc_msg_create(test_string1);
	adc_msg_remove_named_argument(msg, "AA");
	ok = str_match(msg->cache, "IINF BBbar CCwhat\n");
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_replace_arg_2, {
	int ok;
	struct adc_message* msg = adc_msg_create(test_string1);
	adc_msg_remove_named_argument(msg, "BB");
	ok = str_match(msg->cache, "IINF AAfoo CCwhat\n");
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_replace_arg_3, {
	int ok;
	struct adc_message* msg = adc_msg_create(test_string1);
	adc_msg_remove_named_argument(msg, "CC");
	ok = str_match(msg->cache, "IINF AAfoo BBbar\n");
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_get_arg_1, {
	int ok;
	struct adc_message* msg = adc_msg_create(test_string1);
	char* c = adc_msg_get_argument(msg, 0);
	ok = str_match(c, "AAfoo");
	adc_msg_free(msg);
	hub_free(c);
	return ok;
});

EXO_TEST(adc_message_get_arg_2, {
	int ok;
	struct adc_message* msg = adc_msg_create(test_string1);
	char* c = adc_msg_get_argument(msg, 1);
	ok = str_match(c, "BBbar");
	adc_msg_free(msg);
	hub_free(c);
	return ok;
});

EXO_TEST(adc_message_get_arg_3, {
	struct adc_message* msg = adc_msg_create(test_string1);
	char* c = adc_msg_get_argument(msg, 2);
	int ok = str_match(c, "CCwhat");
	adc_msg_free(msg);
	hub_free(c);
	return ok;
});

EXO_TEST(adc_message_get_arg_4, {
	struct adc_message* msg = adc_msg_create(test_string1);
	char* c = adc_msg_get_argument(msg, 3);
	int ok = c == 0;
	adc_msg_free(msg);
	hub_free(c);
	return ok;
});

EXO_TEST(adc_message_get_named_arg_1, {
	struct adc_message* msg = adc_msg_create(test_string1);
	char* c = adc_msg_get_named_argument(msg, "AA");
	int ok = str_match(c, "foo");
	adc_msg_free(msg);
	hub_free(c);
	return ok;
});

EXO_TEST(adc_message_get_named_arg_2, {
	struct adc_message* msg = adc_msg_create(test_string1);
	char* c = adc_msg_get_named_argument(msg, "BB");
	int ok = str_match(c, "bar");
	adc_msg_free(msg);
	hub_free(c);
	return ok;
});

EXO_TEST(adc_message_get_named_arg_3, {
	struct adc_message* msg = adc_msg_create(test_string1);
	char* c = adc_msg_get_named_argument(msg, "CC");
	int ok = str_match(c, "what");
	adc_msg_free(msg);
	hub_free(c);
	return ok;
});

EXO_TEST(adc_message_get_named_arg_4, {
	struct adc_message* msg = adc_msg_create(test_string1);
	char* c = adc_msg_get_named_argument(msg, "XX");
	int ok = c == 0;
	adc_msg_free(msg);
	hub_free(c);
	return ok;
});

EXO_TEST(adc_message_has_named_arg_1, {
	struct adc_message* msg = adc_msg_create(test_string1);
	int n = adc_msg_has_named_argument(msg, "XX");
	adc_msg_free(msg);
	return n == 0;
});

EXO_TEST(adc_message_has_named_arg_2, {
	struct adc_message* msg = adc_msg_create(test_string1);
	int n = adc_msg_has_named_argument(msg, "BB");
	adc_msg_free(msg);
	return n == 1;
});

EXO_TEST(adc_message_has_named_arg_3, {
	struct adc_message* msg = adc_msg_create(test_string1);
	int n = adc_msg_has_named_argument(msg, "CC");
	adc_msg_free(msg);
	return n == 1;
});

EXO_TEST(adc_message_has_named_arg_4, {
	int n;
	struct adc_message* msg = adc_msg_create(test_string1);
	adc_msg_add_argument(msg, "XXwtf?");
	n = adc_msg_has_named_argument(msg, "XX");
	adc_msg_free(msg);
	return n == 1;
});

EXO_TEST(adc_message_has_named_arg_5, {
	int n;
	struct adc_message* msg = adc_msg_create(test_string1);
	adc_msg_add_argument(msg, "XXone");
	adc_msg_add_argument(msg, "XXtwo");
	n = adc_msg_has_named_argument(msg, "XX");
	adc_msg_free(msg);
	return n == 2;
});

EXO_TEST(adc_message_has_named_arg_6, {
	int n;
	struct adc_message* msg = adc_msg_create(test_string1);
	adc_msg_add_argument(msg, "XXone");
	adc_msg_add_argument(msg, "XXtwo");
	adc_msg_add_argument(msg, "XXthree");
	n = adc_msg_has_named_argument(msg, "XX");
	adc_msg_free(msg);
	return n == 3;
});

EXO_TEST(adc_message_has_named_arg_7, {
	struct adc_message* msg = adc_msg_create(test_string1);
	int n = adc_msg_has_named_argument(msg, "AA");
	adc_msg_free(msg);
	return n == 1;
});


EXO_TEST(adc_message_get_named_arg_idx_1, {
	struct adc_message* msg = adc_msg_create(test_string1);
	int n = adc_msg_get_named_argument_index(msg, "AA");
	adc_msg_free(msg);
	return n == 0;
});

EXO_TEST(adc_message_get_named_arg_idx_2, {
	struct adc_message* msg = adc_msg_create(test_string1);
	int n = adc_msg_get_named_argument_index(msg, "BB");
	adc_msg_free(msg);
	return n == 1;
});

EXO_TEST(adc_message_get_named_arg_idx_3, {
	struct adc_message* msg = adc_msg_create(test_string1);
	int n = adc_msg_get_named_argument_index(msg, "CC");
	adc_msg_free(msg);
	return n == 2;
});

EXO_TEST(adc_message_get_named_arg_idx_4, {
	struct adc_message* msg = adc_msg_create(test_string1);
	int n = adc_msg_get_named_argument_index(msg, "DD");
	adc_msg_free(msg);
	return n == -1;
});

EXO_TEST(adc_message_get_named_arg_idx_5, {
	struct adc_message* msg = adc_msg_create(test_string1);
	int n = adc_msg_get_named_argument_index(msg, "II");
	adc_msg_free(msg);
	return n == -1;
});

EXO_TEST(adc_message_get_named_arg_idx_6, {
	struct adc_message* msg = adc_msg_create(test_string1);
	int n = adc_msg_get_named_argument_index(msg, "Af");
	adc_msg_free(msg);
	return n == -1;
});

EXO_TEST(adc_message_get_named_arg_idx_7, {
	struct adc_message* msg = adc_msg_create(test_string1);
	int n = adc_msg_get_named_argument_index(msg, "oo");
	adc_msg_free(msg);
	return n == -1;
});

/* Make sure it doesn't mangle the msg */
EXO_TEST(adc_message_get_named_arg_idx_8, {
	struct adc_message* msg = adc_msg_create(test_string1);
	int a = adc_msg_get_named_argument_index(msg, "AA");
	int b = adc_msg_get_named_argument_index(msg, "BB");
	int c = adc_msg_get_named_argument_index(msg, "CC");
	int d = adc_msg_get_named_argument_index(msg, "DD");
	int i = adc_msg_get_named_argument_index(msg, "II");
	int match = str_match(msg->cache, test_string1);
	adc_msg_free(msg);
	return a == 0 && b == 1 && c == 2 && d == -1 && i == -1 && match;
});

EXO_TEST(adc_message_terminate_1, {
	int ok;
	struct adc_message* msg = adc_msg_create("IINF AAfoo BBbar CCwhat");
	adc_msg_unterminate(msg);
	ok = str_match(msg->cache, "IINF AAfoo BBbar CCwhat");
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_terminate_2, {
	int ok;
	struct adc_message* msg = adc_msg_create(test_string1);
	adc_msg_unterminate(msg);
	adc_msg_terminate(msg);
	ok = str_match(msg->cache, test_string1);
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_terminate_3, {
	int ok;
	struct adc_message* msg = adc_msg_create(test_string1);
	adc_msg_unterminate(msg);
	adc_msg_terminate(msg);
	adc_msg_unterminate(msg);
	ok = str_match(msg->cache, "IINF AAfoo BBbar CCwhat");
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_terminate_4, {
	int ok;
	struct adc_message* msg = adc_msg_create(test_string1);
	adc_msg_unterminate(msg);
	adc_msg_terminate(msg);
	adc_msg_terminate(msg);
	ok = str_match(msg->cache, test_string1);
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_terminate_5, {
	int ok;
	struct adc_message* msg = adc_msg_create(test_string1);
	adc_msg_terminate(msg);
	adc_msg_terminate(msg);
	ok = str_match(msg->cache, test_string1);
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_terminate_6, {
	int ok;
	struct adc_message* msg = adc_msg_create(test_string1);
	adc_msg_unterminate(msg);
	adc_msg_unterminate(msg);
	ok = str_match(msg->cache, "IINF AAfoo BBbar CCwhat");
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_escape_1, {
	int ok;
	char* s = adc_msg_escape(test_string1);
	ok = str_match(s, "IINF\\sAAfoo\\sBBbar\\sCCwhat\\n");
	hub_free(s);
	return ok;
});

EXO_TEST(adc_message_escape_2, {
	char* s = adc_msg_escape(test_string1);
	char* s2 = adc_msg_unescape(s);
	int ok = str_match(s2, test_string1);
	hub_free(s);
	hub_free(s2);
	return ok;
});

EXO_TEST(adc_message_escape_3, {
	char* s = adc_msg_unescape(test_string1);
	int ok = str_match(s, test_string1);
	hub_free(s);
	return ok;
});

EXO_TEST(adc_message_escape_4, {
	char* s = adc_msg_escape(test_string2);
	char* s2 = adc_msg_unescape(s);
	int ok = str_match(s2, test_string2);
	hub_free(s);
	hub_free(s2);
	return ok;
});

EXO_TEST(adc_message_copy_1, {
	struct adc_message* msg1 = adc_msg_create(test_string1);
	struct adc_message* msg2 = adc_msg_copy(msg1);
	int ok = strncmp(msg1->cache, msg2->cache, msg1->length) == 0;
	adc_msg_free(msg1);
	adc_msg_free(msg2);
	return ok;
});

EXO_TEST(adc_message_copy_2, {
	struct adc_message* msg1 = adc_msg_parse_verify(g_user, test_string2, strlen(test_string2));
	struct adc_message* msg2 = adc_msg_copy(msg1);
	int ok = msg1->source == msg2->source;
	adc_msg_free(msg1);
	adc_msg_free(msg2);
	return ok;
});

EXO_TEST(adc_message_copy_3, {
	struct adc_message* msg1 = adc_msg_parse_verify(g_user, test_string2, strlen(test_string2));
	struct adc_message* msg2 = adc_msg_copy(msg1);
	int ok = (	msg1->cmd        == msg2->cmd &&
				msg1->source     == msg2->source &&
				msg1->target     == msg2->target &&
				msg1->length     == msg2->length &&
				msg1->priority   == msg2->priority &&
				msg1->capacity   == msg2->capacity && /* might not be true! */
				str_match(msg1->cache, msg2->cache));
	adc_msg_free(msg1);
	adc_msg_free(msg2);
	return ok;
});

EXO_TEST(adc_message_copy_4, {
	struct adc_message* msg1 = adc_msg_parse_verify(g_user, test_string2, strlen(test_string2));
	struct adc_message* msg2 = adc_msg_copy(msg1);
	int ok = msg2->target == 0;
	adc_msg_free(msg1);
	adc_msg_free(msg2);
	return ok;
});


EXO_TEST(adc_message_update_1, {
	updater1 = adc_msg_parse_verify(g_user, update_info1, strlen(update_info1));
	return updater1 != NULL;
});

EXO_TEST(adc_message_update_2, {
	user_set_info(g_user, updater1);
	return str_match(g_user->info->cache, updater1->cache) && g_user->info == updater1;
});

EXO_TEST(adc_message_update_3, {
	updater2 = adc_msg_parse_verify(g_user, update_info2, strlen(update_info2));
	return updater2 != NULL;
});

EXO_TEST(adc_message_update_4, {
	user_update_info(g_user, updater2);
	return strlen(g_user->info->cache) == 159;
});

EXO_TEST(adc_message_update_4_cleanup, {
	adc_msg_free(updater1);
	updater1 = 0;
	adc_msg_free(updater2);
	updater2 = 0;
	adc_msg_free(g_user->info);
	g_user->info = 0;
	return 1;
});


EXO_TEST(adc_message_empty_1, {
	struct adc_message* msg = adc_msg_parse_verify(g_user, test_string2, strlen(test_string2));
	int ok = adc_msg_is_empty(msg) == 0;
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_empty_2, {
	struct adc_message* msg = adc_msg_parse_verify(g_user, test_string4, strlen(test_string4));
	int ok = adc_msg_is_empty(msg);
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_empty_3, {
	struct adc_message* msg = adc_msg_parse_verify(g_user, test_string5, strlen(test_string5));
	int ok = adc_msg_is_empty(msg) == 0; /* arguably not empty, contains a space */
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_construct_source_1, {
	int ok;
	struct adc_message* msg = adc_msg_construct_source(ADC_CMD_BMSG, 2, 0);
	ok = str_match(msg->cache, "BMSG AAAC\n");
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_construct_source_dest_1, {
	int ok;
	struct adc_message* msg = adc_msg_construct_source_dest(ADC_CMD_DMSG, 3, 4, 0);
	ok = str_match(msg->cache, "DMSG AAAD AAAE\n");
	adc_msg_free(msg);
	return ok;
});

EXO_TEST(adc_message_last, {
	hub_free(g_user);
	g_user = 0;
	return g_user == 0;
});

