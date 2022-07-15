// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRHIGLES2_H
#define QRHIGLES2_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qrhi_p.h>
#include <QtGui/qsurfaceformat.h>

QT_BEGIN_NAMESPACE

class QOpenGLContext;
class QOffscreenSurface;
class QSurface;
class QWindow;

struct Q_GUI_EXPORT QRhiGles2InitParams : public QRhiInitParams
{
    QRhiGles2InitParams();

    QSurfaceFormat format;
    QSurface *fallbackSurface = nullptr;
    QWindow *window = nullptr;
    QOpenGLContext *shareContext = nullptr;

    static QOffscreenSurface *newFallbackSurface(const QSurfaceFormat &format = QSurfaceFormat::defaultFormat());
};

struct Q_GUI_EXPORT QRhiGles2NativeHandles : public QRhiNativeHandles
{
    QOpenGLContext *context = nullptr;
};

QT_END_NAMESPACE

#endif
