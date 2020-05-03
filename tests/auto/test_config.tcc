#include <uhub.h>

static struct hub_config config1;
static struct hub_config config2;
static struct hub_config config3;

EXO_TEST(zero_configs, {
	memset(&config1, 0, sizeof(struct hub_config));
	memset(&config2, 0, sizeof(struct hub_config));
	memset(&config3, 0, sizeof(struct hub_config));
	return 1;
});

EXO_TEST(default_config_1, {
	config_defaults(&config1);
	return memcmp(&config1, &config2, sizeof(struct hub_config)) != 0;
});

EXO_TEST(apply_config_bool_1, {
	config1.hub_enabled = 2;
	return !apply_config(&config1, "hub_enabled", "1", 101) && config1.hub_enabled == 1;
});

EXO_TEST(apply_config_bool_2, {
	config1.hub_enabled = 2;
	return !apply_config(&config1, "hub_enabled", "0", 101) && !config1.hub_enabled;
});

EXO_TEST(apply_config_bool_3, {
	config1.hub_enabled = 2;
	return apply_config(&config1, "hub_enabled", "3", 102) && config1.hub_enabled == 2;
});

EXO_TEST(apply_config_int_1, {
	/* Save the current value, then change it */
	int port = config1.server_port;
	return !apply_config(&config1, "server_port", uhub_itoa(port), 201) && config1.server_port == port;
});

EXO_TEST(apply_config_int_2, {
	return !apply_config(&config1, "server_port", "1", 202) && config1.server_port == 1;
});

EXO_TEST(apply_config_int_3, {
	return !apply_config(&config1, "server_port", "65535", 203) && config1.server_port == 65535;
});

EXO_TEST(apply_config_int_4, {
	return !apply_config(&config1, "server_port", "1511", 204) && config1.server_port == 1511;
});

EXO_TEST(apply_config_int_5, {
	int port = config1.server_port;
	return apply_config(&config1, "server_port", "", 205) && config1.server_port == port;
});

EXO_TEST(apply_config_int_6, {
	int port = config1.server_port;
	return apply_config(&config1, "server_port", "q123", 206) && config1.server_port == port;
});

EXO_TEST(apply_config_int_7, {
	int port = config1.server_port;
	return apply_config(&config1, "server_port", "123456789012345678901234567890", 207) && config1.server_port == port;
});

EXO_TEST(apply_config_int_8, {
	int port = config1.server_port;
	return apply_config(&config1, "server_port", "-123456789012345678901234567890", 208) && config1.server_port == port;
});

EXO_TEST(apply_config_int_9, {
	int port = config1.server_port;
	return apply_config(&config1, "server_port", "0", 209) && config1.server_port == port;
});

EXO_TEST(apply_config_int_10, {
	int port = config1.server_port;
	return apply_config(&config1, "server_port", "-1", 210) && config1.server_port == port;
});

EXO_TEST(apply_config_int_11, {
	int port = config1.server_port;
	return apply_config(&config1, "server_port", "65536", 211) && config1.server_port == port;
});

EXO_TEST(apply_config_str_1, {
	const char *alt = "127.0.0.1";
	return !apply_config(&config1, "server_bind_addr", alt, 301) && str_match(alt, config1.server_bind_addr);
});

EXO_TEST(apply_config_str_2, {
	const char *alt = "";
	return !apply_config(&config1, "server_bind_addr", alt, 302) && str_match(alt, config1.server_bind_addr);
});

EXO_TEST(prepare_config_1, {
	return
		!apply_config(&config1, "hub_enabled", "0", 401) &&
		!apply_config(&config1, "server_port", "1234", 402) &&
		!apply_config(&config1, "server_bind_addr", "127.0.0.2", 403);
});

/* Changed settings */
EXO_TEST(check_config_1_1, {
	return
		config1.hub_enabled == 0 &&
		config1.server_port == 1234 &&
		str_match(config1.server_bind_addr, "127.0.0.2");
});

/* Default settings */
EXO_TEST(check_config_1_2, {
	return
		config1.show_banner == 1 &&
		config1.max_users == 500 &&
		str_match(config1.hub_name, "uhub");
});

EXO_TEST(dump_config, {
	FILE *cf = fopen("dump.conf", "w");
	if (!cf)
		return 0;

	dump_config(&config1, cf, 0);

	int ok = (ftell(cf) != 0);
	fclose(cf);

	return ok;
});

EXO_TEST(dump_config_no_defaults, {
	FILE *cf = fopen("dump2.conf", "w");
	if (!cf)
		return 0;

	dump_config(&config1, cf, 1);

	int ok = (ftell(cf) != 0);
	fclose(cf);

	return ok;
});

EXO_TEST(read_config, {
	return !read_config("dump.conf", &config2, 0);
});

EXO_TEST(read_config_no_defaults, {
	return !read_config("dump2.conf", &config3, 0);
});

/* Changed settings */
EXO_TEST(check_config_2_1, {
	return
		config2.hub_enabled == 0 &&
		config2.server_port == 1234 &&
		str_match(config2.server_bind_addr, "127.0.0.2");
});

/* Default settings */
EXO_TEST(check_config_2_2, {
	return
		config2.show_banner == 1 &&
		config2.max_users == 500 &&
		str_match(config2.hub_name, "uhub");
});

EXO_TEST(check_config_3_1, {
	return
		config3.hub_enabled == 0 &&
		config3.server_port == 1234 &&
		str_match(config3.server_bind_addr, "127.0.0.2");
});

EXO_TEST(check_config_3_2, {
	return
		config3.show_banner == 1 &&
		config3.max_users == 500 &&
		str_match(config3.hub_name, "uhub");
});

EXO_TEST(remove_config_file, {
	if(remove("dump.conf") == -1)
		LOG_ERROR("Could not delete 'dump.conf'");
	if(remove("dump2.conf") == -1)
		LOG_ERROR("Could not delete 'dump2.conf'");
	return 1;
});

EXO_TEST(free_configs, {
	free_config(&config1);
	free_config(&config2);
	free_config(&config3);
	return 1;
});
