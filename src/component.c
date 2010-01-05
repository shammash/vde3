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

#include <stdbool.h>

#include <vde3.h>

#include <vde3/component.h>
#include <vde3/module.h>

struct vde_component {
  vde_context *ctx;
  component_ops *cops;
  vde_quark qname;
  vde_component_kind kind;
  vde_char *family;
  int refcount;
  vde_hash *commands;
  vde_hash *signals;
  void *priv;
  bool initialized;
  // Ops connection_manager specific:
  cm_listen cm_listen;
  cm_connect cm_connect;
  // Ops transport specific:
  tr_listen tr_listen;
  tr_connect tr_connect;
  // Ops engine specific:
  eng_new_conn eng_new_conn;
  // transport - connection manager specific callbacks:
  cm_connect_cb cm_connect_cb;
  cm_accept_cb cm_accept_cb;
  cm_error_cb cm_error_cb;
  void *cm_cb_arg;
};

int vde_component_new(vde_component **component)
{
  vde_return_val_if_fail(component != NULL, -1);

  *component = (vde_component *)vde_calloc(sizeof(vde_component));
  vde_return_val_if_fail(*component != NULL, -2);

  return 0;
}

/* XXX(godog): add to the component something like a format string to validate
 * va_list?
 * XXX(shammash): instead of a va_list we can use a getsubopt(3) string, and add
 * the tokens and their description to the module
 */
int vde_component_init(vde_component *component, vde_quark qname,
                       vde_module *module, vde_context *ctx, va_list args)
{
  int retval;

  vde_return_val_if_fail(component != NULL, -1);
  vde_return_val_if_fail(module != NULL, -1);

  component->ctx = ctx;
  component->qname = qname;
  component->kind = vde_module_get_kind(module);
  // XXX using the family string directly from module
  component->family = (char *)vde_module_get_family(module);
  component->cops = vde_module_get_component_ops(module);
  component->commands = vde_hash_init();
  component->signals = vde_hash_init();

  retval = component->cops->init(component, args);
  if (retval) {
    vde_error("%s: cannot initialize component, init returned %d",
              __PRETTY_FUNCTION__, retval);
    return -2;
  }
  component->initialized = true;
  return 0;
}

void vde_component_fini(vde_component *component)
{
  vde_return_if_fail(component != NULL);
  vde_return_if_fail(component->initialized == true);

  /* XXX(godog): switch this on only if debugging
  if (component->refcount) {
    vde_warning("%s: component reference count %d, cannot fini",
                __PRETTY_FUNCTION__, component->refcount);
  }
  */

  // XXX clean these up before delete
  //vde_hash_delete(component->commands);
  //vde_hash_delete(component->signals);

  component->cops->fini(component);

  component->initialized = false;
}

void vde_component_delete(vde_component *component)
{
  vde_return_if_fail(component != NULL);
  vde_return_if_fail(component->initialized == false);

  vde_free(component);
}

int vde_component_get(vde_component *component, int *count)
{
  vde_return_val_if_fail(component != NULL, -1);

  component->refcount++;
  if(count) {
      *count = component->refcount;
  }
  return 0;
}

int vde_component_put(vde_component *component, int *count)
{
  vde_return_val_if_fail(component != NULL, -1);

  component->refcount--;
  if(count) {
      *count = component->refcount;
  }
  return 0;
}

int vde_component_put_if_last(vde_component *component, int *count)
{
  vde_return_val_if_fail(component != NULL, -1);

  if (component->refcount != 1) {
    return 1;
  } else {
    return vde_component_put(component, count);
  }
}

void *vde_component_get_priv(vde_component *component)
{
  vde_return_val_if_fail(component != NULL, NULL);

  return component->priv;
}

void vde_component_set_priv(vde_component *component, void *priv)
{
  vde_return_if_fail(component != NULL);

  component->priv = priv;
}

vde_context *vde_component_get_context(vde_component *component)
{
  vde_return_val_if_fail(component != NULL, NULL);

  return component->ctx;
}

vde_component_kind vde_component_get_kind(vde_component *component)
{
  vde_return_val_if_fail(component != NULL, -1);

  return component->kind;
}

vde_quark vde_component_get_qname(vde_component *component)
{
  vde_return_val_if_fail(component != NULL, -1);

  return component->qname;
}

const char *vde_component_get_name(vde_component *component)
{
  vde_return_val_if_fail(component != NULL, NULL);

  return vde_quark_to_string(component->qname);
}

int vde_component_commands_register(vde_component *component,
                                    vde_command *commands)
{
  vde_return_val_if_fail(component != NULL, -1);
  vde_return_val_if_fail(commands != NULL, -2);

  while (vde_command_get_name(commands) != NULL) {
    if (vde_component_command_add(component, commands)) {
      return -3;
    }

    commands++;
  }

  return 0;
}

