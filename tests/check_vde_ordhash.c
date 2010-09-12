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
  void *fk = (void *)1, *fv = (void *)2, *rfv = NULL;
  void *sk = (void *)3, *sv = (void *)4, *rsv = NULL;
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

V_START_TEST (test_ordhash_lookup_after_remove)
{
  void *fk = (void *)1, *fv = (void *)2, *rfv = NULL;
  int rv = 0;

  vde_ordhash_insert(f_oh, fk, fv);

  rfv = vde_ordhash_lookup(f_oh, fk);
  fail_unless (rfv == fv, "could not lookup first value");

  rv = vde_ordhash_remove(f_oh, fk);
  fail_unless (rv != 0, "remove first item returned zero");

  rfv = vde_ordhash_lookup(f_oh, fk);
  fail_unless (rfv == NULL, "element still present after remove");

}
END_TEST

V_START_TEST (test_ordhash_iterator_empty)
{
  vde_ordhash_entry *e;

  e = vde_ordhash_first(f_oh);
  fail_unless (e == NULL, "iterator on empty hash must be NULL");

  e = vde_ordhash_last(f_oh);
  fail_unless (e == NULL, "iterator on empty hash must be NULL");

}
END_TEST

V_START_TEST (test_ordhash_iterator_iterates)
{
  long i;
  void *el;
  vde_ordhash_entry *e;

  // this test aims at testing the ordhash iterator forward and backward by
  // using a known set of keys

#define MAXKEY 51 // must be odd

  // invariant: keys are odd and values are even
  for (i=1; i<=MAXKEY; i+=2) {
    vde_ordhash_insert(f_oh, (void *)i, (void *)(i+1));
  }

  // test youngest->oldest
  i = MAXKEY;
  e = vde_ordhash_last(f_oh);
  while (e != NULL) {
    el = vde_ordhash_entry_lookup(f_oh, e);
    fail_unless (el != NULL, "the element must be present");
    fail_unless (((long)el % 2) == 0, "the element is not even");
    fail_unless ((long)el == (i+1), "the element is not following the index");

    i -= 2;
    e = vde_ordhash_prev(e);
  }
  fail_unless (i==-1, "the iterator has not iterated the whole ordhash");

  // test oldest->youngest
  i = 1;
  e = vde_ordhash_first(f_oh);
  while (e != NULL) {
    el = vde_ordhash_entry_lookup(f_oh, e);
    fail_unless (el != NULL, "the element must be present");
    fail_unless (((long)el % 2) == 0, "the element is not even");
    fail_unless ((long)el == (i+1), "the element is not following the index");

    i += 2;
    e = vde_ordhash_next(e);
  }
  fail_unless (i==MAXKEY+2, "the iterator has not iterated the whole ordhash");

  // empty the hash otherwise "teardown" fails
  vde_ordhash_remove_all(f_oh);
}
END_TEST

Suite *
vde_ordhash_suite (void)
{
  Suite *s = suite_create ("vde_ordhash");

  /* Core test case */
  // XXX test NULL value set/get
  TCase *tc_core = tcase_create ("Core");
  tcase_add_checked_fixture (tc_core, setup, teardown);
  tcase_add_test (tc_core, test_ordhash_insert_lookup_remove);
  tcase_add_test (tc_core, test_ordhash_lookup_after_remove);
  suite_add_tcase (s, tc_core);

  /* Component test case */
  TCase *tc_iterators = tcase_create ("Iterators");
  tcase_add_checked_fixture (tc_iterators, setup, teardown);
  tcase_add_test (tc_iterators, test_ordhash_iterator_empty);
  tcase_add_test (tc_iterators, test_ordhash_iterator_iterates);
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
