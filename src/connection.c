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

#include <vde3/priv/connection.h>

struct vde_connection {
  int (*read)(vde_connection *, vde_packet *), // probably not needed
  int (*write)(vde_connection *, vde_packet *),
  void (*close)(vde_connection *), // probably called in fini()
  vde_attributes *attributes,
  vde_context *context,
  void *priv,
  conn_read_cb read_cb,
  conn_write_cb write_cb,
  conn_error_cb error_cb,
  void *cb_priv // Callbacks private data (probably component ptr)
};

int vde_connection_new(vde_connection **conn) {
  if (!conn) {
    vde_error("%s: connection pointer reference is NULL", __PRETTY_FUNCTION__);
    return -1;
  }
  *conn = (vde_connection *)vde_calloc(sizeof(vde_connection));
  if (*conn == NULL) {
    vde_error("%s: cannot create connection", __PRETTY_FUNCTION__);
    return -2;
  }
  return 0;
}

void vde_connection_set_callbacks(vde_connection *conn,
                                  conn_read_cb read_cb,
                                  conn_write_cb write_cb,
                                  conn_error_cb error_cb,
                                  void *cb_priv)
{
  conn->read_cb = read_cb;
  conn->write_cb = write_cb;
  conn->error_cb = error_cb;
  conn->cb_priv = cb_priv;
}

