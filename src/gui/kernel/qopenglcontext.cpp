/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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
#include "qopenglcontext.h"
#include "qopenglcontext_p.h"
#include "qwindow.h"

#include <QtCore/QThreadStorage>
#include <QtCore/QThread>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/QScreen>

#include <private/qopenglextensions_p.h>

#include <QDebug>

QT_BEGIN_NAMESPACE

class QGuiGLThreadContext
{
public:
    ~QGuiGLThreadContext() {
        if (context)
            context->doneCurrent();
    }
    QOpenGLContext *context;
};

static QThreadStorage<QGuiGLThreadContext *> qwindow_context_storage;

void QOpenGLContextPrivate::setCurrentContext(QOpenGLContext *context)
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
QOpenGLContext* QOpenGLContext::currentContext()
{
    QGuiGLThreadContext *threadContext = qwindow_context_storage.localData();
    if(threadContext) {
        return threadContext->context;
    }
    return 0;
}

bool QOpenGLContext::areSharing(QOpenGLContext *first, QOpenGLContext *second)
{
    return first->shareGroup() == second->shareGroup();
}

QPlatformOpenGLContext *QOpenGLContext::handle() const
{
    Q_D(const QOpenGLContext);
    return d->platformGLContext;
}

QPlatformOpenGLContext *QOpenGLContext::shareHandle() const
{
    Q_D(const QOpenGLContext);
    if (d->shareContext)
        return d->shareContext->handle();
    return 0;
}

/*!
  Creates a new GL context instance, you need to call create() before it can be used.
*/
QOpenGLContext::QOpenGLContext(QObject *parent)
    : QObject(*new QOpenGLContextPrivate(), parent)
{
    Q_D(QOpenGLContext);
    d->screen = QGuiApplication::primaryScreen();
}

/*!
  Sets the format the GL context should be compatible with. You need to call create() before it takes effect.
*/
void QOpenGLContext::setFormat(const QSurfaceFormat &format)
{
    Q_D(QOpenGLContext);
    d->requestedFormat = format;
}

/*!
  Sets the context to share textures, shaders, and other GL resources with. You need to call create() before it takes effect.
*/
void QOpenGLContext::setShareContext(QOpenGLContext *shareContext)
{
    Q_D(QOpenGLContext);
    d->shareContext = shareContext;
}

/*!
  Sets the screen the GL context should be valid for. You need to call create() before it takes effect.
*/
void QOpenGLContext::setScreen(QScreen *screen)
{
    Q_D(QOpenGLContext);
    d->screen = screen;
    if (!d->screen)
        d->screen = QGuiApplication::primaryScreen();
}

/*!
  Attempts to create the GL context with the desired parameters.

  Returns true if the native context was successfully created and is ready to be used.
*/
bool QOpenGLContext::create()
{
    destroy();

    Q_D(QOpenGLContext);
    d->platformGLContext = QGuiApplicationPrivate::platformIntegration()->createPlatformOpenGLContext(this);
    d->platformGLContext->setContext(this);
    if (!d->platformGLContext->isSharing())
        d->shareContext = 0;
    d->shareGroup = d->shareContext ? d->shareContext->shareGroup() : new QOpenGLContextGroup;
    d->shareGroup->d_func()->addContext(this);
    return d->platformGLContext;
}

void QOpenGLContext::destroy()
{
    Q_D(QOpenGLContext);
    if (d->platformGLContext)
        emit aboutToBeDestroyed();
    if (QOpenGLContext::currentContext() == this)
        doneCurrent();
    if (d->shareGroup)
        d->shareGroup->d_func()->removeContext(this);
    d->shareGroup = 0;
    delete d->platformGLContext;
    d->platformGLContext = 0;
    delete d->functions;
    d->functions = 0;
}

/*!
    \fn void QOpenGLContext::aboutToBeDestroyed()

    This signal is emitted before the underlying native OpenGL context is
    destroyed, such that users may clean up OpenGL resources that might otherwise
    be left dangling in the case of shared OpenGL contexts.

    If you wish to make the context current in order to do clean-up, make sure to
    only connect to the signal using a direct connection.
*/

/*!
  If this is the current context for the thread, doneCurrent is called
*/
QOpenGLContext::~QOpenGLContext()
{
    destroy();
}

/*!
  Returns if this context is valid, i.e. has been successfully created.
*/
bool QOpenGLContext::isValid() const
{
    Q_D(const QOpenGLContext);
    return d->platformGLContext && d->platformGLContext->isValid();
}

