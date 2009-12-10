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

#ifndef __VDE3_LOGGING_H__
#define __VDE3_LOGGING_H__

#include <syslog.h>

#define VDE3_LOG_ERROR   LOG_ERR
#define VDE3_LOG_WARNING LOG_WARNING
#define VDE3_LOG_NOTICE  LOG_NOTICE
#define VDE3_LOG_INFO    LOG_INFO
#define VDE3_LOG_DEBUG   LOG_DEBUG

/** 
 * @brief Log handler function prototype
 * 
 * @param priority The message priority: VDE3_LOG_ERROR, ..., VDE3_LOG_DEBUG
 * @param format The format string
 * @param arg Arguments for the format string
 */
typedef void (*vde3_log_handler)(int priority, const char *format, va_list arg);

/** 
 * @brief Set handler in the logging system
 * 
 * @param handler The function responsible to print logs, if NULL stderr will be
 *                used
 */
void vde3_log_set_handler(vde3_log_handler handler);

/** 
 * @brief Log a message using va_list
 * 
 * @param priority Logging priority
 * @param format Message format
 * @param arg Variable arguments list
 */
void vvde3_log(int priority, const char *format, va_list arg);

/** 
 * @brief Log a message
 * 
 * @param priority Logging priority
 * @param format Message format
 */
void vde3_log(int priority, const char *format, ...);

#define vde3_error(fmt, ...) vde3_log(VDE3_LOG_ERROR, fmt, ##__VA_ARGS__)
#define vde3_warning(fmt, ...) vde3_log(VDE3_LOG_WARNING, fmt, ##__VA_ARGS__)
#define vde3_notice(fmt, ...) vde3_log(VDE3_LOG_NOTICE, fmt, ##__VA_ARGS__)
#define vde3_info(fmt, ...) vde3_log(VDE3_LOG_INFO, fmt, ##__VA_ARGS__)
#ifdef VDE3_DEBUG
#define vde3_debug(fmt, ...) vde3_log(VDE3_LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define vde3_debug(fmt, ...)
#endif

#endif /* __VDE3_LOGGING_H__ */

