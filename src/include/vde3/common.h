/* Copyright (C) 2009 - Virtualsquare Team
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
/**
 * @file
 */

#ifndef __VDE3_COMMON_H__
#define __VDE3_COMMON_H__

#include <json.h>
#include <glib.h>

#include <errno.h>

#ifdef VDE3_DEBUG
#include <assert.h>
#endif

#define vde_cached_alloc(s) g_malloc(s)
#define vde_cached_calloc(s) g_slice_alloc0(s)
#define vde_cached_free_type(t, d) g_free(d)
/*
 * XXX(shammash):
 *  This cannot be easily remapped to a non-sliced allocator, so let's not use
 *  it until we're sure sliced-stuff will give us a good speedup.
#define vde_cached_free_chunk(s, d) g_slice_free1(s, d)
 */

/*
 * NOTE: g_malloc _aborts_ if the underlying malloc fails and
 * returns NULL only if s == 0
 */
#define vde_alloc(s) g_malloc(s)
#define vde_calloc(s) g_malloc0(s)
#define vde_free(s) g_free(s)

typedef GList vde_list;
#define vde_list_first(list) g_list_first(list)
#define vde_list_next(list) g_list_next(list)
#define vde_list_length(list) g_list_length(list)
#define vde_list_get_data(list) g_list_nth_data(list, 0)
#define vde_list_prepend(list, data) g_list_prepend(list, data)
#define vde_list_remove(list, data) g_list_remove(list, data)
#define vde_list_delete(list) g_list_free(list)

typedef GHashTable vde_hash;
#define vde_hash_init() g_hash_table_new(NULL, NULL)
#define vde_hash_insert(h, k, v) g_hash_table_insert(h, (gpointer)k, v)
#define vde_hash_remove(h, k) g_hash_table_remove(h, (gconstpointer)k)
#define vde_hash_lookup(h, k) g_hash_table_lookup(h, (gconstpointer)k)
#define vde_hash_size(h) g_hash_table_size(h)
#define vde_hash_delete(h) g_hash_table_destroy(h)

typedef GQueue vde_queue;
#define vde_queue_init() g_queue_new()
#define vde_queue_delete(q) g_queue_free(q)
#define vde_queue_get_length(q) g_queue_get_length(q)
#define vde_queue_is_empty(q) g_queue_is_empty(q)
#define vde_queue_pop_head(q) g_queue_pop_head(q)
#define vde_queue_pop_tail(q) g_queue_pop_tail(q)
#define vde_queue_push_head(q, data) g_queue_push_head(q, data)
#define vde_queue_push_tail(q, data) g_queue_push_tail(q, data)

typedef GQuark vde_quark;
#define vde_quark_try_string(s) g_quark_try_string(s)
#define vde_quark_from_string(s) g_quark_from_string(s)
#define vde_quark_to_string(q) g_quark_to_string(q)

typedef gchar vde_char;
#define vde_strdup(s) g_strdup(s)
#define vde_strndup(s, n) g_strndup(s, n)

/*
 * Decorating assert instead of defining NDEBUG if we don't want assert because
 * we can add further instructions if needed.
 */
#ifdef VDE3_DEBUG
#define vde_assert(expr) assert(expr)
#else
#define vde_assert(expr)
#endif

#ifdef __GNUC__
#define UNUSED __attribute__ ((unused))
#else
#define UNUSED
#endif

// logical XOR
// use double negation to make it possible to compare truth values
#define XOR(a, b) ((!!a) != (!!b))

// from vde2 port.h
#define ETH_ALEN 6
#define ETH_DATA_LEN 1500
#define ETH_TRAILER_LEN 4

/* a full ethernet 802.3 frame */
struct eth_hdr {
  unsigned char dest[ETH_ALEN];
  unsigned char src[ETH_ALEN];
  unsigned char proto[2];
};

struct eth_frame {
  struct eth_hdr header;
  unsigned char data[ETH_DATA_LEN + ETH_TRAILER_LEN];
};

#endif /* __VDE3_COMMON_H__ */
