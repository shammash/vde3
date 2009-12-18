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
#include <vde3/common.h>

#include <stdio.h>
#include <string.h>

#include <event.h>

/*
 * vde_event_handler which uses libevent as a backend, handling of recurrent
 * timeouts (aka periodic) is implemented as well.
 *
 * usage can be like something this:
 *
 * event_init();
 * ...
 * vde_context_init(ctx, &libevent_eh);
 * ...
 * event_dispatch();
 *
 */

// recurring timeout handling
struct rtimeout {
  struct event *ev;
  struct timeval *timeout;
  event_cb cb;
  void *arg;
};

void rtimeout_cb(int fd, short events, void *arg)
{
  struct rtimeout *rt = (struct rtimeout *)arg;

  // call user callback and reschedule the timeout
  rt->cb(fd, events, rt->arg);
  timeout_add(rt->ev, rt->timeout);
}

// XXX check if libevent has been initialized?
void *libevent_event_add(int fd, short events, const struct timeval *timeout,
                         event_cb cb, void *arg)
{
  struct event *ev;

  ev = (struct event *)vde_alloc(sizeof(struct event));
  if (!ev) {
    vde_error("%s: can't allocate memory for new event", __PRETTY_FUNCTION__);
    return NULL;
  }

  event_set(ev, fd, events, cb, arg);
  event_add(ev, timeout);

  return ev;
}

void libevent_event_del(void *event)
{
  struct event *ev = (struct event *)event;

  event_del(ev);
  vde_free(ev);
}

void *libevent_timeout_add(const struct timeval *timeout, short events,
                           event_cb cb, void *arg)
{
  struct event *ev;
  struct rtimeout *rt;

  ev = (struct event *)vde_alloc(sizeof(struct event));
  if (!ev) {
    vde_error("%s: can't allocate memory for timeout", __PRETTY_FUNCTION__);
    return NULL;
  }

  rt = (struct rtimeout *)vde_calloc(sizeof(struct rtimeout));
  if (!rt) {
    if (ev) {
      vde_free(ev);
    }
    vde_error("%s: can't allocate memory for timeout", __PRETTY_FUNCTION__);
    return NULL;
  }
  rt->ev = ev;

  // if it is a recurrent timeout hook our callback instead of user's
  if (events & VDE_EV_PERSIST) {
    rt->timeout = (struct timeval *)vde_alloc(sizeof(struct timeval));
    memcpy(rt->timeout, timeout, sizeof(struct timeval));
    rt->cb = cb;
    rt->arg = arg;
    timeout_set(ev, rtimeout_cb, rt);
    timeout_add(ev, timeout);
  } else {
    timeout_set(ev, cb, arg);
    timeout_add(ev, timeout);
  }
  // XXX check calls to timeout_set / timeout_add
  return rt;
}

void libevent_timeout_del(void *timeout)
{
  struct rtimeout *rt = (struct rtimeout *)timeout;

  event_del(rt->ev);
  vde_free(rt->ev);

  if(rt->timeout) {
    vde_free(rt->timeout);
  }

  vde_free(rt);
}

vde_event_handler libevent_eh = {
  .event_add = libevent_event_add,
  .event_del = libevent_event_del,
  .timeout_add = libevent_timeout_add,
  .timeout_del = libevent_timeout_del,
};
