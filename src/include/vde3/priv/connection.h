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

#ifndef __VDE3_PRIV_CONNECTION_H__
#define __VDE3_PRIV_CONNECTION_H__

struct vde_connection {
  int (*read)(vde_connection *, vde_packet *), // probably not needed
  int (*write)(vde_connection *, vde_packet *),
  int (*read_cb)(vde_connection *, vde_packet *, void *),
  int (*write_cb)(vde_connection *, vde_packet *, void *),
  void (*close)(vde_connection *), // probably called in fini()
  int (*error_cb)(vde_connection *, error_type, void *);
  vde_attributes *attributes,
  vde_context *context,
  void *priv,
  void *cb_priv // Callbacks private data (probably component ptr)
};

// A local connection is a coupled data structure: when write is called on a
// local connection a read_cb is triggered on its peer. Each peer is registered
// as a connection on two different engines.

// Memory management:
// - a connection calls read_cb iff a packet is ready
// - when write() is called the connection must be responsible for copying the
//   packet because because it will be free()ed right after write() returns
// - a local connection duplicate the packet passed with write(), calls its
//   peer's read_cb and then free()s the duplicated packet
//
// DGRAM/STACK flow:
// - connection: read on stack space
// - connection: call read_cb()
// - engine/cm: does stuff considering that when read_cb() returns the packet
//              will be free()d. e.g.:
//              for each c in connections:
//                if c != incoming_connection:
//                  c.write(pkt)
// - connection: memcpy(pkt) -> add(packetq)
//
// STREAM/BUFFER, incoming flow:
//   n = 0;
//   len = read(pktbuf+n, MAXBUFSIZE-n)
//   n+=len
//   if n < HDR_SIZE
//     return
//   if n < HDR_SIZE + pktbuf->hdr.len
//     return
//   read_cb(pktbuf)
//   n = 0

// Connection manager and rpcengine must deal with fragmentation.

typedef struct vde_connection vde_connection;

int vde_conn_read(vde_connection *conn, ...) {
  return conn->read(conn, ...);
}

void vde_conn_set_callbacks(vde_connection *conn, read_cb, write_cb, error_cb,
                            void *cb_priv);

// LocalConnection
//
// Created by context, then each engine closes its side.
//
// A LocalConnection derives from a vde_connection and has its peer stored in
// connection->priv
//
// new_localconn()
//   c = new_conn()
//   c2 = new_conn()
//   c.set_peer(c2)
//   c2.set_peer(c)
//
// vde_context_connect_engines(ctx, e1, e2, req1, req2)
//   lc = new_localconn()
//   lcpeer = lc.get_peer()
//   /* normalize requests */
//   e1.new_conn(lc, req1)
//   e2.new_conn(lcpeer, req2)
//
// localconn_write(pkt)
//   peer = get_peer()
//   /* Here we can have a flag in localconn which says if we will call
//    * read_cb() directly or we will schedule an event with timeout 0.
//    */
//   rv = peer.read_cb(pkt, peer.cb_priv)
//   if rv == error
//     error_cb(error, cb_priv)
//
// localconn_close()
//   peer = get_peer()
//   peer.error_cb(error_close, peer.cb_priv)

#endif /* __VDE3_PRIV_CONNECTION_H__ */
