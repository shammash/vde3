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

#include <vde3/priv/component.h>

// per adesso usiamo le union, poi vediamo come mettere qualcosa che permetta
// di avere sotto-sottoclassi in un modo piu` pulito (adesso non si riesce a
// fare una sottoclasse di transport.. ma nemmeno ci serve per ora)
struct vde_component {
  vde_context *ctx,
  struct component_ops *cops,
  vde_quark qname,
  vde_component_kind kind,
  vde_char *family,
  int refcnt,
  struct vde_command commands[],
  struct vde_signal signals[],
  void *priv,
  bool initialized,
  // Ops connection_manager specific:
  cm_listen cm_listen,
  cm_connect cm_connect,
  // Ops transport specific:
  // Ops engine specific:
};

int vde_component_new(vde_component **component)
{
  if (!component) {
    vde_error("%s: component pointer reference is NULL", __PRETTY_FUNCTION__);
    return -1;
  }
  *component = (vde_component *)vde_calloc(sizeof(vde_component));
  if (*component == NULL) {
    vde_error("%s: cannot create component", __PRETTY_FUNCTION__);
    return -2;
  }
  return 0;
}

/* XXX(godog): add to the component something like a format string to validate
 * va_list?
 */
int vde_component_init(vde_component *component, vde_quark qname,
                       vde_module *module, va_list args)
{
  int retval;

  if (component == NULL) {
    vde_error("%s: cannot initialize NULL component", __PRETTY_FUNCTION__);
    return -1;
  }

  component->qname = qname;
  component->kind = vde_module_get_kind(module);
  component->family = vde_module_get_family(module);
  component->cops = vde_module_get_component_ops(module);

  // XXX(godog): init can be NULL here? needs checking
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
  if (component == NULL || component->initialized != true) {
    vde_error("%s: component not initialized or NULL", __PRETTY_FUNCTION__);
    return;
  }

  /* XXX(godog): switch this on only if debugging
  if (component->refcount) {
    vde_warning("%s: component reference count %d, cannot fini",
                __PRETTY_FUNCTION__, component->refcount);
  }
  */

  component->cops->fini(component);

  component->initialized = false;
}

void vde_component_delete(vde_component *component)
{
  if (component == NULL || component->initialized != false) {
    vde_error("%s: component initialized or NULL", __PRETTY_FUNCTION__);
    return;
  }

  vde_free(component);
}

int vde_component_get(vde_component *component, int *count)
{
  if (component == NULL) {
    vde_error("%s: cannot get NULL component", __PRETTY_FUNCTION__);
    return -1;
  }
  component->refcount++;
  if(count) {
      *count = component->refcount;
  }
  return 0;
}

int vde_component_put(vde_component *component, int *count)
{
  if (component == NULL) {
    vde_error("%s: cannot put NULL component", __PRETTY_FUNCTION__);
    return -1;
  }
  component->refcount--;
  if(count) {
      *count = component->refcount;
  }
  return 0;
}

int vde_component_put_if_last(vde_component *component, int *count)
{
  if (component == NULL) {
    vde_error("%s: cannot put NULL component", __PRETTY_FUNCTION__);
    return -1;
  }
  if (component->refcount != 1) {
    return 1;
  } else {
    return vde_component_put(component, count);
  }
}

void *vde_component_get_priv(vde_component *component)
{
  if (component == NULL) {
    vde_error("%s: NULL component", __PRETTY_FUNCTION__);
    return -1;
  }
  return component->priv;
}

void vde_component_set_priv(vde_component *component, void *priv)
{
  if (component == NULL) {
    vde_error("%s: NULL component", __PRETTY_FUNCTION__);
    return -1;
  }
  component->priv = priv;
}

vde_context *vde_component_get_context(vde_component *component)
{
  if (component == NULL) {
    vde_error("%s: NULL component", __PRETTY_FUNCTION__);
    return -1;
  }
  return component->ctx;
}

vde_component_kind vde_component_get_kind(vde_component *component)
{
  if (component == NULL) {
    vde_error("%s: NULL component", __PRETTY_FUNCTION__);
    return -1;
  }
  return component->kind;
}

