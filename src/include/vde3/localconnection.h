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


// XXX(shammash): define the right place to put this header

#ifndef __VDE3_LOCALCONNECTION_H__
#define __VDE3_LOCALCONNECTION_H__

#include <vde3/connection.h>

// A local connection is a coupled data structure: when write is called on a
// local connection a read_cb is triggered on its peer. Each peer is registered
// as a connection on two different engines.

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

#endif /* __VDE3_LOCALCONNECTION_H__ */
