/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QGuiApplication>
#include <QSurfaceFormat>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QDebug>
#include <QPair>
#include "glwindow.h"

bool OGLSupports(int major, int minor, bool gles = false)
{
    QOpenGLContext ctx;
    QSurfaceFormat fmt;
    fmt.setVersion(major, minor);
    if (gles)
        fmt.setRenderableType(QSurfaceFormat::OpenGLES);
    else
        fmt.setRenderableType(QSurfaceFormat::OpenGL);

    ctx.setFormat(fmt);
    ctx.create();
    if (!ctx.isValid())
        return false;
    int ctxMajor = ctx.format().majorVersion();
    int ctxMinor = ctx.format().minorVersion();
    bool isGles = (ctx.format().renderableType() == QSurfaceFormat::OpenGLES);

    if (isGles != gles) return false;
    if (ctxMajor < major) return false;
    if (ctxMajor == major && ctxMinor < minor)
        return false;
    return true;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qDebug() << "Support for GL 2.0   "<<( OGLSupports(2,0) ? "yes" : "no");
    qDebug() << "Support for GL 2.1   "<<( OGLSupports(2,1) ? "yes" : "no");
    qDebug() << "Support for GL 3.0   "<<( OGLSupports(3,0) ? "yes" : "no");
    qDebug() << "Support for GL 3.1   "<<( OGLSupports(3,1) ? "yes" : "no");
    qDebug() << "Support for GL 3.2   "<<( OGLSupports(3,2) ? "yes" : "no");
    qDebug() << "Support for GL 3.3   "<<( OGLSupports(3,3) ? "yes" : "no");
    qDebug() << "Support for GL 4.0   "<<( OGLSupports(4,0) ? "yes" : "no");
    qDebug() << "Support for GL 4.1   "<<( OGLSupports(4,1) ? "yes" : "no");
    qDebug() << "Support for GL 4.2   "<<( OGLSupports(4,2) ? "yes" : "no");
    qDebug() << "Support for GL 4.3   "<<( OGLSupports(4,3) ? "yes" : "no");
    qDebug() << "Support for GL 4.4   "<<( OGLSupports(4,4) ? "yes" : "no");
    qDebug() << "Support for GL 4.5   "<<( OGLSupports(4,5) ? "yes" : "no");
    qDebug() << "Support for GLES 2.0 "<<( OGLSupports(2,0,true) ? "yes" : "no");
    qDebug() << "Support for GLES 3.0 "<<( OGLSupports(3,0,true) ? "yes" : "no");
    qDebug() << "Support for GLES 3.1 "<<( OGLSupports(3,1,true) ? "yes" : "no");
    qDebug() << "Support for GLES 3.2 "<<( OGLSupports(3,2,true) ? "yes" : "no");

    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);

    // Request OpenGL 4.3 compatibility or OpenGL ES 3.1.
    if (OGLSupports(4,3)) {
        qDebug("Requesting 4.3 compatibility context");
        fmt.setVersion(4, 3);
        fmt.setRenderableType(QSurfaceFormat::OpenGL);
        fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    } else if (OGLSupports(3,1,true)) {
        qDebug("Requesting 3.1 GLES context");
        fmt.setVersion(3, 1);
        fmt.setRenderableType(QSurfaceFormat::OpenGLES);
    } else {
        qWarning("Error: This system does not support OpenGL Compute Shaders! Exiting.");
        return -1;
    }
    QSurfaceFormat::setDefaultFormat(fmt);

    GLWindow glWindow;
    glWindow.showMaximized();

    return app.exec();
}
