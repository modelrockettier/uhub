#include <uhub.h>

static struct hub_info* hub = NULL;
static struct hub_command* cmd = NULL;
static struct hub_user user;
static struct command_base* cbase = NULL;
static struct command_handle* c_test1  = NULL;
static struct command_handle* c_test2  = NULL;
static struct command_handle* c_test3  = NULL;
static struct command_handle* c_test4  = NULL;
static struct command_handle* c_test5  = NULL;
static struct command_handle* c_test6  = NULL;
static struct command_handle* c_test7  = NULL;
static struct command_handle* c_test8  = NULL;
static struct command_handle* c_test9  = NULL;
static struct command_handle* c_test10 = NULL;
static struct command_handle* c_test11 = NULL;
static struct command_handle* c_test12 = NULL;
static struct command_handle* c_test13 = NULL;

// for results:
static int result = 0;

// for address and ip range argument processing
EXO_TEST(cmd_prepare_network, {
	return net_initialize() == 0;
});

EXO_TEST(setup, {
	hub = hub_malloc_zero(sizeof(struct hub_info));
	cbase = command_initialize(hub);
	hub->commands = cbase;
	hub->users = uman_init();
	return cbase && hub && hub->users;
});

static int test_handler(struct command_base* cbase, struct hub_user* user, struct hub_command* hcmd)
{
	printf("test_handler\n");
	result = 1;
	return 0;
}

static struct command_handle* create_handler(const char* prefix, const char* args, enum auth_credentials cred)
{
	struct command_handle* c = hub_malloc_zero(sizeof(struct command_handle));
	c->prefix = prefix;
	c->length = strlen(prefix);
	c->args = args;
	c->cred = cred;
	c->handler = test_handler;
	c->description = "A handler added by autotest.";
	c->origin = "exotic test";
	c->ptr = &c->ptr;
	return c;
}

EXO_TEST(command_setup_user, {
	memset(&user, 0, sizeof(user));
	user.id.sid = 1;
	strcpy(user.id.nick, "tester");
	strcpy(user.id.cid, "3AGHMAASJA2RFNM22AA6753V7B7DYEPNTIWHBAY");
	user.credentials = auth_cred_guest;
	return 1;
});

#define ADD_TEST(var, prefix, args, cred) \
	var = create_handler(prefix, args, cred); \
	if (!command_add(cbase, var, NULL)) \
		return 0;

#define DEL_TEST(var) \
		if (var) \
		{ \
			if (!command_del(cbase, var)) \
				return 0; \
			hub_free(var); \
			var = NULL; \
		}

EXO_TEST(command_create, {
	ADD_TEST(c_test1,  "test1",  "",       auth_cred_guest);
	ADD_TEST(c_test2,  "test2",  "",       auth_cred_operator);
	ADD_TEST(c_test3,  "test3",  "N?N?N",  auth_cred_guest);
	ADD_TEST(c_test4,  "test4",  "u",      auth_cred_guest);
	ADD_TEST(c_test5,  "test5",  "i",      auth_cred_guest);
	ADD_TEST(c_test6,  "test6",  "?c",     auth_cred_guest);
	ADD_TEST(c_test7,  "test7",  "C",      auth_cred_guest);
	ADD_TEST(c_test8,  "test8",  "n",      auth_cred_guest);
	ADD_TEST(c_test9,  "test9",  "+m",     auth_cred_guest);
	ADD_TEST(c_test10, "test10", "?+p", auth_cred_guest);
	/* Weird arg string to test the parser, the arg is required since + is before the ? */
	ADD_TEST(c_test11, "test11", "++?+?p", auth_cred_guest);
	ADD_TEST(c_test12, "test12", "a",      auth_cred_guest);
	ADD_TEST(c_test13, "test13", "r",      auth_cred_guest);
	return 1;
});

extern void command_destroy(struct hub_command* cmd);

static int verify(const char* str, enum command_parse_status expected)
{
	struct hub_command* cmd = command_parse(cbase, hub, &user, str);
	enum command_parse_status status = cmd->status;
	command_free(cmd);
	return status == expected;
}

static struct hub_command_arg_data* verify_argument(struct hub_command* cmd, enum hub_command_arg_type type)
{
	return  hub_command_arg_next(cmd, type);
}

static int verify_arg_integer(struct hub_command* cmd, int expected)
{
	struct hub_command_arg_data* data = verify_argument(cmd, type_integer);
	return data->data.integer == expected;
}

static int verify_arg_user(struct hub_command* cmd, struct hub_user* expected)
{
	struct hub_command_arg_data* data = verify_argument(cmd, type_user);
	return data->data.user == expected;
}

static int verify_arg_cred(struct hub_command* cmd, enum auth_credentials cred)
{
	struct hub_command_arg_data* data = verify_argument(cmd, type_credentials);
	return data->data.credentials == cred;
}

static int verify_arg_string(struct hub_command* cmd, const char* expected)
{
	struct hub_command_arg_data* data = verify_argument(cmd, type_string);
	return str_match(data ? data->data.string : NULL, expected);
}

