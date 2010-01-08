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
#include <vde3/engine.h>
#include <vde3/transport.h>
#include <vde3/conn_manager.h>

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
  vde_assert(component != NULL);

  *component = (vde_component *)vde_calloc(sizeof(vde_component));
  if (*component == NULL) {
    errno = ENOMEM;
    return -1;
  }

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
  int retval, tmp_errno;

  vde_assert(component != NULL);
  vde_assert(module != NULL);

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
    tmp_errno = errno;
    vde_error("%s: cannot initialize component", __PRETTY_FUNCTION__);
    errno = tmp_errno;
    return -1;
  }

  component->initialized = true;
  return 0;
}

void vde_component_fini(vde_component *component)
{
  vde_assert(component != NULL);
  vde_assert(component->initialized == true);

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
  vde_assert(component != NULL);
  vde_assert(component->initialized == false);

  vde_free(component);
}

void vde_component_get(vde_component *component, int *count)
{
  vde_assert(component != NULL);

  component->refcount++;
  if(count) {
      *count = component->refcount;
  }
}

void vde_component_put(vde_component *component, int *count)
{
  vde_assert(component != NULL);

  component->refcount--;
  if(count) {
      *count = component->refcount;
  }
}

int vde_component_put_if_last(vde_component *component, int *count)
{
  vde_assert(component != NULL);

  if (component->refcount != 1) {
    return 1;
  } else {
    vde_component_put(component, count);
    return 0;
  }
}

void *vde_component_get_priv(vde_component *component)
{
  vde_assert(component != NULL);

  return component->priv;
}

void vde_component_set_priv(vde_component *component, void *priv)
{
  vde_assert(component != NULL);

  component->priv = priv;
}

vde_context *vde_component_get_context(vde_component *component)
{
  vde_assert(component != NULL);

  return component->ctx;
}

vde_component_kind vde_component_get_kind(vde_component *component)
{
  vde_assert(component != NULL);

  return component->kind;
}

vde_quark vde_component_get_qname(vde_component *component)
{
  vde_assert(component != NULL);

  return component->qname;
}

const char *vde_component_get_name(vde_component *component)
{
  vde_assert(component != NULL);

  return vde_quark_to_string(component->qname);
}

