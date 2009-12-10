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

#include <vde3.h>

#include <vde3/priv/logging.h>

static vde3_log_handler global_log_handler = NULL;

int vde3_log_set_handler(vde3_log_handler handler)
{
  global_log_handler = handler;
}

void vvde3_log(int priority, const char *format, va_list arg)
{
  if (global_log_handler)
    global_log_handler(priority,format,arg);
  else {
    vfprintf(stderr,format,arg);
    fprintf(stderr,"\n");
  }
}

void vde3_log(int priority, const char *format, ...)
{
  va_list arg;
  va_start (arg, format);
  vvde3_log(priority, format, arg);
  va_end (arg);
}

