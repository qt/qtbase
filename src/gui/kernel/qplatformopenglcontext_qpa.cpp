/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qplatformopenglcontext_qpa.h"

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformOpenGLContext
    \since 4.8
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformOpenGLContext class provides an abstraction for native GL contexts.

    In QPA the way to support OpenGL or OpenVG or other technologies that requires a native GL
    context is through the QPlatformOpenGLContext wrapper.

    There is no factory function for QPlatformOpenGLContexts, but rather only one accessor function.
    The only place to retrieve a QPlatformOpenGLContext from is through a QPlatformWindow.

    The context which is current for a specific thread can be collected by the currentContext()
    function. This is how QPlatformOpenGLContext also makes it possible to use the QtGui module
    withhout using QOpenGLWidget. When using QOpenGLContext::currentContext(), it will ask
    QPlatformOpenGLContext for the currentContext. Then a corresponding QOpenGLContext will be returned,
    which maps to the QPlatformOpenGLContext.
*/

/*! \fn void QPlatformOpenGLContext::swapBuffers()
    Reimplement in subclass to native swap buffers calls

    The implementation must support being called in a thread different than the gui-thread.
*/

/*! \fn void *QPlatformOpenGLContext::getProcAddress(const QString &procName)
    Reimplement in subclass to native getProcAddr calls.

    Note: its convenient to use qPrintable(const QString &str) to get the const char * pointer
*/

/*! \fn QPlatformWindowFormat QPlatformOpenGLContext::platformWindowFormat() const
    QWidget has the function qplatformWindowFormat(). That function is for the application
    programmer to request the format of the window and the context that he wants.

    Reimplement this function in a subclass to indicate what format the glContext actually has.
*/

struct QPlatformOpenGLContextPrivate
{
    QOpenGLContext *context;
};

QPlatformOpenGLContext::QPlatformOpenGLContext()
    : d_ptr(new QPlatformOpenGLContextPrivate)
{
    Q_D(QPlatformOpenGLContext);
    d->context = 0;
}

QPlatformOpenGLContext::~QPlatformOpenGLContext()
{
}

QOpenGLContext *QPlatformOpenGLContext::context() const
{
    Q_D(const QPlatformOpenGLContext);
    return d->context;
}

void QPlatformOpenGLContext::setContext(QOpenGLContext *context)
{
    Q_D(QPlatformOpenGLContext);
    d->context = context;
}

QT_END_NAMESPACE