static int verify_arg_addr(struct hub_command* cmd, const char* expected)
{
	struct hub_command_arg_data* data = verify_argument(cmd, type_address);
	struct ip_addr_encap ip;
	return ip_convert_to_binary(expected, &ip) && !ip_compare(data ? data->data.address : NULL, &ip);
}

static int verify_arg_range(struct hub_command* cmd, const char* lo, const char* hi)
{
	struct hub_command_arg_data* data = verify_argument(cmd, type_range);
	struct ip_range range;
	return
		ip_convert_to_binary(lo, &range.lo) &&
		ip_convert_to_binary(hi, &range.hi) &&
		!ip_compare(data ? &data->data.range->lo : NULL, &range.lo) &&
		!ip_compare(data ? &data->data.range->hi : NULL, &range.hi);
}

EXO_TEST(command_access_1, { return verify("!test1", cmd_status_ok); });
EXO_TEST(command_access_2, { return verify("!test2", cmd_status_access_error); });
EXO_TEST(command_access_3, { user.credentials = auth_cred_operator; return verify("!test2", cmd_status_ok); });

EXO_TEST(command_syntax_1, { return verify("",  cmd_status_syntax_error); });
EXO_TEST(command_syntax_2, { return verify("!", cmd_status_syntax_error); });

EXO_TEST(command_missing_args_1, { return verify("!test3",         cmd_status_missing_args); });
EXO_TEST(command_missing_args_2, { return verify("!test3 12345",   cmd_status_ok); });
EXO_TEST(command_missing_args_3, { return verify("!test3 1 2 345", cmd_status_ok); });

EXO_TEST(command_number_1, { return verify("!test3 abc", cmd_status_arg_number); });
EXO_TEST(command_number_2, { return verify("!test3 -",   cmd_status_arg_number); });
EXO_TEST(command_number_3, { return verify("!test3 -12", cmd_status_ok); });

EXO_TEST(command_user_1, { return verify("!test4 tester", cmd_status_arg_nick); });
EXO_TEST(command_user_2, { return verify("!test5 3AGHMAASJA2RFNM22AA6753V7B7DYEPNTIWHBAY", cmd_status_arg_cid); });
EXO_TEST(command_user_3, { return uman_add(hub->users, &user) == 0; });
EXO_TEST(command_user_4, { return verify("!test4 tester", cmd_status_ok); });
EXO_TEST(command_user_5, { return verify("!test5 3AGHMAASJA2RFNM22AA6753V7B7DYEPNTIWHBAY", cmd_status_ok); });

EXO_TEST(command_command_1, { return verify("!test6 test1", cmd_status_ok); });
EXO_TEST(command_command_2, { return verify("!test6 test2", cmd_status_ok); });
EXO_TEST(command_command_3, { return verify("!test6 test3", cmd_status_ok); });
EXO_TEST(command_command_4, { return verify("!test6 test4", cmd_status_ok); });
EXO_TEST(command_command_5, { return verify("!test6 test5", cmd_status_ok); });
EXO_TEST(command_command_6, { return verify("!test6 test6", cmd_status_ok); });
EXO_TEST(command_command_7, { return verify("!test6 fail",  cmd_status_arg_command); });
EXO_TEST(command_command_8, { return verify("!test6",       cmd_status_ok); });

EXO_TEST(command_cred_1, { return verify("!test7 guest",    cmd_status_ok); });
EXO_TEST(command_cred_2, { return verify("!test7 user",     cmd_status_ok); });
EXO_TEST(command_cred_3, { return verify("!test7 operator", cmd_status_ok); });
EXO_TEST(command_cred_4, { return verify("!test7 super",    cmd_status_ok); });
EXO_TEST(command_cred_5, { return verify("!test7 admin",    cmd_status_ok); });
EXO_TEST(command_cred_6, { return verify("!test7 nobody",   cmd_status_arg_cred); });
EXO_TEST(command_cred_7, { return verify("!test7 bot",      cmd_status_ok); });
EXO_TEST(command_cred_8, { return verify("!test7 link",     cmd_status_ok); });

EXO_TEST(command_string_1,  { return verify("!test8",                       cmd_status_missing_args); });
EXO_TEST(command_string_2,  { return verify("!test8 ",                      cmd_status_missing_args); });
EXO_TEST(command_string_3,  { return verify("!test8 hello",                 cmd_status_ok); });
EXO_TEST(command_string_4,  { return verify("!test8 hello world",           cmd_status_ok); });
EXO_TEST(command_string_5,  { return verify("!test8 123",                   cmd_status_ok); });
EXO_TEST(command_string_6,  { return verify("!test8 127.0.0.1",             cmd_status_ok); });
EXO_TEST(command_string_7,  { return verify("!test8 tester",                cmd_status_ok); });
EXO_TEST(command_string_8,  { return verify("!test8 admin",                 cmd_status_ok); });
EXO_TEST(command_string_9,  { return verify("!test8 tester 123 127.0.0.1",  cmd_status_ok); });

