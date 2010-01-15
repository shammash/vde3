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
 * @file vde3.h
 *
 * Public header of VDE3.
 */

/**
 * @mainpage
 *
 * @section intro Introduction
 *
 * intro
 *
 * @section notes Notes
 *
 * notes
 */

#ifndef __VDE3_H__
#define __VDE3_H__

#include <stdarg.h>
#include <syslog.h>
#include <sys/types.h>

/*
 * event handler
 *
 */

#define VDE_EV_READ     0x02
#define VDE_EV_WRITE    0x04
#define VDE_EV_PERSIST  0x10
#define VDE_EV_TIMEOUT  0x01

/**
 * @brief The callback to be called on events.
 *
 * The events argument must contain the event(s) which are available on fd,
 * plus VDE_EV_TIMEOUT if the callback is being called due to a timeout.
 */
typedef void (*event_cb)(int fd, short events, void *arg);

/**
 * @brief This is the event handler the application must supply.
 *
 * It contains several functions pointers for handling events from file
 * descriptors and timeouts.  It is modeled after libevent and thus shares some
 * API details.
 */
typedef struct {
  /**
   * @brief Function to add a new event
   *
   * This function is called by vde whenever there's interest on events for a
   * file descriptor.
   *
   * @param fd The interested fd
   * @param events The events to monitor for this event
   * @param timeout The timeout, can be NULL
   * @param cb The function to call as a callback for this event
   * @param arg The argument to pass to the callback
   *
   * @return a token representing the event, can be anything
   * application-specific, vde uses this to refer to the event e.g. in calls to
   * event_del, NULL on error.
   *
   * Events is a mask combining one or more of
   *   VDE_EV_READ to monitor read-availability
   *   VDE_EV_WRITE to monitor write-availability
   *   VDE_EV_PERSIST to keep calling the callback even after an event has
   *                  occured
   *
   * If timeout is not NULL and no events occur within timeout then the callback
   * is called, if timeout is NULL then the callback is called only if events of
   * the specified type occur on fd.
   *
   */
  void *(*event_add)(int fd, short events, const struct timeval *timeout,
                     event_cb cb, void *arg);

  /**
   * @brief Function to delete an event
   *
   * @param ev The event to delete, as returned by event_add
   *
   * This function is called by vde to delete an event for which the callback
   * has not been called yet and/or to explicitly delete events with the
   * VDE_EV_PERSIST flag.
   */
  void (*event_del)(void *ev);

  /**
   * @brief Function to add a new timeout
   *
   * @param timeout The timeout, cannot be NULL
   * @param events The events for this timeout
   * @param cb The function to call as a callback when the timeout expires
   * @param arg The argument to pass to the callback
   *
   * @return a token representing the timeout, likewise for event this is an
   * opaque object and defined by the application. NULL on error.
   *
   * This function is called whenever the callback must be called after the time
   * represented by timeout, if VDE_EV_PERSIST is present in events then the
   * callback is called repeatedly every timeout until timeout_del is called.
   *
   */
  void *(*timeout_add)(const struct timeval *timeout, short events, event_cb cb,
                      void *arg);

  /**
   * @brief Function to delete a timeout
   *
   * @param tout The timeout to delete, as returned by timeout_add
   *
   * This function is called by vde to delete a timeout for which the callback
   * has not been called yet and/or to explicitly delete timeouts with the
   * VDE_EV_PERSIST flag.
   */
  void (*timeout_del)(void *tout);
} vde_event_handler;


/*
 * vde request
 *
 */

// XXX to be defined
typedef struct {
} vde_request;


/*
 * component kind
 *
 */


/**
 * @brief This enum represents the possible component kinds
 */
typedef enum {
  VDE_ENGINE,
  VDE_TRANSPORT,
  VDE_CONNECTION_MANAGER
} vde_component_kind;


/*
 * component
 *
 */

/**
 * @brief VDE 3 Component
 */
typedef struct vde_component vde_component;


/**
 * @brief The callback called by connection manager on successful connect
 *
 * @param cm The calling connection manager
 * @param arg The callback private data
 */
typedef void (*vde_connect_success_cb)(vde_component *cm, void *arg);

/**
 * @brief The callback called by connection manager on unsuccessful connect
 *
 * @param cm The calling connection manager
 * @param arg The callback private data
 */
typedef void (*vde_connect_error_cb)(vde_component *cm, void *arg);

