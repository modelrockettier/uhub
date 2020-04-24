#include <uhub.h>

#define CRED_TO_STRING(CRED, EXPECT) return str_match(auth_cred_to_string((enum auth_credentials)CRED), EXPECT);

EXO_TEST(cred_to_string_1,  { CRED_TO_STRING(auth_cred_none,     "none");     });
EXO_TEST(cred_to_string_2,  { CRED_TO_STRING(auth_cred_bot,      "bot");      });
EXO_TEST(cred_to_string_3,  { CRED_TO_STRING(auth_cred_ubot,     "ubot");     });
EXO_TEST(cred_to_string_4,  { CRED_TO_STRING(auth_cred_opbot,    "opbot");    });
EXO_TEST(cred_to_string_5,  { CRED_TO_STRING(auth_cred_opubot,   "opubot");   });
EXO_TEST(cred_to_string_6,  { CRED_TO_STRING(auth_cred_guest,    "guest");    });
EXO_TEST(cred_to_string_7,  { CRED_TO_STRING(auth_cred_user,     "user");     });
EXO_TEST(cred_to_string_8,  { CRED_TO_STRING(auth_cred_operator, "operator"); });
EXO_TEST(cred_to_string_9,  { CRED_TO_STRING(auth_cred_super,    "super");    });
EXO_TEST(cred_to_string_10, { CRED_TO_STRING(auth_cred_link,     "link");     });
EXO_TEST(cred_to_string_11, { CRED_TO_STRING(auth_cred_admin,    "admin");    });
EXO_TEST(cred_to_string_12, { CRED_TO_STRING(-1,                 "unknown");  });
EXO_TEST(cred_to_string_13, { CRED_TO_STRING(49,                 "unknown");  });

#define CRED_FROM_STRING(STR, EXPECT) enum auth_credentials cred; return auth_string_to_cred(STR, &cred) && cred == EXPECT;
#define BAD_CRED_FROM_STRING(STR) enum auth_credentials cred = auth_cred_super; return !auth_string_to_cred(STR, &cred) && cred == auth_cred_super;

EXO_TEST(cred_from_string_1,  { CRED_FROM_STRING("none",     auth_cred_none);     });
EXO_TEST(cred_from_string_2,  { CRED_FROM_STRING("bot",      auth_cred_bot);      });
EXO_TEST(cred_from_string_3,  { CRED_FROM_STRING("ubot",     auth_cred_ubot);     });
EXO_TEST(cred_from_string_4,  { CRED_FROM_STRING("opbot",    auth_cred_opbot);    });
EXO_TEST(cred_from_string_5,  { CRED_FROM_STRING("opubot",   auth_cred_opubot);   });
EXO_TEST(cred_from_string_6,  { CRED_FROM_STRING("guest",    auth_cred_guest);    });
EXO_TEST(cred_from_string_7,  { CRED_FROM_STRING("user",     auth_cred_user);     });
EXO_TEST(cred_from_string_8,  { CRED_FROM_STRING("reg",      auth_cred_user);     });
EXO_TEST(cred_from_string_9,  { CRED_FROM_STRING("operator", auth_cred_operator); });
EXO_TEST(cred_from_string_10, { CRED_FROM_STRING("op",       auth_cred_operator); });
EXO_TEST(cred_from_string_11, { CRED_FROM_STRING("super",    auth_cred_super);    });
EXO_TEST(cred_from_string_12, { CRED_FROM_STRING("link",     auth_cred_link);     });
EXO_TEST(cred_from_string_13, { CRED_FROM_STRING("admin",    auth_cred_admin);    });
EXO_TEST(cred_from_string_14, { CRED_FROM_STRING("ADMIN",    auth_cred_admin);    });
EXO_TEST(cred_from_string_15, { CRED_FROM_STRING("aDmIn",    auth_cred_admin);    });
EXO_TEST(cred_from_string_16, { CRED_FROM_STRING("AdMiN",    auth_cred_admin);    });

EXO_TEST(bad_cred_from_string_1,  { return !auth_string_to_cred("admin", NULL); });
EXO_TEST(bad_cred_from_string_2,  { BAD_CRED_FROM_STRING(NULL);                 });
EXO_TEST(bad_cred_from_string_3,  { BAD_CRED_FROM_STRING("");                   });
EXO_TEST(bad_cred_from_string_4,  { BAD_CRED_FROM_STRING("0");                  });
EXO_TEST(bad_cred_from_string_5,  { BAD_CRED_FROM_STRING("no");                 });
EXO_TEST(bad_cred_from_string_6,  { BAD_CRED_FROM_STRING("opp");                });
EXO_TEST(bad_cred_from_string_7,  { BAD_CRED_FROM_STRING("nope");               });
EXO_TEST(bad_cred_from_string_8,  { BAD_CRED_FROM_STRING("extra");              });
EXO_TEST(bad_cred_from_string_9,  { BAD_CRED_FROM_STRING("SUPERS");             });
EXO_TEST(bad_cred_from_string_10, { BAD_CRED_FROM_STRING("OPSERVER");           });

