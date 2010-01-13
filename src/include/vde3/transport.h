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

#ifndef __VDE3_TRANSPORT_H__
#define __VDE3_TRANSPORT_H__

#include <vde3/component.h>

/**
 * @brief Prototype of the callback called by the transport to notify a new
 * connection has been accepted.
 *
 * @param conn The new connection
 * @param arg Callback private data
 */
typedef void (*cm_accept_cb)(vde_connection *conn, void *arg);

/**
 * @brief Prototype of the callback called by the transport to notify the
 * success of a previous connect request.
 *
 * @param conn The connection successfully connected
 * @param arg Callback private data
 */
typedef void (*cm_connect_cb)(vde_connection *conn, void *arg);

/**
 * @brief Prototype of the callback called by the transport to notify an error
 * has occurred during listen or connect.
 *
 * @param conn If not NULL it represents the connection which has caused the
 * error
 * @param arg Callback private data
 */
typedef void (*cm_error_cb)(vde_connection *conn, int tr_errno,
                            void *arg);

/**
 * @brief Put the transport in listen state
 *
 * @param transport The transport
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_transport_listen(vde_component *transport);

/**
 * @brief Connect using the given transport
 *
 * @param transport The transport
 * @param conn The connection to use
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_transport_connect(vde_component *transport, vde_connection *conn);

/**
 * @brief Fill the connection manager callbacks of a transport
 *
 * @param transport The transport to set callbacks to
 * @param connect_cb Connect callback
 * @param accept_cb Accept callback
 * @param error_cb Error callback
 * @param arg Callbacks private data
 */
void vde_transport_set_cm_callbacks(vde_component *transport,
                                    cm_connect_cb connect_cb,
                                    cm_accept_cb accept_cb,
                                    cm_error_cb error_cb, void *arg);

/**
 * @brief Function called by transport implementation to tell the attached
 * connection manager its previous connect() succeeded.
 *
 * @param transport The calling transport
 * @param conn The new connection
 */
void vde_transport_call_cm_connect_cb(vde_component *transport,
                                      vde_connection *conn);

/**
 * @brief Function called by transport implementation to tell the attached
 * connection manager a new connection has been accepted.
 *
 * @param transport The calling transport
 * @param conn The new connection
 */
void vde_transport_call_cm_accept_cb(vde_component *transport,
                                     vde_connection *conn);

/**
 * @brief Function called by transport implementation to tell the attached
 * connection manager an error occurred in the transport.
 *
 * @param transport The calling transport
 * @param conn The connection which was being created when error occurred (can
 * be NULL)
 * @param tr_errno The errno value
 */
void vde_transport_call_cm_error_cb(vde_component *transport,
                                    vde_connection *conn, int tr_errno);

#endif /* __VDE3_TRANSPORT_H__ */
