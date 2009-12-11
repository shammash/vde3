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

struct vde_module {
  vde_component_kind kind,
  char* .family,
  struct component_ops *ops, // verra' copiato dal contesto nel componente
                             // vero e proprio quando viene creato
};

typedef struct vde_module vde_module;

vde_component_kind vde_module_get_kind(vde_module *module) {
  return module->kind;
}

const char *vde_module_get_family(vde_module *module) {
  return module->family;
}

// funzione invocata al caricamento del modulo, _deve_ essere chiamata
// "vde_module_init"
int vde_module_init(vde_context *ctx);

// esempio:
struct vde_module {
  char* .kind = "transport",
  char* .family = "unix",
  struct component_ops *ops,
} unixtransport_module;

// function name needs to be the same because we need to dlsym
int vde_module_init(vde_context *ctx) {
  vde_context_register_module(ctx, unixtransport_module);
}

#endif /* __VDE3_PRIV_MODULE_H__ */
