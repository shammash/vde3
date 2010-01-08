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

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <vde3.h>

#include <vde3/common.h>
#include <vde3/module.h>
#include <vde3/connection.h>
#include <vde3/transport.h>
#include <vde3/context.h>
#include <vde3/packet.h>

#define LISTEN_QUEUE 15
#define MAX_HEAD_SZ 4 /* size of prellocated space before payload */
#define MAX_TAIL_SZ 0 /* size of prellocated space after payload */
#define PKT_DATA_SZ (sizeof(vde_hdr) + MAX_HEAD_SZ + sizeof(struct eth_frame) \
                     + MAX_TAIL_SZ)
/*
 * pkt_data_sz = vde 3 header (sizeof(vde_hdr))
 *             + space reserved for vlan tags (4)
 *             + frame (1500)
 *             + trailing space (4)
 *             + further tail space (0)
 */

// size of sockaddr_un.sun_path
#define UNIX_PATH_MAX 108

// taken from vde2 packetq.c
#define MAXQLEN 4192
// end of vde2 packetq.c

// taken from vde2 datasock.c
#define DATA_BUF_SIZE 131072
#define SWITCH_MAGIC 0xfeedface
#define REQBUFLEN 256

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

struct vde2_pkt {
  unsigned int numtries;
  vde_pkt pkt;
  char data[PKT_DATA_SZ];
};
typedef struct vde2_pkt vde2_pkt;

struct vde2_conn {
  int data_fd;
  void *data_ev_rd;
  void *data_ev_wr;
  int ctl_fd;
  void *ctl_ev;
  vde_queue *pkt_queue;
  struct sockaddr_un local_sa;
  struct sockaddr_un remote_sa;
  request *remote_request;
  vde_connection *conn;
  vde_component *transport;
};
typedef struct vde2_conn vde2_conn;

struct vde2_tr {
  char *vdesock_dir;
  int listen_fd;
  void *listen_event;
  unsigned int connections;
  vde_list *pending_conns;
};
typedef struct vde2_tr vde2_tr;

void vde2_conn_read_ctl_event(int ctl_fd, short event_type, void *arg)
{
  int len;
  char reqbuf[REQBUFLEN+1];
  vde2_conn *v2_conn = (vde2_conn *)arg;
  vde_connection *conn = v2_conn->conn;
  len = read(v2_conn->ctl_fd, reqbuf, REQBUFLEN);
  if (len < 0) {
    if (errno == EAGAIN) {
      vde_warning("%s: got EAGAIN on ctl_fd %d", __PRETTY_FUNCTION__,
                  v2_conn->ctl_fd);
      return;
    }
    if (vde_connection_call_error(conn, NULL, CONN_READ_CLOSED) &&
        (errno == EPIPE)) {
      goto err_close;
    }
    vde_warning("%s: got fatal error on ctl_fd %d but connection not closed",
                __PRETTY_FUNCTION__, v2_conn->ctl_fd);
    return;
  }
  if (len > 0) {
    vde_warning("%s: unexpected data exchange on ctl_fd", __PRETTY_FUNCTION__);
    return;
  }
  if (len == 0) {
    if (vde_connection_call_error(conn, NULL, CONN_READ_CLOSED) &&
        (errno == EPIPE)) {
      goto err_close;
    }
    vde_warning("%s: got fatal error on ctl_fd %d but connection not closed",
                __PRETTY_FUNCTION__, v2_conn->ctl_fd);
    return;
  }

  return;

err_close:
  vde_connection_fini(conn);
  vde_connection_delete(conn);
}

