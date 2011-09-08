/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
