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

#include <stdlib.h>
#include <string.h>

#include <vde3/module.h>
#include <vde3/component.h>
#include <vde3/context.h>
#include <vde3/connection.h>

#define MAX_INBUF_SZ 8192

// XXX '/' is escaped by json
#define SEP_CHAR '.'
#define SEP_STRING "."

struct ctrl_engine {
  // - aliases table
  vde_component *component;
};
typedef struct ctrl_engine ctrl_engine;

struct ctrl_conn {
  vde_connection *conn;
  char inbuf[MAX_INBUF_SZ];
  size_t inbuf_len;
  vde_queue *out_queue;
  // - permission level
  ctrl_engine *engine;
};
typedef struct ctrl_conn ctrl_conn;

static int ctrl_engine_conn_write(ctrl_conn *cc, vde_sobj *out_obj) {
  const char *out_str;
  vde_pkt *new_pkt, *send_pkt;
  unsigned int out_len, payload_sz, last_chunk_sz, num_chunks, sent_chunks,
               cpy_sz, rv;

  // no need to free out_str, will be garbage-collected when out_obj is
  // destroyed
  out_str = vde_sobj_to_string(out_obj);
  if (!out_str) {
    // XXX must be fatal because some component has a bug
    vde_error("%s: cannot serialize out_obj", __PRETTY_FUNCTION__);
    return -1;
  }

  out_len = strlen(out_str) + 1; // send \0 as well
  payload_sz = vde_connection_max_payload(cc->conn);
  last_chunk_sz = out_len % payload_sz;
  num_chunks = out_len / payload_sz;
  if (last_chunk_sz) {
    // add non-full chunk
    num_chunks++;
  }

  // create packets
  sent_chunks = 0;
  while (num_chunks > sent_chunks) {

    if ((num_chunks == sent_chunks + 1) && last_chunk_sz) {
      // sending non-complete last chunk
      cpy_sz = last_chunk_sz;
    } else {
      cpy_sz = payload_sz;
    }

    new_pkt = vde_pkt_new(cpy_sz, 0, 0);
    // XXX check pkt NULL
    new_pkt->hdr->pkt_len = cpy_sz;
    // XXX: set type and version
    memcpy(new_pkt->payload, out_str + (sent_chunks*payload_sz), cpy_sz);

    vde_queue_push_head(cc->out_queue, new_pkt);

    sent_chunks++;
  }

  // try to send packets
  send_pkt = vde_queue_pop_tail(cc->out_queue);
  while (send_pkt) {
    rv = vde_connection_write(cc->conn, send_pkt);
    if (rv != 0) {
      // couldn't write, requeue
      vde_queue_push_tail(cc->out_queue, send_pkt);
      break;
    }
    vde_free(send_pkt);
    send_pkt = vde_queue_pop_tail(cc->out_queue);
  }

  return 0;
}

/**
 * @brief Build a JSON-RPC 1.0 method call reply, one between result and error
 * must be NULL
 *
 * @param id The reply id
 * @param result The object result
 * @param error The object error
 *
 * @return The newly constructed reply, or NULL in case of error
 */
vde_sobj *rpc_10_build_reply(vde_sobj *id, vde_sobj *result, vde_sobj *error)
{
  vde_sobj *reply;

  vde_return_val_if_fail(XOR(result, error), NULL);

  reply = vde_sobj_new_hash();
  vde_sobj_hash_insert(reply, "id", vde_sobj_get(id));
  vde_sobj_hash_insert(reply, "result", vde_sobj_get(result));
  vde_sobj_hash_insert(reply, "error", vde_sobj_get(error));

  return reply;
}

/**
 * @brief Validate a vde_sobj WRT JSON-RPC 1.0 method call
 *
 * @param sobj The sobj to validate
 *
 * @return zero on success, an error code otherwise
 */
