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

#include <vde3/localconnection.h>

#include <vde3/common.h>
#include <vde3/engine.h>

/*
 * Unqueued Local Connection
 * (no write callback is performed as packets are delivered immediately).
 *
 */

typedef struct __vde_lc {
  vde_connection *conn;
  struct __vde_lc *peer;
} vde_lc;

int vde_lc_write(vde_connection *conn, vde_pkt *pkt)
{
  int tmp_errno;
  vde_lc *lc = (vde_lc *)vde_connection_get_priv(conn);
  vde_lc *peer = lc->peer;
  vde_connection *peer_conn = peer->conn;

  if (peer == NULL) {
    return -1;
  }
  if (vde_connection_call_read(peer_conn, pkt)) {
    tmp_errno = errno;
    if (errno == EPIPE) {
      vde_connection_fini(peer_conn);
      vde_connection_delete(peer_conn);
    }
    errno = tmp_errno;
    return -1;
  }
  return 0;
}

void vde_lc_close(vde_connection *conn)
{
  vde_lc *lc = (vde_lc *)vde_connection_get_priv(conn);
  vde_lc *peer = lc->peer;
  vde_connection *peer_conn = peer->conn;

  if (peer != NULL) {
    peer->peer = NULL; // detach from peer to avoid circular close calls
    if (vde_connection_call_error(peer_conn, NULL, CONN_READ_CLOSED) &&
        (errno == EPIPE)) {
        vde_connection_fini(peer_conn);
        vde_connection_delete(peer_conn);
    } else {
      vde_warning("%s: called fatal error but engine did not close",
          __PRETTY_FUNCTION__);
    }
  }

  vde_free(lc);
}

int vde_connect_engines_unqueued(vde_context *ctx, vde_component *engine1,
                                 vde_request *req1, vde_component *engine2,
                                 vde_request *req2)
{
  int tmp_errno;

  vde_assert(ctx != NULL);
  vde_assert(engine1 != NULL);
  vde_assert(engine2 != NULL);

  // XXX: request must be normalized here (assert != NULL as well?)

  vde_connection *c1, *c2;
  vde_lc *lc1, *lc2;

  lc1 = (vde_lc *)vde_calloc(sizeof(vde_lc));
  if (lc1 == NULL) {
    vde_error("%s: cannot create local connection data");
    tmp_errno = ENOMEM;
    goto err_out;
  }
  lc2 = (vde_lc *)vde_calloc(sizeof(vde_lc));
  if (lc2 == NULL) {
    vde_error("%s: cannot create local connection data");
    tmp_errno = ENOMEM;
    goto err_lc1;
  }

  vde_connection_new(&c1);
  vde_connection_new(&c2);

  lc1->conn = c1;
  lc2->conn = c2;

  lc1->peer = lc2;
  lc2->peer = lc1;

  vde_connection_init(c1, ctx, 0, &vde_lc_write, &vde_lc_close, (void *)lc1);
  vde_connection_init(c2, ctx, 0, &vde_lc_write, &vde_lc_close, (void *)lc2);

  if (vde_engine_new_connection(engine1, c1, req1) != 0) {
    vde_error("%s: cannot connect to first engine");
    goto err_lc2;
  }
  if (vde_engine_new_connection(engine2, c2, req2) != 0) {
    vde_error("%s: cannot connect to second engine");
    goto err_eng1;
  }

  return 0;

err_eng1:
  // XXX consider assert errno == EPIPE in these cases
  if (vde_connection_call_error(c1, NULL, CONN_READ_CLOSED) &&
      (errno == EPIPE)) {
      vde_connection_fini(c1);
      vde_connection_delete(c1);
  } else {
    vde_warning("%s: called fatal error but engine did not close",
        __PRETTY_FUNCTION__);
  }
err_lc2:
  free(lc2);
err_lc1:
  free(lc1);
err_out:
  errno = tmp_errno;
  return -1;
}

