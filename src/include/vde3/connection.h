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

#ifndef __VDE3_CONNECTION_H__
#define __VDE3_CONNECTION_H__

#include <sys/time.h>
#include <limits.h>

#include <vde3/attributes.h>
#include <vde3/packet.h>
#include <vde3/common.h>


/**
 * @brief An error code passed to conn_error_cb
 */
typedef enum {
  CONN_OK, //!< Connection OK
  CONN_READ_CLOSED, //!< fatal error occurred during read
  CONN_READ_DELAY, //!< non-fatal error occurred during read
  CONN_WRITE_CLOSED,  //!< fatal error occurred during write
  CONN_WRITE_DELAY, //!< non-fatal error occurred during write
} vde_conn_error;

/**
 * @brief A VDE 3 connection
 */
typedef struct vde_connection vde_connection;

/*
 * Functions to be implemented by a connection backend/transport
 *
 */

/**
 * @brief Backend implementation for writing a packet
 *
 * @param conn The connection to send the packet to
 * @param pkt The packet to send, if the backend doesn't send the packet
 * immediately it must reserve its own copy of the packet.
 *
 * @return zero on success, an error code otherwise
 */
/*
 * XXX(shammash): add a flag here to tell the connection backend if the packet
 * needs to be copied or the same memory area / pointer can be used? This can be
 * useful if, for instance, we have a send-queue in the engine and in the
 * connection: right now the engine would have its own copy of the packet which
 * needs to be copied again in the connection.
 */
typedef int (*conn_be_write)(vde_connection *conn, vde_pkt *pkt);

/**
 * @brief Backend implementation for closing a connection, when called the
 * backend must free all its resources for this connection.
 *
 * @param The connection to close
 */
typedef void (*conn_be_close)(vde_connection *conn);


/*
 * Functions set by a component which uses the connection.
 *
 * These functions return 0 on success, -1 on error (and errno is set
 * appropriately). The following errno values have special meanings:
 * - EAGAIN: requeue the packet
 * - EPIPE: close the connection
 *
 */

/**
 * @brief Callback called when a connection has a packet ready to serve, after
 * this callback returns the pkt will be free()d
 *
 * @param conn The connection with the packet ready
 * @param pkt The new packet
 * @param arg The argument which has previously been set by connection user
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
typedef int (*conn_read_cb)(vde_connection *conn, vde_pkt *pkt, void *arg);

/**
 * @brief (Optional) Callback called when a packet has been sent by the
 * connection, after this callback returns the pkt will be free()d.
 *
 * @param conn The connection which has sent the packet
 * @param pkt The sent packet
 * @param arg The argument which has previously been set by connection user
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
typedef int (*conn_write_cb)(vde_connection *conn, vde_pkt *pkt, void *arg);

/**
 * @brief Callback called when an error occur.
 *
 * @param conn The connection generating the error
 * @param pkt The packet which was being sent when the error occurred, can be
 * NULL.
 * @param err The error type
 * @param arg The argument which has previously been set by connection user
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
/* XXX(godog): consider introducing the following semantic for conn_error_cb
 * return value: if it's non-zero and the error is non-fatal then the pkt is
 * re-queued for transmission
 */
typedef int (*conn_error_cb)(vde_connection *conn, vde_pkt *pkt,
                             vde_conn_error err, void *arg);


/**
 * @brief A vde connection.
 */
struct vde_connection {
  vde_attributes *attributes;
  vde_context *context;
  unsigned int max_pload;
  unsigned int pkt_head_sz;
  unsigned int pkt_tail_sz;
  unsigned int send_maxtries;
  struct timeval send_maxtimeout;
  conn_be_write be_write;
  conn_be_close be_close;
  void *be_priv;
  conn_read_cb read_cb;
  conn_write_cb write_cb;
  conn_error_cb error_cb;
  void *cb_priv;
};