int vde_component_command_add(vde_component *component,
                              vde_command *command)
{
  const char *cmd_name;
  vde_quark qname;

  vde_return_val_if_fail(component != NULL, -1);
  vde_return_val_if_fail(command != NULL, -2);

  cmd_name = vde_command_get_name(command);
  qname = vde_quark_from_string(cmd_name);

  if (vde_hash_lookup(component->commands, (long)qname)) {
    vde_error("%s: command %s already registered",
              __PRETTY_FUNCTION__, cmd_name);
    return -3;
  }

  vde_hash_insert(component->commands, (long)qname, command);

  return 0;
}

int vde_component_command_del(vde_component *component,
                              vde_command *command)
{
  const char *cmd_name;
  vde_quark qname;

  vde_return_val_if_fail(component != NULL, -1);
  vde_return_val_if_fail(command != NULL, -2);

  cmd_name = vde_command_get_name(command);
  qname = vde_quark_try_string(cmd_name);

  if (!vde_hash_remove(component->commands, (long)qname)) {
    vde_error("%s: unable to remove command %s",
              __PRETTY_FUNCTION__, cmd_name);
    return -3;
  }

  return 0;
}

vde_command *vde_component_command_get(vde_component *component,
                                       const char *name)
{
  vde_quark qname;

  vde_return_val_if_fail(component != NULL, NULL);
  vde_return_val_if_fail(name != NULL, NULL);

  qname = vde_quark_try_string(name);

  return vde_hash_lookup(component->commands, (long)qname);
}

/**
* @brief List all commands of a component
*
* @param component The component
*
* @return A null terminated array of commands
*/
vde_command **vde_component_commands_list(vde_component *component);

int vde_component_signals_register(vde_component *component,
                                   vde_signal *signals)
{
  vde_return_val_if_fail(component != NULL, -1);
  vde_return_val_if_fail(signals != NULL, -2);

  while (vde_signal_get_name(signals) != NULL) {
    if (vde_component_signal_add(component, signals)) {
      return -3;
    }

    signals++;
  }

  return 0;
}

int vde_component_signal_add(vde_component *component,
                             vde_signal *signal)
{
  const char *sig_name;
  vde_quark qname;

  vde_return_val_if_fail(component != NULL, -1);
  vde_return_val_if_fail(signal != NULL, -2);

  // XXX: signals must be duplicated before adding them to component->signals
  // hash otherwise two components of the same family will share the list of
  // callbacks
  sig_name = vde_signal_get_name(signal);
  qname = vde_quark_from_string(sig_name);

  if (vde_hash_lookup(component->signals, (long)qname)) {
    vde_error("%s: signal %s already registered",
              __PRETTY_FUNCTION__, sig_name);
    return -3;
  }

  vde_hash_insert(component->signals, (long)qname, signal);

  return 0;
}

int vde_component_signal_del(vde_component *component,
                             vde_signal *signal)
{
  const char *sig_name;
  vde_quark qname;

  vde_return_val_if_fail(component != NULL, -1);
  vde_return_val_if_fail(signal != NULL, -2);

  sig_name = vde_signal_get_name(signal);
  qname = vde_quark_try_string(sig_name);

  if (!vde_hash_remove(component->signals, (long)qname)) {
    vde_error("%s: unable to remove signal %s",
              __PRETTY_FUNCTION__, sig_name);
    return -3;
  }

  vde_signal_fini(signal, component);

  return 0;
}

vde_signal *vde_component_signal_get(vde_component *component,
                                     const char *name)
{
  vde_quark qname;

  vde_return_val_if_fail(component != NULL, NULL);
  vde_return_val_if_fail(name != NULL, NULL);

  qname = vde_quark_try_string(name);

  return vde_hash_lookup(component->signals, (long)qname);
}

/**
* @brief List all signals of a component
*
* @param component The component
*
* @return A null terminated array of signals
*/
vde_signal **vde_component_signals_list(vde_component *component);

int vde_component_signal_attach(vde_component *component, const char *signal,
                                vde_signal_cb cb,
                                vde_signal_destroy_cb destroy_cb,
                                void *data)
{
  vde_signal *sig;

  vde_return_val_if_fail(cb != NULL, -1);

  sig = vde_component_signal_get(component, signal);

  if (!sig) {
    return -2;
  }

  return vde_signal_attach(sig, cb, destroy_cb, data);
}

int vde_component_signal_detach(vde_component *component, const char *signal,
                                vde_signal_cb cb,
                                vde_signal_destroy_cb destroy_cb,
                                void *data)
{
  vde_signal *sig;

  vde_return_val_if_fail(cb != NULL, -1);

  sig = vde_component_signal_get(component, signal);

  if (!sig) {
    return -2;
  }

  return vde_signal_detach(sig, cb, destroy_cb, data);
}

void vde_component_signal_raise(vde_component *component, const char *signal,
                               vde_sobj *info)
{
  vde_signal *sig;

  sig = vde_component_signal_get(component, signal);

  if (!sig) {
    return;
  }

  vde_signal_raise(sig, info, component);
}

