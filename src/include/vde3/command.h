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

#ifndef __VDE3_COMMAND_H__
#define __VDE3_COMMAND_H__

#include <stdbool.h>

#include <vde3/common.h>

// TODO: funzioni per allocare/inizializzare comando e parametro

// TODO(shammash):
//  se si vogliono esporre delle funzionalita` dei comandi alle applicazioni si
//  puo` considerare una cosa tipo:
//   - funzionalita` esposta all'applicazione
//    struct vlan *vlan_add(component, int puppa, int fuffa, char* name);
//   - comando che wrappa in qualche modo vlan_add(), da cui poi viene generato
//     automaticamente il wrapper vlan_add_cmq_wrapper()
//    int vlan_add_cmd(component, int puppa, int fuffa, char* name, json_object);

// un singolo argomento
typedef struct {
  char const * const name;
  char const * const description;
  char const * const type;
} vde_argument;

// un comando
typedef struct {
  char const * const name;
  // TODO definire qualcosa al posto di struct json_object
  bool const (*func)(vde_serial_obj *in, vde_serial_obj **out);
  char const * const description;
  vde_argument const * const args;
} vde_command;



#endif /* __VDE3_COMMAND_H__ */
