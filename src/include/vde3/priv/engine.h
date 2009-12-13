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

#ifndef __VDE3_PRIV_ENGINE_H__
#define __VDE3_PRIV_ENGINE_H__

struct engine_ops {
  int (*new_conn)(vde_connection *, vde_request *),
};

typedef struct engine_ops engine_ops;

// needed callbacks:
read_cb(vde_connection *conn, vde_pkt *pkt, void *cb_priv);
error_cb(vde_connection *conn, error_type, vde_pkt *pkt, void *cb_priv);


#endif /* __VDE3_PRIV_ENGINE_H__ */
