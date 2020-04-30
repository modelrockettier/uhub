#include <uhub.h>

static struct sid_pool* sid_pool = 0;

struct dummy_user
{
	sid_t sid;
};

static struct dummy_user* last = 0;
sid_t last_sid = 0;

EXO_TEST(sid_create_pool, {
	sid_pool = sid_pool_create(4);
	return sid_pool != 0;
});

EXO_TEST(sid_check_0a, {
	struct dummy_user* user = (struct dummy_user*) sid_lookup(sid_pool, 0);
	return user == 0;
});

EXO_TEST(sid_check_0b, {
	struct dummy_user* user = (struct dummy_user*) sid_lookup(sid_pool, 5);
	return user == 0;
});

EXO_TEST(sid_alloc_1, {
	struct dummy_user* user = hub_malloc_zero(sizeof(struct dummy_user));
	user->sid = sid_alloc(sid_pool, (struct hub_user*) user);
	last = user;
	last_sid = user->sid;
	return (user->sid > 0 && user->sid < 1048576);
});

EXO_TEST(sid_check_1a, {
	struct dummy_user* user = (struct dummy_user*) sid_lookup(sid_pool, last_sid);
	return last == user;
});

EXO_TEST(sid_check_1b, {
	struct dummy_user* user = (struct dummy_user*) sid_lookup(sid_pool, last_sid+1);
	return user == 0;
});

EXO_TEST(sid_alloc_2, {
	struct dummy_user* user = hub_malloc_zero(sizeof(struct dummy_user));
	user->sid = sid_alloc(sid_pool, (struct hub_user*) user);
	last_sid = user->sid;
	return (user->sid > 0 && user->sid < 1048576);
});

EXO_TEST(sid_check_2, {
	struct dummy_user* user = (struct dummy_user*) sid_lookup(sid_pool, last_sid);
	return last != user;
});

EXO_TEST(sid_alloc_3, {
	struct dummy_user* user = hub_malloc_zero(sizeof(struct dummy_user));
	user->sid = sid_alloc(sid_pool, (struct hub_user*) user);
	last_sid = user->sid;
	return (user->sid > 0 && user->sid < 1048576);
});

EXO_TEST(sid_check_3, {
	struct dummy_user* user = (struct dummy_user*) sid_lookup(sid_pool, last_sid);
	return last != user;
});

EXO_TEST(sid_alloc_4, {
	struct dummy_user* user = hub_malloc_zero(sizeof(struct dummy_user));
	user->sid = sid_alloc(sid_pool, (struct hub_user*) user);
	last_sid = user->sid;
	return (user->sid > 0 && user->sid < 1048576);
});

EXO_TEST(sid_check_4, {
	struct dummy_user* user = (struct dummy_user*) sid_lookup(sid_pool, last_sid);
	return last != user;
});

EXO_TEST(sid_alloc_5, {
	struct dummy_user user;
	sid_t sid;
	sid = sid_alloc(sid_pool, (struct hub_user*) &user);
	return sid == 0;
});

EXO_TEST(sid_check_6, {
	struct dummy_user* user = (struct dummy_user*) sid_lookup(sid_pool, 0);
	return user == 0;
});


EXO_TEST(sid_list_all_1, {
	sid_t s;
	size_t n = 0;
	int ok = 1;
	for (s = last->sid; s <= last_sid; s++)
	{
		struct dummy_user* user = (struct dummy_user*) sid_lookup(sid_pool, s);
		if (s != (user ? user->sid : (sid_t) -1))
		{
			ok = 0;
			break;
		}
		n++;
	}
	return ok && n == 4;
});

#define FREE_SID(N) \
	struct dummy_user* user = (struct dummy_user*) sid_lookup(sid_pool, N); \
	sid_free(sid_pool, N); \
	hub_free(user); \
	return sid_lookup(sid_pool, N) == NULL;

EXO_TEST(sid_remove_1, {
	FREE_SID(2);
});

EXO_TEST(sid_remove_2, {
	FREE_SID(1);
});

EXO_TEST(sid_remove_3, {
	FREE_SID(4);
});

EXO_TEST(sid_remove_4, {
	FREE_SID(3);
});

EXO_TEST(sid_destroy_pool, {
	sid_pool_destroy(sid_pool);
	sid_pool = 0;
	return sid_pool == 0;
});

