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

#include <vde3/common.h>
#include <vde3/connection.h>

#include <limits.h>

int vde_connection_new(vde_connection **conn) {

  vde_return_val_if_fail(conn, -1);

  *conn = (vde_connection *)vde_calloc(sizeof(vde_connection));
  if (*conn == NULL) {
    vde_error("%s: cannot create connection", __PRETTY_FUNCTION__);
    errno = ENOMEM;
    return -1;
  }
  return 0;
}

int vde_connection_init(vde_connection *conn, vde_context *ctx,
                        unsigned int payload_size, conn_be_write be_write,
                        conn_be_close be_close, void *be_priv)
{
  // XXX(shammash):
  // - check errors, only be_priv can prolly be NULL
  // - 'initialized' field needed
  conn->context = ctx;
  conn->max_pload = payload_size;
  conn->be_write = be_write;
  conn->be_close = be_close;
  conn->be_priv = be_priv;
  return 0;
}

void vde_connection_fini(vde_connection *conn)
{
  // XXX(shammash):
  // - check errors, conn can't be NULL (stuff initialized?)
  // - this is expected to be called from connection owner so we don't need to
  //   trigger a conn_error_cb() with a fatal error right ?
  conn->be_close(conn);
}

void vde_connection_delete(vde_connection *conn)
{
  vde_return_if_fail(conn != NULL);

  // XXX(shammash):
  // - check errors, conn can't be NULL (stuff initialized?)
  // - free attributes here
  vde_free(conn);
}

void vde_connection_set_callbacks(vde_connection *conn,
                                  conn_read_cb read_cb,
                                  conn_write_cb write_cb,
                                  conn_error_cb error_cb,
                                  void *cb_priv)
{
  vde_return_if_fail(conn != NULL);
  vde_return_if_fail(read_cb != NULL && error_cb != NULL);

  conn->read_cb = read_cb;
  conn->write_cb = write_cb;
  conn->error_cb = error_cb;
  conn->cb_priv = cb_priv;
}

unsigned int vde_connection_max_payload(vde_connection *conn)
{
  vde_return_val_if_fail(conn != NULL, 1);

  return conn->max_pload;
}

void vde_connection_set_pkt_properties(vde_connection *conn,
                                       unsigned int head_sz,
                                       unsigned int tail_sz)
{
  vde_return_if_fail(conn != NULL);

  conn->pkt_head_sz = head_sz;
  conn->pkt_tail_sz = tail_sz;
}

void vde_connection_set_send_properties(vde_connection *conn,
                                        unsigned int max_tries,
                                        struct timeval *max_timeout)
{
  vde_return_if_fail(conn != NULL);
  vde_return_if_fail(max_timeout != NULL);

  conn->send_maxtries = max_tries;
  timerclear(&conn->send_maxtimeout);
  timeradd(&conn->send_maxtimeout, max_timeout, &conn->send_maxtimeout);
}

void vde_connection_set_attributes(vde_connection *conn,
                                   vde_attributes *attributes)
{
  vde_return_if_fail(conn != NULL);

  conn->attributes = attributes;
}

vde_attributes *vde_connection_get_attributes(vde_connection *conn)
{
  vde_return_val_if_fail(conn != NULL, NULL);

  return conn->attributes;
}

