// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial
#include "qt_xlib_wrapper.h"

#include <X11/Xlib.h>

void qt_XFlush(Display *dpy) { XFlush(dpy); }
