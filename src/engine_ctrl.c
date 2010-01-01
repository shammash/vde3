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

#include <vde3/module.h>
#include <vde3/component.h>
#include <vde3/context.h>
#include <vde3/connection.h>

struct ctrl_conn {
  // - vde_connection
  // - in_buf
  // - out_buf
  // - permission level
  // - ctrl engine pointer
};
typedef struct ctrl_conn ctrl_conn;

struct ctrl {
  // - aliases table
  // - vde_component pointer
};
typedef struct ctrl ctrl;

static int ctrl_engine_conn_write(ctrl_conn *cconn, vde_sobj *obj) {
  // serialize obj
  // if error serializing:
  //   fatal: some component has a bug
  // add string to out_buf
  // while out_buf is empty:
  //  get from out_buf MAX(out_buf.length, connection_max_payload)
  //  create a vde_pkt
  //  connection.write(pkt)
  //  (connection here will duplicate pkt...)
  //  free vde_pkt
  //  if write to connection was ok:
  //    remove from out_buf string just sent
  //  else:
  //    break from while
  //    (to try to flush out_buf again we can schedule an event or simply
  //     register a write callback to the connection and try in the callback)
  return 0;
}

static void ctrl_engine_process_sobj(ctrl_conn *cconn, vde_sobj *obj)
{
  // command aliases resolution
  // split component_name/command_name
  // if component_name != ctrl name:
  //   lookup component in context
  // if no component:
  //   reply with "no such component" error
  //   return
  // cmd = component.get_command(command_name)
  // if no cmd:
  //   reply with "no such command" error
  //   return
  // if permission level not high enough
  //   reply with "cannot access" error
  //   return
}

conn_cb_result ctrl_engine_readcb(vde_connection *conn, vde_pkt *pkt, void *arg)
{
  // check pkt type is CTRL
  // add payload to in_buf
  // if in_buf contains a complete sobj_string (terminated by 0x03):
  //   extract sobj_string from in_buf
  //   deserialize sobj_string
  //   if error deserializing:
  //     reply with deserialization error
  //     (if we close connection here deserialization error won't be sent..)
  //   free sobj_string
  //   ctrl_engine_process_sobj(ctrl_conn, sobj_string)
  return CONN_CB_OK;
}

conn_cb_result ctrl_engine_errorcb(vde_connection *conn, vde_pkt *pkt,
                                   vde_conn_error err, void *arg)
{
  return CONN_CB_CLOSE;
}

int ctrl_engine_newconn(vde_component *component, vde_connection *conn,
                        vde_request *req)
{
  return 0;
}

static int engine_ctrl_init(vde_component *component)
{
  return 0;
}

int engine_ctrl_va_init(vde_component *component, va_list args)
{
  return engine_ctrl_init(component);
}

void engine_ctrl_fini(vde_component *component)
{
}

struct component_ops engine_ctrl_component_ops = {
  .init = engine_ctrl_va_init,
  .fini = engine_ctrl_fini,
  .get_configuration = NULL,
  .set_configuration = NULL,
  .get_policy = NULL,
  .set_policy = NULL,
};

struct vde_module engine_ctrl_module = {
  .kind = VDE_ENGINE,
  .family = "ctrl",
  .cops = &engine_ctrl_component_ops,
};

int engine_ctrl_module_init(vde_context *ctx)
{
  return vde_context_register_module(ctx, &engine_ctrl_module);
}

