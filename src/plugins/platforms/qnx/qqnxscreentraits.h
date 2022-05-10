// Copyright (C) 2018 QNX Software Systems. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXSCREENTRAITS_H
#define QQNXSCREENTRAITS_H

#include <screen/screen.h>

QT_BEGIN_NAMESPACE

template <typename T>
class screen_traits
{
};

template <>
class screen_traits<screen_context_t>
{
public:
    typedef screen_context_t screen_type;
    static const int propertyName = SCREEN_PROPERTY_CONTEXT;
    static int destroy(screen_context_t context) { return screen_destroy_context(context); }
};

template <>
class screen_traits<screen_device_t>
{
public:
    typedef screen_device_t screen_type;
    static const int propertyName = SCREEN_PROPERTY_DEVICE;
    static int destroy(screen_device_t device) { return screen_destroy_device(device); }
};

template <>
class screen_traits<screen_display_t>
{
public:
    typedef screen_display_t screen_type;
    static const int propertyName = SCREEN_PROPERTY_DISPLAY;
};

template <>
class screen_traits<screen_group_t>
{
public:
    typedef screen_group_t screen_type;
    static const int propertyName = SCREEN_PROPERTY_GROUP;
    static int destroy(screen_group_t group) { return screen_destroy_group(group); }
};

template <>
class screen_traits<screen_pixmap_t>
{
public:
    typedef screen_pixmap_t screen_type;
    static const int propertyName = SCREEN_PROPERTY_PIXMAP;
    static int destroy(screen_pixmap_t pixmap) { return screen_destroy_pixmap(pixmap); }
};

template <>
class screen_traits<screen_session_t>
{
public:
    typedef screen_session_t screen_type;
    static const int propertyName = SCREEN_PROPERTY_SESSION;
    static int destroy(screen_session_t session) { return screen_destroy_session(session); }
};

#if _SCREEN_VERSION >= _SCREEN_MAKE_VERSION(2, 0, 0)
template <>
class screen_traits<screen_stream_t>
{
public:
    typedef screen_stream_t screen_type;
    static const int propertyName = SCREEN_PROPERTY_STREAM;
    static int destroy(screen_stream_t stream) { return screen_destroy_stream(stream); }
};
#endif

template <>
class screen_traits<screen_window_t>
{
public:
    typedef screen_window_t screen_type;
    static const int propertyName = SCREEN_PROPERTY_WINDOW;
    static int destroy(screen_window_t window) { return screen_destroy_window(window); }
};

QT_END_NAMESPACE

#endif // QQNXSCREENTRAITS_H
