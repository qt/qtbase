// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct _XDisplay Display;
    void qt_XFlush(Display *dpy);

#ifdef __cplusplus
}
#endif