EXO_TEST(command_string_10, { return verify("!test9",                       cmd_status_missing_args); });
EXO_TEST(command_string_11, { return verify("!test9 ",                      cmd_status_missing_args); });
EXO_TEST(command_string_12, { return verify("!test9 hello",                 cmd_status_ok); });
EXO_TEST(command_string_13, { return verify("!test9 hello world",           cmd_status_ok); });
EXO_TEST(command_string_14, { return verify("!test9 123",                   cmd_status_ok); });
EXO_TEST(command_string_15, { return verify("!test9 127.0.0.1",             cmd_status_ok); });
EXO_TEST(command_string_16, { return verify("!test9 tester",                cmd_status_ok); });
EXO_TEST(command_string_17, { return verify("!test9 admin",                 cmd_status_ok); });
EXO_TEST(command_string_18, { return verify("!test9 tester 123 127.0.0.1",  cmd_status_ok); });

EXO_TEST(command_string_19, { return verify("!test10",                      cmd_status_ok); });
EXO_TEST(command_string_20, { return verify("!test10 ",                     cmd_status_ok); });
EXO_TEST(command_string_21, { return verify("!test10 hello",                cmd_status_ok); });
EXO_TEST(command_string_22, { return verify("!test10 hello world",          cmd_status_ok); });
EXO_TEST(command_string_23, { return verify("!test10 123",                  cmd_status_ok); });
EXO_TEST(command_string_24, { return verify("!test10 127.0.0.1",            cmd_status_ok); });
EXO_TEST(command_string_25, { return verify("!test10 tester",               cmd_status_ok); });
EXO_TEST(command_string_26, { return verify("!test10 admin",                cmd_status_ok); });
EXO_TEST(command_string_27, { return verify("!test10 tester 123 127.0.0.1", cmd_status_ok); });

EXO_TEST(command_string_28, { return verify("!test11",                      cmd_status_missing_args); });
EXO_TEST(command_string_29, { return verify("!test11 ",                     cmd_status_missing_args); });
EXO_TEST(command_string_30, { return verify("!test11 hello",                cmd_status_ok); });
EXO_TEST(command_string_31, { return verify("!test11 hello world",          cmd_status_ok); });
EXO_TEST(command_string_32, { return verify("!test11 123",                  cmd_status_ok); });
EXO_TEST(command_string_33, { return verify("!test11 127.0.0.1",            cmd_status_ok); });
EXO_TEST(command_string_34, { return verify("!test11 tester",               cmd_status_ok); });
EXO_TEST(command_string_35, { return verify("!test11 admin",                cmd_status_ok); });
EXO_TEST(command_string_36, { return verify("!test11 tester 123 127.0.0.1", cmd_status_ok); });

EXO_TEST(command_addr_1,  { return verify("!test12 0.0.0.0",                 cmd_status_ok); });
EXO_TEST(command_addr_2,  { return verify("!test12 255.255.255.255",         cmd_status_ok); });
EXO_TEST(command_addr_3,  { return verify("!test12 127.0.0.1",               cmd_status_ok); });
EXO_TEST(command_addr_4,  { return verify("!test12 10.18.1.178",             cmd_status_ok); });
EXO_TEST(command_addr_5,  { return verify("!test12 224.0.0.1",               cmd_status_ok); });
EXO_TEST(command_addr_6,  { return verify("!test12 224.0.0. ",               cmd_status_arg_address); });
EXO_TEST(command_addr_7,  { return verify("!test12 invalid",                 cmd_status_arg_address); });
EXO_TEST(command_addr_8,  { return verify("!test12 localhost",               cmd_status_arg_address); });
EXO_TEST(command_addr_9,  { return verify("!test12 123.45.67.890",           cmd_status_arg_address); });
EXO_TEST(command_addr_10, { return verify("!test12 777.777.777.777",         cmd_status_arg_address); });
EXO_TEST(command_addr_11, { return verify("!test12 224.0.0.0-224.0.0.255",   cmd_status_arg_address); });
EXO_TEST(command_addr_12, { return verify("!test12 224.0.0.0/24",            cmd_status_arg_address); });
EXO_TEST(command_addr_13, { return verify("!test12 ::",                      cmd_status_ok); });
EXO_TEST(command_addr_14, { return verify("!test12 0:0:0:0:0:0:0:0",         cmd_status_ok); });
EXO_TEST(command_addr_15, { return verify("!test12 ::1",                     cmd_status_ok); });
EXO_TEST(command_addr_16, { return verify("!test12 ::ffff:0.0.0.0",          cmd_status_ok); });
EXO_TEST(command_addr_17, { return verify("!test12 ::ffff:127.0.0.1",        cmd_status_ok); });
EXO_TEST(command_addr_18, { return verify("!test12 ::ffff:255.255.255.255",  cmd_status_ok); });
EXO_TEST(command_addr_19, { return verify("!test12 2001::",                  cmd_status_ok); });
EXO_TEST(command_addr_20, { return verify("!test12 2001::201:2ff:fefa:fffe", cmd_status_ok); });
EXO_TEST(command_addr_21, { return verify("!test12 ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", cmd_status_ok); });
EXO_TEST(command_addr_22, { return verify("!test12 2001:",                   cmd_status_arg_address); });
EXO_TEST(command_addr_23, { return verify("!test12 :2001",                   cmd_status_arg_address); });
EXO_TEST(command_addr_24, { return verify("!test12 :ffff:0.0.0.0",           cmd_status_arg_address); });
EXO_TEST(command_addr_25, { return verify("!test12 2001::x01:2ff:fefa:fffe", cmd_status_arg_address); });
EXO_TEST(command_addr_26, { return verify("!test12 2001::201:2ff:fefa::",    cmd_status_arg_address); });
EXO_TEST(command_addr_27, { return verify("!test12 2.0.0.1::2ff",            cmd_status_arg_address); });
EXO_TEST(command_addr_28, { return verify("!test12 ::ffff:224.0.0.",         cmd_status_arg_address); });
EXO_TEST(command_addr_29, { return verify("!test12 0:0:0:0:0:0:0:0:0",       cmd_status_arg_address); });
EXO_TEST(command_addr_30, { return verify("!test12 ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", cmd_status_arg_address); });
EXO_TEST(command_addr_31, { return verify("!test12 2001::/120",              cmd_status_arg_address); });
EXO_TEST(command_addr_32, { return verify("!test12 2001::-2002::",           cmd_status_arg_address); });

