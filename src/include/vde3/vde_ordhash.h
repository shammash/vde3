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

/**
 * @file
 */

#ifndef __VDE_ORDHASH_H__
#define __VDE_ORDHASH_H__

#include <vde3/common.h>

/**
 * @brief VDE 3 ordered hash
 *
 * This data structure can be used as a normal hash table, but it also keep
 * track of elements' insertion time. The provided iterator is guaranteed to
 * start from the oldest element and end to the most recent one.
 *
 */
typedef struct vde_ordhash vde_ordhash;

/**
 * @brief VDE 3 ordered hash entry
 *
 * An element handled by ordhash iterator.
 *
 */
typedef vde_list vde_ordhash_entry;


/**
 * @brief Alloc a new ordered hash
 *
 * @return an ordhash on succes, NULL on error (and errno is set appropriately)
 */
vde_ordhash *vde_ordhash_new();

/**
 * @brief Deallocate an empty ordered hash
 *
 * @param oh The ordhash to delete
 */
void vde_ordhash_delete(vde_ordhash *oh);

/**
 * @brief Add an element in the ordhash
 *
 * @param oh The ordhash to insert the element into
 * @param k The key used for looking up the element (must not be present)
 * @param v The value to insert
 */
void vde_ordhash_insert(vde_ordhash *oh, void *k, void *v);

/**
 * @brief Remove an element from the ordhash
 *
 * @param oh The ordhash to remove the element from
 * @param k The element key
 *
 * @return Nonzero if the element has been removed successfully
 */
int vde_ordhash_remove(vde_ordhash *oh, void *k);

/**
 * @brief Lookup an element in the ordhash by its key
 *
 * @param oh The ordhash to lookup the element into
 * @param k The element key
 *
 * @return The element if success, NULL if no element has been found
 */
void *vde_ordhash_lookup(vde_ordhash *oh, void *k);

/**
 * @brief Get an ordhash iterator referring to the oldest element
 *
 * @param oh The ordhash to get an iterator from
 *
 * @return The iterator or NULL if the ordhash is empty
 */
vde_ordhash_entry *vde_ordhash_first(vde_ordhash *oh);

/**
 * @brief Get an ordhash iterator referring to the youngest element
 *
 * @param oh The ordhash to get an iterator from
 *
 * @return The iterator or NULL if the ordhash is empty
 */
vde_ordhash_entry *vde_ordhash_last(vde_ordhash *oh);

/**
 * @brief Get a reference to the element added immediately after this entry
 *
 * @param e The entry
 *
 * @return The new entry or NULL if the entry was the last one
 */
vde_ordhash_entry *vde_ordhash_next(vde_ordhash_entry *e);

/**
 * @brief Get a reference to the element added immediately before this entry
 *
 * @param e The entry
 *
 * @return The new entry or NULL if the entry was the first one
 */
vde_ordhash_entry *vde_ordhash_prev(vde_ordhash_entry *e);

/**
 * @brief Lookup an element in the ordhash by its entry
 *
 * @param oh The ordhash to lookup the element into
 * @param e The entry
 *
 * @return The element if success, NULL if no element has been found
 */
void *vde_ordhash_lookup_entry(vde_ordhash *oh, vde_ordhash_entry *e);

#endif /* __VDE_ORDHASH_H__ */
