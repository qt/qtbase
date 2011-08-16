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
#include "qguiglcontext_qpa_p.h"
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

bool QGuiGLContext::areSharing(QGuiGLContext *first, QGuiGLContext *second)
{
    return first->shareGroup() == second->shareGroup();
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
    : QObject(*new QGuiGLContextPrivate())
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

  Returns true if the native context was successfully created and is ready to be used.
*/
bool QGuiGLContext::create()
{
    destroy();

    Q_D(QGuiGLContext);
    d->platformGLContext = QGuiApplicationPrivate::platformIntegration()->createPlatformGLContext(this);
    d->platformGLContext->setContext(this);
    d->shareGroup = d->shareContext ? d->shareContext->shareGroup() : new QGuiGLContextGroup;
    d->shareGroup->d_func()->addContext(this);
    return d->platformGLContext;
}

void QGuiGLContext::destroy()
{
    Q_D(QGuiGLContext);
    if (QGuiGLContext::currentContext() == this)
        doneCurrent();
    if (d->shareGroup)
        d->shareGroup->d_func()->removeContext(this);
    d->shareGroup = 0;
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

        d->shareGroup->d_func()->deletePendingResources(this);

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

    if (QGuiGLContext::currentContext() == this)
        d->shareGroup->d_func()->deletePendingResources(this);

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

QGuiGLContextGroup *QGuiGLContext::shareGroup() const
{
    Q_D(const QGuiGLContext);
    return d->shareGroup;
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

QGuiGLContextGroup::QGuiGLContextGroup()
    : QObject(*new QGuiGLContextGroupPrivate())
{
}

QGuiGLContextGroup::~QGuiGLContextGroup()
{
    Q_D(QGuiGLContextGroup);

    QList<QGLSharedResource *>::iterator it = d->m_sharedResources.begin();
    QList<QGLSharedResource *>::iterator end = d->m_sharedResources.end();

    while (it != end) {
        (*it)->invalidateResource();
        (*it)->m_group = 0;
        ++it;
    }

    qDeleteAll(d->m_pendingDeletion.begin(), d->m_pendingDeletion.end());
}

QList<QGuiGLContext *> QGuiGLContextGroup::shares() const
{
    Q_D(const QGuiGLContextGroup);
    return d->m_shares;
}

QGuiGLContextGroup *QGuiGLContextGroup::currentContextGroup()
{
    QGuiGLContext *current = QGuiGLContext::currentContext();
    return current ? current->shareGroup() : 0;
}

void QGuiGLContextGroupPrivate::addContext(QGuiGLContext *ctx)
{
    QMutexLocker locker(&m_mutex);
    m_refs.ref();
    m_shares << ctx;
}

void QGuiGLContextGroupPrivate::removeContext(QGuiGLContext *ctx)
{
    Q_Q(QGuiGLContextGroup);

    QMutexLocker locker(&m_mutex);
    m_shares.removeOne(ctx);

    if (ctx == m_context && !m_shares.isEmpty())
        m_context = m_shares.first();

    if (!m_refs.deref())
        q->deleteLater();
}

void QGuiGLContextGroupPrivate::deletePendingResources(QGuiGLContext *ctx)
{
    QMutexLocker locker(&m_mutex);

    QList<QGLSharedResource *>::iterator it = m_pendingDeletion.begin();
    QList<QGLSharedResource *>::iterator end = m_pendingDeletion.end();
    while (it != end) {
        (*it)->freeResource(ctx);
        delete *it;
        ++it;
    }
    m_pendingDeletion.clear();
}

QGLSharedResource::QGLSharedResource(QGuiGLContextGroup *group)
    : m_group(group)
{
    QMutexLocker locker(&m_group->d_func()->m_mutex);
    m_group->d_func()->m_sharedResources << this;
}

QGLSharedResource::~QGLSharedResource()
{
}

// schedule the resource for deletion at an appropriate time
void QGLSharedResource::free()
{
    if (!m_group) {
        delete this;
        return;
    }

    QMutexLocker locker(&m_group->d_func()->m_mutex);
    m_group->d_func()->m_sharedResources.removeOne(this);
    m_group->d_func()->m_pendingDeletion << this;

    // can we delete right away?
    QGuiGLContext *current = QGuiGLContext::currentContext();
    if (current && current->shareGroup() == m_group) {
        m_group->d_func()->deletePendingResources(current);
    }
}

QGLMultiGroupSharedResource::QGLMultiGroupSharedResource()
    : active(0)
{
#ifdef QT_GL_CONTEXT_RESOURCE_DEBUG
    qDebug("Creating context group resource object %p.", this);
#endif
}

QGLMultiGroupSharedResource::~QGLMultiGroupSharedResource()
{
#ifdef QT_GL_CONTEXT_RESOURCE_DEBUG
    qDebug("Deleting context group resource %p. Group size: %d.", this, m_groups.size());
#endif
    for (int i = 0; i < m_groups.size(); ++i) {
        QGuiGLContext *context = m_groups.at(i)->shares().first();
        QGLSharedResource *resource = value(context);
        if (resource)
            resource->free();
        m_groups.at(i)->d_func()->m_resources.remove(this);
        active.deref();
    }
#ifndef QT_NO_DEBUG
    if (active != 0) {
        qWarning("QtOpenGL: Resources are still available at program shutdown.\n"
                 "          This is possibly caused by a leaked QGLWidget, \n"
                 "          QGLFramebufferObject or QGLPixelBuffer.");
    }
#endif
}

void QGLMultiGroupSharedResource::insert(QGuiGLContext *context, QGLSharedResource *value)
{
#ifdef QT_GL_CONTEXT_RESOURCE_DEBUG
    qDebug("Inserting context group resource %p for context %p, managed by %p.", value, context, this);
#endif
    QGuiGLContextGroup *group = context->shareGroup();
    Q_ASSERT(!group->d_func()->m_resources.contains(this));
    group->d_func()->m_resources.insert(this, value);
    m_groups.append(group);
    active.ref();
}

QGLSharedResource *QGLMultiGroupSharedResource::value(QGuiGLContext *context)
{
    QGuiGLContextGroup *group = context->shareGroup();
    return group->d_func()->m_resources.value(this, 0);
}

void QGLMultiGroupSharedResource::cleanup(QGuiGLContext *ctx)
{
    QGLSharedResource *resource = value(ctx);

    if (resource != 0) {
        resource->free();

        QGuiGLContextGroup *group = ctx->shareGroup();
        group->d_func()->m_resources.remove(this);
        m_groups.removeOne(group);
        active.deref();
    }
}

void QGLMultiGroupSharedResource::cleanup(QGuiGLContext *ctx, QGLSharedResource *value)
{
#ifdef QT_GL_CONTEXT_RESOURCE_DEBUG
    qDebug("Cleaning up context group resource %p, for context %p in thread %p.", this, ctx, QThread::currentThread());
#endif
    value->free();
    active.deref();

    QGuiGLContextGroup *group = ctx->shareGroup();
    m_groups.removeOne(group);
}

