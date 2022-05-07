/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QEGLFSCONTEXT_H
#define QEGLFSCONTEXT_H

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

#include "qeglfsglobal_p.h"
#include "qeglfscursor_p.h"
#include <QtGui/private/qeglplatformcontext_p.h>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE

class Q_EGLFS_EXPORT QEglFSContext : public QEGLPlatformContext
{
public:
    using QEGLPlatformContext::QEGLPlatformContext;
    QEglFSContext() = default; // workaround for INTEGRITY compiler
    QEglFSContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display,
                  EGLConfig *config);
    EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface) override;
    EGLSurface createTemporaryOffscreenSurface() override;
    void destroyTemporaryOffscreenSurface(EGLSurface surface) override;
    void runGLChecks() override;
    void swapBuffers(QPlatformSurface *surface) override;

    QEglFSCursorData cursorData;

private:
    EGLNativeWindowType m_tempWindow = 0;
};

QT_END_NAMESPACE

#endif // QEGLFSCONTEXT_H
