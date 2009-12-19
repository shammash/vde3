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

struct vde_connection {
  vde_attributes *attributes;
  vde_context *context;
  unsigned int pkt_head_sz;
  unsigned int pkt_tail_sz;
  unsigned int send_maxtries;
  struct timeval send_maxtimeout;
  conn_be_write be_write;
  conn_be_close be_close;
  void *be_priv;
  conn_read_cb read_cb;
  conn_write_cb write_cb;
  conn_error_cb error_cb;
  void *cb_priv; // Callbacks private data (probably component ptr)
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

int vde_connection_init(vde_connection *conn, vde_context *ctx,
                        conn_be_write be_write, conn_be_close be_close,
                        void *be_priv)
{
  // XXX(shammash):
  // - check errors, only be_priv can prolly be NULL
  // - 'initialized' field needed
  conn->context = ctx;
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
  // XXX(shammash):
  // - check errors, conn can't be NULL (stuff initialized?)
  // - free attributes here
  vde_free(conn);
}

int vde_connection_write(vde_connection *conn, vde_pkt *pkt)
{
  // XXX(shammash): check conn is not NULL
  return conn->be_write(conn, pkt);
}

int vde_connection_call_read(vde_connection *conn, vde_pkt *pkt)
{
  // XXX(shammash): check conn and read_cb are not NULL
  return conn->read_cb(conn, pkt, conn->cb_priv);
}

int vde_connection_call_write(vde_connection *conn, vde_pkt *pkt)
{
  // XXX(shammash):
  // - check conn is not NULL
  // - change final return when defining
  if (conn->write_cb != NULL) {
    return conn->write_cb(conn, pkt, conn->cb_priv);
  }
  return 0;
}

int vde_connection_call_error(vde_connection *conn, vde_pkt *pkt,
                              vde_conn_error err)
{
  // XXX(shammash): check conn and error_cb are not NULL
  return conn->error_cb(conn, pkt, err, conn->cb_priv);
}

void vde_connection_set_callbacks(vde_connection *conn,
                                  conn_read_cb read_cb,
                                  conn_write_cb write_cb,
                                  conn_error_cb error_cb,
                                  void *cb_priv)
{
  // XXX(shammash): check errors, conn, read_cb, error_cb can't be NULL
  conn->read_cb = read_cb;
  conn->write_cb = write_cb;
  conn->error_cb = error_cb;
  conn->cb_priv = cb_priv;
}

vde_context *vde_connection_get_context(vde_connection *conn)
{
  // XXX: check conn is not NULL
  return conn->context;
}

void *vde_connection_get_priv(vde_connection *conn)
{
  // XXX(shammash): check conn is not NULL
  return conn->be_priv;
}

void vde_connection_set_pkt_properties(vde_connection *conn,
                                       unsigned int head_sz,
                                       unsigned int tail_sz)
{
  // XXX(shammash): check error: conn can't be NULL
  conn->pkt_head_sz = head_sz;
  conn->pkt_tail_sz = tail_sz;
}

unsigned int vde_connection_get_pkt_headsize(vde_connection *conn)
{
  // XXX(shammash): check error: conn can't be NULL
  return conn->pkt_head_sz;
}

unsigned int vde_connection_get_pkt_tailsize(vde_connection *conn)
{
  // XXX(shammash): check error: conn can't be NULL
  return conn->pkt_tail_sz;
}

void vde_connection_set_send_properties(vde_connection *conn,
                                        unsigned int max_tries,
                                        struct timeval *max_timeout)
{
  // XXX(shammash): check error: conn and max_timeout can't be NULL
  conn->send_maxtries = max_tries;
  timerclear(&conn->send_maxtimeout);
  timeradd(&conn->send_maxtimeout, max_timeout, &conn->send_maxtimeout);
}

unsigned int vde_connection_get_send_maxtries(vde_connection *conn)
{
  // XXX(shammash): check error: conn can't be NULL
  return conn->send_maxtries;
}

struct timeval *vde_connection_get_send_maxtimeout(vde_connection *conn)
{
  // XXX(shammash): check error: conn can't be NULL
  return &conn->send_maxtimeout;
}


void vde_connection_set_attributes(vde_connection *conn,
                                   vde_attributes *attributes)
{
  // XXX(shammash): check error: conn can't be NULL
  conn->attributes = attributes;
}

vde_attributes *vde_connection_get_attributes(vde_connection *conn)
{
  // XXX(shammash): check error: conn can't be NULL
  return conn->attributes;
}

