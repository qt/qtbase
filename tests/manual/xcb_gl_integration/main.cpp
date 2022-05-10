// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>

#include <QtGui/qpa/qplatformnativeinterface.h>

#include <QtCore/QDebug>

int main (int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QWindow window;
    window.setSurfaceType(QSurface::OpenGLSurface);
    window.create();

    QOpenGLContext context;
    context.create();

    QPlatformNativeInterface *ni = QGuiApplication::platformNativeInterface();

    qDebug() << "EGLDisplay" << ni->nativeResourceForWindow(QByteArrayLiteral("egldisplay"), &window);
    qDebug() << "EGLContext" << ni->nativeResourceForContext(QByteArrayLiteral("eglcontext"), &context);
    qDebug() << "EGLConfig" << ni->nativeResourceForContext(QByteArrayLiteral("eglconfig"), &context);
    qDebug() << "GLXContext" << ni->nativeResourceForContext(QByteArrayLiteral("glxcontext"), &context);
    qDebug() << "GLXConfig" << ni->nativeResourceForContext(QByteArrayLiteral("glxconfig"), &context);

    return 0;
}
