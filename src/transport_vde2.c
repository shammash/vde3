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

#include <vde3.h>

#include <vde3/priv/module.h>
#include <vde3/priv/component.h>

struct transport_ops {

} transport_vde2_ops;

struct component_ops {
  .init = transport_vde2_init,
  .fini = transport_vde2_fini,
  .get_configuration = transport_vde2_get_configuration,
  .set_configuration = transport_vde2_set_configuration,
  .get_policy = transport_vde2_get_policy,
  .set_policy = transport_vde2_set_policy,
} transport_vde2_component_ops;

struct vde_module {
  kind = VDE_TRANSPORT,
  family = "vde2",
  cops = transport_vde2_component_ops,
} transport_vde2_module;

int transport_vde2_module_init(vde_context *ctx)
{
  return vde_context_register_module(ctx, &transport_vde2_module);
}
