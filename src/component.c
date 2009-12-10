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
  struct component_ops *cops;
  union ops {
    struct transport_ops *transport;
    struct engine_ops *engine;
    struct connection_manager_ops *connection_manager;
  },
  vde_quark *qname,
  vde_component_kind kind,
  char *family,
  int refcnt,
  struct vde_command commands[],
  struct vde_signal signals[],
  void *priv;
  bool initialized;
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
int vde_component_init(vde_component *component, vde_quark quark,
                       vde_module *module, va_list args)
{
  if (component == NULL) {
    vde_error("%s: cannot initialize NULL component", __PRETTY_FUNCTION__);
    return -1;
  }
  // set kind family name, component_ops
  // call module component init with args
}

