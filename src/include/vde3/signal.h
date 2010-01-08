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

#ifndef __VDE3_SIGNAL_H__
#define __VDE3_SIGNAL_H__

#include <vde3.h>
#include <vde3/command.h>
#include <vde3/common.h>

struct vde_signal {
  char const * const name;
  char const * const description;
  vde_argument const * const args;
  vde_list * callbacks;
};
typedef struct vde_signal vde_signal;

/**
 * @brief Signature of a signal callback
 *
 * @param component The component raising the signal
 * @param signal The signal name
 * @param infos Serialized signal parameters, if NULL the signal is being
 *              destroyed
 * @param data Callback private data
 */
typedef void (*vde_signal_cb)(vde_component *component,
                                        const char *signal,
                                        vde_sobj *infos, void *data);

/**
 * @brief Signature of a signal destroy callback, called when the signal is
 * being removed
 *
 * @param component The component removing the signal
 * @param signal The signal name
 * @param data Callback private data
 */
typedef void (*vde_signal_destroy_cb)(vde_component *component,
                                                const char *signal,
                                                void *data);


/**
 * @brief Attach a callback to signal
 *
 * @param signal The signal
 * @param cb The callback function
 * @param destroy_cb The callback destroy function
 * @param data Callback private data
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_signal_attach(vde_signal *signal,
                      vde_signal_cb cb,
                      vde_signal_destroy_cb destroy_cb,
                      void *data);

/**
 * @brief Detach a callback from signal
 *
 * @param signal The signal
 * @param cb The callback function
 * @param destroy_cb The callback destroy function
 * @param data Callback private data
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_signal_detach(vde_signal *signal,
                      vde_signal_cb cb,
                      vde_signal_destroy_cb destroy_cb,
                      void *data);

/**
 * @brief Raise signal
 *
 * @param signal The signal
 * @param info The information attached to the signal
 * @param component The component raising the signal
 */
void vde_signal_raise(vde_signal *signal, vde_sobj *info,
                      vde_component *component);

/**
 * @brief Duplicate a signal struct. The callbacks list won't be duplicated.
 *
 * @param signal The signal to duplicate
 *
 * @return The duplicated signal, NULL if an error occurs (and errno is set
 * appropriately)
 */
vde_signal *vde_signal_dup(vde_signal *signal);

/**
 * @brief Finalize signal and call destroy_cb on every callback
 *
 * @param signal The signal
 * @param component The component finalizing the signal
 */
void vde_signal_fini(vde_signal *signal, vde_component *component);

/**
 * @brief Release the memory occupied by a vde_signal. The list of callbacks
 * won't be touched so vde_signal_fini() must be used before.
 *
 * @param signal The signal to free
 */
void vde_signal_delete(vde_signal *signal);

static inline const char *vde_signal_get_name(vde_signal *signal)
{
  if (!signal) {
    return NULL;
  }

  return signal->name;
}

// XXX(shammash): Think if it's possible to register signals to be logged on
// vde log system. Probably this can be done with a component which registers
// itself for the signals and then writes the received signals using
// vde-logging system.

#endif /* __VDE3_SIGNAL_H__ */
