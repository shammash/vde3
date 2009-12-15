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

#ifndef __VDE3_COMPONENT_H__
#define __VDE3_COMPONENT_H__

struct vde_component;

/**
* @brief VDE 3 Component
*/
typedef struct vde_component vde_component;

// XXX: vde_request

// Callbacks set by application to receive results of after a connect is called
// on the connection manager
typedef void (*vde_connect_success_cb)(vde_component *cm, void *arg);
typedef void (*vde_connect_error_cb)(vde_component *cm, void *arg);

#endif /* __VDE3_COMPONENT_H__ */
