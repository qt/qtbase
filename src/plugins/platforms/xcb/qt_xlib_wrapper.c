// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qt_xlib_wrapper.h"

#include <X11/Xlib.h>

void qt_XFlush(Display *dpy) { XFlush(dpy); }