void vde2_conn_read_data_event(int data_fd, short event_type, void *arg)
{
  vde2_pkt stack_pkt;
  vde_pkt *pkt;
  struct sockaddr sock;
  int len;
  int cb_errno = 0;
  socklen_t socklen = sizeof(sock);
  vde2_conn *v2_conn = (vde2_conn *)arg;
  vde_connection *conn = v2_conn->conn;

  if ( (vde_connection_get_pkt_headsize(conn) <= MAX_HEAD_SZ)
        && (vde_connection_get_pkt_tailsize(conn) <= MAX_TAIL_SZ) ) {
    pkt = &stack_pkt.pkt;
    vde_pkt_init(pkt, PKT_DATA_SZ, vde_connection_get_pkt_headsize(conn),
                 vde_connection_get_pkt_tailsize(conn));
  } else {
    // XXX: perform dynamic allocation
    vde_warning("%s: requested head + tail size too large, skipping",
                __PRETTY_FUNCTION__);
    return;
  }

  len = recvfrom(v2_conn->data_fd, pkt->payload, sizeof(struct eth_frame), 0,
                 &sock, &socklen);
  // XXX: check received sock with remote path??
  if (len >= sizeof(struct eth_hdr)) {
    // XXX: set hdr version and type
    pkt->hdr->pkt_len = len;
    if (vde_connection_call_read(conn, pkt)) {
      cb_errno = errno;
    }
  } else if (len < 0) {
    if (errno == EAGAIN) {
      vde_warning("%s: got EAGAIN on data_fd %d", __PRETTY_FUNCTION__,
                  v2_conn->data_fd);
    } else {
    // XXX: handle this error situation, call error_cb?
    vde_warning("%s: error reading from data_fd %d: %s", __PRETTY_FUNCTION__,
                v2_conn->data_fd, strerror(errno));
    }
  } else if (len == 0) {
    vde_warning("%s: EOF from data_fd %d: %s", __PRETTY_FUNCTION__,
                v2_conn->data_fd, strerror(errno));
  }

  // XXX: free packet if previously allocated with dynamic allocation

  if (cb_errno == EPIPE) {
    vde_connection_fini(conn);
    vde_connection_delete(conn);
  }
}

void vde2_conn_write_data_event(int data_fd, short event_type, void *arg)
{
  int len;
  vde2_pkt *v2_pkt;
  vde_pkt *pkt;
  int cb_errno = 0;
  vde2_conn *v2_conn = (vde2_conn *)arg;
  vde_connection *conn = v2_conn->conn;

  v2_pkt = vde_queue_pop_tail(v2_conn->pkt_queue);
  while (v2_pkt != NULL) {
    pkt = &v2_pkt->pkt;
    len = sendto(v2_conn->data_fd, pkt->payload, pkt->hdr->pkt_len, 0,
                 (const struct sockaddr *)&v2_conn->remote_sa,
                 sizeof(struct sockaddr_un));
    if (len == pkt->hdr->pkt_len) {
      if (vde_connection_call_write(conn, pkt)) {
        cb_errno = errno;
      }
      vde_cached_free_type(vde2_pkt, v2_pkt);
      if (cb_errno == EPIPE) {
        goto err_close;
      }
    } else if ((len < 0) && (errno != EAGAIN)) {
      if (vde_connection_call_error(conn, pkt, CONN_WRITE_CLOSED)) {
        cb_errno = errno;
      }
      vde_cached_free_type(vde2_pkt, v2_pkt);
      if (cb_errno == EPIPE) {
        goto err_close;
      } else {
        vde_warning("%s: fatal error on data_fd %d but connection not closed",
                    __PRETTY_FUNCTION__, v2_conn->data_fd);
        break;
      }
    } else { /* (0 < len < pkt_len) || (len < 0 && errno == EAGAIN) */
      v2_pkt->numtries++;
      if (v2_pkt->numtries > vde_connection_get_send_maxtries(conn)) {
        if (vde_connection_call_error(conn, pkt, CONN_WRITE_DELAY)) {
          cb_errno = errno;
        }
        vde_cached_free_type(vde2_pkt, v2_pkt);
        if (cb_errno == EPIPE) {
          goto err_close;
        }
      } else {
        vde_queue_push_tail(v2_conn->pkt_queue, v2_pkt);
      }
      break; // give up sending
    }
    v2_pkt = vde_queue_pop_tail(v2_conn->pkt_queue);
  }

  if (v2_pkt == NULL) {
    vde_context_event_del(vde_connection_get_context(conn),
                          v2_conn->data_ev_wr);
    v2_conn->data_ev_wr = NULL;
  }

  return;

err_close:
  vde_connection_fini(conn);
  vde_connection_delete(conn);
}

