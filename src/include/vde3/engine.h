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

#ifndef __VDE3_ENGINE_H__
#define __VDE3_ENGINE_H__

#include <vde3/component.h>

/**
 * @brief Function called on an engine to add a new connection. Engines must
 * implement it.
 *
 * @param engine The engine to add the connection to
 * @param conn The connection
 * @param req The connection parameters
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
typedef int (*eng_new_conn)(vde_component *engine, vde_connection *conn,
                            vde_request *req);

/**
 * @brief Fill engine ops in a component
 *
 * @param engine The engine
 * @param new_conn New Connection op
 */
void vde_engine_set_ops(vde_component *engine, eng_new_conn new_conn);

/**
 * @brief Attach a connection to an engine
 *
 * @param engine The engine to attach the connection to
 * @param conn The connection to attach
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_engine_new_connection(vde_component *engine, vde_connection *conn,
                              vde_request *req);

#endif /* __VDE3_ENGINE_H__ */