/**
 * @brief Alloc a new VDE 3 connection
 *
 * @param conn reference to new connection pointer
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_connection_new(vde_connection **conn);

/**
 * @brief Initialize a new connection
 *
 * @param conn The connection to initialize
 * @param ctx The context in which the connection will run
 * @param payload_size The maximum payload size a connection implementation can
 * handle, 0 for unlimited.
 * @param be_write The backend implementation for writing packets
 * @param be_close The backend implementation for closing a connection
 * @param be_priv Backend private data
 *
 * @return zero on success, an error code otherwise
 */
int vde_connection_init(vde_connection *conn, vde_context *ctx,
                        unsigned int payload_size, conn_be_write be_write,
                        conn_be_close be_close, void *be_priv);

/**
 * @brief Finalize a VDE 3 connection
 *
 * @param conn The connection to finalize
 */
void vde_connection_fini(vde_connection *conn);

/**
 * @brief Deallocate a VDE 3 connection
 *
 * @param conn The connection to free
 */
void vde_connection_delete(vde_connection *conn);

/**
 * @brief Function used by connection user to send a packet
 *
 * @param conn The connection to send the packet into
 * @param pkt The packet to send
 *
 * @return zero on success, an error code otherwise
 */
static inline int vde_connection_write(vde_connection *conn, vde_pkt *pkt)
{
  vde_assert(conn != NULL);

  return conn->be_write(conn, pkt);
}

/**
 * @brief Function called by connection backend to tell the connection user a
 * new packet is available.
 *
 * @param conn The connection whom backend has a new packet available
 * @param pkt The new packet. Connection users will duplicate the pkt if they
 * need it after this callback will return, so it can be free()d afterwards
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
static inline int vde_connection_call_read(vde_connection *conn, vde_pkt *pkt)
{
  vde_assert(conn != NULL);
  vde_assert(conn->read_cb != NULL);

  return conn->read_cb(conn, pkt, conn->cb_priv);
}

/**
 * @brief Function called by connection backend to tell the connection user a
 * packet has been successfully sent.
 *
 * @param conn The connection whom backend has a sent the packet
 * @param pkt The sent packet. Connection users will duplicate the pkt if they
 * need it after this callback will return, so it can be free()d afterwards
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
static inline int vde_connection_call_write(vde_connection *conn, vde_pkt *pkt)
{
  vde_assert(conn != NULL);

  if (conn->write_cb != NULL) {
    return conn->write_cb(conn, pkt, conn->cb_priv);
  }
  return 0;
}

/**
 * @brief Function called by connection backend to tell the connection user an
 * error occurred.
 *
 * @param conn The connection whom backend is having an error
 * @param pkt The packet which was being sent when the error occurred, can be
 * NULL
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
static inline int vde_connection_call_error(vde_connection *conn, vde_pkt *pkt,
                                            vde_conn_error err)
{
  vde_assert(conn != NULL);
  vde_assert(conn->error_cb != NULL);

  return conn->error_cb(conn, pkt, err, conn->cb_priv);
}

/**
 * @brief Set user's callbacks in a connection
 *
 * @param conn The connection to set callbacks to
 * @param read_cb Function called when a new packet is available
 * @param write_cb Function called when a packet has been sent (can be NULL)
 * @param error_cb Function called when an error occurs
 * @param cb_priv User's private data
 */
void vde_connection_set_callbacks(vde_connection *conn,
                                  conn_read_cb read_cb,
                                  conn_write_cb write_cb,
                                  conn_error_cb error_cb,
                                  void *cb_priv);

/**
 * @brief Get connection context
 *
 * @param conn The connection
 *
 * @return The context in which the connection is running
 */
static inline vde_context *vde_connection_get_context(vde_connection *conn)
{
  vde_assert(conn != NULL);

  return conn->context;
}

/**
 * @brief Get maximum payload size a connection can handle
 *
 * @param conn The connection
 *
 * @return The maximum payload size, if 0 no limit is set.
 */
unsigned int vde_connection_max_payload(vde_connection *conn);

/**
 * @brief Get connection backend private data
 *
 * @param conn The connection to receive private data from
 *
 * @return The private data
 */
