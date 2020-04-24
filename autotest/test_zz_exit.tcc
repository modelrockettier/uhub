#include <uhub.h>

EXO_TEST(exit_log, {
	hub_log_shutdown();
	return 1;
});
