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

#ifndef __VDE3_CONTEXT_H__
#define __VDE3_CONTEXT_H__

#include <vde3/module.h>

struct vde_context {
  int initialized;
  vde_event_handler event_handler;
  // hash table vde_quark component_name: vde_component *component
  vde_hash *components; /* XXX(shammash): couple this hash with a list to keep
                           an order, needed in save configuration */
  // list of vde_module*
  vde_list *modules;
  // configuration path
  // list of startup commands (from configuration)
};

/**
 * @brief Register a vde_module inside the context
 *
 * @param ctx The context to register the module into
 * @param module The module to register
 *
 * @return zero on success, otherwise an error code
 */
int vde_context_register_module(vde_context *ctx, vde_module *module);

static inline void *vde_context_event_add(vde_context *ctx, int fd,
                                          short events,
                                          const struct timeval *timeout,
                                          event_cb cb, void *arg)
{
  vde_assert(ctx != NULL);
  vde_assert(ctx->initialized == 1);

  return ctx->event_handler.event_add(fd, events, timeout, cb, arg);
}

static inline void vde_context_event_del(vde_context *ctx, void *event)
{
  vde_assert(ctx != NULL);
  vde_assert(ctx->initialized == 1);
  vde_assert(event != NULL);

  ctx->event_handler.event_del(event);
}

static inline void *vde_context_timeout_add(vde_context *ctx, short events,
                                            const struct timeval *timeout,
                                            event_cb cb, void *arg)
{
  vde_assert(ctx != NULL);
  vde_assert(ctx->initialized == 1);

  return ctx->event_handler.timeout_add(timeout, events, cb, arg);
}

static inline void vde_context_timeout_del(vde_context *ctx, void *timeout)
{
  vde_assert(ctx != NULL);
  vde_assert(ctx->initialized == 1);
  vde_assert(timeout != NULL);

  ctx->event_handler.timeout_del(timeout);
}

#endif /* __VDE3_CONTEXT_H__ */
