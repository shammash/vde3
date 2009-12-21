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

#include <vde3/module.h>
#include <vde3/component.h>
#include <vde3/context.h>
#include <vde3/connection.h>

// from vde_switch/packetq.c
#define TIMEOUT 5
#define TIMES 10
// end from vde_switch/packetq.c

struct hub_engine {
  vde_component *component;
  vde_list *ports;
};
typedef struct hub_engine hub_engine;

conn_cb_result hub_engine_readcb(vde_connection *conn, vde_pkt *pkt, void *arg)
{
  vde_list *iter;
  vde_connection *port;

  hub_engine *hub = (hub_engine *)arg;

  /* Send to all the ports */
  iter = vde_list_first(hub->ports);
  while (iter != NULL) {
    port = vde_list_get_data(iter);
    if (port != conn) {
      // XXX: check write retval
      vde_connection_write(port, pkt);
    }
    iter = vde_list_next(iter);
  }

  return CONN_CB_OK;
}

conn_cb_result hub_engine_errorcb(vde_connection *conn, vde_pkt *pkt,
                                  vde_conn_error err, void *arg)
{
  hub_engine *hub = (hub_engine *)arg;

  // XXX: handle different errors, the following is just the fatal case

  hub->ports = vde_list_remove(hub->ports, conn);

  return CONN_CB_CLOSE;
}

int hub_engine_newconn(vde_component *component, vde_connection *conn,
                       vde_request *req)
{
  struct timeval send_timeout;
  hub_engine *hub = vde_component_get_priv(component);

  // XXX: check ports not NULL
  // XXX: here new port is added as the first one
  hub->ports = vde_list_prepend(hub->ports, conn);

  /* Setup connection */
  vde_connection_set_callbacks(conn, &hub_engine_readcb, NULL,
                               &hub_engine_errorcb, (void *)hub);
  vde_connection_set_pkt_properties(conn, 0, 0);
  send_timeout.tv_sec = TIMEOUT;
  send_timeout.tv_usec = 0;
  vde_connection_set_send_properties(conn, TIMES, &send_timeout);
  // XXX negotiate MTU with connection here?

  return 0;
}

// XXX: add a max_ports argument?
static int engine_hub_init(vde_component *component)
{

  hub_engine *hub;

  if (component == NULL) {
    vde_error("%s: component is NULL", __PRETTY_FUNCTION__);
    return -1;
  }

  hub = (hub_engine *)vde_calloc(sizeof(hub_engine));
  if (hub == NULL) {
    vde_error("%s: could not allocate private data", __PRETTY_FUNCTION__);
    return -3;
  }

  hub->component = component;

  vde_component_set_engine_ops(component, &hub_engine_newconn);
  vde_component_set_priv(component, (void *)hub);
  return 0;
}

int engine_hub_va_init(vde_component *component, va_list args)
{
  return engine_hub_init(component);
}

void engine_hub_fini(vde_component *component)
{

  vde_list *iter;
  vde_connection *port;
  hub_engine *hub = (hub_engine *)vde_component_get_priv(component);

  iter = vde_list_first(hub->ports);
  while (iter != NULL) {
    port = vde_list_get_data(iter);
    // XXX check if this is safe here
    vde_connection_fini(port);
    vde_connection_delete(port);

    iter = vde_list_next(iter);
  }
  vde_list_delete(hub->ports);
}

struct component_ops engine_hub_component_ops = {
  .init = engine_hub_va_init,
  .fini = engine_hub_fini,
  .get_configuration = NULL,
  .set_configuration = NULL,
  .get_policy = NULL,
  .set_policy = NULL,
};

struct vde_module engine_hub_module = {
  .kind = VDE_ENGINE,
  .family = "hub",
  .cops = &engine_hub_component_ops,
};

int engine_hub_module_init(vde_context *ctx)
{
  return vde_context_register_module(ctx, &engine_hub_module);
}

