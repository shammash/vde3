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

#include <stdio.h>

#include <vde3.h>

#include <vde3/module.h>
#include <vde3/engine.h>
#include <vde3/context.h>
#include <vde3/connection.h>

#include <engine_hub_commands.h>

// from vde_switch/packetq.c
#define TIMEOUT 5
#define TIMES 10
// end from vde_switch/packetq.c


// START temporary signals declaration
// XXX as for commands, signals should be auto-generated
#include <vde3/signal.h>
static vde_signal engine_hub_signals [] = {
  { "port_new", NULL, NULL, NULL },
  { "port_del", NULL, NULL, NULL },
  { NULL, NULL, NULL, NULL },
};
// END temporary signals declaration


typedef struct {
  vde_component *component;
  vde_list *ports;
} hub_engine;

int engine_hub_status(vde_component *component, vde_sobj **out)
{
  hub_engine *hub = vde_component_get_priv(component);

  *out = vde_sobj_new_int(vde_list_length(hub->ports));

  return 0;
}

int engine_hub_printport(vde_component *component, int port, vde_sobj **out)
{
  char str[128];

  sprintf(str, "please print something useful for port %i", port);
  *out = vde_sobj_new_string(str);

  return 0;
}

int hub_engine_readcb(vde_connection *conn, vde_pkt *pkt, void *arg)
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

  return 0;
}

int hub_engine_errorcb(vde_connection *conn, vde_pkt *pkt,
                                  vde_conn_error err, void *arg)
{
  vde_sobj *info;
  hub_engine *hub = (hub_engine *)arg;

  if (err == CONN_WRITE_DELAY) {
    vde_warning("%s: dropping packet", __PRETTY_FUNCTION__);
    return 0;
  }

  // XXX: handle different errors, the following is just the fatal case

  hub->ports = vde_list_remove(hub->ports, conn);

  info = vde_sobj_new_array();
  // XXX check info not null
  // XXX print new port number instead of the total number of ports in the hub
  vde_sobj_array_add(info, vde_sobj_new_int(vde_list_length(hub->ports)));
  vde_component_signal_raise(hub->component, "port_del", info);
  vde_sobj_put(info);

  errno = EPIPE;
  return -1;
}

int hub_engine_newconn(vde_component *component, vde_connection *conn,
                       vde_request *req)
{
  unsigned int max_payload;
  struct timeval send_timeout;
  vde_sobj *info;
  hub_engine *hub = vde_component_get_priv(component);

  max_payload = vde_connection_max_payload(conn);
  if (max_payload != 0 && max_payload < sizeof(struct eth_frame)) {
    vde_warning("%s: connection can't handle full eth frames, rejecting");
    return -1;
  }

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

  info = vde_sobj_new_array();
  // XXX check info not null
  // XXX print new port number instead of the total number of ports in the hub
  vde_sobj_array_add(info, vde_sobj_new_int(vde_list_length(hub->ports)));
  vde_component_signal_raise(component, "port_new", info);
  vde_sobj_put(info);

  return 0;
}

// XXX: add a max_ports argument?
static int engine_hub_init(vde_component *component, vde_sobj *params)
{
  int tmp_errno;
  hub_engine *hub;

  vde_assert(component != NULL);

  hub = (hub_engine *)vde_calloc(sizeof(hub_engine));
  if (hub == NULL) {
    vde_error("%s: could not allocate private data", __PRETTY_FUNCTION__);
    errno = ENOMEM;
    return -1;
  }

  hub->component = component;

  // command registration phase
  // - the header for the wrappers has been included at the top
  // - register the commands array, the name is in the json definition
  if (vde_component_commands_register(component, engine_hub_commands)) {
    tmp_errno = errno;
    vde_error("%s: could not register commands", __PRETTY_FUNCTION__);
    vde_free(hub);
    errno = tmp_errno;
    return -1;
  }

  if (vde_component_signals_register(component, engine_hub_signals)) {
    tmp_errno = errno;
    vde_error("%s: could not register signals", __PRETTY_FUNCTION__);
    vde_free(hub);
    errno = tmp_errno;
    return -1;
  }

  vde_component_set_priv(component, (void *)hub);
  return 0;
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

  vde_free(hub);

  vde_component_commands_deregister(component, engine_hub_commands);
  vde_component_signals_deregister(component, engine_hub_signals);
}

component_ops engine_hub_component_ops = {
  .init = engine_hub_init,
  .fini = engine_hub_fini,
  .get_configuration = NULL,
  .set_configuration = NULL,
  .get_policy = NULL,
  .set_policy = NULL,
};

vde_module VDE_MODULE_START = {
  .kind = VDE_ENGINE,
  .family = "hub",
  .cops = &engine_hub_component_ops,
  .eng_new_conn = &hub_engine_newconn,
};

