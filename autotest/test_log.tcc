#include <uhub.h>

#define LOG_VERB_TO_STRING(VERB, EXPECT) str_match(hub_log_verbosity_to_string(VERB), EXPECT)
EXO_TEST(log_verb_to_string_1,  { return LOG_VERB_TO_STRING(log_fatal,    "fatal");    });
EXO_TEST(log_verb_to_string_2,  { return LOG_VERB_TO_STRING(log_error,    "error");    });
EXO_TEST(log_verb_to_string_3,  { return LOG_VERB_TO_STRING(log_warning,  "warning");  });
EXO_TEST(log_verb_to_string_4,  { return LOG_VERB_TO_STRING(log_user,     "user");     });
EXO_TEST(log_verb_to_string_5,  { return LOG_VERB_TO_STRING(log_info,     "info");     });
EXO_TEST(log_verb_to_string_6,  { return LOG_VERB_TO_STRING(log_debug,    "debug");    });
EXO_TEST(log_verb_to_string_7,  { return LOG_VERB_TO_STRING(log_trace,    "trace");    });
EXO_TEST(log_verb_to_string_8,  { return LOG_VERB_TO_STRING(log_dump,     "dump");     });
EXO_TEST(log_verb_to_string_9,  { return LOG_VERB_TO_STRING(log_memory,   "memory");   });
EXO_TEST(log_verb_to_string_10, { return LOG_VERB_TO_STRING(log_protocol, "protocol"); });
EXO_TEST(log_verb_to_string_11, { return LOG_VERB_TO_STRING(log_plugin,   "plugin");   });
EXO_TEST(log_verb_to_string_12, { return LOG_VERB_TO_STRING(-1,           "unknown");  });
EXO_TEST(log_verb_to_string_13, { return LOG_VERB_TO_STRING(19,           "unknown");  });

EXO_TEST(log_verb_from_str_1,  { return hub_log_string_to_verbosity("fatal")    == log_fatal;    });
EXO_TEST(log_verb_from_str_2,  { return hub_log_string_to_verbosity("error")    == log_error;    });
EXO_TEST(log_verb_from_str_3,  { return hub_log_string_to_verbosity("warning")  == log_warning;  });
EXO_TEST(log_verb_from_str_4,  { return hub_log_string_to_verbosity("user")     == log_user;     });
EXO_TEST(log_verb_from_str_5,  { return hub_log_string_to_verbosity("info")     == log_info;     });
EXO_TEST(log_verb_from_str_6,  { return hub_log_string_to_verbosity("debug")    == log_debug;    });
EXO_TEST(log_verb_from_str_7,  { return hub_log_string_to_verbosity("trace")    == log_trace;    });
EXO_TEST(log_verb_from_str_8,  { return hub_log_string_to_verbosity("dump")     == log_dump;     });
EXO_TEST(log_verb_from_str_9,  { return hub_log_string_to_verbosity("memory")   == log_memory;   });
EXO_TEST(log_verb_from_str_10, { return hub_log_string_to_verbosity("protocol") == log_protocol; });
EXO_TEST(log_verb_from_str_11, { return hub_log_string_to_verbosity("plugin")   == log_plugin;   });
EXO_TEST(log_verb_from_str_12, { return hub_log_string_to_verbosity("PLUGIN")   == log_plugin;   });
EXO_TEST(log_verb_from_str_13, { return hub_log_string_to_verbosity("Plugin")   == log_plugin;   });
EXO_TEST(log_verb_from_str_14, { return hub_log_string_to_verbosity("pLuGiN")   == log_plugin;   });

EXO_TEST(log_verb_from_int_1,  { return hub_log_string_to_verbosity("0")  == log_fatal;    });
EXO_TEST(log_verb_from_int_2,  { return hub_log_string_to_verbosity("1")  == log_error;    });
EXO_TEST(log_verb_from_int_3,  { return hub_log_string_to_verbosity("2")  == log_warning;  });
EXO_TEST(log_verb_from_int_4,  { return hub_log_string_to_verbosity("3")  == log_user;     });
EXO_TEST(log_verb_from_int_5,  { return hub_log_string_to_verbosity("4")  == log_info;     });
EXO_TEST(log_verb_from_int_6,  { return hub_log_string_to_verbosity("5")  == log_debug;    });
EXO_TEST(log_verb_from_int_7,  { return hub_log_string_to_verbosity("6")  == log_trace;    });
EXO_TEST(log_verb_from_int_8,  { return hub_log_string_to_verbosity("7")  == log_dump;     });
EXO_TEST(log_verb_from_int_9,  { return hub_log_string_to_verbosity("8")  == log_memory;   });
EXO_TEST(log_verb_from_int_10, { return hub_log_string_to_verbosity("9")  == log_protocol; });
EXO_TEST(log_verb_from_int_11, { return hub_log_string_to_verbosity("10") == log_plugin;   });

EXO_TEST(log_verb_from_bad_str, { return hub_log_string_to_verbosity("unknown") == -1; });
