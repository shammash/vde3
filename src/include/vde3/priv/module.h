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

#ifndef __VDE3_PRIV_MODULE_H__
#define __VDE3_PRIV_MODULE_H__

// XXX as of now modules can't be unloaded because components are using its
// functions once they are in use.
// supporting module removal will probably need a refcount in the module
// tracking how many components are using it

// XXX(shammash): consider adding here a union with struct ops which are
//                kind-related, it should make new components creation easier
struct vde_module {
  vde_component_kind kind,
  char* family,
  struct component_ops *cops,
};

typedef struct vde_module vde_module;

vde_component_kind vde_module_get_kind(vde_module *module);

const char *vde_module_get_family(vde_module *module);

/**
 * @brief Function invoked when initializing a module, must be exported by
 * plugin modules.
 *
 * @param ctx The context
 *
 * @return zero on success, an error code otherwise
 */
int vde_module_init(vde_context *ctx);

#endif /* __VDE3_PRIV_MODULE_H__ */
