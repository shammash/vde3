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

#ifndef __VDE3_COMMAND_H__
#define __VDE3_COMMAND_H__

#include <vde3.h>

#include <vde3/common.h>

/**
 * @brief The signature of a function implementing a command
 *
 * @param component The component executing the command
 * @param in Command serialized parameters
 * @param out Command output
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
/* XXX: Define behaviour in case of error. Some ideas:
 * - out sobject is used as an error as-is
 * - out sobject is included in an error object with the following structure:
 *   + err.code = errno
 *   + err.msg  = out sobject
 */
typedef int (*command_func)(vde_component *component, vde_sobj *in,
                            vde_sobj **out);

/**
 * @brief Description of a single command argument
 */
struct vde_argument {
  char const * const name;
  char const * const description;
  char const * const type;
};
typedef struct vde_argument vde_argument;

/**
 * @brief A command
 */
struct vde_command {
  char const * const name;
  command_func func;
  char const * const description;
  vde_argument const * const args;
};
typedef struct vde_command vde_command;

static inline const char *vde_command_get_name(vde_command *command)
{
  vde_assert(command != NULL);

  return command->name;
}

static inline command_func vde_command_get_func(vde_command *command)
{
  vde_assert(command != NULL);

  return command->func;
}

#endif /* __VDE3_COMMAND_H__ */
