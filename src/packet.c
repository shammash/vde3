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

#include <string.h>

#include <vde3/packet.h>

void vde_pkt_init(vde_pkt *pkt, unsigned int data, unsigned int head,
                  unsigned int tail) {
  pkt->hdr = (vde_hdr *)pkt->data;
  pkt->head = pkt->data + sizeof(vde_hdr);
  pkt->payload = pkt->head + head;
  pkt->tail = pkt->data + data - tail;
  pkt->data_size = data;
}

void vde_pkt_cpy(vde_pkt *dst, vde_pkt *src) {
  vde_pkt_init(dst, src->data_size,
               src->payload - src->head,
               src->data + src->data_size - src->tail);
  memcpy(&dst->data, &src->data, src->data_size);
}

void vde_pkt_compact_cpy(vde_pkt *dst, vde_pkt *src) {
  vde_pkt_init(dst, src->data_size, 0, 0);
  memcpy(dst->hdr, src->hdr, sizeof(vde_hdr));
  memcpy(dst->payload, src->payload, src->hdr->pkt_len);
}

