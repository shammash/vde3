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

#include <vde3.h>

#include <vde3/signal.h>

#include <string.h>

typedef struct {
  vde_signal_cb cb;
  vde_signal_destroy_cb destroy_cb;
  void *data;
} signal_cb;

int vde_signal_attach(vde_signal *signal,
                      vde_signal_cb cb,
                      vde_signal_destroy_cb destroy_cb,
                      void *data)
{
  vde_list *iter;
  signal_cb *entry;

  iter = vde_list_first(signal->callbacks);
  while (iter != NULL) {
    entry = (signal_cb *)vde_list_get_data(iter);

    if (entry->cb == cb &&
        entry->destroy_cb == destroy_cb &&
        entry->data == data) {
      vde_warning("%s: callbacks already registered for %s, skipping",
                  __PRETTY_FUNCTION__, signal->name);
      errno = EEXIST;
      return -1;
    }
    iter = vde_list_next(iter);
  }

  entry = vde_calloc(sizeof(signal_cb));
  if (entry == NULL) {
    vde_error("%s: cannot create data for new callback", __PRETTY_FUNCTION__);
    errno = ENOMEM;
    return -1;
  }
  entry->cb = cb;
  entry->destroy_cb = destroy_cb;
  entry->data = data;

  signal->callbacks = vde_list_prepend(signal->callbacks, entry);

  return 0;
}

int vde_signal_detach(vde_signal *signal,
                      vde_signal_cb cb,
                      vde_signal_destroy_cb destroy_cb,
                      void *data)
{
  vde_list *iter;
  signal_cb *entry;

  iter = vde_list_first(signal->callbacks);
  while (iter != NULL) {
    entry = (signal_cb *)vde_list_get_data(iter);

    if (entry->cb == cb &&
        entry->destroy_cb == destroy_cb &&
        entry->data == data) {
      signal->callbacks = vde_list_remove(signal->callbacks, entry);
      vde_free(entry);
      return 0;
    }
    iter = vde_list_next(iter);
  }

  vde_warning("%s: callbacks not found for %s, skipping",
              __PRETTY_FUNCTION__, signal->name);
  errno = ENOENT;
  return -1;
}

void vde_signal_raise(vde_signal *signal, vde_sobj *info,
                      vde_component *component)
{
  vde_list *iter;
  signal_cb *entry;

  iter = vde_list_first(signal->callbacks);
  while (iter != NULL) {
    entry = (signal_cb *)vde_list_get_data(iter);
    entry->cb(component, signal->name, info, entry->data);
    iter = vde_list_next(iter);
  }
}

vde_signal *vde_signal_dup(vde_signal *signal)
{
  vde_signal *new = (vde_signal *)vde_calloc(sizeof(vde_signal));
  if (new == NULL) {
    errno = ENOMEM;
    return NULL;
  }

  memcpy(new, signal, sizeof(vde_signal));
  new->callbacks = NULL; /* no callbacks on new signal */

  return new;
}

void vde_signal_fini(vde_signal *signal, vde_component *component)
{
  vde_list *iter;
  signal_cb *entry;

  iter = vde_list_first(signal->callbacks);
  while (iter != NULL) {
    entry = (signal_cb *)vde_list_get_data(iter);
    entry->destroy_cb(component, signal->name, entry->data);
    vde_free(entry);
    iter = vde_list_next(iter);
  }
  vde_list_delete(signal->callbacks);
}

void vde_signal_delete(vde_signal *signal)
{
  vde_free(signal);
}