void vde_component_set_conn_manager_ops(vde_component *cm, cm_listen listen,
                                        cm_connect connect)
{
  vde_return_if_fail(cm != NULL);
  vde_return_if_fail(listen != NULL);
  vde_return_if_fail(connect != NULL);

  cm->cm_connect = connect;
  cm->cm_listen = listen;
}

void vde_component_set_transport_ops(vde_component *transport,
                                     tr_listen listen,
                                     tr_connect connect)
{
  vde_return_if_fail(transport != NULL);
  vde_return_if_fail(listen != NULL);
  vde_return_if_fail(connect != NULL);

  transport->tr_connect = connect;
  transport->tr_listen = listen;
}

void vde_component_set_engine_ops(vde_component *engine, eng_new_conn new_conn)
{
  vde_return_if_fail(engine != NULL);
  vde_return_if_fail(engine->kind == VDE_ENGINE);
  vde_return_if_fail(new_conn != NULL);

  engine->eng_new_conn = new_conn;
}

// XXX add application callback on accept and/or on error?
int vde_component_conn_manager_listen(vde_component *cm)
{
  if (cm == NULL) {
    vde_error("%s: NULL component", __PRETTY_FUNCTION__);
    return -1;
  }
  if (cm->kind != VDE_CONNECTION_MANAGER) {
    vde_error("%s: component is not a conn. manager", __PRETTY_FUNCTION__);
    return -2;
  }
  return cm->cm_listen(cm);
}

/* TODO: memory handling, what happens to local_request/remote_request? should
 * the caller hand them over? */
int vde_component_conn_manager_connect(vde_component *cm,
                                       vde_request *local,
                                       vde_request *remote,
                                       vde_connect_success_cb success_cb,
                                       vde_connect_error_cb error_cb,
                                       void *arg)
{
  if (cm == NULL) {
    vde_error("%s: NULL component", __PRETTY_FUNCTION__);
    return -1;
  }
  if (cm->kind != VDE_CONNECTION_MANAGER) {
    vde_error("%s: component is not a conn. manager", __PRETTY_FUNCTION__);
    return -2;
  }
  return cm->cm_connect(cm, local, remote, success_cb, error_cb, arg);
}

int vde_component_transport_listen(vde_component *transport)
{
  vde_return_val_if_fail(transport != NULL, -1);
  vde_return_val_if_fail(transport->kind == VDE_TRANSPORT, -1);

  return transport->tr_listen(transport);
}

int vde_component_transport_connect(vde_component *transport,
                                    vde_connection *conn)
{
  vde_return_val_if_fail(transport != NULL, -1);
  vde_return_val_if_fail(transport->kind == VDE_TRANSPORT, -1);
  vde_return_val_if_fail(conn != NULL, -1);

  return transport->tr_connect(transport, conn);
}

int vde_component_engine_new_conn(vde_component *engine, vde_connection *conn,
                                  vde_request *req)
{
  vde_return_val_if_fail(engine != NULL, -1);
  vde_return_val_if_fail(engine->kind == VDE_ENGINE, -1);
  vde_return_val_if_fail(conn != NULL, -1);

  return engine->eng_new_conn(engine, conn, req);
}

void vde_component_set_transport_cm_callbacks(vde_component *transport,
                                              cm_connect_cb connect_cb,
                                              cm_accept_cb accept_cb,
                                              cm_error_cb error_cb, void *arg)
{
  vde_return_if_fail(transport != NULL);
  vde_return_if_fail(transport->kind == VDE_TRANSPORT);
  vde_return_if_fail(connect_cb != NULL &&
                     accept_cb != NULL &&
                     error_cb != NULL);

  transport->cm_connect_cb = connect_cb;
  transport->cm_accept_cb = accept_cb;
  transport->cm_error_cb = error_cb;
  transport->cm_cb_arg = arg;
}

void vde_component_transport_call_cm_connect_cb(vde_component *transport,
                                                vde_connection *conn)
{
  vde_return_if_fail(transport != NULL);
  vde_return_if_fail(transport->kind == VDE_TRANSPORT);
  vde_return_if_fail(conn != NULL);

  transport->cm_connect_cb(conn, transport->cm_cb_arg);
}

void vde_component_transport_call_cm_accept_cb(vde_component *transport,
                                               vde_connection *conn)
{
  vde_return_if_fail(transport != NULL);
  vde_return_if_fail(transport->kind == VDE_TRANSPORT);
  vde_return_if_fail(conn != NULL);

  transport->cm_accept_cb(conn, transport->cm_cb_arg);
}

void vde_component_transport_call_cm_error_cb(vde_component *transport,
                                              vde_connection *conn,
                                              vde_transport_error err)
{
  vde_return_if_fail(transport != NULL);
  vde_return_if_fail(transport->kind == VDE_TRANSPORT);
  vde_return_if_fail(conn != NULL);

  transport->cm_error_cb(conn, err, transport->cm_cb_arg);
}