vde_quark vde_component_get_qname(vde_component *component)
{
  if (component == NULL) {
    vde_error("%s: NULL component", __PRETTY_FUNCTION__);
    return -1;
  }
  return component->qname;
}

/**
* @brief vde_component utility to add a command
*
* @param component The component to add the command to
* @param command The command to add
*
* @return zero on success, otherwise an error code
*/
int vde_component_command_add(vde_component *component,
                               vde_command *command);

/**
* @brief vde_component utility to remove a command
*
* @param component The component to remove the command from
* @param command The command to remove
*
* @return zero on success, otherwise an error code
*/
int vde_component_command_del(vde_component *component,
                               vde_command *command);

/**
* @brief Lookup for a command in a component
*
* @param component The component to look into
* @param name The name of the command
*
* @return a vde command, NULL if not found
*/
vde_command *vde_component_command_get(vde_component *component,
                                        const char *name);

/**
* @brief List all commands of a component
*
* @param component The component
*
* @return A null terminated array of commands
*/
vde_command **vde_component_commands_list(vde_component *component);

/**
* @brief vde_component utility to add a signal
*
* @param component The component to add the signal to
* @param signal The signal to add
*
* @return zero on success, otherwise an error code
*/
int vde_component_signal_add(vde_component *component,
                              vde_signal *signal);

/**
* @brief vde_component utility to remove a signal
*
* @param component The component to remove the signal from
* @param signal The signal to remove
*
* @return zero on success, otherwise an error code
*/
int vde_component_signal_del(vde_component *component,
                              vde_signal *signal);

/**
* @brief Lookup for a signal in a component
*
* @param component The component to look into
* @param name The name of the signal
*
* @return a vde signal, NULL if not found
*/
vde_signal *vde_component_signal_get(vde_component *component,
                                       const char *name);

/**
* @brief List all signals of a component
*
* @param component The component
*
* @return A null terminated array of signals
*/
vde_signals **vde_component_signals_list(vde_component *component);

/**
* @brief Signature of a signal callback
*
* @param component The component raising the signal
* @param signal The signal name
* @param infos Serialized signal parameters, if NULL the signal is being
*              destroyed
* @param data Callback private data
*/
void vde_component_signal_callback(vde_component *component,
                                    const char *signal, json_object *infos,
                                    void *data);

/**
* @brief Attach a callback to a signal
*
* @param component The component to receive signals from
* @param signal The signal name
* @param callback The callback function
* @param data Callback private data
*
* @return zero on success, otherwise an error code
*/
int vde_component_signal_attach(vde_component *component, const char *signal,
                                  vde_component_signal_callback (*callback),
                                  void *data);

/**
* @brief Detach a callback from a signal
*
* @param component The component to stop receiving signals from
* @param signal The signal name
* @param callback The callback function to detach
*
* @return zero on success, otherwise an error code
*/
int vde_component_signal_detach(vde_component *component, const char *signal,
                                 vde_component_signal_callback (*callback));

void vde_component_set_conn_manager_ops(vde_component *cm, cm_listen listen,
                                        cm_connect connect)
{
  cm->cm_connect = connect;
  cm->cm_listen = listen;
}

int vde_component_conn_manager_listen(vde_component *cm)
{
  if (cm == NULL) {
    vde_error("%s: NULL component", __PRETTY_FUNCTION__);
    return -1;
  }
  if (cm.type != VDE_CONNECTION_MANAGER) {
    vde_error("%s: component is not a conn. manager", __PRETTY_FUNCTION__);
    return -2;
  }
  return cm->cm_listen(cm);
}

/* TODO: memory handling, what happens to local_request/remote_request? should
 * the caller hand them over? */
int vde_component_conn_manager_connect(vde_component *cm,
                                       vde_request *local,
                                       vde_request *remote)
{
  if (cm == NULL) {
    vde_error("%s: NULL component", __PRETTY_FUNCTION__);
    return -1;
  }
  if (cm.type != VDE_CONNECTION_MANAGER) {
    vde_error("%s: component is not a conn. manager", __PRETTY_FUNCTION__);
    return -2;
  }
  return cm->cm_connect(cm, local, remote);
}

