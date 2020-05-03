#include <uhub.h>

static struct flood_control flood;

EXO_TEST(flood_reset, { flood_control_reset(&flood); return 1; });

#define FCHECK(max_count, time_delay, now) flood_control_check(&flood, max_count, time_delay, now)

/* max_count == 0 */
EXO_TEST(flood_check_1, { return !FCHECK(0, 1, 2); });
/* time_delay == 0 */
EXO_TEST(flood_check_2, { return !FCHECK(1, 0, 2); });
/* data->time == 0 */
EXO_TEST(flood_check_3, { return !FCHECK(1, 2, 3); });

EXO_TEST(flood_set_time, { flood.time = 1; return 1; });

/* (now - data->time) > time_delay */
EXO_TEST(flood_check_4, { return !FCHECK(1, 2, 4); });
/* data->count < max_count */
EXO_TEST(flood_check_5, { return !FCHECK(3, 2, 2); });
/* data->count >= max_count */
EXO_TEST(flood_check_6, { return FCHECK(3, 2, 2); });
EXO_TEST(flood_check_7, { return FCHECK(3, 2, 2); });
EXO_TEST(flood_check_8, { return FCHECK(3, 2, 2); });