/*!
  Get the QOpenGLFunctions instance for this context.

  The context or a sharing context must be current.
*/

QOpenGLFunctions *QOpenGLContext::functions() const
{
    Q_D(const QOpenGLContext);
    if (!d->functions)
        const_cast<QOpenGLFunctions *&>(d->functions) = new QOpenGLExtensions(QOpenGLContext::currentContext());
    return d->functions;
}

/*!
  If surface is 0 this is equivalent to calling doneCurrent().

  Do not call this function from a different thread than the one the QOpenGLContext instance lives in. If
  you wish to use QOpenGLContext from a different thread you should first call make sure it's not current
  in the current thread, by calling doneCurrent() if necessary. Then call moveToThread(otherThread)
  before using it in the other thread.
*/
bool QOpenGLContext::makeCurrent(QSurface *surface)
{
    Q_D(QOpenGLContext);
    if (!isValid())
        return false;

    if (thread() != QThread::currentThread())
        qFatal("Cannot make QOpenGLContext current in a different thread");

    if (!surface) {
        doneCurrent();
        return true;
    }

    if (!surface->surfaceHandle())
        return false;

    if (d->platformGLContext->makeCurrent(surface->surfaceHandle())) {
        QOpenGLContextPrivate::setCurrentContext(this);
        d->surface = surface;

        d->shareGroup->d_func()->deletePendingResources(this);

        return true;
    }

    return false;
}

/*!
    Convenience function for calling makeCurrent with a 0 surface.
*/
void QOpenGLContext::doneCurrent()
{
    Q_D(QOpenGLContext);
    if (!isValid())
        return;

    if (QOpenGLContext::currentContext() == this)
        d->shareGroup->d_func()->deletePendingResources(this);

    d->platformGLContext->doneCurrent();
    QOpenGLContextPrivate::setCurrentContext(0);

    d->surface = 0;
}

/*!
    Returns the surface the context is current for.
*/
QSurface *QOpenGLContext::surface() const
{
    Q_D(const QOpenGLContext);
    return d->surface;
}


void QOpenGLContext::swapBuffers(QSurface *surface)
{
    Q_D(QOpenGLContext);
    if (!isValid())
        return;

    if (!surface) {
        qWarning() << "QOpenGLContext::swapBuffers() called with null argument";
        return;
    }

    QPlatformSurface *surfaceHandle = surface->surfaceHandle();
    if (surfaceHandle)
        d->platformGLContext->swapBuffers(surfaceHandle);
}

QFunctionPointer QOpenGLContext::getProcAddress(const QByteArray &procName)
{
    Q_D(QOpenGLContext);
    if (!d->platformGLContext)
        return 0;
    return d->platformGLContext->getProcAddress(procName);
}

QSurfaceFormat QOpenGLContext::format() const
{
    Q_D(const QOpenGLContext);
    if (!d->platformGLContext)
        return d->requestedFormat;
    return d->platformGLContext->format();
}

QOpenGLContextGroup *QOpenGLContext::shareGroup() const
{
    Q_D(const QOpenGLContext);
    return d->shareGroup;
}

QOpenGLContext *QOpenGLContext::shareContext() const
{
    Q_D(const QOpenGLContext);
    return d->shareContext;
}

QScreen *QOpenGLContext::screen() const
{
    Q_D(const QOpenGLContext);
    return d->screen;
}

/*
  internal: Needs to have a pointer to qGLContext. But since this is in QtGui we cant
  have any type information.
*/
void *QOpenGLContext::qGLContextHandle() const
{
    Q_D(const QOpenGLContext);
    return d->qGLContextHandle;
}

void QOpenGLContext::setQGLContextHandle(void *handle,void (*qGLContextDeleteFunction)(void *))
{
    Q_D(QOpenGLContext);
    d->qGLContextHandle = handle;
    d->qGLContextDeleteFunction = qGLContextDeleteFunction;
}

void QOpenGLContext::deleteQGLContext()
{
    Q_D(QOpenGLContext);
    if (d->qGLContextDeleteFunction && d->qGLContextHandle) {
        d->qGLContextDeleteFunction(d->qGLContextHandle);
        d->qGLContextDeleteFunction = 0;
        d->qGLContextHandle = 0;
    }
}

