// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QXLIBEGLINTEGRATION_H
#define QXLIBEGLINTEGRATION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qeglconvenience_p.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QXlibEglIntegration
{
public:
    static VisualID getCompatibleVisualId(Display *display, EGLDisplay eglDisplay, EGLConfig config);
};

QT_END_NAMESPACE

#endif // QXLIBEGLINTEGRATION_H
