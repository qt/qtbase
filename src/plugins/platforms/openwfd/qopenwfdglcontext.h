/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QOPENWFDGLCONTEXT_H
#define QOPENWFDGLCONTEXT_H

#include <qpa/qplatformopenglcontext.h>

#include "qopenwfddevice.h"

class QOpenWFDGLContext : public QPlatformOpenGLContext
{
public:
    QOpenWFDGLContext(QOpenWFDDevice *device);

    QSurfaceFormat format() const;

    bool makeCurrent(QPlatformSurface *surface);
    void doneCurrent();

    void swapBuffers(QPlatformSurface *surface);

    QFunctionPointer getProcAddress(const char *procName);

    EGLContext eglContext() const;
private:
    QOpenWFDDevice *mWfdDevice;
};

#endif // QOPENWFDGLCONTEXT_H
