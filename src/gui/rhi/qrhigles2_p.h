/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Gui module
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
class QWindow;

struct Q_GUI_EXPORT QRhiGles2InitParams : public QRhiInitParams
{
    QRhiGles2InitParams();

    QSurfaceFormat format;
    QOffscreenSurface *fallbackSurface = nullptr;
    QWindow *window = nullptr;
    QOpenGLContext *shareContext = nullptr;

    static QOffscreenSurface *newFallbackSurface(const QSurfaceFormat &format = QSurfaceFormat::defaultFormat());
    static QSurfaceFormat adjustedFormat(const QSurfaceFormat &format = QSurfaceFormat::defaultFormat());
};

struct Q_GUI_EXPORT QRhiGles2NativeHandles : public QRhiNativeHandles
{
    QOpenGLContext *context = nullptr;
};

QT_END_NAMESPACE

#endif
