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


// XXX(shammash): define the right place to put this header

#ifndef __VDE3_LOCALCONNECTION_H__
#define __VDE3_LOCALCONNECTION_H__

#include <vde3.h>

#include <vde3/connection.h>

/*
 * A local connection is composed of two connections (peers) registered on two
 * different engines. When a write is called on one connection a read_cb is
 * triggered on its peer and vice-versa. A connection keeps reference of its
 * peer in vde_connection private data.
 *
 * There are two kinds of local connection: queued and unqueued.
 *
 * Queued connections behave exactly like non-local connection: when an
 * engine delivers the packet a packet copy is performed and an event is
 * registered to deliver the copy to the other engine. After the read callback
 * on the second engine has returned with success a write callback on the first
 * one is called.
 *
 * Non queued connections deliver the packet to the second engine as soon as a
 * write is called, so they don't create a copy of the packet and neither they
 * call the write callback of the first engine when the second engine reads the
 * packet.
 */

/**
 * @brief Connect two engines together using an unqueued local connection.
 *
 * @param ctx The context of the two engines.
 * @param engine1 The first engine to connect
 * @param req1 The request for the first engine
 * @param engine2 The second engine to connect
 * @param req2 The request for the second engine
 *
 * @return zero on success, an error code otherwise
 */
// XXX: find a better name for this function :)
int vde_connect_engines_unqueued(vde_context *ctx, vde_component *engine1,
                                 vde_request *req1, vde_component *engine2,
                                 vde_request *req2);

#endif /* __VDE3_LOCALCONNECTION_H__ */

