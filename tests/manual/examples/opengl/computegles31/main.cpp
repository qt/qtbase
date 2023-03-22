// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QSurfaceFormat>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QDebug>
#include <QPair>
#include "glwindow.h"

bool OGLSupports(int major, int minor, bool gles = false, QSurfaceFormat::OpenGLContextProfile profile = QSurfaceFormat::NoProfile)
{
    QOpenGLContext ctx;
    QSurfaceFormat fmt;
    fmt.setVersion(major, minor);
    if (gles) {
        fmt.setRenderableType(QSurfaceFormat::OpenGLES);
    } else {
        fmt.setRenderableType(QSurfaceFormat::OpenGL);
        fmt.setProfile(profile);
    }

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
    if (!gles && ctx.format().profile() != profile)
        return false;
    return true;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qDebug() << "Support for GL 2.0 noprof "<<( OGLSupports(2,0,false) ? "yes" : "no");
    qDebug() << "Support for GL 2.0 core   "<<( OGLSupports(2,0,false, QSurfaceFormat::CoreProfile) ? "yes" : "no");
    qDebug() << "Support for GL 2.0 compat "<<( OGLSupports(2,0,false, QSurfaceFormat::CompatibilityProfile) ? "yes" : "no");
    qDebug() << "Support for GL 2.1 noprof "<<( OGLSupports(2,1,false) ? "yes" : "no");
    qDebug() << "Support for GL 2.1 core   "<<( OGLSupports(2,1,false, QSurfaceFormat::CoreProfile) ? "yes" : "no");
    qDebug() << "Support for GL 2.1 compat "<<( OGLSupports(2,1,false, QSurfaceFormat::CompatibilityProfile) ? "yes" : "no");
    qDebug() << "Support for GL 3.0 noprof "<<( OGLSupports(3,0,false) ? "yes" : "no");
    qDebug() << "Support for GL 3.0 core   "<<( OGLSupports(3,0,false, QSurfaceFormat::CoreProfile) ? "yes" : "no");
    qDebug() << "Support for GL 3.0 compat "<<( OGLSupports(3,0,false, QSurfaceFormat::CompatibilityProfile) ? "yes" : "no");
    qDebug() << "Support for GL 3.1 noprof "<<( OGLSupports(3,1,false) ? "yes" : "no");
    qDebug() << "Support for GL 3.1 core   "<<( OGLSupports(3,1,false, QSurfaceFormat::CoreProfile) ? "yes" : "no");
    qDebug() << "Support for GL 3.1 compat "<<( OGLSupports(3,1,false, QSurfaceFormat::CompatibilityProfile) ? "yes" : "no");
    qDebug() << "Support for GL 3.2 core   "<<( OGLSupports(3,2,false,QSurfaceFormat::CoreProfile) ? "yes" : "no");
    qDebug() << "Support for GL 3.2 compat "<<( OGLSupports(3,2,false,QSurfaceFormat::CompatibilityProfile) ? "yes" : "no");
    qDebug() << "Support for GL 3.3 core   "<<( OGLSupports(3,3,false,QSurfaceFormat::CoreProfile) ? "yes" : "no");
    qDebug() << "Support for GL 3.3 compat "<<( OGLSupports(3,3,false,QSurfaceFormat::CompatibilityProfile) ? "yes" : "no");
    qDebug() << "Support for GL 4.0 core   "<<( OGLSupports(4,0,false,QSurfaceFormat::CoreProfile) ? "yes" : "no");
    qDebug() << "Support for GL 4.0 compat "<<( OGLSupports(4,0,false,QSurfaceFormat::CompatibilityProfile) ? "yes" : "no");
    qDebug() << "Support for GL 4.1 core   "<<( OGLSupports(4,1,false,QSurfaceFormat::CoreProfile) ? "yes" : "no");
    qDebug() << "Support for GL 4.1 compat "<<( OGLSupports(4,1,false,QSurfaceFormat::CompatibilityProfile) ? "yes" : "no");
    qDebug() << "Support for GL 4.2 core   "<<( OGLSupports(4,2,false,QSurfaceFormat::CoreProfile) ? "yes" : "no");
    qDebug() << "Support for GL 4.2 compat "<<( OGLSupports(4,2,false,QSurfaceFormat::CompatibilityProfile) ? "yes" : "no");
    qDebug() << "Support for GL 4.3 core   "<<( OGLSupports(4,3,false,QSurfaceFormat::CoreProfile) ? "yes" : "no");
    qDebug() << "Support for GL 4.3 compat "<<( OGLSupports(4,3,false,QSurfaceFormat::CompatibilityProfile) ? "yes" : "no");
    qDebug() << "Support for GL 4.4 core   "<<( OGLSupports(4,4,false,QSurfaceFormat::CoreProfile) ? "yes" : "no");
    qDebug() << "Support for GL 4.4 compat "<<( OGLSupports(4,4,false,QSurfaceFormat::CompatibilityProfile) ? "yes" : "no");
    qDebug() << "Support for GL 4.5 core   "<<( OGLSupports(4,5,false,QSurfaceFormat::CoreProfile) ? "yes" : "no");
    qDebug() << "Support for GL 4.5 compat "<<( OGLSupports(4,5,false,QSurfaceFormat::CompatibilityProfile) ? "yes" : "no");
    qDebug() << "Support for GLES 2.0 "<<( OGLSupports(2,0,true) ? "yes" : "no");
    qDebug() << "Support for GLES 3.0 "<<( OGLSupports(3,0,true) ? "yes" : "no");
    qDebug() << "Support for GLES 3.1 "<<( OGLSupports(3,1,true) ? "yes" : "no");
    qDebug() << "Support for GLES 3.2 "<<( OGLSupports(3,2,true) ? "yes" : "no");

    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);

    // Request OpenGL ES 3.1 context, as this is a GLES example. If not available, go for OpenGL 4.3 core.
    if (OGLSupports(3,1,true)) {
        qDebug("Requesting 3.1 GLES context");
        fmt.setVersion(3, 1);
        fmt.setRenderableType(QSurfaceFormat::OpenGLES);
    } else if (OGLSupports(4,3,false,QSurfaceFormat::CoreProfile)) {
        qDebug("Requesting 4.3 core context");
        fmt.setVersion(4, 3);
        fmt.setRenderableType(QSurfaceFormat::OpenGL);
        fmt.setProfile(QSurfaceFormat::CoreProfile);
    } else {
        qWarning("Error: This system does not support OpenGL Compute Shaders! Exiting.");
        return -1;
    }
    QSurfaceFormat::setDefaultFormat(fmt);

    GLWindow glWindow;
    glWindow.showMaximized();

    return app.exec();
}
