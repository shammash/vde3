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
#include <stdio.h>
#include <event.h>

extern vde_event_handler libevent_eh;

int main(int argc, char **argv)
{
  int res;
  vde_context *ctx;
  vde_component *transport, *engine, *cm;

  event_init();

  res = vde_context_new(&ctx);
  if (res) {
    printf("no new ctx, %d\n", res);
  }

  res = vde_context_init(ctx, &libevent_eh);
  if (res) {
    printf("no init ctx: %d\n", res);
  }

  res = vde_context_new_component(ctx, VDE_TRANSPORT, "vde2", "tr1", &transport,
                                  "/tmp/vde3_test");
  if (res) {
    printf("no new transport: %d\n", res);
  }

  res = vde_context_new_component(ctx, VDE_ENGINE, "hub", "e1", &engine);
  if (res) {
    printf("no new engine: %d\n", res);
  }

  res = vde_context_new_component(ctx, VDE_CONNECTION_MANAGER, "default", "cm1",
                                  &cm, transport, engine, 0);
  if (res) {
    printf("no new cm: %d\n", res);
  }

  res = vde_component_conn_manager_listen(cm);
  if (res) {
    printf("no listen on cm: %d\n", res);
  }

  event_dispatch();

  return 0;
}
