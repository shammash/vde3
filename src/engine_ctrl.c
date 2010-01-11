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
#include <stdio.h>

#include <vde3/module.h>
#include <vde3/engine.h>
#include <vde3/context.h>
#include <vde3/connection.h>

#include <engine_ctrl_commands.h>

#define MAX_INBUF_SZ 8192

// XXX '/' is escaped by json
#define SEP_CHAR '.'
#define SEP_STRING "."

static char const * const builtin_commands[] = {
  "notify_add",
  "notify_del",
  NULL
};

static int is_builtin(vde_command *command) {
  int i = 0;
  for (i=0 ; builtin_commands[i] != NULL ; i++) {
    if (!strncmp(vde_command_get_name(command), builtin_commands[i],
                 strlen(builtin_commands[i]))) {
        return 1;
    }
  }
  return 0;
}

struct ctrl_engine {
  // XXX aliases table
  vde_list *ctrl_conns;
  vde_component *component;
};
typedef struct ctrl_engine ctrl_engine;

struct ctrl_conn {
  vde_connection *conn;
  char inbuf[MAX_INBUF_SZ];
  size_t inbuf_len;
  vde_queue *out_queue;
  vde_list *reg_signals;
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
 * @param id The reply id, an integer
 * @param result The object result
 * @param error The object error
 *
 * @return The newly constructed reply, or NULL in case of error
 */
vde_sobj *rpc_10_build_reply(vde_sobj *id, vde_sobj *result, vde_sobj *error)
{
  vde_sobj *reply;

  vde_assert(XOR(result, error));

  vde_assert(vde_sobj_is_type(id, vde_sobj_type_int));

  // XXX check reply == NULL
  reply = vde_sobj_new_hash();
  vde_sobj_hash_insert(reply, "id", vde_sobj_get(id));
  vde_sobj_hash_insert(reply, "result", vde_sobj_get(result));
  vde_sobj_hash_insert(reply, "error", vde_sobj_get(error));

  return reply;
}

/**
 * @brief Build a JSON-RPC 1.0 notification message
 *
 * @param method Method name, a string
 * @param params Notification parameters, an array
 *
 * @return The newly constructed notification, or NULL in case of error
 */
vde_sobj *rpc_10_build_notice(vde_sobj *method, vde_sobj *params)
{
  vde_sobj *notice;

  vde_assert(method != NULL);
  vde_assert(params != NULL);

  vde_assert(vde_sobj_is_type(method, vde_sobj_type_string));
  vde_assert(vde_sobj_is_type(params, vde_sobj_type_array));

  // XXX check notice == NULL
  notice = vde_sobj_new_hash();
  vde_sobj_hash_insert(notice, "id", NULL);
  vde_sobj_hash_insert(notice, "method", vde_sobj_get(method));
  vde_sobj_hash_insert(notice, "params", vde_sobj_get(params));

  return notice;
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

/**
 * @brief Check if a full path is well formed. If it is, duplicate the
 * component name and the callable path storing new memory in the passed
 * references. Duplicated strings must be freed with free() and not vde_free()
 *
 * @param full_path The full path to check and eventually split
 * @param component_name Reference to the pointer which will contain component
 * name string
 * @param callable_path Reference to the pointer which will contain callable
 * path string
 *
 * @return zero if checks, split and string duplications succeded, -1 on error
 * (no need to free duplicated strings in that case)
 */
static int check_split_path(const char *full_path, char **component_name,
                            char **callable_path)
{
  char *sep;

  vde_assert(full_path != NULL);
  vde_assert(component_name != NULL);
  vde_assert(callable_path != NULL);

  sep = memchr(full_path, SEP_CHAR, strlen(full_path));
  if (!sep) {
    errno = EINVAL;
    return -1;
  }

  /* component_name length == 0  or callable_path length == 0 */
  if ((sep == full_path) || (sep == (full_path + strlen(full_path) - 1))) {
    errno = EINVAL;
    return -1;
  }

  *component_name = strndup(full_path, sep - full_path);
  if (*component_name == NULL) {
    errno = ENOMEM;
    return -1;
  }
  *callable_path = strdup(sep + 1);
  if (*callable_path == NULL) {
    free(*component_name);
    *component_name = NULL;
    errno = ENOMEM;
    return -1;
  }

  return 0;
}

static inline char *build_signal_path(vde_component *component,
                                      const char *signal_path)
{
  const char *component_name;
  char *full_path;
  int full_path_len;

  component_name = vde_component_get_name(component);

  full_path_len = strlen(component_name) +
                  strlen(SEP_STRING) +
                  strlen(signal_path) +
                  1;
  // XXX check error on allocation
  full_path = vde_calloc(sizeof(char) * full_path_len);

  snprintf(full_path, full_path_len, "%s%s%s",
           component_name, SEP_STRING, signal_path);

  return full_path;
}

static void signal_callback(vde_component *component,
                            const char *signal_path, vde_sobj *info,
                            void *arg)
{
  char *full_path;
  vde_sobj *full_path_obj, *notice;
  ctrl_conn *cc = (ctrl_conn *)arg;

  full_path = build_signal_path(component, signal_path);

  full_path_obj = vde_sobj_new_string(full_path);

  notice = rpc_10_build_notice(full_path_obj, info);
  // XXX check notice == NULL
  ctrl_engine_conn_write(cc, notice);

  vde_sobj_put(full_path_obj);
  vde_sobj_put(notice);

  vde_free(full_path);
}

static void signal_destroy_callback(vde_component *component,
                                    const char *signal_path, void *arg)
{
  char *full_path, *reg_full_path;
  ctrl_conn *cc = (ctrl_conn *)arg;
  vde_list *iter;

  // XXX check full_path == NULL
  full_path = build_signal_path(component, signal_path);

  iter = vde_list_first(cc->reg_signals);
  while (iter != NULL) {
    reg_full_path = (char *)vde_list_get_data(iter);

    if (!strncmp(reg_full_path, full_path, strlen(reg_full_path))) {
      cc->reg_signals = vde_list_remove(cc->reg_signals, reg_full_path);
      free(reg_full_path);
      break;
    }
    iter = vde_list_next(iter);
  }

  vde_free(full_path);
}

int engine_ctrl_notify_add(vde_component *component, const char *full_path,
                           vde_sobj **out)
{
  char *s_component_name, *signal_name, *reg_full_path;
  vde_component *s_component;
  int rv;

  // builtin command, casting component
  ctrl_conn *cc = (ctrl_conn *)component;

  if (check_split_path(full_path, &s_component_name, &signal_name) == -1) {
    // XXX: what if errno == ENOMEM ?
    *out = vde_sobj_new_string("Signal path not well-formed");
    rv = -1;
    goto out;
  }

  s_component = vde_context_get_component(vde_connection_get_context(cc->conn),
                                          s_component_name);
  if (!s_component) {
    *out = vde_sobj_new_string("Component not found");
    rv = -1;
    goto cleannames;
  }
  rv = vde_component_signal_attach(s_component, signal_name, signal_callback,
                                   signal_destroy_callback, (void *)cc);
  if (rv != 0) {
    *out = vde_sobj_new_string("Failed to attach to signal");
  } else {
    *out = vde_sobj_new_string("Signal attached");
    // XXX check NULL
    reg_full_path = strndup(full_path, strlen(full_path));
    cc->reg_signals = vde_list_prepend(cc->reg_signals, reg_full_path);
  }

cleannames:
  free(s_component_name);
  free(signal_name);
out:
  return rv;
}

int engine_ctrl_notify_del(vde_component *component, const char *full_path,
                           vde_sobj **out)
{
  char *s_component_name, *signal_name;
  char *iter_path, *reg_full_path = NULL;
  vde_component *s_component;
  vde_list *iter;
  int rv;

  // builtin command, casting component
  ctrl_conn *cc = (ctrl_conn *)component;

  // search full_path inside cc
  iter = vde_list_first(cc->reg_signals);
  while (iter != NULL) {
    iter_path = (char *)vde_list_get_data(iter);
    if (!strncmp(iter_path, full_path, strlen(iter_path))) {
      reg_full_path = iter_path;
      break;
    }
    iter = vde_list_next(iter);
  }
  if (reg_full_path == NULL) {
    *out = vde_sobj_new_string("Signal not registered in connection");
    rv = -1;
    goto out;
  }

  if (check_split_path(full_path, &s_component_name, &signal_name) == -1) {
    // XXX: what if errno == ENOMEM ?
    *out = vde_sobj_new_string("Signal path not well-formed");
    rv = -1;
    goto out;
  }

  s_component = vde_context_get_component(vde_connection_get_context(cc->conn),
                                          s_component_name);
  if (!s_component) {
    *out = vde_sobj_new_string("Component not found");
    rv = -1;
    goto cleannames;
  }
  rv = vde_component_signal_detach(s_component, signal_name, signal_callback,
                                   signal_destroy_callback, (void *)cc);
  if (rv != 0) {
    *out = vde_sobj_new_string("Failed to detach from signal");
    goto cleannames;
  }

  cc->reg_signals = vde_list_remove(cc->reg_signals, reg_full_path);
  free(reg_full_path);

  *out = vde_sobj_new_string("Signal detached");

cleannames:
  free(s_component_name);
  free(signal_name);
out:
  return rv;
}

// XXX write errors back to connection instead of vde_warning only
static void ctrl_engine_deserialize_string(char *string, void *arg)
{
  ctrl_conn *cc = (ctrl_conn *)arg;
  vde_sobj *in_sobj, *out_sobj = NULL, *mesg_id, *reply;
  const char *method_name;
  char *component_name, *command_name;
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

  if (check_split_path(method_name, &component_name, &command_name) == -1) {
    // XXX: what if errno == ENOMEM ?
    vde_warning("%s: method name not well-formed", __PRETTY_FUNCTION__);
    goto cleaninsobj;
  }

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

  if (component == cc->engine->component && is_builtin(command)) {
    // ctrl engine builtin commands just need ctrl connection, passing cc
    // instead of component
    rv = func((vde_component *)cc, vde_sobj_hash_lookup(in_sobj, "params"),
              &out_sobj);
  } else {
    rv = func(component, vde_sobj_hash_lookup(in_sobj, "params"), &out_sobj);
  }

  if (rv) {
    reply = rpc_10_build_reply(mesg_id, NULL, out_sobj);
  } else {
    reply = rpc_10_build_reply(mesg_id, out_sobj, NULL);
  }
  // XXX check reply == NULL

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
static int ctrl_split_payload(vde_pkt *pkt,
                             char *inbuf, size_t *inbuf_len, int inbuf_sz,
                             void (*func)(char *string, void *priv),
                             void *priv)
{
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
        errno = ENOMEM;
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
      errno = ENOSPC;
      rv = -1;
      goto exit;
    } else {
      memcpy(inbuf + *inbuf_len, buf, remaining);
      *inbuf_len = *inbuf_len + remaining;
    }
  }

exit:
  return rv;
}

static void ctrl_conn_fini_noengine(ctrl_conn *cc)
{
  int rv;
  vde_pkt *pkt;
  vde_list *iter;
  char *sig_full_path, *component_name, *sig_path;
  vde_context *ctx;
  vde_component *component;

  // cleanup outgoing packets
  pkt = vde_queue_pop_tail(cc->out_queue);
  while (pkt != NULL) {
    vde_free(pkt);
    pkt = vde_queue_pop_tail(cc->out_queue);
  }
  vde_queue_delete(cc->out_queue);

  ctx = vde_connection_get_context(cc->conn);

  // detach from all signals
  iter = vde_list_first(cc->reg_signals);
  while (iter != NULL) {
    sig_full_path = vde_list_get_data(iter);
    if (check_split_path(sig_full_path, &component_name, &sig_path) == -1) {
      // XXX fatal here if errno != ENOMEM ?
      vde_error("%s: cannot split path %s", __PRETTY_FUNCTION__,
                sig_full_path);
    } else {
      component = vde_context_get_component(ctx, component_name);
      if (!component) {
        // XXX fatal here?
        vde_error("%s: cannot lookup component %s", __PRETTY_FUNCTION__,
                  component_name);
      } else {
        rv = vde_component_signal_detach(component, sig_path, signal_callback,
                                         signal_destroy_callback, (void *)cc);
        if (rv != 0) {
          // XXX fatal here?
          vde_error("%s: cannot detach %s", __PRETTY_FUNCTION__,
                    sig_full_path);
        }
      }

      free(component_name);
      free(sig_path);
    }
    free(sig_full_path);

    iter = vde_list_next(iter);
  }
  vde_list_delete(cc->reg_signals);

  // free ctrl_conn
  vde_free(cc);
}

static void ctrl_conn_fini(ctrl_conn *cc)
{
  // deregister from ctrl_engine
  cc->engine->ctrl_conns = vde_list_remove(cc->engine->ctrl_conns, cc);

  ctrl_conn_fini_noengine(cc);
}

int ctrl_engine_readcb(vde_connection *conn, vde_pkt *pkt, void *arg)
{
  ctrl_conn *cc = (ctrl_conn *)arg;

  // XXX check pkt type is CTRL

  if (ctrl_split_payload(pkt, cc->inbuf, &cc->inbuf_len, MAX_INBUF_SZ,
                        ctrl_engine_deserialize_string, (void *)cc)) {

    // XXX gracefully handle cases where payload cannot fit in inbuf
    vde_debug("%s: error splitting payload, closing", __PRETTY_FUNCTION__);

    ctrl_conn_fini(cc);

    errno = EPIPE;
    return -1;
  }

  return 0;
}

int ctrl_engine_writecb(vde_connection *conn, vde_pkt *pkt, void *arg)
{
  vde_pkt *q_pkt;
  int rv;
  ctrl_conn *cc = (ctrl_conn *)arg;

  // try to flush out queue if some packets are waiting
  q_pkt = vde_queue_pop_tail(cc->out_queue);
  while (q_pkt) {
    rv = vde_connection_write(cc->conn, q_pkt);
    if (rv != 0) {
      // couldn't write, requeue
      vde_queue_push_tail(cc->out_queue, q_pkt);
      break;
    }
    vde_free(q_pkt);
    q_pkt = vde_queue_pop_tail(cc->out_queue);
  }

  return 0;
}

int ctrl_engine_errorcb(vde_connection *conn, vde_pkt *pkt,
                                   vde_conn_error err, void *arg)
{
  ctrl_conn *cc = (ctrl_conn *)arg;

  if (err == CONN_WRITE_DELAY) {
    // XXX: packet is dropped here, the whole message will be corrupted.
    // To avoid this we must tell the connection to re-queue the packet.
    // Another option is to share send-queue with the connection and thus
    // re-queue the message by ourselves.
    vde_error("%s: connection is dropping packets, expect corrupted messages",
              __PRETTY_FUNCTION__);
    return 0;
  }

  ctrl_conn_fini(cc);

  errno = EPIPE;
  return -1;
}

int ctrl_engine_newconn(vde_component *component, vde_connection *conn,
                        vde_request *req)
{
  ctrl_engine *ctrl = vde_component_get_priv(component);
  ctrl_conn *cc;

  cc = vde_calloc(sizeof(ctrl_conn));
  if (cc == NULL) {
    vde_error("%s: could not allocate ctrl_conn", __PRETTY_FUNCTION__);
    errno = ENOMEM;
    return -1;
  }
  cc->conn = conn;
  cc->out_queue = vde_queue_init();
  cc->engine = ctrl;
  cc->reg_signals = NULL;

  ctrl->ctrl_conns = vde_list_prepend(ctrl->ctrl_conns, cc);

  vde_connection_set_callbacks(conn, &ctrl_engine_readcb, &ctrl_engine_writecb,
                               &ctrl_engine_errorcb, (void *)cc);
  vde_connection_set_pkt_properties(conn, 0, 0);
  /*
   * XXX: define send properties and policy for discarding packets
  send_timeout.tv_sec = TIMEOUT;
  send_timeout.tv_usec = 0;
  vde_connection_set_send_properties(conn, TIMES, &send_timeout);
  */
  return 0;
}

static int engine_ctrl_init(vde_component *component)
{
  int tmp_errno;
  ctrl_engine *ctrl;

  vde_assert(component != NULL);

  ctrl = (ctrl_engine *)vde_calloc(sizeof(ctrl_engine));
  if (ctrl == NULL) {
    vde_error("%s: could not allocate private data", __PRETTY_FUNCTION__);
    errno = ENOMEM;
    return -1;
  }

  ctrl->component = component;

  if (vde_component_commands_register(component, engine_ctrl_commands)) {
    tmp_errno = errno;
    vde_error("%s: could not register commands", __PRETTY_FUNCTION__);
    vde_free(ctrl);
    errno = tmp_errno;
    return -1;
  }

  vde_engine_set_ops(component, &ctrl_engine_newconn);
  vde_component_set_priv(component, (void *)ctrl);

  return 0;
}

int engine_ctrl_va_init(vde_component *component, va_list args)
{
  return engine_ctrl_init(component);
}

void engine_ctrl_fini(vde_component *component)
{
  vde_list *iter;
  ctrl_conn *cc;
  ctrl_engine *ctrl = vde_component_get_priv(component);

  iter = vde_list_first(ctrl->ctrl_conns);
  while (iter != NULL) {
    cc = vde_list_get_data(iter);
    vde_connection_fini(cc->conn);
    vde_connection_delete(cc->conn);
    ctrl_conn_fini_noengine(cc);

    iter = vde_list_next(iter);
  }
  vde_list_delete(ctrl->ctrl_conns);

  vde_free(ctrl);
}

struct component_ops engine_ctrl_component_ops = {
  .init = engine_ctrl_va_init,
  .fini = engine_ctrl_fini,
  .get_configuration = NULL,
  .set_configuration = NULL,
  .get_policy = NULL,
  .set_policy = NULL,
};

vde_module VDE_MODULE_START = {
  .kind = VDE_ENGINE,
  .family = "ctrl",
  .cops = &engine_ctrl_component_ops,
};
