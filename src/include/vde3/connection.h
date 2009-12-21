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

#ifndef __VDE3_CONNECTION_H__
#define __VDE3_CONNECTION_H__

#include <sys/time.h>

#include <vde3/attributes.h>
#include <vde3/packet.h>

enum vde_conn_error {
  CONN_OK,
  CONN_READ_CLOSED, // fatal error occurred during read
  CONN_READ_DELAY, // non-fatal error occurred during read
  CONN_WRITE_CLOSED,  // fatal error occurred during write
  CONN_WRITE_DELAY, // non-fatal error occurred during write
};

typedef enum vde_conn_error vde_conn_error;

/**
 * @brief A VDE 3 connection
 */
struct vde_connection;

typedef struct vde_connection vde_connection;

enum conn_cb_result {
  CONN_CB_OK, // the callback was successful
  CONN_CB_ERROR, // error while invoking callback
  CONN_CB_CLOSE, // the connection must be closed
  CONN_CB_REQUEUE, // the packet should be requeued for transmission
};

typedef enum conn_cb_result conn_cb_result;

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
typedef int (*conn_be_write)(vde_connection *conn, vde_pkt *pkt);

/**
 * @brief Backend implementation for closing a connection, when called the
 * backend must free all its resources for this connection.
 *
 * @param The connection to close
 */
typedef void (*conn_be_close)(vde_connection *conn);


/*
 * Functions set by a component which uses the connection
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
 * @return The result of callback
 */
typedef conn_cb_result (*conn_read_cb)(vde_connection *conn, vde_pkt *pkt,
                                       void *arg);

/**
 * @brief (Optional) Callback called when a packet has been sent by the
 * connection, after this callback returns the pkt will be free()d.
 *
 * @param conn The connection which has sent the packet
 * @param pkt The sent packet
 * @param arg The argument which has previously been set by connection user
 *
 * @return The result of callback
 */
typedef conn_cb_result (*conn_write_cb)(vde_connection *conn, vde_pkt *pkt,
                                        void *arg);

/**
 * @brief Callback called when an error occur.
 *
 * @param conn The connection generating the error
 * @param pkt The packet which was being sent when the error occurred, can be
 * NULL.
 * @param err The error type
 * @param arg The argument which has previously been set by connection user
 *
 * @return The result of callback
 */
/* XXX(godog): consider introducing the following semantic for conn_error_cb
 * return value: if it's non-zero and the error is non-fatal then the pkt is
 * re-queued for transmission
 */
typedef conn_cb_result (*conn_error_cb)(vde_connection *conn, vde_pkt *pkt,
                                        vde_conn_error err, void *arg);

/**
* @brief Alloc a new VDE 3 connection
*
* @param conn reference to new connection pointer
*
* @return zero on success, an error code otherwise
*/
int vde_connection_new(vde_connection **conn);

/**
 * @brief Initialize a new connection
 *
 * @param conn The connection to initialize
 * @param ctx The context in which the connection will run
 * @param be_write The backend implementation for writing packets
 * @param be_close The backend implementation for closing a connection
 * @param be_priv Backend private data
 *
 * @return zero on success, an error code otherwise
 */
int vde_connection_init(vde_connection *conn, vde_context *ctx,
                        conn_be_write be_write, conn_be_close be_close,
                        void *be_priv);

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
int vde_connection_write(vde_connection *conn, vde_pkt *pkt);

/**
 * @brief Function called by connection backend to tell the connection user a
 * new packet is available.
 *
 * @param conn The connection whom backend has a new packet available
 * @param pkt The new packet. Connection users will duplicate the pkt if they
 * need it after this callback will return, so it can be free()d afterwards
 *
 * @return TO BE DEFINED, probably void
 */
conn_cb_result vde_connection_call_read(vde_connection *conn, vde_pkt *pkt);

/**
 * @brief Function called by connection backend to tell the connection user a
 * packet has been successfully sent.
 *
 * @param conn The connection whom backend has a sent the packet
 * @param pkt The sent packet. Connection users will duplicate the pkt if they
 * need it after this callback will return, so it can be free()d afterwards
 *
 * @return TO BE DEFINED, probably void
 */
conn_cb_result vde_connection_call_write(vde_connection *conn, vde_pkt *pkt);

/**
 * @brief Function called by connection backend to tell the connection user an
 * error occurred.
 *
 * @param conn The connection whom backend is having an error
 * @param pkt The packet which was being sent when the error occurred, can be
 * NULL
 *
 * @return TO BE DEFINED..
 */
conn_cb_result vde_connection_call_error(vde_connection *conn, vde_pkt *pkt,
                              vde_conn_error err);

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
vde_context *vde_connection_get_context(vde_connection *conn);

/**
 * @brief Get connection backend private data
 *
 * @param conn The connection to receive private data from
 *
 * @return The private data
 */
void *vde_connection_get_priv(vde_connection *conn);

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
unsigned int vde_connection_get_pkt_headsize(vde_connection *conn);

/**
 * @brief Get the amount of free space to be reserved after packet payload
 *
 * @param conn The connection to get tailsize from
 *
 * @return The amount of space
 */
unsigned int vde_connection_get_pkt_tailsize(vde_connection *conn);

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
unsigned int vde_connection_get_send_maxtries(vde_connection *conn);

/**
 * @brief Get the maximum timeout between two send attempts
 *
 * @param conn The connection to get the maximum timeout from
 *
 * @return A reference to maximum timeout
 */
struct timeval *vde_connection_get_send_maxtimeout(vde_connection *conn);

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
