#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include <check.h>
#include <vde3.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_VALGRIND_VALGRIND_H
#include <valgrind/valgrind.h>
#define V_START_TEST(n) START_TEST(n) VALGRIND_PRINTF("starting test "#n"\n");
#else
#define V_START_TEST(n) START_TEST(n)
#endif

// fixture components, always present
vde_context *f_ctx;
vde_event_handler f_eh = {(void *)0x1, (void *)0x1, (void *)0x1, (void *)0x1};

void
setup (void)
{
  vde_context_new(&f_ctx);
  vde_context_init(f_ctx, &f_eh, NULL);
}

void
teardown (void)
{
  vde_context_fini(f_ctx);
  vde_context_delete(f_ctx);
}


V_START_TEST (test_context_new)
{
  int rv;
  vde_context *ctx;

  rv = vde_context_new(&ctx);
  fail_unless (rv == 0, "fail on valid ctx");
  vde_context_delete(ctx);
}
END_TEST

V_START_TEST (test_context_new_invalid)
{
  int rv;

  rv = vde_context_new(NULL);
  fail_unless (rv == -1, "success on ctx == NULL");
}
END_TEST

// XXX test module loading
V_START_TEST (test_context_init)
{
  int rv;
  vde_event_handler eh = {(void *)0x1, (void *)0x1, (void *)0x1, (void *)0x1};
  vde_context *ctx;

  vde_context_new(&ctx);
  rv = vde_context_init(ctx, &eh, NULL);
  fail_unless (rv == 0, "fail on valid arguments");
}
END_TEST

V_START_TEST (test_context_init_invalid)
{
  int rv;
  vde_event_handler eh;
  vde_context *ctx;
  char *mpath[] = {"nonexistantpath", NULL};

  rv = vde_context_init(NULL, &eh, NULL);
  fail_unless (rv == -1, "success on null ctx");

  rv = vde_context_init(ctx, NULL, NULL);
  fail_unless (rv == -1, "success on null eh");

  rv = vde_context_init(ctx, NULL, mpath);
  fail_unless (rv == -1, "success on invalid module path");
}
END_TEST

V_START_TEST (test_component_new)
{
  int rv;
  vde_event_handler eh;
  vde_component *comp;

  rv = vde_context_new_component(f_ctx, VDE_ENGINE, "hub", "test_e", &comp,
                                 NULL);
  fail_unless(rv == 0, "new fails on valid arguments %s", strerror(errno));

  rv = vde_context_component_del(f_ctx, comp);
  fail_unless(rv == 0, "del fails on valid arguments %s", strerror(errno));
}
END_TEST

V_START_TEST (test_component_new_invalid)
{
  int rv;
  vde_event_handler eh;
  vde_component *comp;

  rv = vde_context_new_component(f_ctx, VDE_ENGINE,
                                 "thisisnotsupposedtoexists", "test", &comp,
                                 NULL);
  fail_unless(rv == -1 && errno == ENOENT, "success on unknown family %s",
              strerror(errno));


  vde_context_new_component(f_ctx, VDE_ENGINE, "hub", "testname", &comp,
                            NULL);
  rv = vde_context_new_component(f_ctx, VDE_ENGINE, "hub", "testname", &comp,
                                 NULL);
  fail_unless(rv == -1 && errno == EEXIST,
              "success on double new component %s", strerror(errno));
}
END_TEST

V_START_TEST (test_component_get)
{
  int rv;
  vde_event_handler eh;
  vde_component *comp;

  vde_context_new_component(f_ctx, VDE_ENGINE, "hub", "test_e", &comp, NULL);

  fail_unless(vde_context_get_component(f_ctx, "test_e") == comp,
              "fail to return component");

  fail_unless(vde_context_get_component(f_ctx, "") == NULL,
              "fail to return NULL on empty name");
}
END_TEST

V_START_TEST (test_component_del)
{
  int rv;
  vde_event_handler eh;
  vde_component *comp;

  vde_context_new_component(f_ctx, VDE_ENGINE, "hub", "test_e", &comp, NULL);
  rv = vde_context_component_del(f_ctx, comp);
  fail_unless(rv == 0, "del fails on valid arguments %s", strerror(errno));
}
END_TEST

V_START_TEST (test_component_del_invalid)
{
  int rv;
  vde_event_handler eh;
  vde_component *comp;

  rv = vde_context_component_del(f_ctx, NULL);
  fail_unless(rv == -1 && errno == EINVAL, "success on NULL component %s",
              strerror(errno));

  vde_context_new_component(f_ctx, VDE_ENGINE, "hub", "test_e", &comp, NULL);
  vde_context_component_del(f_ctx, comp);
  rv = vde_context_component_del(f_ctx, comp);
  fail_unless(rv == -1 && errno == ENOENT, "success on double del %s",
              strerror(errno));
}
END_TEST

Suite *
context_suite (void)
{
  Suite *s = suite_create ("context");

  /* Core test case */
  TCase *tc_core = tcase_create ("Core");
  tcase_add_checked_fixture (tc_core, setup, teardown);
  tcase_add_test (tc_core, test_context_new);
  tcase_add_test (tc_core, test_context_new_invalid);
  tcase_add_test (tc_core, test_context_init);
  tcase_add_test (tc_core, test_context_init_invalid);
  suite_add_tcase (s, tc_core);

  /* Component test case */
  TCase *tc_component = tcase_create ("Component");
  tcase_add_checked_fixture (tc_component, setup, teardown);
  tcase_add_test (tc_component, test_component_new);
  tcase_add_test (tc_component, test_component_new_invalid);
  tcase_add_test (tc_component, test_component_get);
  tcase_add_test (tc_component, test_component_del);
  tcase_add_test (tc_component, test_component_del_invalid);
  suite_add_tcase (s, tc_component);
  return s;
}

int
main (void)
{
  int number_failed;
  Suite *s = context_suite ();
  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
