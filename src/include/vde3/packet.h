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

#ifndef __VDE3_PACKET_H__
#define __VDE3_PACKET_H__

#include <stdint.h>
#include <string.h>

#include <vde3/common.h>

// A packet exchanged by vde engines
// (it should be used more or less like Linux socket buffers).
//
// - allocated/freed by the same connection (probably using cached memory)
// - copied mostly by connections, but also by engines if they need to cache it
//   or to mangle it in particular ways

/**
 * @brief A vde packet header.
 */
// XXX(shammash): check alignment
typedef struct {
  uint8_t version; //!< Header version
  uint8_t type; //!< Type of payload
  uint16_t pkt_len; //!< Payload length
} vde_hdr;


/**
 * @brief A vde packet.
 */
typedef struct {
  vde_hdr *hdr; //!< Pointer to vde_header inside data
  char *head; //!< Pointer to an empty head space inside data
  char *payload; //!< Pointer to payload inside data
  char *tail; //!< Pointer to an empty tail space inside data
  unsigned int data_size; //!< The total size of memory allocated in data
  char data[0]; //!< Allocated memory
} vde_pkt;

/**
 * @brief Initialize vde packet fields.
 *
 * @param pkt The packet to initialize
 * @param data The size of preallocated memory
 * @param head The size of the space before payload
 * @param tail The size of the space after payload
 */
static inline void vde_pkt_init(vde_pkt *pkt, unsigned int data,
                                unsigned int head, unsigned int tail) {
  pkt->hdr = (vde_hdr *)pkt->data;
  pkt->head = pkt->data + sizeof(vde_hdr);
  pkt->payload = pkt->head + head;
  pkt->tail = pkt->data + data - tail;
  pkt->data_size = data;
}

/**
 * @brief Allocate and initialize a new vde_pkt
 *
 * @param payload_sz The size of the payload
 * @param head The size of the space before payload
 * @param tail The size of the space after payload
 *
 * @return The new packet on success, NULL on error (and errno is set
 * appropriately)
 */
static inline vde_pkt *vde_pkt_new(unsigned int payload_sz, unsigned int head,
                                   unsigned int tail)
{
  unsigned int data_sz = sizeof(vde_hdr) + head + payload_sz + tail;
  unsigned int pkt_sz = sizeof(vde_pkt) + data_sz;
  // XXX consider using cached allocator
  vde_pkt *pkt = (vde_pkt *)vde_calloc(pkt_sz);

  if (pkt == NULL) {
    errno = ENOMEM;
    return NULL;
  }
  vde_pkt_init(pkt, data_sz, head, tail);
  return pkt;
}

/**
 * @brief Copy the content of a packet into another pre-allocated packet
 *
 * @param dst The destination of the copy, the user of this function must check
 * in advance that the destination data_size can contain the copy.
 * @param src The source of the copy
 */
static inline void vde_pkt_cpy(vde_pkt *dst, vde_pkt *src) {
  vde_pkt_init(dst, src->data_size,
               src->payload - src->head,
               src->data + src->data_size - src->tail);
  memcpy(&dst->data, &src->data, src->data_size);
}

/**
 * @brief Copy the content of a packet into another pre-allocated packet. Does
 * not keep head/tail space
 *
 * @param dst The destination of the copy, the user of this function must check
 * in advance that the destination data_size can contain the copy.
 * @param src The source of the copy
 */
static inline void vde_pkt_compact_cpy(vde_pkt *dst, vde_pkt *src) {
  vde_pkt_init(dst, src->data_size, 0, 0);
  memcpy(dst->hdr, src->hdr, sizeof(vde_hdr));
  memcpy(dst->payload, src->payload, src->hdr->pkt_len);
}

// When a packet is read from the network by a connection the payload always
// follows the header, so head size and tail size are zero.
// If a connection implementation does not handle generic vde data but specific
// payload types (e.g.: vde2-compatibile transport or tap transport) it will
// populate the header of a new packet.

// An engine/connection_manager can instruct the connection to pre-allocate
// additional memory around the payload for further elaboration:
//void vde_connection_set_pkt_properties(vde_connection *conn,
//                                       unsigned int head_sz,
//                                       unsigned int tail_sz);

// For further speedup certain implementations might want to define their own
// data structure to be used when specific properties are set by the engine.
// struct vde2_transport_pkt {
//   vde_pkt pkt;
//   char data[1540]; // sizeof(vde_hdr) +
//                    // 4 (head reserved for vlan tags) +
//                    // 1504 (frame eth+trailing)
// };
//
// Inside a connection handling read event:
// struct vde2_transport_pkt stack_pkt;
// vde_pkt *pkt;
// ...
// if(conn->head_sz == 4 && conn->tail_sz == 0) {
//   ... set fields as explained above ...
//   read(stack_pkt.pkt.payload, MAX_ETH_FRAME_SIZE);
//   *pkt = stack_pkt;
// } else {
//   ... alloc a new vde_pkt with the necessary data and read ...
// }
// ... set packet fields and pass it to the engine ...


#endif /* __VDE3_PACKET_H__ */
