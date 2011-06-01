/*
 * Copyright © 2010 Kristian Høgsberg
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */


#ifndef WAYLAND_WINDOWMANAGER_CLIENT_PROTOCOL_H
#define WAYLAND_WINDOWMANAGER_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-util.h"

struct wl_client;

struct wl_windowmanager;

struct wl_proxy;

extern void
wl_proxy_marshal(struct wl_proxy *p, uint32_t opcode, ...);
extern struct wl_proxy *
wl_proxy_create(struct wl_proxy *factory,
                const struct wl_interface *interface);
extern struct wl_proxy *
wl_proxy_create_for_id(struct wl_display *display,
                       const struct wl_interface *interface, uint32_t id);
extern void
wl_proxy_destroy(struct wl_proxy *proxy);

extern int
wl_proxy_add_listener(struct wl_proxy *proxy,
                      void (**implementation)(void), void *data);

extern void
wl_proxy_set_user_data(struct wl_proxy *proxy, void *user_data);

extern void *
wl_proxy_get_user_data(struct wl_proxy *proxy);

extern const struct wl_interface wl_windowmanager_interface;

#define wl_WINDOWMANAGER_MAP_CLIENT_TO_PROCESS	0
#define wl_WINDOWMANAGER_AUTHENTICATE_WITH_TOKEN	1

static inline struct wl_windowmanager *
wl_windowmanager_create(struct wl_display *display, uint32_t id)
{
        return (struct wl_windowmanager *)
                wl_proxy_create_for_id(display, &wl_windowmanager_interface, id);
}

static inline void
wl_windowmanager_set_user_data(struct wl_windowmanager *wl_windowmanager, void *user_data)
{
        wl_proxy_set_user_data((struct wl_proxy *) wl_windowmanager, user_data);
}

static inline void *
wl_windowmanager_get_user_data(struct wl_windowmanager *wl_windowmanager)
{
        return wl_proxy_get_user_data((struct wl_proxy *) wl_windowmanager);
}

static inline void
wl_windowmanager_destroy(struct wl_windowmanager *wl_windowmanager)
{
        wl_proxy_destroy((struct wl_proxy *) wl_windowmanager);
}

static inline void
wl_windowmanager_map_client_to_process(struct wl_windowmanager *wl_windowmanager, uint32_t processid)
{
        wl_proxy_marshal((struct wl_proxy *) wl_windowmanager,
                         wl_WINDOWMANAGER_MAP_CLIENT_TO_PROCESS, processid);
}

static inline void
wl_windowmanager_authenticate_with_token(struct wl_windowmanager *wl_windowmanager, const char *wl_authentication_token)
{
        wl_proxy_marshal((struct wl_proxy *) wl_windowmanager,
                         wl_WINDOWMANAGER_AUTHENTICATE_WITH_TOKEN, wl_authentication_token);
}

#ifdef  __cplusplus
}
#endif

#endif