static int rpc_10_sobj_validate_call(vde_sobj *sobj)
{
  vde_sobj *method, *params, *id;

  if (!vde_sobj_is_type(sobj, vde_sobj_type_hash)) {
    return -1;
  }

  method = vde_sobj_hash_lookup(sobj, "method");
  if (!method || !vde_sobj_is_type(method, vde_sobj_type_string)) {
    vde_debug("%s: method not present or not string", __PRETTY_FUNCTION__);
    return -1;
  }

  params = vde_sobj_hash_lookup(sobj, "params");
  if (!params || !vde_sobj_is_type(params, vde_sobj_type_array)) {
    vde_debug("%s: params not present or not array", __PRETTY_FUNCTION__);
    return -1;
  }

  id = vde_sobj_hash_lookup(sobj, "id");
  if (!id || !vde_sobj_is_type(id, vde_sobj_type_int)) {
    vde_debug("%s: id not present or not int", __PRETTY_FUNCTION__);
    return -1;
  }
  if (vde_sobj_get_int(id) < 0) {
    vde_debug("%s: id less than 0", __PRETTY_FUNCTION__);
    return -1;
  }

  return 0;
}

// XXX write errors back to connection instead of vde_warning only
static void ctrl_engine_deserialize_string(char *string, void *arg)
{
  ctrl_conn *cc = (ctrl_conn *)arg;
  vde_sobj *in_sobj, *out_sobj, *mesg_id, *reply;
  const char *method_name;
  char *sep, *component_name, *command_name;
  command_func func;
  int rv;

  vde_command *command;
  vde_component *component;

  in_sobj = vde_sobj_from_string(string);
  if (!in_sobj) {
    vde_warning("%s: error deserializing input command", __PRETTY_FUNCTION__);
    goto out;
  }

  if (rpc_10_sobj_validate_call(in_sobj)) {
    vde_warning("%s: invalid method call received", __PRETTY_FUNCTION__);
    goto cleaninsobj;
  }

  // mesg_id will be freed by vde_sobj_put(in_sobj)
  mesg_id = vde_sobj_hash_lookup(in_sobj, "id");

  // XXX command aliases resolution

  method_name = vde_sobj_get_string(vde_sobj_hash_lookup(in_sobj, "method"));

  sep = memchr(method_name, SEP_CHAR, strlen(method_name));
  if (!sep) {
    vde_warning("%s: cannot find separator in method name",
                __PRETTY_FUNCTION__);
    goto cleaninsobj;
  }
  // XXX check for corner cases: method_name = '/' or 'foo/' or '/foo'

  component_name = strndup(method_name, sep - method_name);
  command_name = strdup(sep + 1);

  component = vde_context_get_component(vde_connection_get_context(cc->conn),
                                        component_name);
  if (!component) {
    vde_warning("%s: component name %s not found", __PRETTY_FUNCTION__,
                component_name);
    goto cleannames;
  }

  command = vde_component_command_get(component, command_name);
  if (!command) {
    vde_warning("%s: command %s not found", __PRETTY_FUNCTION__,
                command_name);
    goto cleannames;
  }

  func = vde_command_get_func(command);
  // XXX check permission level

  rv = func(component, vde_sobj_hash_lookup(in_sobj, "params"), &out_sobj);
  if (rv) {
    reply = rpc_10_build_reply(mesg_id, NULL, out_sobj);
  } else {
    reply = rpc_10_build_reply(mesg_id, out_sobj, NULL);
  }
  vde_sobj_put(out_sobj);

  // XXX check error
  ctrl_engine_conn_write(cc, reply);

  vde_sobj_put(reply);

cleannames:
  // using free as a result of using strdup as well instead of vde_free
  free(component_name);
  free(command_name);
cleaninsobj:
  vde_sobj_put(in_sobj);
out:
  return;
}

/**
 * @brief Split packet payload using \0 as separator
 *
 * @param pkt The packet to operate on
 * @param inbuf A buffer where to put remainder string
 * @param inbuf_len Length of valid bytes inside inbuf
 * @param inbuf_sz Total length of inbuf
 * @param func The callback to call for each complete string found
 * @param priv A private data pointer passed to func
 *
 * @return zero on success, an error code otherwise
 */