QOpenGLContextGroup::QOpenGLContextGroup()
    : QObject(*new QOpenGLContextGroupPrivate())
{
}

QOpenGLContextGroup::~QOpenGLContextGroup()
{
    Q_D(QOpenGLContextGroup);
    d->cleanup();
}

QList<QOpenGLContext *> QOpenGLContextGroup::shares() const
{
    Q_D(const QOpenGLContextGroup);
    return d->m_shares;
}

QOpenGLContextGroup *QOpenGLContextGroup::currentContextGroup()
{
    QOpenGLContext *current = QOpenGLContext::currentContext();
    return current ? current->shareGroup() : 0;
}

void QOpenGLContextGroupPrivate::addContext(QOpenGLContext *ctx)
{
    QMutexLocker locker(&m_mutex);
    m_refs.ref();
    m_shares << ctx;
}

void QOpenGLContextGroupPrivate::removeContext(QOpenGLContext *ctx)
{
    Q_Q(QOpenGLContextGroup);

    QMutexLocker locker(&m_mutex);
    m_shares.removeOne(ctx);

    if (ctx == m_context && !m_shares.isEmpty())
        m_context = m_shares.first();

    if (!m_refs.deref()) {
        cleanup();
        q->deleteLater();
    }
}

void QOpenGLContextGroupPrivate::cleanup()
{
    Q_Q(QOpenGLContextGroup);
    {
        QHash<QOpenGLMultiGroupSharedResource *, QOpenGLSharedResource *>::const_iterator it, end;
        end = m_resources.constEnd();
        for (it = m_resources.constBegin(); it != end; ++it)
            it.key()->cleanup(q, it.value());
        m_resources.clear();
    }

    QList<QOpenGLSharedResource *>::iterator it = m_sharedResources.begin();
    QList<QOpenGLSharedResource *>::iterator end = m_sharedResources.end();

    while (it != end) {
        (*it)->invalidateResource();
        (*it)->m_group = 0;
        ++it;
    }

    m_sharedResources.clear();

    qDeleteAll(m_pendingDeletion.begin(), m_pendingDeletion.end());
    m_pendingDeletion.clear();
}

void QOpenGLContextGroupPrivate::deletePendingResources(QOpenGLContext *ctx)
{
    QMutexLocker locker(&m_mutex);

    QList<QOpenGLSharedResource *> pending = m_pendingDeletion;
    m_pendingDeletion.clear();

    QList<QOpenGLSharedResource *>::iterator it = pending.begin();
    QList<QOpenGLSharedResource *>::iterator end = pending.end();
    while (it != end) {
        (*it)->freeResource(ctx);
        delete *it;
        ++it;
    }
}

/*!
    \class QOpenGLSharedResource
    \internal
    \since 5.0
    \brief The QOpenGLSharedResource class is used to keep track of resources that
    are shared between OpenGL contexts (like textures, framebuffer objects, shader
    programs, etc), and clean them up in a safe way when they're no longer needed.

    The QOpenGLSharedResource instance should never be deleted, instead free()
    should be called when it's no longer needed. Thus it will be put on a queue
    and freed at an appropriate time (when a context in the share group becomes
    current).

    The sub-class needs to implement two pure virtual functions. The first,
    freeResource() must be implemented to actually do the freeing, for example
    call glDeleteTextures() on a texture id. Qt makes sure a valid context in
    the resource's share group is current at the time. The other, invalidateResource(),
    is called by Qt in the circumstance when the last context in the share group is
    destroyed before free() has been called. The implementation of invalidateResource()
    should set any identifiers to 0 or set a flag to prevent them from being used
    later on.
*/
QOpenGLSharedResource::QOpenGLSharedResource(QOpenGLContextGroup *group)
    : m_group(group)
{
    QMutexLocker locker(&m_group->d_func()->m_mutex);
    m_group->d_func()->m_sharedResources << this;
}

QOpenGLSharedResource::~QOpenGLSharedResource()
{
}

// schedule the resource for deletion at an appropriate time
void QOpenGLSharedResource::free()
{
    if (!m_group) {
        delete this;
        return;
    }

    QMutexLocker locker(&m_group->d_func()->m_mutex);
    m_group->d_func()->m_sharedResources.removeOne(this);
    m_group->d_func()->m_pendingDeletion << this;

    // can we delete right away?
    QOpenGLContext *current = QOpenGLContext::currentContext();
    if (current && current->shareGroup() == m_group) {
        m_group->d_func()->deletePendingResources(current);
    }
}

