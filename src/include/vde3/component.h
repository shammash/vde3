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

// XXX to be defined, maybe unify with vde_connection_error?
enum vde_transport_error {
  TRANSPORT_GENERIC_ERROR,
};

typedef enum vde_transport_error vde_transport_error;

// Connection manager ops
typedef int (*cm_listen)(vde_component *cm);
typedef int (*cm_connect)(vde_component *cm, vde_request *local,
                          vde_request *remote,
                          vde_connect_success_cb success_cb,
                          vde_connect_error_cb error_cb,
                          void *arg);
// Transport ops
typedef int (*tr_listen)(vde_component *transport);
typedef int (*tr_connect)(vde_component *transport, vde_connection *conn);

// Transport connection manager callbacks
typedef void (*cm_connect_cb)(vde_connection *conn, void *arg);
typedef void (*cm_accept_cb)(vde_connection *conn, void *arg);
typedef void (*cm_error_cb)(vde_connection *conn, vde_transport_error err,
                            void *arg);

/**
 * @brief Alloc a new VDE 3 component
 *
 * @param component reference to new component pointer
 *
 * @return zero on success, otherwise an error code
 */
int vde_component_new(vde_component **component);

/**
 * @brief Init a VDE 3 component
 *
 * @param component The component to init
 * @param quark The quark representing component name
 * @param module The module which will implement the component functionalities
 * @param args Parameters for initialization of module properties
 *
 * @return zero on success, otherwise an error code
 */
int vde_component_init(vde_component *component, vde_quark qname,
                       vde_module *module, va_list args);

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
*
* @return zero on success, otherwise an error
*/
int vde_component_get(vde_component *component, int *count);

/**
* @brief Decrease reference counter
*
* @param component The component
* @param count The pointer where to store reference counter value (might be
* NULL)
*
* @return zero on success, otherwise an error
*/
int vde_component_put(vde_component *component, int *count);

/**
 * @brief Decrease reference counter if there's only one reference
 *
 * @param component The component
 * @param count The pointer where to store reference counter value (might be
 * NULL)
 *
 * @return zero if there was only one reference, 1 otherwise, an error code
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
 * @brief Retrieve the context kind
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
* @brief vde_component utility to add a command
*
* @param component The component to add the command to
* @param command The command to add
*
* @return zero on success, otherwise an error code
*/
int vde_component_command_add(vde_component *component,
                              vde_command *command);

/**
* @brief vde_component utility to remove a command
*
* @param component The component to remove the command from
* @param command The command to remove
*
* @return zero on success, otherwise an error code
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

/**
* @brief vde_component utility to add a signal
*
* @param component The component to add the signal to
* @param signal The signal to add
*
* @return zero on success, otherwise an error code
*/
int vde_component_signal_add(vde_component *component,
                              vde_signal *signal);

/**
* @brief vde_component utility to remove a signal
*
* @param component The component to remove the signal from
* @param signal The signal to remove
*
* @return zero on success, otherwise an error code
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
* @brief Signature of a signal callback
*
* @param component The component raising the signal
* @param signal The signal name
* @param infos Serialized signal parameters, if NULL the signal is being
*              destroyed
* @param data Callback private data
*/
void vde_component_signal_callback(vde_component *component,
                                    const char *signal, vde_serial_obj *infos,
                                    void *data);

/**
* @brief Attach a callback to a signal
*
* @param component The component to receive signals from
* @param signal The signal name
* @param callback The callback function
* @param data Callback private data
*
* @return zero on success, otherwise an error code
*/

// XXX to be defined
//int vde_component_signal_attach(vde_component *component, const char *signal,
//                                  vde_component_signal_callback (*callback),
//                                  void *data);

/**
* @brief Detach a callback from a signal
*
* @param component The component to stop receiving signals from
* @param signal The signal name
* @param callback The callback function to detach
*
* @return zero on success, otherwise an error code
*/

// XXX to be defined
// int vde_component_signal_detach(vde_component *component, const char *signal,
//                                  vde_component_signal_callback (*callback));

/**
 * @brief Fill the connection manager ops in a component
 *
 * @param cm The connection manager
 * @param listen Listen op
 * @param connect Connect op
 */
void vde_component_set_conn_manager_ops(vde_component *cm, cm_listen listen,
                                        cm_connect connect);

/**
 * @brief Fill the transport ops in a component
 *
 * @param transport The transport
 * @param listen Listen op
 * @param connect Connect op
 */
void vde_component_set_transport_ops(vde_component *transport,
                                     tr_listen listen,
                                     tr_connect connect);

/**
 * @brief Put the underlying transport in listen mode
 *
 * @param cm The connection manager to use
 *
 * @return zero on successful listen, an error code otherwise
 */
int vde_component_conn_manager_listen(vde_component *cm);

/**
 * @brief Initiate a new connection using the underlying transport
 *
 * @param cm The connection manager for the connection
 * @param local_request The request for the local system
 * @param remote_request The request for the remote system
 *
 * @return zero on successful queuing of connection, an error code otherwise
 */
int vde_component_conn_manager_connect(vde_component *cm,
                                       vde_request *local_request,
                                       vde_request *remote_request,
                                       vde_connect_success_cb success_cb,
                                       vde_connect_error_cb error_cb,
                                       void *arg);

/**
 * @brief Fill the connection manager callbacks of a transport
 *
 * @param transport The transport to set callbacks to
 * @param connect_cb Connect callback
 * @param accept_cb Accept callback
 * @param error_cb Error callback
 * @param arg Callbacks private data
 */
void vde_component_set_transport_cm_callbacks(vde_component *transport,
                                              cm_connect_cb connect_cb,
                                              cm_accept_cb accept_cb,
                                              cm_error_cb error_cb, void *arg);

#endif /* __VDE3_COMPONENT_H__ */
