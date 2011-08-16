/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformglcontext_qpa.h"

/*!
    \class QPlatformGLContext
    \since 4.8
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformGLContext class provides an abstraction for native GL contexts.

    In QPA the way to support OpenGL or OpenVG or other technologies that requires a native GL
    context is through the QPlatformGLContext wrapper.

    There is no factory function for QPlatformGLContexts, but rather only one accessor function.
    The only place to retrieve a QPlatformGLContext from is through a QPlatformWindow.

    The context which is current for a specific thread can be collected by the currentContext()
    function. This is how QPlatformGLContext also makes it possible to use the QtOpenGL module
    withhout using QGLWidget. When using QGLContext::currentContext(), it will ask
    QPlatformGLContext for the currentContext. Then a corresponding QGLContext will be returned,
    which maps to the QPlatformGLContext.
*/

/*! \fn void QPlatformGLContext::swapBuffers()
    Reimplement in subclass to native swap buffers calls

    The implementation must support being called in a thread different than the gui-thread.
*/

/*! \fn void *QPlatformGLContext::getProcAddress(const QString &procName)
    Reimplement in subclass to native getProcAddr calls.

    Note: its convenient to use qPrintable(const QString &str) to get the const char * pointer
*/

/*! \fn QPlatformWindowFormat QPlatformGLContext::platformWindowFormat() const
    QWidget has the function qplatformWindowFormat(). That function is for the application
    programmer to request the format of the window and the context that he wants.

    Reimplement this function in a subclass to indicate what format the glContext actually has.
*/

struct QPlatformGLContextPrivate
{
    QGuiGLContext *context;
};

QPlatformGLContext::QPlatformGLContext()
    : d_ptr(new QPlatformGLContextPrivate)
{
    Q_D(QPlatformGLContext);
    d->context = 0;
}

QPlatformGLContext::~QPlatformGLContext()
{
}

QGuiGLContext *QPlatformGLContext::context() const
{
    Q_D(const QPlatformGLContext);
    return d->context;
}

void QPlatformGLContext::setContext(QGuiGLContext *context)
{
    Q_D(QPlatformGLContext);
    d->context = context;
}
