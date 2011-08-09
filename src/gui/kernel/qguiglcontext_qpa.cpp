/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformglcontext_qpa.h"
#include "qguiglcontext_qpa.h"
#include "qwindow.h"

#include <QtCore/QThreadStorage>
#include <QtCore/QThread>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/QScreen>

#include <QDebug>

class QGuiGLThreadContext
{
public:
    ~QGuiGLThreadContext() {
        if (context)
            context->doneCurrent();
    }
    QGuiGLContext *context;
};

static QThreadStorage<QGuiGLThreadContext *> qwindow_context_storage;

class QGuiGLContextPrivate
{
public:
    QGuiGLContextPrivate()
        : qGLContextHandle(0)
        , platformGLContext(0)
        , shareContext(0)
        , screen(0)
        , surface(0)
    {
    }

    virtual ~QGuiGLContextPrivate()
    {
        //do not delete the QGLContext handle here as it is deleted in
        //QWidgetPrivate::deleteTLSysExtra()
    }
    void *qGLContextHandle;
    void (*qGLContextDeleteFunction)(void *handle);

    QSurfaceFormat requestedFormat;
    QPlatformGLContext *platformGLContext;
    QGuiGLContext *shareContext;
    QScreen *screen;
    QSurface *surface;

    static void setCurrentContext(QGuiGLContext *context);
};

void QGuiGLContextPrivate::setCurrentContext(QGuiGLContext *context)
{
    QGuiGLThreadContext *threadContext = qwindow_context_storage.localData();
    if (!threadContext) {
        if (!QThread::currentThread()) {
            qWarning("No QTLS available. currentContext wont work");
            return;
        }
        threadContext = new QGuiGLThreadContext;
        qwindow_context_storage.setLocalData(threadContext);
    }
    threadContext->context = context;
}

/*!
  Returns the last context which called makeCurrent. This function is thread aware.
*/
QGuiGLContext* QGuiGLContext::currentContext()
{
    QGuiGLThreadContext *threadContext = qwindow_context_storage.localData();
    if(threadContext) {
        return threadContext->context;
    }
    return 0;
}

QPlatformGLContext *QGuiGLContext::handle() const
{
    Q_D(const QGuiGLContext);
    return d->platformGLContext;
}

QPlatformGLContext *QGuiGLContext::shareHandle() const
{
    Q_D(const QGuiGLContext);
    if (d->shareContext)
        return d->shareContext->handle();
    return 0;
}

/*!
  Creates a new GL context instance, you need to call create() before it can be used.
*/
QGuiGLContext::QGuiGLContext()
    : d_ptr(new QGuiGLContextPrivate())
{
    Q_D(QGuiGLContext);
    d->screen = QGuiApplication::primaryScreen();
}

/*!
  Sets the format the GL context should be compatible with. You need to call create() before it takes effect.
*/
void QGuiGLContext::setFormat(const QSurfaceFormat &format)
{
    Q_D(QGuiGLContext);
    d->requestedFormat = format;
}

/*!
  Sets the context to share textures, shaders, and other GL resources with. You need to call create() before it takes effect.
*/
void QGuiGLContext::setShareContext(QGuiGLContext *shareContext)
{
    Q_D(QGuiGLContext);
    d->shareContext = shareContext;
}

/*!
  Sets the screen the GL context should be valid for. You need to call create() before it takes effect.
*/
void QGuiGLContext::setScreen(QScreen *screen)
{
    Q_D(QGuiGLContext);
    d->screen = screen;
    if (!d->screen)
        d->screen = QGuiApplication::primaryScreen();
}

/*!
  Attempts to create the GL context with the desired parameters.

  Returns true if the native context was successfully created and is ready to be used.d
*/
bool QGuiGLContext::create()
{
    destroy();

    Q_D(QGuiGLContext);
    d->platformGLContext = QGuiApplicationPrivate::platformIntegration()->createPlatformGLContext(this);
    d->platformGLContext->setContext(this);
    return d->platformGLContext;
}

void QGuiGLContext::destroy()
{
    Q_D(QGuiGLContext);
    if (QGuiGLContext::currentContext() == this)
        doneCurrent();
    delete d->platformGLContext;
    d->platformGLContext = 0;
}

/*!
  If this is the current context for the thread, doneCurrent is called
*/
QGuiGLContext::~QGuiGLContext()
{
    destroy();
}

/*!
  Returns if this context is valid, i.e. has been successfully created.
*/
bool QGuiGLContext::isValid() const
{
    Q_D(const QGuiGLContext);
    return d->platformGLContext != 0;
}

/*!
  If surface is 0 this is equivalent to calling doneCurrent().
*/
bool QGuiGLContext::makeCurrent(QSurface *surface)
{
    Q_D(QGuiGLContext);
    if (!d->platformGLContext)
        return false;

    if (!surface) {
        doneCurrent();
        return true;
    }

    if (!surface->surfaceHandle())
        return false;

    if (d->platformGLContext->makeCurrent(surface->surfaceHandle())) {
        QGuiGLContextPrivate::setCurrentContext(this);
        d->surface = surface;
        return true;
    }

    return false;
}

/*!
    Convenience function for calling makeCurrent with a 0 surface.
*/
void QGuiGLContext::doneCurrent()
{
    Q_D(QGuiGLContext);
    if (!d->platformGLContext)
        return;

    d->platformGLContext->doneCurrent();
    QGuiGLContextPrivate::setCurrentContext(0);

    d->surface = 0;
}

/*!
    Returns the surface the context is current for.
*/
QSurface *QGuiGLContext::surface() const
{
    Q_D(const QGuiGLContext);
    return d->surface;
}


void QGuiGLContext::swapBuffers(QSurface *surface)
{
    Q_D(QGuiGLContext);
    if (!d->platformGLContext)
        return;

    if (!surface) {
        qWarning() << "QGuiGLContext::swapBuffers() called with null argument";
        return;
    }

    d->platformGLContext->swapBuffers(surface->surfaceHandle());
}

void (*QGuiGLContext::getProcAddress(const QByteArray &procName)) ()
{
    Q_D(QGuiGLContext);
    if (!d->platformGLContext)
        return 0;
    return d->platformGLContext->getProcAddress(procName);
}

QSurfaceFormat QGuiGLContext::format() const
{
    Q_D(const QGuiGLContext);
    if (!d->platformGLContext)
        return d->requestedFormat;
    return d->platformGLContext->format();
}

QGuiGLContext *QGuiGLContext::shareContext() const
{
    Q_D(const QGuiGLContext);
    return d->shareContext;
}

QScreen *QGuiGLContext::screen() const
{
    Q_D(const QGuiGLContext);
    return d->screen;
}

/*
  internal: Needs to have a pointer to qGLContext. But since this is in QtGui we cant
  have any type information.
*/
void *QGuiGLContext::qGLContextHandle() const
{
    Q_D(const QGuiGLContext);
    return d->qGLContextHandle;
}

void QGuiGLContext::setQGLContextHandle(void *handle,void (*qGLContextDeleteFunction)(void *))
{
    Q_D(QGuiGLContext);
    d->qGLContextHandle = handle;
    d->qGLContextDeleteFunction = qGLContextDeleteFunction;
}

void QGuiGLContext::deleteQGLContext()
{
    Q_D(QGuiGLContext);
    if (d->qGLContextDeleteFunction && d->qGLContextHandle) {
        d->qGLContextDeleteFunction(d->qGLContextHandle);
        d->qGLContextDeleteFunction = 0;
        d->qGLContextHandle = 0;
    }
}
