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


/**
 * @brief Register a vde_module inside the context
 *
 * @param ctx The context to register the module into
 * @param module The module to register
 *
 * @return zero on success, otherwise an error code
 */
int vde_context_register_module(vde_context *ctx, vde_module *module);


#endif /* __VDE3_CONTEXT_H__ */
