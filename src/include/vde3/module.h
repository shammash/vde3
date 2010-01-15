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
/**
 * @file
 */

#ifndef __VDE3_MODULE_H__
#define __VDE3_MODULE_H__

#include <vde3.h>

#include <vde3/connection.h>

#include <stdlib.h>

/**
 * @brief This symbol will be searched when loading a module
 */
#ifndef VDE_MODULE_START
#define VDE_MODULE_START vde_module_start
#define VDE_MODULE_START_S "vde_module_start"
#endif

/**
 * @brief Function called on a connection manager to set it in listen mode.
 * Connection managers must implement it.
 *
 * @param cm The Connection Manager
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
typedef int (*cm_listen)(vde_component *cm);

/**
 * @brief Function called on a connection manager to connect it. Connection
 * managers must implement it.
 *
 * @param cm The Connection Manager
 * @param local Connection parameters for local engine
 * @param remote Connection parameters for remote engine
 * @param success_cb The callback to be called upon successfull connect
 * @param error_cb The callback to be called when an error occurs
 * @param arg Callbacks private data
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
typedef int (*cm_connect)(vde_component *cm, vde_request *local,
                          vde_request *remote,
                          vde_connect_success_cb success_cb,
                          vde_connect_error_cb error_cb,
                          void *arg);

/**
 * @brief Function called on an engine to add a new connection. Engines must
 * implement it.
 *
 * @param engine The engine to add the connection to
 * @param conn The connection
 * @param req The connection parameters
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
typedef int (*eng_new_conn)(vde_component *engine, vde_connection *conn,
                            vde_request *req);

/**
 * @brief Function called on a transport to set it in listen mode. Transports
 * must implement it.
 *
 * @param transport The transport
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
typedef int (*tr_listen)(vde_component *transport);

/**
 * @brief Function called on a transport to connect it. Transports must
 * implement it.
 *
 * @param transport The transport
 * @param conn The connection to connect
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
typedef int (*tr_connect)(vde_component *transport, vde_connection *conn);



/**
 * @brief Common component operations.
 */
struct component_ops {
  int (*init)(vde_component*, va_list);
  //!< called when a context init a new component
  void (*fini)(vde_component*);
  //!< called when a context closes a component
  char* get_configuration; //!< called to get a serializable config
  char* set_configuration; //!< called to set a serializable config
  char* get_policy; //!< called to get a serializable policy
  char* set_policy; //!< called to set a serializable policy
};

typedef struct component_ops component_ops;

// XXX as of now modules can't be unloaded because components are using its
// functions once they are in use.
// supporting module removal will probably need a refcount in the module
// tracking how many components are using it

// XXX(shammash): consider adding here a union with struct ops which are
//                kind-related, it should make new components creation easier

/**
 * @brief A vde module.
 *
 * It is dynamically loaded and is used to implement a component.
 */
struct vde_module {
  vde_component_kind kind;
  char* family;
  component_ops *cops;
  cm_connect cm_connect;
  cm_listen cm_listen;
  eng_new_conn eng_new_conn;
  tr_connect tr_connect;
  tr_listen tr_listen;
  void *dlhandle;
};

typedef struct vde_module vde_module;

static inline vde_component_kind vde_module_get_kind(vde_module *module)
{
  return module->kind;
}

static inline const char *vde_module_get_family(vde_module *module)
{
  return module->family;
}

static inline component_ops *vde_module_get_component_ops(vde_module *module)
{
  return module->cops;
}

static inline cm_connect vde_module_get_cm_connect(vde_module *module)
{
  return module->cm_connect;
}

static inline cm_listen vde_module_get_cm_listen(vde_module *module)
{
  return module->cm_listen;
}

static inline eng_new_conn vde_module_get_eng_new_conn(vde_module *module)
{
  return module->eng_new_conn;
}

static inline tr_connect vde_module_get_tr_connect(vde_module *module)
{
  return module->tr_connect;
}

static inline tr_listen vde_module_get_tr_listen(vde_module *module)
{
  return module->tr_listen;
}

/**
 * @brief Load vde modules found in path
 *
 * @param ctx The context to load modules into
 * @param path The search path of modules, if NULL use default path
 *
 * @return 0 if every valid module has been successfully loaded,
 *         -1 on error (and errno set appropriately)
 */
int vde_modules_load(vde_context *ctx, char **path);

#endif /* __VDE3_MODULE_H__ */