static int vde_split_payload(vde_pkt *pkt,
                             char *inbuf, size_t *inbuf_len, int inbuf_sz,
                             void (*func)(char *string, void *priv),
                             void *priv) {
  char *buf, *next, *inbuf_tmp = NULL, *to_serial;
  int remaining, buf_len = 0, rv = 0;

  buf = pkt->payload;
  remaining = pkt->hdr->pkt_len;
  while ((next = memchr(buf, 0, remaining))) {
    // a token is present in buf
    buf_len = next - buf;
    if (*inbuf_len) {
      // there is a partial string in inbuf, append that and buf into inbuf_tmp
      inbuf_tmp = vde_alloc(sizeof(char) * (*inbuf_len + buf_len));
      if (!inbuf_tmp) {
        vde_error("%s: cannot allocate memory for temp buffer",
                  __PRETTY_FUNCTION__);
        // XXX document somewhere that inbuf is not reset in this case
        rv = -1;
        goto exit;
      }
      memcpy(inbuf_tmp, inbuf, *inbuf_len);
      memcpy(inbuf_tmp + *inbuf_len, buf, buf_len);

      *inbuf_len = 0;
      to_serial = inbuf_tmp;
    } else {
      to_serial = buf;
    }

    func(to_serial, priv);

    if (inbuf_tmp != NULL) {
      vde_free(inbuf_tmp);
      inbuf_tmp = NULL;
    }

    // recalc pointers for next cycle, +1 for separator
    remaining = remaining - (buf_len + 1);
    if (remaining > 0) {
      buf = next + 1;
    }

    // XXX assert(remaining >= 0);
  }

  // no token left, check if we need to store a partial string in inbuf
  if (remaining > 0) {
    if (remaining + *inbuf_len > inbuf_sz) {
      *inbuf_len = 0;
      vde_warning("%s: partial string too long for inbuf, dropping",
                  __PRETTY_FUNCTION__);
      rv = -2;
      goto exit;
    } else {
      memcpy(inbuf + *inbuf_len, buf, remaining);
      *inbuf_len = *inbuf_len + remaining;
    }
  }

exit:
  return rv;
}

conn_cb_result ctrl_engine_readcb(vde_connection *conn, vde_pkt *pkt, void *arg)
{
  ctrl_conn *cc = (ctrl_conn *)arg;

  vde_debug("got control readcb");

  // XXX check pkt type is CTRL
  if (vde_split_payload(pkt, cc->inbuf, &cc->inbuf_len, MAX_INBUF_SZ,
                        ctrl_engine_deserialize_string, (void *)cc)) {
    // XXX gracefully handle cases where payload cannot fit in inbuf

    vde_debug("got control readcb error");

    // XXX fini ctrl_conn
    return CONN_CB_CLOSE;
  }

  return CONN_CB_OK;
}

conn_cb_result ctrl_engine_errorcb(vde_connection *conn, vde_pkt *pkt,
                                   vde_conn_error err, void *arg)
{
  vde_debug("got control error cb");
  // XXX fini ctrl_conn
  return CONN_CB_CLOSE;
}

int ctrl_engine_newconn(vde_component *component, vde_connection *conn,
                        vde_request *req)
{
  ctrl_engine *ctrl = vde_component_get_priv(component);
  ctrl_conn *cc;

  // XXX check for NULL
  cc = vde_calloc(sizeof(ctrl_conn));
  cc->conn = conn;
  cc->out_queue = vde_queue_init();
  cc->engine = ctrl;

  vde_debug("got new control conn");

  // XXX register write callback to flush queue conn
  vde_connection_set_callbacks(conn, &ctrl_engine_readcb, NULL,
                               &ctrl_engine_errorcb, (void *)cc);
  vde_connection_set_pkt_properties(conn, 0, 0);
  /*
  send_timeout.tv_sec = TIMEOUT;
  send_timeout.tv_usec = 0;
  vde_connection_set_send_properties(conn, TIMES, &send_timeout);
  */
  return 0;
}

static int engine_ctrl_init(vde_component *component)
{
  ctrl_engine *ctrl;

  vde_debug("got control init");

  if (component == NULL) {
    vde_error("%s: component is NULL", __PRETTY_FUNCTION__);
    return -1;
  }

  ctrl = (ctrl_engine *)vde_calloc(sizeof(ctrl_engine));
  if (ctrl == NULL) {
    vde_error("%s: could not allocate private data", __PRETTY_FUNCTION__);
    return -3;
  }

  ctrl->component = component;

  vde_component_set_engine_ops(component, &ctrl_engine_newconn);
  vde_component_set_priv(component, (void *)ctrl);
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