#define SID_TO_STR(SID, EXPECT) str_match(sid_to_string(SID), EXPECT)
EXO_TEST(sid_to_str_1,  { return SID_TO_STR(      0, "AAAA"); });
EXO_TEST(sid_to_str_2,  { return SID_TO_STR(      7, "AAAH"); });
EXO_TEST(sid_to_str_3,  { return SID_TO_STR(     14, "AAAO"); });
EXO_TEST(sid_to_str_4,  { return SID_TO_STR(     21, "AAAV"); });
EXO_TEST(sid_to_str_5,  { return SID_TO_STR(     24, "AAAY"); });
EXO_TEST(sid_to_str_6,  { return SID_TO_STR(     25, "AAAZ"); });
EXO_TEST(sid_to_str_7,  { return SID_TO_STR(     26, "AAA2"); });
EXO_TEST(sid_to_str_8,  { return SID_TO_STR(     27, "AAA3"); });
EXO_TEST(sid_to_str_9,  { return SID_TO_STR(     30, "AAA6"); });
EXO_TEST(sid_to_str_10, { return SID_TO_STR(     31, "AAA7"); });
EXO_TEST(sid_to_str_11, { return SID_TO_STR(     32, "AABA"); });
EXO_TEST(sid_to_str_12, { return SID_TO_STR(     42, "AABK"); });
EXO_TEST(sid_to_str_13, { return SID_TO_STR(    100, "AADE"); });
EXO_TEST(sid_to_str_14, { return SID_TO_STR(    500, "AAPU"); });
EXO_TEST(sid_to_str_15, { return SID_TO_STR(   1000, "AA7I"); });
EXO_TEST(sid_to_str_16, { return SID_TO_STR(   1022, "AA76"); });
EXO_TEST(sid_to_str_17, { return SID_TO_STR(   1023, "AA77"); });
EXO_TEST(sid_to_str_18, { return SID_TO_STR(   1024, "ABAA"); });
EXO_TEST(sid_to_str_19, { return SID_TO_STR(   1025, "ABAB"); });
EXO_TEST(sid_to_str_20, { return SID_TO_STR(   1026, "ABAC"); });
EXO_TEST(sid_to_str_21, { return SID_TO_STR(   5000, "AE4I"); });
EXO_TEST(sid_to_str_22, { return SID_TO_STR(  10000, "AJYQ"); });
EXO_TEST(sid_to_str_23, { return SID_TO_STR(  20000, "ATRA"); });
EXO_TEST(sid_to_str_24, { return SID_TO_STR(  32766, "A776"); });
EXO_TEST(sid_to_str_25, { return SID_TO_STR(  32767, "A777"); });
EXO_TEST(sid_to_str_26, { return SID_TO_STR(  32768, "BAAA"); });
EXO_TEST(sid_to_str_27, { return SID_TO_STR(  32769, "BAAB"); });
EXO_TEST(sid_to_str_28, { return SID_TO_STR(  50000, "BQ2Q"); });
EXO_TEST(sid_to_str_29, { return SID_TO_STR( 100000, "DBVA"); });
EXO_TEST(sid_to_str_30, { return SID_TO_STR( 500000, "PIJA"); });
EXO_TEST(sid_to_str_31, { return SID_TO_STR(1000000, "6QSA"); });
EXO_TEST(sid_to_str_32, { return SID_TO_STR(1048574, "7776"); });
EXO_TEST(sid_to_str_33, { return SID_TO_STR(1048575, "7777"); });

#define STR_TO_SID(STR, EXPECT) string_to_sid(STR) == EXPECT
EXO_TEST(sid_from_str_1,  { return STR_TO_SID("AAAA",       0); });
EXO_TEST(sid_from_str_2,  { return STR_TO_SID("AAAH",       7); });
EXO_TEST(sid_from_str_3,  { return STR_TO_SID("AAAO",      14); });
EXO_TEST(sid_from_str_4,  { return STR_TO_SID("AAAV",      21); });
EXO_TEST(sid_from_str_5,  { return STR_TO_SID("AAAY",      24); });
EXO_TEST(sid_from_str_6,  { return STR_TO_SID("AAAZ",      25); });
EXO_TEST(sid_from_str_7,  { return STR_TO_SID("AAA2",      26); });
EXO_TEST(sid_from_str_8,  { return STR_TO_SID("AAA3",      27); });
EXO_TEST(sid_from_str_9,  { return STR_TO_SID("AAA6",      30); });
EXO_TEST(sid_from_str_10, { return STR_TO_SID("AAA7",      31); });
EXO_TEST(sid_from_str_11, { return STR_TO_SID("AABA",      32); });
EXO_TEST(sid_from_str_12, { return STR_TO_SID("AABK",      42); });
EXO_TEST(sid_from_str_13, { return STR_TO_SID("AADE",     100); });
EXO_TEST(sid_from_str_14, { return STR_TO_SID("AAPU",     500); });
EXO_TEST(sid_from_str_15, { return STR_TO_SID("AA7I",    1000); });
EXO_TEST(sid_from_str_16, { return STR_TO_SID("AA76",    1022); });
EXO_TEST(sid_from_str_17, { return STR_TO_SID("AA77",    1023); });
EXO_TEST(sid_from_str_18, { return STR_TO_SID("ABAA",    1024); });
EXO_TEST(sid_from_str_19, { return STR_TO_SID("ABAB",    1025); });
EXO_TEST(sid_from_str_20, { return STR_TO_SID("ABAC",    1026); });
EXO_TEST(sid_from_str_21, { return STR_TO_SID("AE4I",    5000); });
EXO_TEST(sid_from_str_22, { return STR_TO_SID("AJYQ",   10000); });
EXO_TEST(sid_from_str_23, { return STR_TO_SID("ATRA",   20000); });
EXO_TEST(sid_from_str_24, { return STR_TO_SID("A776",   32766); });
EXO_TEST(sid_from_str_25, { return STR_TO_SID("A777",   32767); });
EXO_TEST(sid_from_str_26, { return STR_TO_SID("BAAA",   32768); });
EXO_TEST(sid_from_str_27, { return STR_TO_SID("BAAB",   32769); });
EXO_TEST(sid_from_str_28, { return STR_TO_SID("BQ2Q",   50000); });
EXO_TEST(sid_from_str_29, { return STR_TO_SID("DBVA",  100000); });
EXO_TEST(sid_from_str_30, { return STR_TO_SID("PIJA",  500000); });
EXO_TEST(sid_from_str_31, { return STR_TO_SID("6QSA", 1000000); });
EXO_TEST(sid_from_str_32, { return STR_TO_SID("7776", 1048574); });
EXO_TEST(sid_from_str_33, { return STR_TO_SID("7777", 1048575); });
