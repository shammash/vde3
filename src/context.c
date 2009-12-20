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

#include <vde3/common.h>
#include <vde3/component.h>
#include <vde3/context.h>

struct vde_context {
  bool initialized;
  vde_event_handler *event_handler; /* XXX(shammash): consider embedding the
                                   structure in the context to save 1 pointer
                                   jump for each event add */
  // hash table vde_quark component_name: vde_component *component
  vde_hash *components; /* XXX(shammash): couple this hash with a list to keep
                           an order, needed in save configuration */
  // list of vde_module*
  vde_list *modules;
  // configuration path
  // list of startup commands (from configuration)
};

/**
 * @brief Lookup a vde 3 module in the context
 *
 * @param ctx The context to lookup in
 * @param kind The module kind
 * @param family The module family
 *
 * @return The module or NULL if not present
 */
static vde_module *vde_context_lookup_module(vde_context *ctx,
                                             vde_component_kind kind,
                                             const char *family)
{
  vde_component_kind module_kind;
  const char *module_family;
  vde_quark comp_family, comp_module_family;
  vde_list *iter;
  vde_module *module = NULL;

  vde_return_val_if_fail(ctx != NULL, NULL);
  vde_return_val_if_fail(ctx->initialized == true , NULL);

  comp_family = vde_quark_from_string(family);
  iter = vde_list_first(ctx->modules);
  while(iter != NULL) {
    module = vde_list_get_data(iter);
    module_kind = vde_module_get_kind(module);
    module_family = vde_module_get_family(module);
    comp_module_family = vde_quark_from_string(module_family);
    if (kind == module_kind && comp_family == comp_module_family) {
      return module;
    }
    iter = vde_list_next(iter);
  }
  return NULL;
}

int vde_context_new(vde_context **ctx)
{
  if (!ctx) {
    vde_error("%s: context pointer reference is NULL", __PRETTY_FUNCTION__);
    return -1;
  }
  *ctx = (vde_context *)vde_calloc(sizeof(vde_context));
  if (*ctx == NULL) {
    vde_error("%s: cannot create context", __PRETTY_FUNCTION__);
    return -2;
  }
  return 0;
}

int vde_context_init(vde_context *ctx, vde_event_handler *handler)
{
  if (ctx == NULL || handler == NULL) {
    vde_error("%s: cannot initialize context", __PRETTY_FUNCTION__);
    return -1;
  }
  ctx->event_handler = handler;
  ctx->modules = NULL;
  ctx->components = vde_hash_init();
  ctx->initialized = true;
  return 0;
}

void vde_context_fini(vde_context *ctx)
{
  if (ctx == NULL || ctx->initialized != true) {
    vde_error("%s: cannot finalize context", __PRETTY_FUNCTION__);
    return;
  }
  ctx->event_handler = NULL;
  /* XXX(shammash): modules here are not deleted because are supposed to be
   *                always reachable via a global name in the module object
   */
  vde_list_delete(ctx->modules);
  ctx->modules = NULL;
  // XXX: here components should be fini-shed, but that requires doing two
  // passes, first fini the connection managers and then the other components
  // because CM has dependency on both transport and engine.

  // another option would be to have a global connection manager which manages
  // transports associated with engines, this should be investigated.
  vde_hash_delete(ctx->components);
  ctx->initialized = false;
  return;
}

void vde_context_delete(vde_context *ctx)
{
  if (ctx == NULL || ctx->initialized != false) {
    vde_error("%s: cannot delete context", __PRETTY_FUNCTION__);
    return;
  }
  vde_free(ctx);
  return;
}

int vde_context_new_component(vde_context *ctx, vde_component_kind kind,
                               const char *family, const char *name,
                               vde_component **component, ...)
{
  vde_quark qname;
  vde_module *module;
  va_list arg;
  int refcount;

  if (ctx == NULL || ctx->initialized != true) {
    vde_error("%s: cannot create new component, context not initialized",
              __PRETTY_FUNCTION__);
    return -1;
  }
  // TODO(shammash): check name is not 'context' or 'commands' for config
  if (vde_context_get_component(ctx, name)) {
    vde_error("%s: cannot create new component, %s already exists",
              __PRETTY_FUNCTION__);
    return -2;
  }
  if ((module=vde_context_lookup_module(ctx, kind, family)) == NULL) {
    vde_error("%s: cannot create new component, module %d %s not found",
              __PRETTY_FUNCTION__, kind, family);
    return -3;
  }
  if (vde_component_new(component)) {
    vde_error("%s: cannot allocate new component", __PRETTY_FUNCTION__);
    return -4;
  }
  qname = vde_quark_from_string(name);
  va_start(arg, component);
  if (vde_component_init(*component, qname, module, arg)) {
    vde_component_delete(*component);
    vde_error("%s: cannot init new component", __PRETTY_FUNCTION__);
    return -5;
  }
  va_end(arg);
  vde_hash_insert(ctx->components, qname, *component);
  vde_component_get(*component, &refcount);
  return 0;
}

