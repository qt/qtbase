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

extern const struct wl_interface wl_windowmanager_interface;

struct wl_windowmanager_listener {
	void (*client_onscreen_visibility)(void *data,
					   struct wl_windowmanager *wl_windowmanager,
					   int32_t visible);
	void (*set_screen_rotation)(void *data,
				    struct wl_windowmanager *wl_windowmanager,
				    int32_t rotation);
	void (*set_generic_property)(void *data,
				     struct wl_windowmanager *wl_windowmanager,
				     struct wl_surface *surface,
				     const char *name,
				     struct wl_array *value);
};

static inline int
wl_windowmanager_add_listener(struct wl_windowmanager *wl_windowmanager,
				 const struct wl_windowmanager_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) wl_windowmanager,
				     (void (**)(void)) listener, data);
}

#define WL_WINDOWMANAGER_MAP_CLIENT_TO_PROCESS	0
#define WL_WINDOWMANAGER_AUTHENTICATE_WITH_TOKEN	1
#define WL_WINDOWMANAGER_UPDATE_GENERIC_PROPERTY	2

static inline struct wl_windowmanager *
wl_windowmanager_create(struct wl_display *display, uint32_t id, uint32_t version)
{
	wl_display_bind(display, id, "wl_windowmanager", version);

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
			 WL_WINDOWMANAGER_MAP_CLIENT_TO_PROCESS, processid);
}

static inline void
wl_windowmanager_authenticate_with_token(struct wl_windowmanager *wl_windowmanager, const char *processid)
{
	wl_proxy_marshal((struct wl_proxy *) wl_windowmanager,
			 WL_WINDOWMANAGER_AUTHENTICATE_WITH_TOKEN, processid);
}

static inline void
wl_windowmanager_update_generic_property(struct wl_windowmanager *wl_windowmanager, struct wl_surface *surface, const char *name, struct wl_array *value)
{
	wl_proxy_marshal((struct wl_proxy *) wl_windowmanager,
			 WL_WINDOWMANAGER_UPDATE_GENERIC_PROPERTY, surface, name, value);
}

#ifdef  __cplusplus
}
#endif

#endif