int vde2_conn_write(vde_connection *conn, vde_pkt *pkt)
{
  vde2_pkt *v2_pkt;
  vde2_conn *v2_conn = vde_connection_get_priv(conn);

  if (vde_queue_get_length(v2_conn->pkt_queue) >= MAXQLEN) {
    vde_warning("%s: packet queue for %s is full, discarding",
                __PRETTY_FUNCTION__, v2_conn->data_fd);
    errno = EAGAIN;
    return -1; // discard pkt
  }
  if (pkt->data_size > PKT_DATA_SZ) {
    // XXX: should alloc a struct greater than sizeof(vde2_pkt)
    vde_warning("%s: packet size larger than vde2_pkt, discarding",
                __PRETTY_FUNCTION__);
    errno = EBADMSG;
    return -1;
  }
  v2_pkt = vde_cached_alloc(sizeof(vde2_pkt));
  if (v2_pkt == NULL) {
    vde_warning("%s: cannot alloc new pkt, discarding", __PRETTY_FUNCTION__);
    errno = ENOMEM;
    return -1;
  }

  v2_pkt->numtries = 0;

  vde_pkt_compact_cpy(&v2_pkt->pkt, pkt);

  // XXX: check push ok
  vde_queue_push_head(v2_conn->pkt_queue, v2_pkt);

  if (v2_conn->data_ev_wr == NULL) {
    v2_conn->data_ev_wr = vde_context_event_add(
                            vde_connection_get_context(conn),
                            v2_conn->data_fd,
                            VDE_EV_WRITE|VDE_EV_PERSIST,
                            vde_connection_get_send_maxtimeout(conn),
                            &vde2_conn_write_data_event,
                            (void *)v2_conn);
  }
  return 0;
}

