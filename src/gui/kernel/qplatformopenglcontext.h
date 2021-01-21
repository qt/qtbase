/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QPLATFORMOPENGLCONTEXT_H
#define QPLATFORMOPENGLCONTEXT_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/qnamespace.h>

#ifndef QT_NO_OPENGL

#include <QtGui/qsurfaceformat.h>
#include <QtGui/qwindow.h>
#include <QtGui/qopengl.h>

QT_BEGIN_NAMESPACE


class QPlatformOpenGLContextPrivate;

class Q_GUI_EXPORT QPlatformOpenGLContext
{
    Q_DECLARE_PRIVATE(QPlatformOpenGLContext)
public:
    QPlatformOpenGLContext();
    virtual ~QPlatformOpenGLContext();

    virtual void initialize();

    virtual QSurfaceFormat format() const = 0;

    virtual void swapBuffers(QPlatformSurface *surface) = 0;

    virtual GLuint defaultFramebufferObject(QPlatformSurface *surface) const;

    virtual bool makeCurrent(QPlatformSurface *surface) = 0;
    virtual void doneCurrent() = 0;

    virtual bool isSharing() const { return false; }
    virtual bool isValid() const { return true; }

    virtual QFunctionPointer getProcAddress(const char *procName) = 0;

    QOpenGLContext *context() const;

    static bool parseOpenGLVersion(const QByteArray &versionString, int &major, int &minor);

private:
    friend class QOpenGLContext;

    QScopedPointer<QPlatformOpenGLContextPrivate> d_ptr;

    void setContext(QOpenGLContext *context);

    Q_DISABLE_COPY(QPlatformOpenGLContext)
};

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif // QPLATFORMOPENGLCONTEXT_H
