// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct _XDisplay Display;
    void qt_XFlush(Display *dpy);

#ifdef __cplusplus
}
#endif