void vde2_conn_close(vde_connection *conn)
{
  vde2_pkt *pkt;
  vde2_conn *v2_conn = vde_connection_get_priv(conn);
  vde_context *ctx = vde_connection_get_context(conn);

  if (v2_conn->data_fd >= 0){
    close(v2_conn->data_fd);
  }
  if (v2_conn->data_ev_rd != NULL) {
    vde_context_event_del(ctx, v2_conn->data_ev_rd);
  }
  if (v2_conn->data_ev_wr != NULL) {
    vde_context_event_del(ctx, v2_conn->data_ev_wr);
  }
  if (v2_conn->local_sa.sun_path != NULL) {
    unlink(v2_conn->local_sa.sun_path);
  }
  if (v2_conn->ctl_fd >= 0){
    close(v2_conn->ctl_fd);
  }
  if (v2_conn->ctl_ev != NULL) {
    vde_context_event_del(ctx, v2_conn->ctl_ev);
  }
  if (v2_conn->remote_request) {
    vde_free(v2_conn->remote_request);
  }
  pkt = vde_queue_pop_tail(v2_conn->pkt_queue);
  while (pkt != NULL) {
    // XXX: handle dynamic allocation case
    vde_cached_free_type(vde2_pkt, pkt);
    pkt = vde_queue_pop_tail(v2_conn->pkt_queue);
  }
  vde_queue_delete(v2_conn->pkt_queue);

  vde_free(v2_conn);
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

// XXX: check VDE_DARWIN defines here!!!
void vde2_srv_send_request(int ctl_fd, short event_type, void *arg)
{
  int len;
#ifdef VDE_DARWIN
  int sockbufsize = DATA_BUF_SIZE;
  int optsize = sizeof(sockbufsize);
#endif
  vde2_conn *v2_conn = (vde2_conn *)arg;
  vde_connection *conn = v2_conn->conn;
  vde2_tr *tr = (vde2_tr *)vde_component_get_priv(v2_conn->transport);
  vde_context *ctx = vde_component_get_context(v2_conn->transport);

  vde_context_event_del(ctx, v2_conn->ctl_ev);
  v2_conn->ctl_ev = NULL;

  // XXX: define a behaviour when called if event timeout expired

  if ((v2_conn->data_fd = socket(PF_UNIX, SOCK_DGRAM, 0)) < 0) {
    vde_error("%s: cannot create datagram socket: %s", __PRETTY_FUNCTION__,
              strerror(errno));
    goto error;
  }
  if (fcntl(v2_conn->data_fd, F_SETFL, O_NONBLOCK) < 0) {
    vde_error("%s: cannot set O_NONBLOCK for datagram socket: %s",
              __PRETTY_FUNCTION__, strerror(errno));
    goto error;
  }
#ifdef VDE_DARWIN
  if (setsockopt(v2_conn->data_fd, SOL_SOCKET, SO_SNDBUF, &sockbufsize,
      optsize) < 0) {
      vde_warning("%s: cannot set datagram send bufsize to %s on fd %d: %s",
                  __PRETTY_FUNCTION__, sockbufsize, v2_conn->data_fd,
                  strerror(errno));
  }
  if (setsockopt(v2_conn->data_fd, SOL_SOCKET, SO_RCVBUF, &sockbufsize,
      optsize) < 0) {
      vde_warning("%s: cannot set datagram send bufsize to %s on fd %d: %s",
                  __PRETTY_FUNCTION__, sockbufsize, v2_conn->data_fd,
                  strerror(errno));
  }
#endif

  v2_conn->local_sa.sun_family = AF_UNIX;

  snprintf(v2_conn->local_sa.sun_path, sizeof(v2_conn->local_sa.sun_path),
           "%s/%04d", tr->vdesock_dir, tr->connections);

  if (unlink(v2_conn->local_sa.sun_path) < 0 && errno != ENOENT) {
    vde_error("%s: cannot remove old datagram socket %s: %s",
              __PRETTY_FUNCTION__, v2_conn->local_sa.sun_path,
              strerror(errno));
    goto error;
  }
  if (bind(v2_conn->data_fd, (struct sockaddr *) &v2_conn->local_sa,
           sizeof(struct sockaddr_un)) < 0) {
    vde_error("%s: cannot bind datagram socket %s: %s", __PRETTY_FUNCTION__,
              v2_conn->local_sa.sun_path, strerror(errno));
    goto error;
  }

  len = write(v2_conn->ctl_fd, &v2_conn->local_sa, sizeof(v2_conn->local_sa));
  if (len != sizeof(v2_conn->local_sa)) {
    vde_error("%s: cannot reply to peer", __PRETTY_FUNCTION__);
    goto error;
  }

  tr->connections++;
  tr->pending_conns = vde_list_remove(tr->pending_conns, v2_conn);

  // XXX: check events not NULL
  v2_conn->ctl_ev = vde_context_event_add(ctx, v2_conn->ctl_fd,
                                          VDE_EV_READ|VDE_EV_PERSIST, NULL,
                                          &vde2_conn_read_ctl_event,
                                          (void *)v2_conn);
  v2_conn->data_ev_rd = vde_context_event_add(ctx, v2_conn->data_fd,
                                              VDE_EV_READ|VDE_EV_PERSIST,
                                              NULL, &vde2_conn_read_data_event,
                                              (void *)v2_conn);

  vde_transport_call_cm_accept_cb(v2_conn->transport, conn);

  return;

error:
  // XXX: call connection manager error callback here?
  tr->pending_conns = vde_list_remove(tr->pending_conns, v2_conn);
  vde_connection_fini(conn);
  vde_connection_delete(conn);
}

void vde2_srv_get_request(int ctl_fd, short event_type, void *arg)
{
  int len;
  char reqbuf[REQBUFLEN+1];
  request *req=(request *)reqbuf;
  vde2_conn *v2_conn = (vde2_conn *)arg;
  vde_connection *conn = v2_conn->conn;
  vde2_tr *tr = (vde2_tr *)vde_component_get_priv(v2_conn->transport);
  vde_context *ctx = vde_component_get_context(v2_conn->transport);

  vde_context_event_del(ctx, v2_conn->ctl_ev);
  v2_conn->ctl_ev = NULL;

  // XXX: define a behaviour when called if event timeout expired
  len = read(v2_conn->ctl_fd, reqbuf, REQBUFLEN);
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

    // XXX: for the moment we save whole request..
    v2_conn->remote_request = (request *)vde_alloc(len);
    if (!v2_conn->remote_request) {
      vde_error("%s: cannot allocate memory for remote request",
                __PRETTY_FUNCTION__);
      goto error;
    }
    memcpy(v2_conn->remote_request, reqbuf, len);
    // XXX: add peer credentials to conn.attributes

    memcpy(&v2_conn->remote_sa, &req->sock, sizeof(struct sockaddr_un));
    // XXX: check event NULL and define a timeout
    v2_conn->ctl_ev = vde_context_event_add(ctx, v2_conn->ctl_fd,
                                            VDE_EV_WRITE, NULL,
                                            &vde2_srv_send_request,
                                            (void *)v2_conn);
  }

  return;

error:
  // XXX: call connection manager error callback here?
  tr->pending_conns = vde_list_remove(tr->pending_conns, v2_conn);
  vde_connection_fini(conn);
  vde_connection_delete(conn);
}

