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

#ifndef __VDE3_CONTEXT_H__
#define __VDE3_CONTEXT_H__


struct vde_context;

/**
* @brief VDE 3 Context
*/
typedef struct vde_context vde_context;

/**
* @brief Alloc a new VDE 3 context
*
* @param ctx reference to new context pointer
*
* @return zero on success, otherwise an error code
*/
int vde_context_new(vde_context **ctx);

/**
* @brief Initialize VDE 3 context
*
* @param ctx The context to initialize
* @param handler An implementation of event_handler to use
*
* @return zero on success, otherwise an error code
*/
int vde_context_init(vde_context *ctx, event_handler *handler);

/**
* @brief Stop and reset a VDE 3 context
*
* @param ctx The context to fini
*/
void vde_context_fini(vde_context *ctx);

/**
* @brief Deallocate a VDE 3 context
*
* @param ctx The context to delete
*/
void vde_context_delete(vde_context *ctx);

/**
* @brief Alloc a new VDE 3 component
*
* @param ctx The context where to allocate this component
* @param kind The component kind  (transport, engine, ...)
* @param family The component family (unix, data, ...)
* @param name The component unique name (NULL for auto generation)
* @param component reference to new component pointer
*
* @return zero on success, otherwise an error code
*/
int vde_context_new_component(vde_context *ctx, const char *kind,
                               const char *family, const char *name,
                               vde_component **component);

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

#endif /* __VDE3_CONTEXT_H__ */

