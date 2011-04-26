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
#include "qwindowcontext_qpa.h"

#include <QtCore/QThreadStorage>
#include <QtCore/QThread>

#include <QDebug>

class QWindowThreadContext
{
public:
    ~QWindowThreadContext() {
        if (context)
            context->doneCurrent();
    }
    QWindowContext *context;
};

static QThreadStorage<QWindowThreadContext *> qwindow_context_storage;

class QWindowContextPrivate
{
public:
    QWindowContextPrivate()
        :qGLContextHandle(0)
    {
    }

    virtual ~QWindowContextPrivate()
    {
        //do not delete the QGLContext handle here as it is deleted in
        //QWidgetPrivate::deleteTLSysExtra()
    }
    void *qGLContextHandle;
    void (*qGLContextDeleteFunction)(void *handle);
    QPlatformGLContext *platformGLContext;
    static QWindowContext *staticSharedContext;

    static void setCurrentContext(QWindowContext *context);
};

QWindowContext *QWindowContextPrivate::staticSharedContext = 0;

void QWindowContextPrivate::setCurrentContext(QWindowContext *context)
{
    QWindowThreadContext *threadContext = qwindow_context_storage.localData();
    if (!threadContext) {
        if (!QThread::currentThread()) {
            qWarning("No QTLS available. currentContext wont work");
            return;
        }
        threadContext = new QWindowThreadContext;
        qwindow_context_storage.setLocalData(threadContext);
    }
    threadContext->context = context;
}

/*!
  Returns the last context which called makeCurrent. This function is thread aware.
*/
QWindowContext* QWindowContext::currentContext()
{
    QWindowThreadContext *threadContext = qwindow_context_storage.localData();
    if(threadContext) {
        return threadContext->context;
    }
    return 0;
}

QPlatformGLContext *QWindowContext::handle() const
{
    Q_D(const QWindowContext);
    return d->platformGLContext;
}

/*!
    All subclasses needs to specify the platformWindow. It can be a null window.
*/
QWindowContext::QWindowContext(QWindow *window)
    :d_ptr(new QWindowContextPrivate())
{
    Q_D(QWindowContext);
    Q_ASSERT(window);
    if (!window->handle())
        window->create();
    d->platformGLContext = window->handle()->glContext();
}

/*!
  If this is the current context for the thread, doneCurrent is called
*/
QWindowContext::~QWindowContext()
{
    if (QWindowContext::currentContext() == this) {
        doneCurrent();
    }

}

/*!
    Reimplement in subclass to do makeCurrent on native GL context
*/
void QWindowContext::makeCurrent()
{
    Q_D(QWindowContext);
    QWindowContextPrivate::setCurrentContext(this);
    d->platformGLContext->makeCurrent();
}

/*!
    Reimplement in subclass to release current context.
    Typically this is calling makeCurrent with 0 "surface"
*/
void QWindowContext::doneCurrent()
{
    Q_D(QWindowContext);
    d->platformGLContext->doneCurrent();
    QWindowContextPrivate::setCurrentContext(0);
}

void QWindowContext::swapBuffers()
{
    Q_D(QWindowContext);
    d->platformGLContext->swapBuffers();
}

void (*QWindowContext::getProcAddress(const QByteArray &procName)) ()
{
    Q_D(QWindowContext);
    void *result = d->platformGLContext->getProcAddress(QString::fromAscii(procName.constData()));
    return (void (*)())result;
}

/*
  internal: Needs to have a pointer to qGLContext. But since this is in QtGui we cant
  have any type information.
*/
void *QWindowContext::qGLContextHandle() const
{
    Q_D(const QWindowContext);
    return d->qGLContextHandle;
}

void QWindowContext::setQGLContextHandle(void *handle,void (*qGLContextDeleteFunction)(void *))
{
    Q_D(QWindowContext);
    d->qGLContextHandle = handle;
    d->qGLContextDeleteFunction = qGLContextDeleteFunction;
}

void QWindowContext::deleteQGLContext()
{
    Q_D(QWindowContext);
    if (d->qGLContextDeleteFunction && d->qGLContextHandle) {
        d->qGLContextDeleteFunction(d->qGLContextHandle);
        d->qGLContextDeleteFunction = 0;
        d->qGLContextHandle = 0;
    }
}