void vde2_accept(int listen_fd, short event_type, void *arg)
{
  struct sockaddr sa;
  socklen_t sa_len = sizeof(struct sockaddr);
  int new;
  vde_connection *conn;
  struct vde2_conn *v2_conn;
  vde_component *component = (vde_component *)arg;
  vde_context *ctx = vde_component_get_context(component);
  vde2_tr *tr = (vde2_tr *)vde_component_get_priv(component);

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
  v2_conn = (struct vde2_conn *)vde_calloc(sizeof(struct vde2_conn));
  if (!v2_conn) {
    vde_error("%s: cannot create connection backend", __PRETTY_FUNCTION__);
    goto error_conn_del;
  }

  v2_conn->ctl_fd = new;
  v2_conn->conn = conn;
  v2_conn->transport = component;
  // XXX: check init result
  v2_conn->pkt_queue = vde_queue_init();

  // XXX: check error on list
  tr->pending_conns = vde_list_prepend(tr->pending_conns, v2_conn);

  vde_connection_init(conn, ctx, sizeof(struct eth_frame), &vde2_conn_write,
                      &vde2_conn_close, (void *)v2_conn);

  // XXX: check event NULL and define a timeout
  v2_conn->ctl_ev = vde_context_event_add(ctx, v2_conn->ctl_fd, VDE_EV_READ,
                                          NULL, &vde2_srv_get_request,
                                          (void *)v2_conn);

  return;

error_conn_del:
  // XXX: call connection manager error callback here?
  vde_connection_delete(conn);
error_close:
  close(new);
}

