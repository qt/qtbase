/***************************************************************************
**
** Copyright (C) 2018 QNX Software Systems. All rights reserved.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