int vde_component_commands_register(vde_component *component,
                                    vde_command *commands)
{
  vde_assert(component != NULL);
  vde_assert(commands != NULL);

  while (vde_command_get_name(commands) != NULL) {
    if (vde_component_command_add(component, commands)) {
      return -1;
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

  vde_assert(component != NULL);
  vde_assert(command != NULL);

  cmd_name = vde_command_get_name(command);
  qname = vde_quark_from_string(cmd_name);

  if (vde_hash_lookup(component->commands, (long)qname)) {
    vde_error("%s: command %s already registered",
              __PRETTY_FUNCTION__, cmd_name);
    errno = EEXIST;
    return -1;
  }

  vde_hash_insert(component->commands, (long)qname, command);

  return 0;
}

int vde_component_command_del(vde_component *component,
                              vde_command *command)
{
  const char *cmd_name;
  vde_quark qname;

  vde_assert(component != NULL);
  vde_assert(command != NULL);

  cmd_name = vde_command_get_name(command);
  qname = vde_quark_try_string(cmd_name);

  if (!vde_hash_remove(component->commands, (long)qname)) {
    vde_error("%s: unable to remove command %s",
              __PRETTY_FUNCTION__, cmd_name);
    errno = ENOENT;
    return -1;
  }

  return 0;
}

vde_command *vde_component_command_get(vde_component *component,
                                       const char *name)
{
  vde_quark qname;

  vde_assert(component != NULL);
  vde_assert(name != NULL);

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
  vde_assert(component != NULL);
  vde_assert(signals != NULL);

  while (vde_signal_get_name(signals) != NULL) {
    if (vde_component_signal_add(component, signals)) {
      return -1;
    }

    signals++;
  }

  return 0;
}

int vde_component_signal_add(vde_component *component,
                             vde_signal *signal)
{
  vde_signal *dup;
  const char *sig_name;
  vde_quark qname;

  vde_assert(component != NULL);
  vde_assert(signal != NULL);

  sig_name = vde_signal_get_name(signal);
  qname = vde_quark_from_string(sig_name);

  if (vde_hash_lookup(component->signals, (long)qname)) {
    vde_error("%s: signal %s already registered",
              __PRETTY_FUNCTION__, sig_name);
    errno = EEXIST;
    return -1;
  }

  /* signals are duplicated otherwise two components of the same family end up
   * sharing the same callback list */
  dup = vde_signal_dup(signal);
  if (dup == NULL) {
    vde_error("%s: cannot duplicate signal");
    errno = ENOMEM;
    return -1;
  }

  vde_hash_insert(component->signals, (long)qname, dup);

  return 0;
}

int vde_component_signal_del(vde_component *component,
                             vde_signal *signal)
{
  vde_signal *dup;
  const char *sig_name;
  vde_quark qname;

  vde_assert(component != NULL);
  vde_assert(signal != NULL);

  sig_name = vde_signal_get_name(signal);
  qname = vde_quark_try_string(sig_name);

  if ((dup = vde_hash_lookup(component->signals, (long)qname)) == NULL) {
    vde_error("%s: unable to remove signal %s", __PRETTY_FUNCTION__, sig_name);
    errno = ENOENT;
    return -1;
  }
  vde_hash_remove(component->signals, (long)qname);

  vde_signal_fini(dup, component);
  vde_signal_delete(dup);

  return 0;
}

vde_signal *vde_component_signal_get(vde_component *component,
                                     const char *name)
{
  vde_quark qname;

  vde_assert(component != NULL);
  vde_assert(name != NULL);

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

  vde_assert(cb != NULL);

  sig = vde_component_signal_get(component, signal);

  if (!sig) {
    errno = ENOENT;
    return -1;
  }

  return vde_signal_attach(sig, cb, destroy_cb, data);
}

int vde_component_signal_detach(vde_component *component, const char *signal,
                                vde_signal_cb cb,
                                vde_signal_destroy_cb destroy_cb,
                                void *data)
{
  vde_signal *sig;

  vde_assert(cb != NULL);

  sig = vde_component_signal_get(component, signal);

  if (!sig) {
    errno = ENOENT;
    return -1;
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

/*
 * Engine-specific functions.
 *
 */

void vde_engine_set_ops(vde_component *engine, eng_new_conn new_conn)
{
  vde_assert(engine != NULL);
  vde_assert(engine->kind == VDE_ENGINE);
  vde_assert(new_conn != NULL);

  engine->eng_new_conn = new_conn;
}

int vde_engine_new_connection(vde_component *engine, vde_connection *conn,
                              vde_request *req)
{
  vde_assert(engine != NULL);
  vde_assert(engine->kind == VDE_ENGINE);
  vde_assert(conn != NULL);

  return engine->eng_new_conn(engine, conn, req);
}

/*
 * Transport-specific functions.
 *
 */

void vde_transport_set_ops(vde_component *transport, tr_listen listen,
                           tr_connect connect)
{
  vde_assert(transport != NULL);
  vde_assert(listen != NULL);
  vde_assert(connect != NULL);

  transport->tr_connect = connect;
  transport->tr_listen = listen;
}

int vde_transport_listen(vde_component *transport)
{
  vde_assert(transport != NULL);
  vde_assert(transport->kind == VDE_TRANSPORT);

  return transport->tr_listen(transport);
}

int vde_transport_connect(vde_component *transport, vde_connection *conn)
{
  vde_assert(transport != NULL);
  vde_assert(transport->kind == VDE_TRANSPORT);
  vde_assert(conn != NULL);

  return transport->tr_connect(transport, conn);
}

void vde_transport_set_cm_callbacks(vde_component *transport,
                                    cm_connect_cb connect_cb,
                                    cm_accept_cb accept_cb,
                                    cm_error_cb error_cb, void *arg)
{
  vde_assert(transport != NULL);
  vde_assert(transport->kind == VDE_TRANSPORT);
  vde_assert(connect_cb != NULL &&
                     accept_cb != NULL &&
                     error_cb != NULL);

  transport->cm_connect_cb = connect_cb;
  transport->cm_accept_cb = accept_cb;
  transport->cm_error_cb = error_cb;
  transport->cm_cb_arg = arg;
}

void vde_transport_call_cm_connect_cb(vde_component *transport,
                                      vde_connection *conn)
{
  vde_assert(transport != NULL);
  vde_assert(transport->kind == VDE_TRANSPORT);
  vde_assert(conn != NULL);

  transport->cm_connect_cb(conn, transport->cm_cb_arg);
}

void vde_transport_call_cm_accept_cb(vde_component *transport,
                                     vde_connection *conn)
{
  vde_assert(transport != NULL);
  vde_assert(transport->kind == VDE_TRANSPORT);
  vde_assert(conn != NULL);

  transport->cm_accept_cb(conn, transport->cm_cb_arg);
}

void vde_transport_call_cm_error_cb(vde_component *transport,
                                    vde_connection *conn, int tr_errno)
{
  vde_assert(transport != NULL);
  vde_assert(transport->kind == VDE_TRANSPORT);
  vde_assert(conn != NULL);

  transport->cm_error_cb(conn, tr_errno, transport->cm_cb_arg);
}


/*
 * Connection Manager-specific functions.
 *
 */

void vde_conn_manager_set_ops(vde_component *cm, cm_listen listen,
                              cm_connect connect)
{
  vde_assert(cm != NULL);
  vde_assert(listen != NULL);
  vde_assert(connect != NULL);

  cm->cm_connect = connect;
  cm->cm_listen = listen;
}

// XXX add application callback on accept?
int vde_conn_manager_listen(vde_component *cm)
{
  vde_assert(cm != NULL);
  vde_assert(cm->kind == VDE_CONNECTION_MANAGER);

  return cm->cm_listen(cm);
}

/* XXX: memory handling, what happens to local_request/remote_request? should
 * the caller hand them over? */
int vde_conn_manager_connect(vde_component *cm, vde_request *local,
                             vde_request *remote,
                             vde_connect_success_cb success_cb,
                             vde_connect_error_cb error_cb, void *arg)
{
  vde_assert(cm != NULL);
  vde_assert(cm->kind == VDE_CONNECTION_MANAGER);

  return cm->cm_connect(cm, local, remote, success_cb, error_cb, arg);
}