/**
 * @brief Put the underlying transport in listen mode
 *
 * @param cm The connection manager to use
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_conn_manager_listen(vde_component *cm);

/**
 * @brief Initiate a new connection using the underlying transport
 *
 * @param cm The connection manager for the connection
 * @param local The request for the local system
 * @param remote The request for the remote system
 * @param success_cb A callback to be called on successful connect, can be
 * NULL
 * @param error_cb A callback to be called on error during connect, can be
 * NULL
 * @param arg A pointer to private data, passed to callbacks
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_conn_manager_connect(vde_component *cm, vde_request *local,
                             vde_request *remote,
                             vde_connect_success_cb success_cb,
                             vde_connect_error_cb error_cb, void *arg);


/*
 * context
 *
 */

/**
 * @brief VDE 3 Context
 */
typedef struct vde_context vde_context;

/**
 * @brief Alloc a new VDE 3 context
 *
 * @param ctx reference to new context pointer
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_context_new(vde_context **ctx);

/**
 * @brief Initialize VDE 3 context
 *
 * @param ctx The context to initialize
 * @param handler An implementation of vde_event_handler to use
 * @param modules_path A NULL-terminated array of paths to load modules from
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_context_init(vde_context *ctx, vde_event_handler *handler,
                     char **modules_path);

/**
 * @brief Stop and reset a VDE 3 context
 *
 * @param ctx The context to fini
 */
void vde_context_fini(vde_context *ctx);

/**
 * @brief Deallocate a VDE 3 context
 *
 * @param ctx The context to delete
 */
void vde_context_delete(vde_context *ctx);

/**
 * @brief Alloc a new VDE 3 component
 *
 * @param ctx The context where to allocate this component
 * @param kind The component kind  (transport, engine, ...)
 * @param family The component family (unix, data, ...)
 * @param name The component unique name (NULL for auto generation)
 * @param component The reference to new component pointer
 * @param ... The component-specific arguments
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_context_new_component(vde_context *ctx, vde_component_kind kind,
                              const char *family, const char *name,
                              vde_component **component, ...);

/**
 * @brief Lookup a component by name
 *
 * @param ctx The context where to lookup
 * @param name The component name
 *
 * @return the component, NULL if not found
 */
vde_component* vde_context_get_component(vde_context *ctx, const char *name);

/**
 * @brief Remove a component from a given context
 *
 * @param ctx The context where to remove from
 * @param component The component to remove
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_context_component_del(vde_context *ctx, vde_component *component);

/**
 * @brief Save current configuration in a file
 *
 * @param ctx The context to save the configuration from
 * @param file The file to save the configuration to
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_context_config_save(vde_context *ctx, const char* file);

/**
 * @brief Load configuration from a file
 *
 * @param ctx The context to load the configuration into
 * @param file The file to read the configuration from
 *
 * @return zero on success, -1 on error (and errno is set appropriately)
 */
int vde_context_config_load(vde_context *ctx, const char* file);


/*
 * logging
 *
 */

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
typedef void (*vde_log_handler)(int priority, const char *format, va_list arg);

/**
 * @brief Set handler in the logging system
 *
 * @param handler The function responsible to print logs, if NULL stderr will be
 *                used
 */
void vde_log_set_handler(vde_log_handler handler);

/**
 * @brief Log a message using va_list
 *
 * @param priority Logging priority
 * @param format Message format
 * @param arg Variable arguments list
 */
void vvde_log(int priority, const char *format, va_list arg);

/**
 * @brief Log a message
 *
 * @param priority Logging priority
 * @param format Message format
 */
void vde_log(int priority, const char *format, ...);

// XXX this could prepend __PRETTY_FUNCTION__ to avoid repeating
#define vde_error(fmt, ...) vde_log(VDE3_LOG_ERROR, fmt, ##__VA_ARGS__)
#define vde_warning(fmt, ...) vde_log(VDE3_LOG_WARNING, fmt, ##__VA_ARGS__)
#define vde_notice(fmt, ...) vde_log(VDE3_LOG_NOTICE, fmt, ##__VA_ARGS__)
#define vde_info(fmt, ...) vde_log(VDE3_LOG_INFO, fmt, ##__VA_ARGS__)
#ifdef VDE3_DEBUG
#define vde_debug(fmt, ...) vde_log(VDE3_LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define vde_debug(fmt, ...)
#endif

#endif /* __VDE3_H__ */
