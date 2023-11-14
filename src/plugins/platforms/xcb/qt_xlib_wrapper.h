// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial
#ifndef QT_XLIB_WRAPPER_H
#define QT_XLIB_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct _XDisplay Display;
    void qt_XFlush(Display *dpy);

#ifdef __cplusplus
}
#endif

#endif // QT_XLIB_WRAPPER_H