vde_component* vde_context_get_component(vde_context *ctx, const char *name)
{
  vde_quark qname;

  if (ctx == NULL || ctx->initialized != true) {
    vde_error("%s: cannot get component, context not initialized",
              __PRETTY_FUNCTION__);
    return NULL;
  }
  qname = vde_quark_try_string(name);
  return vde_context_get_component_by_qname(ctx, qname);
}

inline vde_component* vde_context_get_component_by_qname(vde_context *ctx,
                                                         vde_quark qname)
{
  return vde_hash_lookup(ctx->components, qname);
}

/**
* @brief Get all the components of a context
*
* @param ctx The context holding the components
*
* @return a list of all the components, NULL if there are no components
*/
/* XXX(shammash):
 *  - change list with something which doesn't need to be generated
 *  - maybe just names are needed..
 */
/* XXX(godog): this is supposed to be a command for the rpcengine */
/* XXX(godog): this function is not in vde3.h */
vde_list *vde_context_list_components(vde_context *ctx);

int vde_context_component_del(vde_context *ctx, vde_component *component)
{
  int not_in_use;
  vde_quark qname;

  if (ctx == NULL || ctx->initialized != true) {
    vde_error("%s: cannot delete component, context not initialized",
              __PRETTY_FUNCTION__);
    return -1;
  }
  if (component == NULL) {
    vde_error("%s: cannot delete component, component is NULL",
              __PRETTY_FUNCTION__);
    return -2;
  }
  qname = vde_component_get_qname(component);
  if (vde_context_get_component_by_qname(ctx, qname) == NULL) {
    vde_error("%s: cannot delete component, component not found",
              __PRETTY_FUNCTION__);
    return -3;
  }
  not_in_use = vde_component_put_if_last(component, NULL);
  if (!not_in_use) {
    vde_error("%s: cannot delete component, component is in use",
              __PRETTY_FUNCTION__);
    return -4;
  }
  if (!vde_hash_remove(ctx->components, qname)) {
    vde_component_get(component, NULL);
    vde_error("%s: cannot delete component, error removing from hash table",
              __PRETTY_FUNCTION__);
    return -5;
  }

  // here the component is deleted because it doesn't make sense to have it out
  // of the vde_context
  vde_component_fini(component);
  vde_component_delete(component);

  return 0;
}

/**
* @brief Save current configuration in a file
*
* @param ctx The context to save the configuration from
* @param file The file to save the configuration to
*
* @return zero on success, otherwise an error code
*/
int vde_context_config_save(vde_context *ctx, const char* file);

/**
* @brief Load configuration from a file
*
* @param ctx The context to load the configuration into
* @param file The file to read the configuration from
*
* @return zero on success, otherwise an error code
*/
int vde_context_config_load(vde_context *ctx, const char* file);

int vde_context_register_module(vde_context *ctx, vde_module *module)
{
  vde_component_kind kind;
  const char *family;
  component_ops *module_cops;

  vde_return_val_if_fail(ctx != NULL, -1);
  vde_return_val_if_fail(ctx->initialized == true, -1);

  kind = vde_module_get_kind(module);
  family = vde_module_get_family(module);
  if(vde_context_lookup_module(ctx, kind, family)) {
    vde_error("%s: module for kind %d family %s already registered",
              __PRETTY_FUNCTION__, kind, family);
    return -2;
  }

  // module's component_ops sanity checks
  module_cops = vde_module_get_component_ops(module);
  vde_return_val_if_fail(module_cops != NULL, -3);
  vde_return_val_if_fail(module_cops->init != NULL, -3);
  vde_return_val_if_fail(module_cops->fini != NULL, -3);

  ctx->modules = vde_list_prepend(ctx->modules, module);
  return 0;
}

void *vde_context_event_add(vde_context *ctx, int fd, short events,
                            const struct timeval *timeout,
                            event_cb cb, void *arg)
{
  return ctx->event_handler->event_add(fd, events, timeout, cb, arg);
}

void vde_context_event_del(vde_context *ctx, void *event)
{
  ctx->event_handler->event_del(event);
}

void *vde_context_timeout_add(vde_context *ctx, short events,
                              const struct timeval *timeout,
                              event_cb cb, void *arg)
{
  return ctx->event_handler->timeout_add(timeout, events, cb, arg);
}

void vde_context_timeout_del(vde_context *ctx, void *timeout)
{
  ctx->event_handler->timeout_del(timeout);
}