static inline void *vde_connection_get_priv(vde_connection *conn)
{
  vde_assert(conn != NULL);

  return conn->be_priv;
}

/**
 * @brief Called by the component using the connection to set some properties
 * about the packet it will receive.
 *
 * @param conn The connection to set properties to
 * @param head_sz The amount of free space reserved before packet payload
 * @param tail_sz The amount of free space reserved after packet payload
 */
void vde_connection_set_pkt_properties(vde_connection *conn,
                                       unsigned int head_sz,
                                       unsigned int tail_sz);

/**
 * @brief Get the amount of free space to be reserved before packet payload
 *
 * @param conn The connection to get headsize from
 *
 * @return The amount of space
 */
static inline unsigned int vde_connection_get_pkt_headsize(vde_connection *conn)
{
  vde_assert(conn != NULL);

  return conn->pkt_head_sz;
}

/**
 * @brief Get the amount of free space to be reserved after packet payload
 *
 * @param conn The connection to get tailsize from
 *
 * @return The amount of space
 */
static inline
unsigned int vde_connection_get_pkt_tailsize(vde_connection *conn)
{
  vde_assert(conn != NULL);

  return conn->pkt_tail_sz;
}

/**
 * @brief Called by the component using the connection to set some packet
 * sending options. The connection will try to write the packet for AT MOST
 * max_timeout x max_tries time; after that if the send is failed conn_error_cb
 * is called to inform a packet has been dropped.
 *
 * @param conn The connection to set packet sending options to
 * @param max_tries The maximum number of attempts for sending a packet
 * @param max_timeout The maximum amount of time between two packet send
 * attempts
 */
void vde_connection_set_send_properties(vde_connection *conn,
                                        unsigned int max_tries,
                                        struct timeval *max_timeout);

/**
 * @brief Get the maximum number of tries a send should be performed
 *
 * @param conn The connection to get the maximum number of tries from
 *
 * @return The maximum number of tries
 */
static inline
unsigned int vde_connection_get_send_maxtries(vde_connection *conn)
{
  vde_assert(conn != NULL);

  return conn->send_maxtries;
}

/**
 * @brief Get the maximum timeout between two send attempts
 *
 * @param conn The connection to get the maximum timeout from
 *
 * @return A reference to maximum timeout
 */
static inline
struct timeval *vde_connection_get_send_maxtimeout(vde_connection *conn)
{
  vde_assert(conn != NULL);

  return &(conn->send_maxtimeout);
}

/**
 * @brief Set connection attributes, data will be duplicated
 *
 * @param conn The connection to set attributes to
 * @param attributes The attributes to set
 */
void vde_connection_set_attributes(vde_connection *conn,
                                   vde_attributes *attributes);

/**
 * @brief Get connection attributes
 *
 * @param conn The connection to get attributes from
 *
 * @return The attributes
 */
vde_attributes *vde_connection_get_attributes(vde_connection *conn);

/*
 * Memory management:
 * - a connection calls read_cb iff a packet is ready
 * - when write() is called the connection must be responsible for copying the
 * packet because because it will be free()ed right after write() returns
 * - a local connection duplicate the packet passed with write(), calls its
 * peer's read_cb and then free()s the duplicated packet
 *
 * DGRAM/STACK flow:
 * - connection: read on stack space
 * - connection: call read_cb()
 * - engine/cm: does stuff considering that when read_cb() returns the packet
 * will be free()d. e.g.:
 * for each c in connections:
 * if c != incoming_connection:
 * c.write(pkt)
 * - connection: memcpy(pkt) -> add(packetq)
 *
 * STREAM/BUFFER, incoming flow:
 * n = 0;
 * len = read(pktbuf+n, MAXBUFSIZE-n)
 * n+=len
 * if n < HDR_SIZE
 * return
 * if n < HDR_SIZE + pktbuf->hdr.len
 * return
 * read_cb(pktbuf)
 * n = 0
 *
 * Connection manager and rpcengine must deal with fragmentation.
 *
 */

#endif /* __VDE3_CONNECTION_H__ */
