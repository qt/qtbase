#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"

extern const struct wl_interface wl_surface_interface;

static const struct wl_interface *types[] = {
	NULL,
	&wl_surface_interface,
	NULL,
	NULL,
	&wl_surface_interface,
	NULL,
	NULL,
};

static const struct wl_message wl_windowmanager_requests[] = {
	{ "map_client_to_process", "u", types + 0 },
	{ "authenticate_with_token", "s", types + 0 },
	{ "update_generic_property", "osa", types + 1 },
};

static const struct wl_message wl_windowmanager_events[] = {
	{ "client_onscreen_visibility", "i", types + 0 },
	{ "set_screen_rotation", "i", types + 0 },
	{ "set_generic_property", "osa", types + 4 },
};

WL_EXPORT const struct wl_interface wl_windowmanager_interface = {
	"wl_windowmanager", 1,
	ARRAY_LENGTH(wl_windowmanager_requests), wl_windowmanager_requests,
	ARRAY_LENGTH(wl_windowmanager_events), wl_windowmanager_events,
};