/*!
    \class QOpenGLSharedResourceGuard
    \internal
    \since 5.0
    \brief The QOpenGLSharedResourceGuard class is a convenience sub-class of
    QOpenGLSharedResource to be used to track a single OpenGL object with a
    GLuint identifier. The constructor takes a function pointer to a function
    that will be used to free the resource if and when necessary.
*/
void QOpenGLSharedResourceGuard::freeResource(QOpenGLContext *context)
{
    if (m_id) {
        QOpenGLFunctions functions(context);
        m_func(&functions, m_id);
        m_id = 0;
    }
}

/*!
    \class QOpenGLMultiGroupSharedResource
    \internal
    \since 5.0
    \brief The QOpenGLMultiGroupSharedResource keeps track of a shared resource
    that might be needed from multiple contexts, like a glyph cache or gradient
    cache. One instance of the object is created for each group when
    necessary. The shared resource instance should have a constructor that
    takes a QOpenGLContext *. To get an instance for a given context one calls
    T *QOpenGLMultiGroupSharedResource::value<T>(context), where T is a sub-class
    of QOpenGLSharedResource.

    You should not call free() on QOpenGLSharedResources owned by a
    QOpenGLMultiGroupSharedResource instance.
*/
QOpenGLMultiGroupSharedResource::QOpenGLMultiGroupSharedResource()
    : active(0)
{
#ifdef QT_GL_CONTEXT_RESOURCE_DEBUG
    qDebug("Creating context group resource object %p.", this);
#endif
}

QOpenGLMultiGroupSharedResource::~QOpenGLMultiGroupSharedResource()
{
#ifdef QT_GL_CONTEXT_RESOURCE_DEBUG
    qDebug("Deleting context group resource %p. Group size: %d.", this, m_groups.size());
#endif
    for (int i = 0; i < m_groups.size(); ++i) {
        if (!m_groups.at(i)->shares().isEmpty()) {
            QOpenGLContext *context = m_groups.at(i)->shares().first();
            QOpenGLSharedResource *resource = value(context);
            if (resource)
                resource->free();
        }
        m_groups.at(i)->d_func()->m_resources.remove(this);
        active.deref();
    }
#ifndef QT_NO_DEBUG
    if (active.load() != 0) {
        qWarning("QtGui: Resources are still available at program shutdown.\n"
                 "          This is possibly caused by a leaked QOpenGLWidget, \n"
                 "          QOpenGLFramebufferObject or QOpenGLPixelBuffer.");
    }
#endif
}

void QOpenGLMultiGroupSharedResource::insert(QOpenGLContext *context, QOpenGLSharedResource *value)
{
#ifdef QT_GL_CONTEXT_RESOURCE_DEBUG
    qDebug("Inserting context group resource %p for context %p, managed by %p.", value, context, this);
#endif
    QOpenGLContextGroup *group = context->shareGroup();
    Q_ASSERT(!group->d_func()->m_resources.contains(this));
    group->d_func()->m_resources.insert(this, value);
    m_groups.append(group);
    active.ref();
}

QOpenGLSharedResource *QOpenGLMultiGroupSharedResource::value(QOpenGLContext *context)
{
    QOpenGLContextGroup *group = context->shareGroup();
    return group->d_func()->m_resources.value(this, 0);
}

QList<QOpenGLSharedResource *> QOpenGLMultiGroupSharedResource::resources() const
{
    QList<QOpenGLSharedResource *> result;
    for (QList<QOpenGLContextGroup *>::const_iterator it = m_groups.constBegin(); it != m_groups.constEnd(); ++it) {
        QOpenGLSharedResource *resource = (*it)->d_func()->m_resources.value(const_cast<QOpenGLMultiGroupSharedResource *>(this), 0);
        if (resource)
            result << resource;
    }
    return result;
}

void QOpenGLMultiGroupSharedResource::cleanup(QOpenGLContextGroup *group, QOpenGLSharedResource *value)
{
#ifdef QT_GL_CONTEXT_RESOURCE_DEBUG
    qDebug("Cleaning up context group resource %p, for group %p in thread %p.", this, group, QThread::currentThread());
#endif
    value->invalidateResource();
    value->free();
    active.deref();

    Q_ASSERT(m_groups.contains(group));
    m_groups.removeOne(group);
}

QT_END_NAMESPACE
