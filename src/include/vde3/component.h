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

#ifndef __VDE3_COMPONENT_H__
#define __VDE3_COMPONENT_H__

#include <vde3/command.h>
#include <vde3/common.h>
#include <vde3/connection.h>
#include <vde3/module.h>
#include <vde3/signal.h>

/**
 * @brief Alloc a new VDE 3 component
 *
 * @param component reference to new component pointer
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_component_new(vde_component **component);

/**
 * @brief Init a VDE 3 component
 *
 * @param component The component to init
 * @param quark The quark representing component name
 * @param module The module which will implement the component functionalities
 * @param ctx The context in which the component will run
 * @param args Parameters for initialization of module properties
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_component_init(vde_component *component, vde_quark qname,
                       vde_module *module, vde_context *ctx, va_list args);

/**
 * @brief Stop and reset a VDE 3 component
 *
 * @param component The component to fini
 */
void vde_component_fini(vde_component *component);

/**
 * @brief Deallocate a VDE 3 component
 *
 * @param component The component to delete
 */
void vde_component_delete(vde_component *component);

/**
 * @brief Increase reference counter
 *
 * @param component The component
 * @param count The pointer where to store reference counter value (might be
 * NULL)
 */
void vde_component_get(vde_component *component, int *count);

/**
 * @brief Decrease reference counter
 *
 * @param component The component
 * @param count The pointer where to store reference counter value (might be
 * NULL)
 */
void vde_component_put(vde_component *component, int *count);

/**
 * @brief Decrease reference counter if there's only one reference
 *
 * @param component The component
 * @param count The pointer where to store reference counter value (might be
 * NULL)
 *
 * @return zero if there was only one reference, 1 otherwise
 */
int vde_component_put_if_last(vde_component *component, int *count);

/**
 * @brief Get component private data
 *
 * @param component The component to get private data from
 *
 * @return The private data
 */
void *vde_component_get_priv(vde_component *component);

/**
 * @brief Set component private data
 *
 * @param component The component to set private data to
 * @param priv The private data
 */
void vde_component_set_priv(vde_component *component, void *priv);

/**
 * @brief Retrieve the context of a component
 *
 * @param component The component to get context from
 *
 * @return The context
 */
vde_context *vde_component_get_context(vde_component *component);

/**
 * @brief Retrieve the component kind
 *
 * @param component The component to get the kind from
 *
 * @return The component kind
 */
vde_component_kind vde_component_get_kind(vde_component *component);

/**
 * @brief Return component name
 *
 * @param component The component
 *
 * @return The quark of component name
 */
vde_quark vde_component_get_qname(vde_component *component);

/**
 * @brief Return component name
 *
 * @param component The component
 *
 * @return The string with component name
 */
const char *vde_component_get_name(vde_component *component);

/**
 * @brief vde_component utility to register commands
 *
 * @param component The component to add commands to
 * @param commands The NULL-terminated array of commands
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_component_commands_register(vde_component *component,
                                    vde_command *commands);

/**
 * @brief vde_component utility to deregister commands
 *
 * @param component The component to remove commands from
 * @param commands The NULL-terminated array of commands
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_component_commands_deregister(vde_component *component,
                                      vde_command *commands);

/**
 * @brief vde_component utility to add a command
 *
 * @param component The component to add the command to
 * @param command The command to add
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_component_command_add(vde_component *component,
                              vde_command *command);

/**
 * @brief vde_component utility to remove a command
 *
 * @param component The component to remove the command from
 * @param command The command to remove
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_component_command_del(vde_component *component,
                              vde_command *command);

/**
 * @brief Lookup for a command in a component
 *
 * @param component The component to look into
 * @param name The name of the command
 *
 * @return a vde command, NULL if not found
 */
vde_command *vde_component_command_get(vde_component *component,
                                       const char *name);

/**
 * @brief List all commands of a component
 *
 * @param component The component
 *
 * @return A null terminated array of commands
 */
vde_command **vde_component_commands_list(vde_component *component);

// XXX consolidate naming of signals, use path or signal_path
/**
 * @brief vde_component utility to register signals
 *
 * @param component The component to add signals to
 * @param commands The NULL-terminated array of signals
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_component_signals_register(vde_component *component,
                                   vde_signal *signals);

/**
 * @brief vde_component utility to deregister signals
 *
 * @param component The component to remove signals from
 * @param commands The NULL-terminated array of signals
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_component_signals_deregister(vde_component *component,
                                     vde_signal *signals);

/**
 * @brief vde_component utility to add a signal
 *
 * @param component The component to add the signal to
 * @param signal The signal to add
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_component_signal_add(vde_component *component,
                             vde_signal *signal);

/**
 * @brief vde_component utility to remove a signal
 *
 * @param component The component to remove the signal from
 * @param signal The signal to remove
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_component_signal_del(vde_component *component,
                             vde_signal *signal);

/**
 * @brief Lookup for a signal in a component
 *
 * @param component The component to look into
 * @param name The name of the signal
 *
 * @return a vde signal, NULL if not found
 */
vde_signal *vde_component_signal_get(vde_component *component,
                                     const char *name);

/**
 * @brief List all signals of a component
 *
 * @param component The component
 *
 * @return A null terminated array of signals
 */
vde_signal **vde_component_signals_list(vde_component *component);

/**
 * @brief Attach a callback to a signal
 *
 * @param component The component to start receiving signals from
 * @param signal The signal name
 * @param cb The callback function
 * @param destroy_cb The callback destroy function
 * @param data Callback private data
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_component_signal_attach(vde_component *component, const char *signal,
                                vde_signal_cb cb,
                                vde_signal_destroy_cb destroy_cb,
                                void *data);

/**
 * @brief Detach a callback from a signal
 *
 * @param component The component to stop receiving signals from
 * @param signal The signal name
 * @param cb The callback function
 * @param destroy_cb The callback destroy function
 * @param data Callback private data
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_component_signal_detach(vde_component *component, const char *signal,
                                vde_signal_cb cb,
                                vde_signal_destroy_cb destroy_cb,
                                void *data);

/**
 * @brief Raise a signal from this component
 *
 * @param component The component to raise signal from
 * @param signal The signal name
 * @param info The information attached to the signal
 */
void vde_component_signal_raise(vde_component *component, const char *signal,
                                vde_sobj *info);

#endif /* __VDE3_COMPONENT_H__ */
