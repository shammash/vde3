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

#include <vde3/priv/common.h>

#include <vde3/priv/context.h>

struct vde_context {
  bool initialized,
  event_handler *event_handler,
  vde_dataset *components,
  vde_list *modules,
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
  char *comp_family, *module_family, *comp_module_family;
  vde_list *iter;
  vde_module *module = NULL;

  if (ctx == NULL || ctx->initialized != true) {
    vde_error("%s: cannot lookup module, context not initialized",
              __PRETTY_FUNCTION__);
    return -1;
  }
  comp_family = vde_comparable_string(family);
  iter = vde_list_first(ctx->modules);
  while(iter != NULL) {
    module = vde_list_get_data(iter);
    module_kind = vde_module_get_kind(module);
    module_family = vde_module_get_family(module);
    comp_module_family = vde_comparable_string(module_family);
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

int vde_context_init(vde_context *ctx, event_handler *handler)
{
  if (ctx == NULL || handler == NULL) {
    vde_error("%s: cannot initialize context", __PRETTY_FUNCTION__);
    return -1;
  }
  ctx->event_handler = handler;
  ctx->modules = NULL;
  ctx->initialized = true;
  return 0;
}

int vde_context_fini(vde_context *ctx)
{
  if (ctx == NULL || ctx->initialized != true) {
    vde_error("%s: cannot finalize context", __PRETTY_FUNCTION__);
    return -1;
  }
  ctx->event_handler = NULL;
  /* XXX(shammash): modules here are not deleted because are supposed to be
   *                always reachable via a global name in the module object
   */
  vde_list_delete(ctx->modules);
  ctx->modules = NULL;
  ctx->initialized = false;
  return 0;
}

int vde_context_delete(vde_context *ctx)
{
  if (ctx == NULL || ctx->initialized != false) {
    vde_error("%s: cannot delete context", __PRETTY_FUNCTION__);
    return -1;
  }
  vde_free(vde_context, ctx);
  return 0;
}

/**
* @brief Alloc a new VDE 3 component
*
* @param ctx The context where to allocate this component
* @param kind The component kind  (transport, engine, ...)
* @param family The component family (unix, data, ...)
* @param name The component unique name
* @param component reference to new component pointer
*
* @return zero on success, otherwise an error code
*/
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
  qname = vde_quark_try_string(name);
  if (vde_hash_lookup(ctx->components, qname)) {
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
  if (vde_component_init(*component, module, qname, arg)) {
    vde_component_delete(component);
    vde_error("%s: cannot init new component", __PRETTY_FUNCTION__);
    return -5;
  }
  va_end(arg);
  vde_hash_insert(ctx->components, qname, *component);
  vde_component_get(*component, &refcount);
  return 0;
}

/**
* @brief Component lookup
*        Lookup for a component by name
*
* @param ctx The context where to lookup
* @param name The component name
*
* @return the component, NULL if not found
*/
vde_component* vde_context_get_component(vde_context *ctx, const char *name);

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
vde_list *vde_context_list_components(vde_context *ctx);

/**
* @brief Remove a component from a given context
*
* @param ctx The context where to remove from
* @param component the component pointer to remove
*
* @return zero on success, otherwise an error code
*/
/* XXX(shammash):
 *  - this needs to check reference counter and fail if the component is in use
 *    by another component
 */
int vde_context_component_del(vde_context *ctx, vde_component *component);

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
  char *family;

  if (ctx == NULL || ctx->initialized != true) {
    vde_error("%s: cannot add module, context not initialized",
              __PRETTY_FUNCTION__);
    return -1;
  }
  kind = vde_module_get_kind(module);
  family = vde_module_get_family(module);
  if(vde_context_lookup_module(ctx, kind, family)) {
    vde_error("%s: module for kind %d family %s already registered",
              __PRETTY_FUNCTION__, kind, comp_family);
    return -2;
  } else {
    ctx->modules = vde_list_prepend(ctx->modules, module);
    return 0;
  }
}
