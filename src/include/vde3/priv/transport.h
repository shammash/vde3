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

#ifndef __VDE3_PRIV_TRANSPORT_H__
#define __VDE3_PRIV_TRANSPORT_H__

struct transport_ops {
  int (*listen)(vde_component*),
  int (*connect)(vde_component *, vde_connection *),
  void (*set_conn_manager)(vde_component *, vde_conn_manager *cm),
};

// needed callbacks (implemented as conn_manager methods):
vde_conn_manager_accept_cb(cm, conn)
vde_conn_manager_connect_cb(cm, conn)
vde_conn_manager_error_cb(cm, error_type, conn||NULL)

typedef struct transport_ops transport_ops;

#endif /* __VDE3_PRIV_TRANSPORT_H__ */
