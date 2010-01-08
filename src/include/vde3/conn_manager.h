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

#ifndef __VDE3_CONN_MANAGER_H__
#define __VDE3_CONN_MANAGER_H__

#include <vde3/component.h>

/**
 * @brief Function called on a connection manager to set it in listen mode.
 * Connection managers must implement it.
 *
 * @param cm The Connection Manager
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
typedef int (*cm_listen)(vde_component *cm);

/**
 * @brief Function called on a connection manager to connect it. Connection
 * managers must implement it.
 *
 * @param cm The Connection Manager
 * @param local Connection parameters for local engine
 * @param remote Connection parameters for remote engine
 * @param success_cb The callback to be called upon successfull connect
 * @param error_cb The callback to be called when an error occurs
 * @param arg Callbacks private data
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
typedef int (*cm_connect)(vde_component *cm, vde_request *local,
                          vde_request *remote,
                          vde_connect_success_cb success_cb,
                          vde_connect_error_cb error_cb,
                          void *arg);

/**
 * @brief Fill the connection manager ops in a component
 *
 * @param cm The connection manager
 * @param listen Listen op
 * @param connect Connect op
 */
void vde_conn_manager_set_ops(vde_component *cm, cm_listen listen,
                              cm_connect connect);

#endif /* __VDE3_CONN_MANAGER_H__ */
