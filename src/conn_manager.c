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

enum vde_conn_state {};

struct pending_conn {
  vde_connection *connection,
  vde_request *lreq,
  vde_request *rreq,
  vde_conn_state state
};

struct conn_manager {
  vde_component *transport,
  vde_component *engine,
  vde_list *pending_conns,
  bool do_remote_authorization
}

typedef struct conn_manager conn_manager;

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

  vde_transport_set_callbacks(transport,
                              cm_connect_cb, cm_accept_cb, cm_error_cb, cm);

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

int conn_manager_connect(vde_component *cm, vde_request *local_request,
                             vde_request *remote_request)
{
  vde_connection *conn;

  conn = vde_connection_new();
  vde_transport_connect(cm->priv->transport, conn);

  pending_conn = vde_alloc(sizeof(struct pending_conn));
  pending_conn->conn = conn;
  pending_conn->lreq = local_request;
  pending_conn->rreq = remote_request;
  cm->priv->pending_conns = vde_list_prepend(cm->priv->pending_conns,
                                             pending_conn);
}

static int cm_connect_cb(vde_connection *conn, void *arg)
{
  vde_component *cm = (vde_component *)cm;

  vde_connection_set_callbacks(conn, cm_read_cb, cm_error_cb, cm);

  if( cm->priv->do_remote_authorization ){
    // - search remote_request in cm->priv->pending_conns
    // - begin authorization process
  } else {
    post_authorization(cm, conne);
  }
}

int post_authorization(vde_component *cm, vde_connection *conn)
{
  // TODO: search local_request in cm->priv->pending_conns
  vde_engine_new_connnection(cm->engine, conn, local_request);
  // TODO: remove conn from cm->priv->pending_conns (and free requests memory?)
}

int conn_manager_listen(vde_component *cm)
{
  return vde_transport_listen(cm->priv->transport);
}

static int accept_cb(vde_conn *conn, void *cm)
{
  vde_component *cm = (vde_component *)cm;

  vde_conn_set_callbacks(conn, cm_read_cb, cm_error_cb, cm);
  if( cm->priv->do_remote_authorization ){
    // exchange authorization, eventually call post_authorization if
    // successful
  } else {
    post_authorization(cm, conn);
  }
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

