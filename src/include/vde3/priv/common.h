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

#ifndef __VDE3_PRIV_COMMON_H__
#define __VDE3_PRIV_COMMON_H__

#include <glib.h>

#define vde_alloc(s) g_slice_alloc(s)
#define vde_calloc(s) g_slice_alloc0(s)
#define vde_free(t, d) g_slice_free(t, d)

typedef GList vde_list;
#define vde_list_first(list) g_list_first(list)
#define vde_list_next(list) g_list_next(list)
#define vde_list_get_data(list) g_list_nth_data(list, 0)
#define vde_list_prepend(list, data) g_list_prepend(list, data)
#define vde_list_remove(list, data) g_list_remove(list, data)
#define vde_list_delete(list) g_list_free(list)

typedef GHashTable vde_hash;
#define vde_hash_init() g_hash_table_new(NULL, NULL)
#define vde_hash_insert(h, k, v) g_hash_table_insert(h, k, v)
#define vde_hash_remove(h, k) g_hash_table_remove(h, k)
#define vde_hash_lookup(h, k) g_hash_table_lookup(h, k)
#define vde_hash_delete(h) g_hash_table_destroy(h)

typedef GQuark vde_quark;
#define vde_comparable_string

enum vde_component_kind {
  VDE_ENGINE,
  VDE_TRANSPORT,
  VDE_CONNECTION_MANAGER
};

#endif /* __VDE3_PRIV_COMMON_H__ */
