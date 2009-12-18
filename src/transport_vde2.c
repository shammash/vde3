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

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <vde3/module.h>
#include <vde3/component.h>
#include <vde3/context.h>

#define LISTEN_QUEUE 15

// size of sockaddr_un.sun_path
#define UNIX_PATH_MAX 108

// taken from vde2 datasock.c
#define REQBUFLEN 256
#define SWITCH_MAGIC 0xfeedface
enum request_type { REQ_NEW_CONTROL, REQ_NEW_PORT0 };

struct request_v3 {
  uint32_t magic;
  uint32_t version;
  enum request_type type;
  struct sockaddr_un sock;
  char description[];
} __attribute__((packed));

typedef struct request_v3 request;
// end of vde2 datasock.c

struct vde2_pending_conn {
  int fd;
  void *event;
  char *local_path;
  request *remote_request;
  vde_connection *conn;
  vde_component *transport;
};
typedef struct vde2_pending_conn vde2_pending_conn;

struct transport_vde2 {
  char *vdesock_dir;
  char *vdesock_ctl;
  int listen_fd;
  void *listen_event;
  unsigned int connections;
  vde_list *pending_conns;
};
typedef struct transport_vde2 transport_vde2;

void pending_conn_delete(struct vde2_pending_conn *pc) {
  // XXX pc->event is not removed
  if (pc->local_path) {
    vde_free(pc->local_path);
  }

  if (pc->remote_request) {
    vde_free(pc->remote_request);
  }

  vde_free(pc);
}

