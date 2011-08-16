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

#ifndef QGUIGLCONTEXT_P_H
#define QGUIGLCONTEXT_P_H

#include "qguiglcontext_qpa.h"
#include <private/qobject_p.h>
#include <qmutex.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QGuiGLContext;
class QGLMultiGroupSharedResource;

class Q_GUI_EXPORT QGLSharedResource
{
public:
    QGLSharedResource(QGuiGLContextGroup *group);
    virtual ~QGLSharedResource() = 0;

    QGuiGLContextGroup *group() const { return m_group; }

    // schedule the resource for deletion at an appropriate time
    void free();

protected:
    // the resource's share group no longer exists, invalidate the resource
    virtual void invalidateResource() = 0;

    // a valid context in the group is current, free the resource
    virtual void freeResource(QGuiGLContext *context) = 0;

private:
    QGuiGLContextGroup *m_group;

    friend class QGuiGLContextGroup;
    friend class QGuiGLContextGroupPrivate;

    Q_DISABLE_COPY(QGLSharedResource);
};

class Q_GUI_EXPORT QGuiGLContextGroupPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QGuiGLContextGroup);
public:
    QGuiGLContextGroupPrivate()
        : m_context(0)
        , m_mutex(QMutex::Recursive)
        , m_refs(0)
    {
    }

    void addContext(QGuiGLContext *ctx);
    void removeContext(QGuiGLContext *ctx);

    void deletePendingResources(QGuiGLContext *ctx);

    QGuiGLContext *m_context;

    QList<QGuiGLContext *> m_shares;
    QMutex m_mutex;

    QHash<QGLMultiGroupSharedResource *, QGLSharedResource *> m_resources;
    QAtomicInt m_refs;

    QList<QGLSharedResource *> m_sharedResources;
    QList<QGLSharedResource *> m_pendingDeletion;

    void cleanupResources(QGuiGLContext *ctx);
};

class Q_GUI_EXPORT QGLMultiGroupSharedResource
{
public:
    QGLMultiGroupSharedResource();
    ~QGLMultiGroupSharedResource();

    void insert(QGuiGLContext *context, QGLSharedResource *value);
    void cleanup(QGuiGLContext *context);
    void cleanup(QGuiGLContext *context, QGLSharedResource *value);

    QGLSharedResource *value(QGuiGLContext *context);

    template <typename T>
    T *value(QGuiGLContext *context) {
        QGuiGLContextGroup *group = context->shareGroup();
        T *resource = static_cast<T *>(group->d_func()->m_resources.value(this, 0));
        if (!resource) {
            resource = new T(context);
            insert(context, resource);
        }
        return resource;
    }

private:
    QAtomicInt active;
    QList<QGuiGLContextGroup *> m_groups;
};

class Q_GUI_EXPORT QGuiGLContextPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QGuiGLContext);
public:
    QGuiGLContextPrivate()
        : qGLContextHandle(0)
        , platformGLContext(0)
        , shareContext(0)
        , shareGroup(0)
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
    QGuiGLContextGroup *shareGroup;
    QScreen *screen;
    QSurface *surface;

    QHash<QGLMultiGroupSharedResource *, void *> m_resources;

    static void setCurrentContext(QGuiGLContext *context);
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QGUIGLCONTEXT_P_H
