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

#include <vde3/priv/conn_manager.h>

enum vde_conn_state {
  CONNECT_WAIT,
  AUTHORIZATION_REQ_SENT,
  AUTHORIZATION_REQ_WAIT,
  AUTHORIZATION_REPLY_SENT,
  AUTHORIZATION_REPLY_WAIT,
  NOT_AUTHORIZED,
  AUTHORIZED
};

struct pending_conn {
  vde_connection *conn,
  vde_request *lreq,
  vde_request *rreq,
  vde_conn_state state,
  vde_connect_success_cb success_cb,
  vde_connect_error_cb error_cb,
  void *connect_cb_arg
};

struct conn_manager {
  vde_component *transport,
  vde_component *engine,
  vde_list *pending_conns,
  bool do_remote_authorization
}

typedef struct conn_manager conn_manager;

static struct pending_conn *cm_lookup_pending_conn(conn_manager *cm,
                                                   vde_connection *conn)
{
  struct pending_conn *pc = NULL;

  vde_list *iter = vde_list_first(cm->pending_conns);
  while (iter != NULL) {
    pc = vde_list_get_data(iter);
    if (pc->conn == conn) {
      return pc;
    }
    iter = vde_list_next(iter);
  }
  return NULL;
}

// XXX: is it better to pass transport/engine as a quark/string?
int conn_manager_init(vde_component *component, vde_component *transport,
                      vde_component *engine, bool do_remote_authorization)
{
  conn_manager *cm;

  if (component == NULL || transport == NULL || engine == NULL) {
    vde_error("%s: either component, transport or engine is NULL",
              __PRETTY_FUNCTION__);
    return -1;
  }
  if (vde_component_get_kind(transport) != VDE_TRANSPORT) {
    vde_error("%s: component transport is not a transport",
              __PRETTY_FUNCTION__);
    return -2;
  }
  if (vde_component_get_kind(engine) != VDE_ENGINE) {
    vde_error("%s: component engine is not a engine",
              __PRETTY_FUNCTION__);
    return -2;
  }
  cm = (conn_manager *)vde_calloc(sizeof(conn_manager));
  if (cm == NULL) {
    vde_error("%s: could not allocate private data", __PRETTY_FUNCTION__);
    return -4;
  }

  cm->transport = transport;
  cm->engine = engine;
  cm->do_remote_authorization = do_remote_authorization
  cm->pending_conns = NULL;

  /* Increase reference counter of tracked components */
  vde_component_get(transport, NULL);
  vde_component_get(engine, NULL);

  vde_component_set_transport_cm_callbacks(transport, &cm_connect_cb,
                                           &cm_accept_cb, &cm_error_cb,
                                           (void *)component);

  // XXX(shammash): this will be probably done by vde_component_init()
  vde_component_set_conn_manager_ops(component, &conn_manager_listen,
                                     &conn_manager_connect);

  vde_component_set_priv(component, cm);
  return 0;
}

int conn_manager_va_init(vde_component *component, va_list args)
{
  return conn_manager_init(component, va_arg(args, vde_component *),
                           va_arg(args, vde_component *), va_arg(args, bool));
}

// XXX: consider having an application callback here, to be called for each new
//      connection
int conn_manager_listen(vde_component *component)
{
  conn_manager *cm = (conn_manager *)vde_component_get_priv(component);

  return vde_transport_listen(cm->transport);
}