int vde2_listen(vde_component *component)
{
  int tmp_errno; /* errno will be set back in last goto label */
  struct sockaddr_un sa_unix;
  int one = 1;
  vde_context *ctx = vde_component_get_context(component);
  vde2_tr *tr = (vde2_tr *)vde_component_get_priv(component);

  tr->listen_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (tr->listen_fd < 0) {
    tmp_errno = errno;
    vde_error("%s: Could not obtain a BSD socket: %s", __PRETTY_FUNCTION__,
              strerror(errno));
    goto error;
  }
  if (setsockopt(tr->listen_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&one,
                 sizeof(one)) < 0) {
    tmp_errno = errno;
    vde_error("%s: Could not set socket options: %s", __PRETTY_FUNCTION__,
              strerror(errno));
    goto error_close;
  }
  if (fcntl(tr->listen_fd, F_SETFL, O_NONBLOCK) < 0) {
    tmp_errno = errno;
    vde_error("%s: Could not set O_NONBLOCK: %s", __PRETTY_FUNCTION__,
              strerror(errno));
    goto error_close;
  }
  if (((mkdir(tr->vdesock_dir, 0777) < 0) && (errno != EEXIST))) {
    tmp_errno = errno;
    vde_error("%s: Could not create vdesock directory %s: %s",
              __PRETTY_FUNCTION__, tr->vdesock_dir, strerror(errno));
    goto error_close;
  }
  sa_unix.sun_family = AF_UNIX;
  snprintf(sa_unix.sun_path, sizeof(sa_unix.sun_path), "%s/ctl",
           tr->vdesock_dir);
  if (bind(tr->listen_fd, (struct sockaddr *)&sa_unix, sizeof(sa_unix)) < 0) {
    if ((errno == EADDRINUSE) && vde2_remove_sock_if_unused(&sa_unix)) {
      tmp_errno = errno;
      vde_error("%s: Could not bind to %s/ctl: socket in use",
                __PRETTY_FUNCTION__, tr->vdesock_dir);
      goto error_close;
    } else {
      if (bind(tr->listen_fd, (struct sockaddr *)&sa_unix,
               sizeof(sa_unix)) < 0) {
        tmp_errno = errno;
        vde_error("%s: Could not bind to %s/ctl: %s", __PRETTY_FUNCTION__,
                  tr->vdesock_dir, strerror(errno));
        goto error_close;
      }
    }
  }
  if (listen(tr->listen_fd, LISTEN_QUEUE) < 0) {
    tmp_errno = errno;
    vde_error("%s: Could not listen: %s", __PRETTY_FUNCTION__,
              strerror(errno));
    goto error_unlink;
  }

  // XXX: check event not NULL, define a timeout?
  tr->listen_event = vde_context_event_add(ctx, tr->listen_fd,
                                           VDE_EV_READ | VDE_EV_PERSIST, NULL,
                                           &vde2_accept, (void *)component);

  return 0;

error_unlink:
  unlink(sa_unix.sun_path);
  rmdir(tr->vdesock_dir);
error_close:
  close(tr->listen_fd);
  tr->listen_fd = -1;
error:
  errno = tmp_errno;
  return -1;
}

int vde2_connect(vde_component *component, vde_connection *conn)
{
  return -1;
}

static int transport_vde2_init(vde_component *component, const char *dir)
{

  vde2_tr *tr;

  vde_assert(component != NULL);

  if (strlen(dir) > UNIX_PATH_MAX - 4) { // we will add '/ctl' later
    vde_error("%s: directory name is too long", __PRETTY_FUNCTION__);
    errno = EINVAL;
    return -1;
  }
  tr = (vde2_tr *)vde_calloc(sizeof(vde2_tr));
  if (tr == NULL) {
    vde_error("%s: could not allocate private data", __PRETTY_FUNCTION__);
    errno = ENOMEM;
    return -1;
  }

  // XXX: path needs to be normalized/checked somewhere
  tr->vdesock_dir = strdup(dir);
  if (tr->vdesock_dir == NULL) {
    vde_free(tr);
    vde_error("%s: could not allocate private path", __PRETTY_FUNCTION__);
    errno = ENOMEM;
    return -1;
  }

  vde_transport_set_ops(component, &vde2_listen, &vde2_connect);
  vde_component_set_priv(component, (void *)tr);
  return 0;
}

int transport_vde2_va_init(vde_component *component, va_list args)
{
  const char *path = va_arg(args, const char *);

  return transport_vde2_init(component, path);
}

// XXX to be defined
void transport_vde2_fini(vde_component *component) {
  vde_assert(component != NULL);
}

struct component_ops transport_vde2_component_ops = {
  .init = transport_vde2_va_init,
  .fini = transport_vde2_fini,
  .get_configuration = NULL,
  .set_configuration = NULL,
  .get_policy = NULL,
  .set_policy = NULL,
};

vde_module VDE_MODULE_START = {
  .kind = VDE_TRANSPORT,
  .family = "vde2",
  .cops = &transport_vde2_component_ops,
};
