#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include <check.h>
#include <vde3/vde_ordhash.h>

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
vde_ordhash *f_oh;

void
setup (void)
{
  f_oh = vde_ordhash_new();
}

void
teardown (void)
{
  vde_ordhash_delete(f_oh);
}


V_START_TEST (test_ordhash_insert_lookup_remove)
{
  void *fk = 1, *fv = 2, *rfv = NULL;
  void *sk = 3, *sv = 4, *rsv = NULL;
  int rv = 0;

  vde_ordhash_insert(f_oh, fk, fv);
  rfv = vde_ordhash_lookup(f_oh, fk);
  fail_unless (rfv == fv, "could not lookup first value");

  vde_ordhash_insert(f_oh, sk, sv);
  rsv = vde_ordhash_lookup(f_oh, sk);
  fail_unless (rsv == sv, "could not lookup second value");

  rv = vde_ordhash_remove(f_oh, fk);
  fail_unless (rv != 0, "remove first item returned zero");

  rv = vde_ordhash_remove(f_oh, sk);
  fail_unless (rv != 0, "remove second item returned zero");

}
END_TEST

Suite *
vde_ordhash_suite (void)
{
  Suite *s = suite_create ("vde_ordhash");

  /* Core test case */
  TCase *tc_core = tcase_create ("Core");
  tcase_add_checked_fixture (tc_core, setup, teardown);
  tcase_add_test (tc_core, test_ordhash_insert_lookup_remove);
  suite_add_tcase (s, tc_core);

  /* Component test case */
  TCase *tc_iterators = tcase_create ("Iterators");
  tcase_add_checked_fixture (tc_iterators, setup, teardown);
  suite_add_tcase (s, tc_iterators);
  return s;
}

int
main (void)
{
  int number_failed;
  Suite *s = vde_ordhash_suite ();
  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