EXO_TEST(command_ipv4_range_1,  { return verify("!test13 0.0.0.0/0",                       cmd_status_ok); });
EXO_TEST(command_ipv4_range_2,  { return verify("!test13 0.0.0.0-255.255.255.255",         cmd_status_ok); });
EXO_TEST(command_ipv4_range_3,  { return verify("!test13 0.0.0.0/1",                       cmd_status_ok); });
EXO_TEST(command_ipv4_range_4,  { return verify("!test13 0.0.0.0-127.255.255.255",         cmd_status_ok); });
EXO_TEST(command_ipv4_range_5,  { return verify("!test13 255.255.255.254/32",              cmd_status_ok); });
EXO_TEST(command_ipv4_range_6,  { return verify("!test13 255.255.255.254",                 cmd_status_ok); });
EXO_TEST(command_ipv4_range_7,  { return verify("!test13 255.255.255.254-255.255.255.254", cmd_status_ok); });
EXO_TEST(command_ipv4_range_8,  { return verify("!test13 127.0.0.1/8",                     cmd_status_ok); });
EXO_TEST(command_ipv4_range_9,  { return verify("!test13 127.0.0.0-127.255.255.255",       cmd_status_ok); });
EXO_TEST(command_ipv4_range_10, { return verify("!test13 10.18.1.100/30",                  cmd_status_ok); });
EXO_TEST(command_ipv4_range_11, { return verify("!test13 10.18.1.100-10.18.1.103",         cmd_status_ok); });
EXO_TEST(command_ipv4_range_12, { return verify("!test13 192.168.0.0/16",                  cmd_status_ok); });
EXO_TEST(command_ipv4_range_13, { return verify("!test13 192.168.0.0-192.168.255.255",     cmd_status_ok); });
EXO_TEST(command_ipv4_range_14, { return verify("!test13 24.0.0.0-24",                     cmd_status_arg_address); });
EXO_TEST(command_ipv4_range_15, { return verify("!test13 24.0.0.0-24.0.0",                 cmd_status_arg_address); });
EXO_TEST(command_ipv4_range_16, { return verify("!test13 224.0.0. ",                       cmd_status_arg_address); });
EXO_TEST(command_ipv4_range_17, { return verify("!test13 224.0.0.0-",                      cmd_status_arg_address); });
EXO_TEST(command_ipv4_range_18, { return verify("!test13 224.0.0.0/",                      cmd_status_arg_address); });
EXO_TEST(command_ipv4_range_19, { return verify("!test13 224.0.0.0/a",                     cmd_status_arg_address); });
EXO_TEST(command_ipv4_range_20, { return verify("!test13 224.0.0./24 ",                    cmd_status_arg_address); });
EXO_TEST(command_ipv4_range_21, { return verify("!test13 224.0.0.-224.0.0.255",            cmd_status_arg_address); });
EXO_TEST(command_ipv4_range_22, { return verify("!test13 224.0.0.0-224.0.0. ",             cmd_status_arg_address); });
EXO_TEST(command_ipv4_range_23, { return verify("!test13 123.45.67.89-123.45.67.890",      cmd_status_arg_address); });
EXO_TEST(command_ipv4_range_24, { return verify("!test13 123.45.67.890/12",                cmd_status_arg_address); });
EXO_TEST(command_ipv4_range_25, { return verify("!test13 777.777.777.777",                 cmd_status_arg_address); });