static int vde2_remove_sock_if_unused(struct sockaddr_un *sa_unix)
{
  int test_fd, ret = 1;

  if ((test_fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
    vde_error("%s: socket %s", __PRETTY_FUNCTION__, strerror(errno));
    return 1;
  }
  if (connect(test_fd, (struct sockaddr *) sa_unix, sizeof(*sa_unix)) < 0) {
    if (errno == ECONNREFUSED) {
      if (unlink(sa_unix->sun_path) < 0) {
        vde_error("%s: failed to removed unused socket '%s': %s",
            __PRETTY_FUNCTION__, sa_unix->sun_path,strerror(errno));
      }
      ret = 0;
    } else {
      vde_error("%s: connect %s", __PRETTY_FUNCTION__, strerror(errno));
    }
  }
  close(test_fd);
  return ret;
}

void vde2_srv_send_request(int fd, short event_type, void *arg)
{
  int len;
  struct sockaddr_un sun;
  vde2_pending_conn *pc = (vde2_pending_conn *)arg;
  transport_vde2 *tr = (transport_vde2 *)vde_component_get_priv(pc->transport);

  pc->local_path = (char *)vde_calloc(sizeof(sun.sun_path));
  if (!pc->local_path) {
    vde_error("%s: cannot allocate memory for local path",
              __PRETTY_FUNCTION__);
    goto error;
  }

  snprintf(pc->local_path, sizeof(sun.sun_path), "%s/%04d",
           tr->vdesock_dir, tr->connections);

  snprintf(sun.sun_path, sizeof(sun.sun_path), "%s", pc->local_path);

  len = write(fd, &sun, sizeof(sun));
  if (len != sizeof(sun)) {
    vde_error("%s: cannot reply to peer", __PRETTY_FUNCTION__);
    goto error;
  }

  tr->connections++;

  // popolate conn (add fd)
  // add peer credentials to conn.attributes
  // call connection manager accept_cb with the conn

error:
  tr->pending_conns = vde_list_remove(tr->pending_conns, pc);
  pending_conn_delete(pc);
}

void vde2_srv_get_request(int fd, short event_type, void *arg)
{
  int len;
  char reqbuf[REQBUFLEN+1];
  request *req=(request *)reqbuf;
  vde2_pending_conn *pc = (vde2_pending_conn *)arg;
  transport_vde2 *tr = (transport_vde2 *)vde_component_get_priv(pc->transport);

  len = read(pc->fd, reqbuf, REQBUFLEN);
  if (len < 0) {
    if (errno != EAGAIN) {
      goto error;
    }
  } else if (len == 0) {
    goto error;
  } else {
    if (req->magic != SWITCH_MAGIC || req->version != 3) {
      vde_error("%s: received an invalid request", __PRETTY_FUNCTION__);
      goto error;
    }

    reqbuf[len] = 0;

    if (req->sock.sun_path[0] == 0) {
      vde_error("%s: received an invalid socket path", __PRETTY_FUNCTION__);
      goto error;
    }

    if (access(req->sock.sun_path, R_OK | W_OK) != 0) {
      vde_error("%s: cannot access peer socket %s", __PRETTY_FUNCTION__,
                req->sock.sun_path);
      goto error;
    }

    pc->remote_request = (request *)vde_alloc(len);
    if (!pc->remote_request) {
      vde_error("%s: cannot allocate memory for remote request",
                __PRETTY_FUNCTION__);
      goto error;
    }
    memcpy(pc->remote_request, reqbuf, len);

    // XXX: add event write on pc->fd for vde2_srv_send_request
  }

  return;

error:
  tr->pending_conns = vde_list_remove(tr->pending_conns, pc);
  pending_conn_delete(pc);
}

void vde2_accept(int listen_fd, short event_type, void *arg)
{
  struct sockaddr sa;
  socklen_t sa_len;
  int new;
  vde_connection *conn;
  struct vde2_pending_conn *pc;
  vde_component *component = (vde_component *)arg;
  transport_vde2 *tr = (transport_vde2 *)vde_component_get_priv(component);

  // XXX: consistency check: is listen_fd the right one?
  new = accept(listen_fd, &sa, &sa_len);
  if (new < 0) {
    vde_warning("%s: accept %s", __PRETTY_FUNCTION__, strerror(errno));
    return;
  }
  if (fcntl(new, F_SETFL, O_NONBLOCK) < 0) {
    vde_warning("%s: cannot set O_NONBLOCK for new connection %s",
                __PRETTY_FUNCTION__, strerror(errno));
    goto error_close;
  }
  if (vde_connection_new(&conn)) {
    vde_error("%s: cannot create connection", __PRETTY_FUNCTION__);
    goto error_close;
  }
  // XXX: connection init, set connection backend to vde2
  pc = (struct vde2_pending_conn *)
        vde_calloc(sizeof(struct vde2_pending_conn));
  if (!pc) {
    vde_error("%s: cannot create pending connection", __PRETTY_FUNCTION__);
    goto error_conn_del;
  }

  pc->fd = new;
  pc->conn = conn;
  pc->transport = component;
  tr->pending_conns = vde_list_prepend(tr->pending_conns, pc);

  // XXX: add event read on pc->fd for vde2_srv_get_auth

  return;

error_conn_del:
  vde_connection_delete(conn);
error_close:
  close(new);
}

int vde2_listen(vde_component *component)
{
  struct sockaddr_un sa_unix;
  int one = 1;
  transport_vde2 *tr = (transport_vde2 *)vde_component_get_priv(component);

  tr->listen_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (tr->listen_fd < 0) {
    vde_error("%s: Could not obtain a BSD socket: %s", __PRETTY_FUNCTION__,
              strerror(errno));
    return -1;
  }
  if (setsockopt(tr->listen_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&one,
                 sizeof(one)) < 0) {
    vde_error("%s: Could not set socket options: %s", __PRETTY_FUNCTION__,
              strerror(errno));
    goto error_close;
  }
  if (fcntl(tr->listen_fd, F_SETFL, O_NONBLOCK) < 0) {
    vde_error("%s: Could not set O_NONBLOCK: %s", __PRETTY_FUNCTION__,
              strerror(errno));
    goto error_close;
  }
  if (((mkdir(tr->vdesock_dir, 0777) < 0) && (errno != EEXIST))) {
    vde_error("%s: Could not create vdesock directory %s: %s",
              __PRETTY_FUNCTION__, tr->vdesock_dir, strerror(errno));
    goto error_close;
  }
  sa_unix.sun_family = AF_UNIX;
  snprintf(sa_unix.sun_path, sizeof(sa_unix.sun_path), "%s/ctl",
           tr->vdesock_dir);
  if (bind(tr->listen_fd, (struct sockaddr *)&sa_unix, sizeof(sa_unix)) < 0) {
    if ((errno == EADDRINUSE) && vde2_remove_sock_if_unused(&sa_unix)) {
      vde_error("%s: Could not bind to %s/ctl: socket in use",
                __PRETTY_FUNCTION__, tr->vdesock_dir);
      goto error_close;
    } else {
      if (bind(tr->listen_fd, (struct sockaddr *)&sa_unix,
               sizeof(sa_unix)) < 0) {
        vde_error("%s: Could not bind to %s/ctl: %s", __PRETTY_FUNCTION__,
                  tr->vdesock_dir, strerror(errno));
        goto error_close;
      }
    }
  }
  if (listen(tr->listen_fd, LISTEN_QUEUE) < 0) {
    vde_error("%s: Could not listen: %s", __PRETTY_FUNCTION__,
              strerror(errno));
    goto error_unlink;
  }

  // XXX: add event read on listen_fd for vde2_accept

  return 0;

error_unlink:
  unlink(sa_unix.sun_path);
  rmdir(tr->vdesock_dir);
error_close:
  close(tr->listen_fd);
  tr->listen_fd = -1;
  return -2;
}

int vde2_connect(vde_component *component, vde_connection *conn)
{
}

static int transport_vde2_init(vde_component *component, const char *dir)
{

  transport_vde2 *tr;

  if (component == NULL) {
    vde_error("%s: component is NULL", __PRETTY_FUNCTION__);
    return -1;
  }

  if (strlen(dir) > UNIX_PATH_MAX - 4) { // we will add '/ctl' later
    vde_error("%s: directory name is too long", __PRETTY_FUNCTION__);
    return -2;
  }
  tr = (transport_vde2 *)vde_calloc(sizeof(transport_vde2));
  if (tr == NULL) {
    vde_error("%s: could not allocate private data", __PRETTY_FUNCTION__);
    return -3;
  }

  // XXX: path needs to be normalized/checked somewhere
  tr->vdesock_dir = strdup(dir);
  if (tr->vdesock_dir == NULL) {
    vde_free(tr);
    vde_error("%s: could not allocate private path", __PRETTY_FUNCTION__);
    return -4;
  }

  vde_component_set_transport_ops(component, &vde2_listen, &vde2_connect);
  vde_component_set_priv(component, (void *)tr);
  return 0;
}

int transport_vde2_va_init(vde_component *component, va_list args)
{
  return transport_vde2_init(component, va_arg(args, const char*));
}

// XXX to be defined
void transport_vde2_fini(vde_component *component) {
}

struct component_ops transport_vde2_component_ops = {
  .init = transport_vde2_va_init,
  .fini = transport_vde2_fini,
  .get_configuration = NULL,
  .set_configuration = NULL,
  .get_policy = NULL,
  .set_policy = NULL,
};

struct vde_module transport_vde2_module = {
  .kind = VDE_TRANSPORT,
  .family = "vde2",
  .cops = &transport_vde2_component_ops,
};

int transport_vde2_module_init(vde_context *ctx)
{
  return vde_context_register_module(ctx, &transport_vde2_module);
}

/*
 * the connection has several parameters to handle the queuing of packets:
 *  - if a transient error during write to fd is encountered, the write is tried
 * MAX_TRIES * MAX_TIMEOUT times and after that error_cb(CONN_PKT_DROP, pkt) is
 * called for the pkt we are currently trying to drop but before the pkt is
 * freed
 *  - if a permanent error during write to fd is encountered
 * error_cb(CONN_CLOSED) (or sth) is called
 *
 * conn.write() must return error on queue full
 *
 * the queue is sized by the connection
 *
 * write_cb(conn, pkt) if specified is called after each packet has been
 * successfully written to the operating system but before the packet is freed
 *
 * OPTIONAL: the return value of error_cb/write_cb can indicate what to do with
 * the packet
 *
 */

/*
 * per-connection packet dequeue:
 * a callback which flushes the queue is linked to write event for the conn fd.
 * whenever a packet is added to the queue the event is enabled if not already.
 * whenever the queue is empty the event is disabled.
 *
 * struct event_queue {
 *   vde_event event; // the event related to this queue
 *   vde_dequeue *pktqueue; // the packet queue itself
 *   vde_conn *conn; // the connection owning this queue
 * }
 *
 * conn_init(...) {
 *   ...
 *   conn->priv->conn = conn;
 *   event_set(conn->priv->event, conn->fd, conn_write_internal_cb, conn->priv);
 * }
 *
 * conn_write(conn, pkt) {
 *   conn->priv->pktqueue.append(pkt)
 *   if (conn->priv->event is not active)
 *     event_add(conn->priv->event)
 *
 *   return success
 * }
 *
 * conn_write_internal_cb(fd, type, arg) {
 *   event_queue queue = arg;
 *   write(conn_fd, pop_packet, size);
 *   if success:
 *     remove packet from queue
 *
 * }
 *
 * //TODO for the STREAM case also available_data needs to be taken care of to
 * // know when a packet has been fully written to the network and thus can be
 * // removed from the queue
 *
 */

