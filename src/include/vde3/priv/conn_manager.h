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

#ifndef __VDE3_PRIV_CONN_MANAGER_H__
#define __VDE3_PRIV_CONN_MANAGER_H__

enum vde_conn_state {};

struct pending_conn {
  vde_connection *conn,
  vde_conn_state *state,
  local_request,
  remote_request,
};

struct vde_conn_manager {
  vde_component *transport,
  vde_component *engine,
  vde_list *pending_conns,
  bool do_remote_authorization
}

typedef struct vde_conn_manager vde_conn_manager;

int vde_conn_manager_listen(vde_conn_manager *cm);

int vde_conn_manager_connect(vde_conn_manager *cm, local_request,
                             remote_request);

#endif /* __VDE3_PRIV_CONN_MANAGER_H__ */
