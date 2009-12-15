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

#ifndef __VDE3_PACKET_H__
#define __VDE3_PACKET_H__

// A packet exchanged by vde engines
// (it should be used more or less like Linux socket buffers).
//
// - allocated/freed by the same connection (probably using cached memory)
// - copied mostly by connections, but also by engines if they need to cache it
//   or to mangle it in particular ways

// XXX(shammash): check alignment
struct vde_hdr {
  uint8_t version, // Header version
  uint8_t type, // Type of payload
  uint16_t pkt_len, // Payload length
};

typedef struct vde_hdr vde_hdr;

struct vde_pkt {
  vde_hdr *hdr, // Pointer to vde_header inside data
  void *head, // Pointer to an empty head space inside data
  void *payload, // Pointer to payload inside data
  void *tail // Pointer to an empty tail space inside data
  unsigned int data_size, // The total size of memory allocated in data
  char data[0], // Allocated memory
};

typedef struct vde_pkt vde_pkt;

// When a packet is read from the network by a connection the payload always
// follows the header, so head size and tail size are zero.
// If a connection implementation does not handle generic vde data but specific
// payload types (e.g.: vde2-compatibile transport or tap transport) it will
// populate the header of a new packet.

// An engine/connection_manager can instruct the connection to pre-allocate
// additional memory around the payload for further elaboration:
vde_connection_set_pkt_properties(vde_connection *conn,
                                  unsigned int head_sz, unsigned int tail_sz);

// For further speedup certain implementations might want to define their own
// data structure to be used when specific properties are set by the engine.
struct vde2_transport_pkt {
  vde_hdr *hdr, //  = &data
  void *head, //    = &data + sizeof(vde_hdr)
  void *payload, // = &data + sizeof(vde_hdr) + 4
  void *tail //     = NULL (??)
  char data[1540], // sizeof(vde_hdr) + 4 (head reserved for vlan tags) + 1504 (frame eth+trailing)
};

int optimized_read_4_0(vde_pkt **pkt) {
  struct vde2_transport_pkt stack_pkt;
  // set fields as explained above
  rv = read(stack_pkt.payload, MAX_ETH_FRAME_SIZE);
  *pkt = stack_pkt;
  return rv;
}

// ... inside a connection handling read event ...
vde_pkt *pkt;
if(conn->head_sz == 4 && conn->tail_sz == 0) {
  rv = optimized_read_4_0(&pkt);
} else {
  // alloc memory and read
}


#endif /* __VDE3_PACKET_H__ */
