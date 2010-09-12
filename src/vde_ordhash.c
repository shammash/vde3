/* Copyright (C) 2010 - Virtualsquare Team
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <stdlib.h>

#include <vde3/vde_ordhash.h>

struct vde_ordhash {
  vde_hash *hash;
  vde_list *list;
};


vde_ordhash *vde_ordhash_new() {
  vde_ordhash *oh = calloc(1, sizeof(vde_ordhash));
  if (oh == NULL) {
    errno = ENOMEM;
    return NULL;
  }
  oh->hash = vde_hash_init();
  /* list init not needed */
  return oh;
}

void vde_ordhash_delete(vde_ordhash *oh) {
  vde_assert(oh != NULL);
  vde_assert(vde_hash_size(oh->hash) == 0);
  vde_assert(vde_list_length(oh->list) == 0);
  vde_hash_delete(oh->hash);
  /* list delete not needed as it's empty (NULL) */
}

void vde_ordhash_insert(vde_ordhash *oh, void *k, void *v) {
  vde_assert(oh != NULL);
  vde_assert(vde_hash_lookup(oh->hash, k) == NULL);
  oh->list = vde_list_append(oh->list, k);
  /*
   * XXX: consider using vde_list_prepend if optimizations are needed, the list
   * will then express an inverse temporal relation (e.g.: vde_ordhash_first()
   * calls vde_list_last(), vde_ordhash_next() calls vde_list_prev())
   */
  vde_hash_insert(oh->hash, k, v);
}

int vde_ordhash_remove(vde_ordhash *oh, void *k) {
  vde_assert(oh != NULL);
  vde_assert(vde_hash_lookup(oh->hash, k) != NULL);
  oh->list = vde_list_remove(oh->list, k);
  return vde_hash_remove(oh->hash, k);
}

void *vde_ordhash_lookup(vde_ordhash *oh, void *k) {
  vde_assert(oh != NULL);
  return vde_hash_lookup(oh->hash, k);
}

vde_ordhash_entry *vde_ordhash_first(vde_ordhash *oh) {
  vde_assert(oh != NULL);
  return (vde_ordhash_entry *)vde_list_first(oh->list);
}

vde_ordhash_entry *vde_ordhash_last(vde_ordhash *oh) {
  vde_assert(oh != NULL);
  return (vde_ordhash_entry *)vde_list_last(oh->list);
}

vde_ordhash_entry *vde_ordhash_next(vde_ordhash_entry *e) {
  return (vde_ordhash_entry *)vde_list_next(e);
}

vde_ordhash_entry *vde_ordhash_prev(vde_ordhash_entry *e) {
  return (vde_ordhash_entry *)vde_list_prev(e);
}

void *vde_ordhash_entry_lookup(vde_ordhash *oh, vde_ordhash_entry *e) {
  void *k, *v;
  vde_assert(oh != NULL);
  vde_assert(e != NULL);
  k = vde_list_get_data((vde_list *)e);
  v = vde_hash_lookup(oh->hash, k);
  vde_assert(v); /* Ensure the given entry has a corresponding value */
  return v;
}

void *vde_ordhash_entry_getkey(vde_ordhash *oh, vde_ordhash_entry *e) {
  void *k;
  vde_assert(oh != NULL);
  vde_assert(e != NULL);
  k = vde_list_get_data((vde_list *)e);
  vde_assert(k); /* Ensure the given entry has a corresponding key */
  return k;
}

void vde_ordhash_remove_all(vde_ordhash *oh) {
  vde_list *head;
  void *k;
  vde_assert(oh != NULL);
  while ((head = vde_list_first(oh->list))){
    k = vde_list_get_data(head);
    vde_ordhash_remove(oh, k);
  }
}