EXO_TEST(command_ipv6_range_1,  { return verify("!test13 ::",                                              cmd_status_ok); });
EXO_TEST(command_ipv6_range_2,  { return verify("!test13 ::/0",                                            cmd_status_ok); });
EXO_TEST(command_ipv6_range_3,  { return verify("!test13 ::/32",                                           cmd_status_ok); });
EXO_TEST(command_ipv6_range_4,  { return verify("!test13 ::/128",                                          cmd_status_ok); });
EXO_TEST(command_ipv6_range_5,  { return verify("!test13 0:0:0:0:0:0:0:0",                                 cmd_status_ok); });
EXO_TEST(command_ipv6_range_6,  { return verify("!test13 0:0:0:0:0:0:0:0-0:0:0:0:0:0:0:ffff",              cmd_status_ok); });
EXO_TEST(command_ipv6_range_7,  { return verify("!test13 0:0:0:0:0:0:0:0/100",                             cmd_status_ok); });
EXO_TEST(command_ipv6_range_8,  { return verify("!test13 ::-::ffff",                                       cmd_status_ok); });
EXO_TEST(command_ipv6_range_9,  { return verify("!test13 ::-ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",      cmd_status_ok); });
EXO_TEST(command_ipv6_range_10, { return verify("!test13 ::1",                                             cmd_status_ok); });
EXO_TEST(command_ipv6_range_11, { return verify("!test13 ::1/128",                                         cmd_status_ok); });
EXO_TEST(command_ipv6_range_12, { return verify("!test13 ::ffff:0.0.0.0/96",                               cmd_status_ok); });
EXO_TEST(command_ipv6_range_13, { return verify("!test13 ::ffff:0.0.0.0-::ffff:255.255.255.255",           cmd_status_ok); });
EXO_TEST(command_ipv6_range_14, { return verify("!test13 ::ffff:0.0.0.0/97",                               cmd_status_ok); });
EXO_TEST(command_ipv6_range_15, { return verify("!test13 ::ffff:0.0.0.0-::ffff:127.255.255.255",           cmd_status_ok); });
EXO_TEST(command_ipv6_range_16, { return verify("!test13 ::ffff:127.0.0.1",                                cmd_status_ok); });
EXO_TEST(command_ipv6_range_17, { return verify("!test13 ::ffff:255.255.255.255/128",                      cmd_status_ok); });
EXO_TEST(command_ipv6_range_18, { return verify("!test13 2001::",                                          cmd_status_ok); });
EXO_TEST(command_ipv6_range_19, { return verify("!test13 2001::/16",                                       cmd_status_ok); });
EXO_TEST(command_ipv6_range_20, { return verify("!test13 2001::-2001:ffff:ffff:ffff:ffff:ffff:ffff:ffff",  cmd_status_ok); });
EXO_TEST(command_ipv6_range_21, { return verify("!test13 2001::201:2ff:fefa:fff4/126",                     cmd_status_ok); });
EXO_TEST(command_ipv6_range_22, { return verify("!test13 2001::201:2ff:fefa:fff4-2001::201:2ff:fefa:fff7", cmd_status_ok); });
EXO_TEST(command_ipv6_range_23, { return verify("!test13 ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",         cmd_status_ok); });
EXO_TEST(command_ipv6_range_24, { return verify("!test13 ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/128",     cmd_status_ok); });
EXO_TEST(command_ipv6_range_25, { return verify("!test13 2001:",                                           cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_26, { return verify("!test13 2001::x01:2ff:fefa:fffe",                         cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_27, { return verify("!test13 2001::x01:2ff:fefa:fffe/128",                     cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_28, { return verify("!test13 2001::201:2ff:fefa::",                            cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_29, { return verify("!test13 2001::201:2ff:fefa::/120",                        cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_30, { return verify("!test13 2.0.0.1::2ff",                                    cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_31, { return verify("!test13 2.0.0.1::2ff/128",                                cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_32, { return verify("!test13 ::ffff:224.0.0.",                                 cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_33, { return verify("!test13 ::ffff:224.0.0./120",                             cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_34, { return verify("!test13 2001::-2001",                                     cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_35, { return verify("!test13 2001::-2001:",                                    cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_36, { return verify("!test13 2001:0:0:0:0:0:0:0:0/120",                        cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_37, { return verify("!test13 2001:0:0:0:0:0:0:0:0-2002:0:0:0:0:0:0:0",         cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_38, { return verify("!test13 2001:0:0:0:0:0:0:0-2002:0:0:0:0:0:0:0:0",         cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_39, { return verify("!test13 2001:0:0:0:0:0:0:0:0-2002::",                     cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_40, { return verify("!test13 2001::-2002:0:0:0:0:0:0:0:0",                     cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_41, { return verify("!test13 2001::-",                                         cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_42, { return verify("!test13 2001::/",                                         cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_43, { return verify("!test13 2001::/a",                                        cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_44, { return verify("!test13 127.0.0.0-::ffff:127.0.0.255",                    cmd_status_arg_address); });
EXO_TEST(command_ipv6_range_45, { return verify("!test13 ::ffff:127.0.0.0-127.0.0.255",                    cmd_status_arg_address); });


// command not found
EXO_TEST(command_parse_3, { return verify("!fail", cmd_status_not_found); });

// built-in command
EXO_TEST(command_parse_4, { return verify("!help", cmd_status_ok); });

EXO_TEST(command_help_test1,  { return verify("!help test1",  cmd_status_ok); });
EXO_TEST(command_help_test2,  { return verify("!help test2",  cmd_status_ok); });
EXO_TEST(command_help_test3,  { return verify("!help test3",  cmd_status_ok); });
EXO_TEST(command_help_test4,  { return verify("!help test4",  cmd_status_ok); });
EXO_TEST(command_help_test5,  { return verify("!help test5",  cmd_status_ok); });
EXO_TEST(command_help_test6,  { return verify("!help test6",  cmd_status_ok); });
EXO_TEST(command_help_test7,  { return verify("!help test7",  cmd_status_ok); });
EXO_TEST(command_help_test8,  { return verify("!help test8",  cmd_status_ok); });
EXO_TEST(command_help_test9,  { return verify("!help test9",  cmd_status_ok); });
EXO_TEST(command_help_test10, { return verify("!help test10", cmd_status_ok); });
EXO_TEST(command_help_test11, { return verify("!help test11", cmd_status_ok); });
EXO_TEST(command_help_test12, { return verify("!help test12", cmd_status_ok); });
EXO_TEST(command_help_test13, { return verify("!help test13", cmd_status_ok); });

#define SETUP_COMMAND(string) \
	do { \
		if (cmd) command_free(cmd); \
		cmd = command_parse(cbase, hub, &user, string); \
	} while(0)

EXO_TEST(command_argument_integer_1, {
	SETUP_COMMAND("!test3");
	return verify_argument(cmd, type_integer) == NULL;
});

EXO_TEST(command_argument_integer_2, {
	SETUP_COMMAND("!test3 10 42");
	return verify_arg_integer(cmd, 10) && verify_arg_integer(cmd, 42) && verify_argument(cmd, type_integer) == NULL;
});

EXO_TEST(command_argument_integer_3, {
	SETUP_COMMAND("!test3 10 42 6784");
	return verify_arg_integer(cmd, 10) && verify_arg_integer(cmd, 42) && verify_arg_integer(cmd, 6784);
});

EXO_TEST(command_argument_user_1, {
	SETUP_COMMAND("!test4 tester");
	return verify_arg_user(cmd, &user) ;
});

EXO_TEST(command_argument_cid_1, {
	SETUP_COMMAND("!test5 3AGHMAASJA2RFNM22AA6753V7B7DYEPNTIWHBAY");
	return verify_arg_user(cmd, &user) ;
});

EXO_TEST(command_argument_cred_1, {
	SETUP_COMMAND("!test7 admin");
	return verify_arg_cred(cmd, auth_cred_admin);
});

EXO_TEST(command_argument_cred_2, {
	SETUP_COMMAND("!test7 op");
	return verify_arg_cred(cmd, auth_cred_operator);
});

EXO_TEST(command_argument_cred_3, {
	SETUP_COMMAND("!test7 operator");
	return verify_arg_cred(cmd, auth_cred_operator);
});

EXO_TEST(command_argument_cred_4, {
	SETUP_COMMAND("!test7 super");
	return verify_arg_cred(cmd, auth_cred_super);
});

EXO_TEST(command_argument_cred_5, {
	SETUP_COMMAND("!test7 guest");
	return verify_arg_cred(cmd, auth_cred_guest);
});

EXO_TEST(command_argument_cred_6, {
	SETUP_COMMAND("!test7 user");
	return verify_arg_cred(cmd, auth_cred_user);
});

static int test_string(const char* command, const char* expected)
{
	SETUP_COMMAND(command);
	return verify_arg_string(cmd, expected);
}

EXO_TEST(command_argument_string_1,  { return test_string("!test8 hello",                "hello"); });
EXO_TEST(command_argument_string_2,  { return test_string("!test8 hello world",          "hello"); });
EXO_TEST(command_argument_string_3,  { return test_string("!test8 123",                  "123"); });
EXO_TEST(command_argument_string_4,  { return test_string("!test8 127.0.0.1",            "127.0.0.1"); });
EXO_TEST(command_argument_string_5,  { return test_string("!test8 tester",               "tester"); });
EXO_TEST(command_argument_string_6,  { return test_string("!test8 admin",                "admin"); });
EXO_TEST(command_argument_string_7,  { return test_string("!test8 tester 123 127.0.0.1", "tester"); });

EXO_TEST(command_argument_string_8,  { return test_string("!test9 hello",                "hello"); });
EXO_TEST(command_argument_string_9,  { return test_string("!test9 hello world",          "hello world"); });
EXO_TEST(command_argument_string_10, { return test_string("!test9 123",                  "123"); });
EXO_TEST(command_argument_string_11, { return test_string("!test9 127.0.0.1",            "127.0.0.1"); });
EXO_TEST(command_argument_string_12, { return test_string("!test9 tester",               "tester"); });
EXO_TEST(command_argument_string_13, { return test_string("!test9 admin",                "admin"); });
EXO_TEST(command_argument_string_14, { return test_string("!test9 tester 123 127.0.0.1", "tester 123 127.0.0.1"); });

EXO_TEST(command_argument_string_15, { return test_string("!test10",                      NULL); });
EXO_TEST(command_argument_string_16, { return test_string("!test10 ",                     NULL); });
EXO_TEST(command_argument_string_17, { return test_string("!test10 hello",                "hello"); });
EXO_TEST(command_argument_string_18, { return test_string("!test10 hello world",          "hello world"); });
EXO_TEST(command_argument_string_19, { return test_string("!test10 123",                  "123"); });
EXO_TEST(command_argument_string_20, { return test_string("!test10 127.0.0.1",            "127.0.0.1"); });
EXO_TEST(command_argument_string_21, { return test_string("!test10 tester",               "tester"); });
EXO_TEST(command_argument_string_22, { return test_string("!test10 admin",                "admin"); });
EXO_TEST(command_argument_string_23, { return test_string("!test10 tester 123 127.0.0.1", "tester 123 127.0.0.1"); });

EXO_TEST(command_argument_string_24, { return test_string("!test11 hello",                "hello"); });
EXO_TEST(command_argument_string_25, { return test_string("!test11 hello world",          "hello world"); });
EXO_TEST(command_argument_string_26, { return test_string("!test11 123",                  "123"); });
EXO_TEST(command_argument_string_27, { return test_string("!test11 127.0.0.1",            "127.0.0.1"); });
EXO_TEST(command_argument_string_28, { return test_string("!test11 tester",               "tester"); });
EXO_TEST(command_argument_string_29, { return test_string("!test11 admin",                "admin"); });
EXO_TEST(command_argument_string_30, { return test_string("!test11 tester 123 127.0.0.1", "tester 123 127.0.0.1"); });

static int test_addr(const char* command, const char* expected)
{
	SETUP_COMMAND(command);
	return verify_arg_addr(cmd, expected);
}

EXO_TEST(command_argument_addr_1,  { return test_addr("!test12 0.0.0.0",                 "0.0.0.0"); });
EXO_TEST(command_argument_addr_2,  { return test_addr("!test12 255.255.255.255",         "255.255.255.255"); });
EXO_TEST(command_argument_addr_3,  { return test_addr("!test12 127.0.0.1",               "127.0.0.1"); });
EXO_TEST(command_argument_addr_4,  { return test_addr("!test12 10.18.1.178",             "10.18.1.178"); });
EXO_TEST(command_argument_addr_5,  { return test_addr("!test12 224.0.0.1",               "224.0.0.1"); });
EXO_TEST(command_argument_addr_6,  { return test_addr("!test12 ::",                      "::"); });
EXO_TEST(command_argument_addr_7,  { return test_addr("!test12 ::1",                     "::1"); });
EXO_TEST(command_argument_addr_8,  { return test_addr("!test12 ::ffff:0.0.0.0",          "::ffff:0.0.0.0"); });
EXO_TEST(command_argument_addr_9,  { return test_addr("!test12 ::ffff:127.0.0.1",        "::ffff:127.0.0.1"); });
EXO_TEST(command_argument_addr_10, { return test_addr("!test12 ::ffff:255.255.255.255",  "::ffff:255.255.255.255"); });
EXO_TEST(command_argument_addr_11, { return test_addr("!test12 2001::",                  "2001::"); });
EXO_TEST(command_argument_addr_12, { return test_addr("!test12 2001::201:2ff:fefa:fffe", "2001::201:2ff:fefa:fffe"); });
EXO_TEST(command_argument_addr_13, { return test_addr("!test12 ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"); });

static int test_range(const char* command, const char* lo, const char* hi)
{
	SETUP_COMMAND(command);
	return verify_arg_range(cmd, lo, hi);
}

EXO_TEST(command_argument_range4_1,  { return test_range("!test13 0.0.0.0/0",                       "0.0.0.0",         "255.255.255.255"); });
EXO_TEST(command_argument_range4_2,  { return test_range("!test13 0.0.0.0-255.255.255.255",         "0.0.0.0",         "255.255.255.255"); });
EXO_TEST(command_argument_range4_3,  { return test_range("!test13 0.0.0.0/1",                       "0.0.0.0",         "127.255.255.255"); });
EXO_TEST(command_argument_range4_4,  { return test_range("!test13 0.0.0.0-127.255.255.255",         "0.0.0.0",         "127.255.255.255"); });
EXO_TEST(command_argument_range4_5,  { return test_range("!test13 255.255.255.254/32",              "255.255.255.254", "255.255.255.254"); });
EXO_TEST(command_argument_range4_6,  { return test_range("!test13 255.255.255.254",                 "255.255.255.254", "255.255.255.254"); });
EXO_TEST(command_argument_range4_7,  { return test_range("!test13 255.255.255.254-255.255.255.254", "255.255.255.254", "255.255.255.254"); });
EXO_TEST(command_argument_range4_8,  { return test_range("!test13 127.0.0.1/8",                     "127.0.0.0",       "127.255.255.255"); });
EXO_TEST(command_argument_range4_9,  { return test_range("!test13 127.0.0.0-127.255.255.255",       "127.0.0.0",       "127.255.255.255"); });
EXO_TEST(command_argument_range4_10, { return test_range("!test13 10.18.1.100/30",                  "10.18.1.100",     "10.18.1.103"); });
EXO_TEST(command_argument_range4_11, { return test_range("!test13 10.18.1.100-10.18.1.103",         "10.18.1.100",     "10.18.1.103"); });
EXO_TEST(command_argument_range4_12, { return test_range("!test13 192.168.0.0/16",                  "192.168.0.0",     "192.168.255.255"); });
EXO_TEST(command_argument_range4_13, { return test_range("!test13 192.168.0.0-192.168.255.255",     "192.168.0.0",     "192.168.255.255"); });

EXO_TEST(command_argument_range6_1,  { return test_range("!test13 ::",                                              "::",                      "::"); });
EXO_TEST(command_argument_range6_2,  { return test_range("!test13 ::/0",                                            "::",                      "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"); });
EXO_TEST(command_argument_range6_3,  { return test_range("!test13 ::/32",                                           "::",                      "::ffff:ffff:ffff:ffff:ffff:ffff"); });
EXO_TEST(command_argument_range6_4,  { return test_range("!test13 ::/128",                                          "::",                      "::"); });
EXO_TEST(command_argument_range6_5,  { return test_range("!test13 0:0:0:0:0:0:0:0",                                 "::",                      "::"); });
EXO_TEST(command_argument_range6_6,  { return test_range("!test13 0:0:0:0:0:0:0:0-0:0:0:0:0:0:0:ffff",              "::",                      "::ffff"); });
EXO_TEST(command_argument_range6_7,  { return test_range("!test13 0:0:0:0:0:0:0:0/112",                             "::",                      "::ffff"); });
EXO_TEST(command_argument_range6_8,  { return test_range("!test13 ::-::ffff",                                       "::",                      "::ffff"); });
EXO_TEST(command_argument_range6_9,  { return test_range("!test13 ::-ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",      "::",                      "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"); });
EXO_TEST(command_argument_range6_10, { return test_range("!test13 ::1",                                             "::1",                     "::1"); });
EXO_TEST(command_argument_range6_11, { return test_range("!test13 ::1/128",                                         "::1",                     "::1"); });
EXO_TEST(command_argument_range6_12, { return test_range("!test13 ::ffff:0.0.0.0/96",                               "::ffff:0.0.0.0",          "::ffff:255.255.255.255"); });
EXO_TEST(command_argument_range6_13, { return test_range("!test13 ::ffff:0.0.0.0-::ffff:255.255.255.255",           "::ffff:0.0.0.0",          "::ffff:255.255.255.255"); });
EXO_TEST(command_argument_range6_14, { return test_range("!test13 ::ffff:0.0.0.0/97",                               "::ffff:0.0.0.0",          "::ffff:127.255.255.255"); });
EXO_TEST(command_argument_range6_15, { return test_range("!test13 ::ffff:0.0.0.0-::ffff:127.255.255.255",           "::ffff:0.0.0.0",          "::ffff:127.255.255.255"); });
EXO_TEST(command_argument_range6_16, { return test_range("!test13 ::ffff:127.0.0.1",                                "::ffff:127.0.0.1",        "::ffff:127.0.0.1"); });
EXO_TEST(command_argument_range6_17, { return test_range("!test13 ::ffff:255.255.255.255/128",                      "::ffff:255.255.255.255",  "::ffff:255.255.255.255"); });
EXO_TEST(command_argument_range6_18, { return test_range("!test13 2001::",                                          "2001::",                  "2001::"); });
EXO_TEST(command_argument_range6_19, { return test_range("!test13 2001::/16",                                       "2001::",                  "2001:ffff:ffff:ffff:ffff:ffff:ffff:ffff"); });
EXO_TEST(command_argument_range6_20, { return test_range("!test13 2001::-2001:ffff:ffff:ffff:ffff:ffff:ffff:ffff",  "2001::",                  "2001:ffff:ffff:ffff:ffff:ffff:ffff:ffff"); });
EXO_TEST(command_argument_range6_21, { return test_range("!test13 2001::201:2ff:fefa:fff4/126",                     "2001::201:2ff:fefa:fff4", "2001::201:2ff:fefa:fff7"); });
EXO_TEST(command_argument_range6_22, { return test_range("!test13 2001::201:2ff:fefa:fff4-2001::201:2ff:fefa:fff7", "2001::201:2ff:fefa:fff4", "2001::201:2ff:fefa:fff7"); });
EXO_TEST(command_argument_range6_23, { return test_range("!test13 ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",         "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"); });
EXO_TEST(command_argument_range6_24, { return test_range("!test13 ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/128",     "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"); });

#undef SETUP_COMMAND

EXO_TEST(command_user_destroy, { return uman_remove(hub->users, &user) == 0; });

EXO_TEST(command_destroy, {

	command_free(cmd);
	cmd = NULL;

	DEL_TEST(c_test1);
	DEL_TEST(c_test2);
	DEL_TEST(c_test3);
	DEL_TEST(c_test4);
	DEL_TEST(c_test5);
	DEL_TEST(c_test6);
	DEL_TEST(c_test7);
	DEL_TEST(c_test8);
	DEL_TEST(c_test9);
	DEL_TEST(c_test10);
	DEL_TEST(c_test11);
	DEL_TEST(c_test12);
	DEL_TEST(c_test13);
	return 1;
});

EXO_TEST(cleanup, {
	uman_shutdown(hub->users);
	command_shutdown(hub->commands);
	hub_free(hub);
	return 1;
});

EXO_TEST(cmd_shutdown_network, {
	return net_destroy() == 0;
});
