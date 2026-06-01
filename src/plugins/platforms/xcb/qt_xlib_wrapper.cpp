// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qt_xlib_wrapper.h"

#include <X11/Xlib.h>

QT_BEGIN_NAMESPACE
void qt_XFlush(Display *dpy)
{
    XFlush(dpy);
}
QT_END_NAMESPACE
