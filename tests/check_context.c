#include <stdlib.h>
#include <check.h>
#include <vde3.h>

// the fixture context, always present
vde_context *f_ctx;

void
setup (void)
{
  vde_context_new(&f_ctx);
}

void
teardown (void)
{
  vde_context_delete(f_ctx);
}


START_TEST (test_context_new)
{
  int rv;
  vde_context *ctx;

  rv = vde_context_new(&ctx);
  fail_unless (rv == 0, "context_new failed");
}
END_TEST

START_TEST (test_context_new_null)
{
  int rv;

  rv = vde_context_new(NULL);
  fail_unless (rv == -1, "context_new(NULL) didn't fail");
}
END_TEST

START_TEST (test_context_init)
{
  int rv;
  vde_event_handler eh;

  rv = vde_context_init(f_ctx, &eh);
  fail_unless (rv == 0, "context_init failed");
}
END_TEST

START_TEST (test_context_init_null)
{
  int rv;
  vde_event_handler eh;

  rv = vde_context_init(NULL, &eh);
  fail_unless (rv == -1, "context_init(NULL) didn't fail");

  rv = vde_context_init(f_ctx, NULL);
  fail_unless (rv == -1, "context_init(NULL) didn't fail");
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
  tcase_add_test (tc_core, test_context_new_null);
  tcase_add_test (tc_core, test_context_init);
  tcase_add_test (tc_core, test_context_init_null);
  suite_add_tcase (s, tc_core);

  /* Limits test case */
  /*
  TCase *tc_limits = tcase_create ("Limits");
  tcase_add_test (tc_limits, test_money_create_neg);
  tcase_add_test (tc_limits, test_money_create_zero);
  suite_add_tcase (s, tc_limits);
  */
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