int conn_manager_connect(vde_component *component,
                         vde_request *local_request,
                         vde_request *remote_request,
                         vde_connect_success_cb success_cb,
                         vde_connect_error_cb error_cb, void *arg)
{
  conn_manager *cm;
  vde_connection *conn;
  struct pending_conn *pc;

  if (vde_connection_new(&conn)) {
    vde_error("%s: cannot create connection", __PRETTY_FUNCTION__);
    return -1;
  }

  pc = (struct pending_conn *)vde_calloc(sizeof(struct pending_conn));
  if (!pc) {
    vde_connection_delete(conn);
    vde_error("%s: cannot create pending connection", __PRETTY_FUNCTION__);
    return -2;
  }

  // XXX: check requests with do_remote_authorization,
  //      what about normalization?
  //      memdup requests here?
  pc->conn = conn;
  pc->lreq = local_request;
  pc->rreq = remote_request;
  pc->state = CONNECT_WAIT;
  pc->success_cb = success_cb;
  pc->error_cb = error_cb;
  pc->connect_cb_arg = arg;

  cm = vde_component_get_priv(component);
  cm->pending_conns = vde_list_prepend(cm->pending_conns, pc);

  vde_transport_connect(cm->priv->transport, conn);
}

void cm_connect_cb(vde_connection *conn, void *arg)
{
  struct pending_conn *pc;
  vde_component *component = (vde_component *)arg;
  conn_manager *cm = (conn_manager *)vde_component_get_priv(component);

  pc = cm_lookup_pending_conn(cm, conn);
  if (!pc) {
    vde_connection_fini(conn);
    vde_connection_delete(conn);
    vde_error("%s: cannot lookup pending connection", __PRETTY_FUNCTION__);
    return;
  }
  if (cm->do_remote_authorization) {
    vde_connection_set_callbacks(conn, cm_read_cb, cm_error_cb, component);
    // - fill defaults for remote_request?
    // - search remote_request in cm->priv->pending_conns
    // - begin authorization process
    // pc->state = AUTHORIZATION_REQ_SENT
  } else {
    pc->state = AUTHORIZED;
    post_authorization(cm, pc);
  }
}

void cm_accept_cb(vde_connection *conn, void *arg)
{
  struct pending_conn *pc;
  vde_component *component = (vde_component *)arg;
  conn_manager *cm = (conn_manager *)vde_component_get_priv(component);

  // XXX: safety check: conn not already added to pending_conns

  pc = (struct pending_conn *)vde_calloc(sizeof(struct pending_conn));
  if (!pc) {
    vde_connection_fini(conn);
    vde_connection_delete(conn);
    vde_error("%s: cannot create pending connection", __PRETTY_FUNCTION__);
    return;
  }

  pc->conn = conn;

  cm->pending_conns = vde_list_prepend(cm->pending_conns, pc);

  if (cm->do_remote_authorization) {
    vde_conn_set_callbacks(conn, cm_read_cb, cm_error_cb, component);
    // exchange authorization, eventually call post_authorization if
    // successful
    // pc->state = AUTHORIZATION_REQ_WAIT
  } else {
    // fill local_request with defaults
    pc->state = AUTHORIZED;
    post_authorization(cm, pc);
  }
}

void cm_error_cb(vde_connection *conn, vde_transport_error err, void *arg)
{
  // if conn in pending_conn and there's an application callback call it,
  // delete pending_conn and delete conn
}

int post_authorization(conn_manager *cm, struct pending_conn *pc)
{
  vde_engine_new_connection(cm->engine, conn, local_request);
  // if there's an application callback call it
  cm->pending_conns = vde_list_remove(cm->pending_conns, pc);
  vde_free(pc); // free requests here ?
}

// in engine.new_conn: vde_conn_set_callbacks(conn, engine_callbacks..)

// TODO cm_read_cb / cm_error_cb, they need to be different for accept/connect
// callbacks?

struct component_ops {
  .init = conn_manager_va_init,
  .fini = conn_manager_fini,
  .get_configuration = transport_vde2_get_configuration,
  .set_configuration = transport_vde2_set_configuration,
  .get_policy = transport_vde2_get_policy,
  .set_policy = transport_vde2_set_policy,
} conn_manager_component_ops;

struct vde_module {
  kind = VDE_CONNECTION_MANAGER,
  family = "default",
  cops = conn_manager_component_ops,
} conn_manager_module;

int conn_manager_module_init(vde_context *ctx)
{
  return vde_context_register_module(ctx, &conn_manager_module);
}

