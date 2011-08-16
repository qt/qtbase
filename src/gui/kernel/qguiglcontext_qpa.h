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

#ifndef QGUIGLCONTEXT_H
#define QGUIGLCONTEXT_H

#include <QtCore/qnamespace.h>
#include <QtCore/QScopedPointer>

#include <QSurfaceFormat>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QGuiGLContextPrivate;
class QGuiGLContextGroupPrivate;
class QPlatformGLContext;
class QSurface;

class Q_GUI_EXPORT QGuiGLContextGroup : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QGuiGLContextGroup)
public:
    ~QGuiGLContextGroup();

    QList<QGuiGLContext *> shares() const;

    static QGuiGLContextGroup *currentContextGroup();

private:
    QGuiGLContextGroup();

    friend class QGuiGLContext;
    friend class QGLContextGroupResourceBase;
    friend class QGLSharedResource;
    friend class QGLMultiGroupSharedResource;
};

class Q_GUI_EXPORT QGuiGLContext : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QGuiGLContext);
public:
    QGuiGLContext();
    ~QGuiGLContext();

    void setFormat(const QSurfaceFormat &format);
    void setShareContext(QGuiGLContext *shareContext);
    void setScreen(QScreen *screen);

    bool create();
    bool isValid() const;

    QSurfaceFormat format() const;
    QGuiGLContext *shareContext() const;
    QGuiGLContextGroup *shareGroup() const;
    QScreen *screen() const;

    bool makeCurrent(QSurface *surface);
    void doneCurrent();

    void swapBuffers(QSurface *surface);
    void (*getProcAddress(const QByteArray &procName)) ();

    QSurface *surface() const;

    static QGuiGLContext *currentContext();
    static bool areSharing(QGuiGLContext *first, QGuiGLContext *second);

    QPlatformGLContext *handle() const;
    QPlatformGLContext *shareHandle() const;

private:
    //hack to make it work with QGLContext::CurrentContext
    friend class QGLContext;
    friend class QGLContextResourceBase;
    friend class QWidgetPrivate;

    void *qGLContextHandle() const;
    void setQGLContextHandle(void *handle,void (*qGLContextDeleteFunction)(void *));
    void deleteQGLContext();

    void destroy();
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QGUIGLCONTEXT_H
