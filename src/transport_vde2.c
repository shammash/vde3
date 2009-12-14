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

#include <vde3/priv/module.h>
#include <vde3/priv/component.h>

struct transport_ops {

} transport_vde2_ops;

struct component_ops {
  .init = transport_vde2_init,
  .fini = transport_vde2_fini,
  .get_configuration = transport_vde2_get_configuration,
  .set_configuration = transport_vde2_set_configuration,
  .get_policy = transport_vde2_get_policy,
  .set_policy = transport_vde2_set_policy,
} transport_vde2_component_ops;

struct vde_module {
  kind = VDE_TRANSPORT,
  family = "vde2",
  cops = transport_vde2_component_ops,
} transport_vde2_module;

int transport_vde2_module_init(vde_context *ctx)
{
  return vde_context_register_module(ctx, &transport_vde2_module);
}

/*
 * the connection has several parameters to handle the queuing of packets:
 *  - if a transient error during write to fd is encountered, the write is tried
 * MAX_TRIES * MAX_TIMEOUT times and after that error_cb(CONN_PKT_DROP, pkt) is
 * called for the pkt we are currently trying to drop but before the pkt is
 * freed
 *  - if a permanent error during write to fd is encountered
 * error_cb(CONN_CLOSED) (or sth) is called
 *
 * conn.write() must return error on queue full
 *
 * the queue is sized by the connection
 *
 * write_cb(conn, pkt) if specified is called after each packet has been
 * successfully written to the operating system but before the packet is freed
 *
 * OPTIONAL: the return value of error_cb/write_cb can indicate what to do with
 * the packet
 *
 */

/*
 * per-connection packet dequeue:
 * a callback which flushes the queue is linked to write event for the conn fd.
 * whenever a packet is added to the queue the event is enabled if not already.
 * whenever the queue is empty the event is disabled.
 *
 * struct event_queue {
 *   vde_event event; // the event related to this queue
 *   vde_dequeue *pktqueue; // the packet queue itself
 *   vde_conn *conn; // the connection owning this queue
 * }
 *
 * conn_init(...) {
 *   ...
 *   conn->priv->conn = conn;
 *   event_set(conn->priv->event, conn->fd, conn_write_internal_cb, conn->priv);
 * }
 *
 * conn_write(conn, pkt) {
 *   conn->priv->pktqueue.append(pkt)
 *   if (conn->priv->event is not active)
 *     event_add(conn->priv->event)
 *
 *   return success
 * }
 *
 * conn_write_internal_cb(fd, type, arg) {
 *   event_queue queue = arg;
 *   write(conn_fd, pop_packet, size);
 *   if success:
 *     remove packet from queue
 *
 * }
 *
 * //TODO for the STREAM case also available_data needs to be taken care of to
 * // know when a packet has been fully written to the network and thus can be
 * // removed from the queue
 *
 */

