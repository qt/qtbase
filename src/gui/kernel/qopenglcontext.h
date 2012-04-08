/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QOPENGLCONTEXT_H
#define QOPENGLCONTEXT_H

#ifndef QT_NO_OPENGL

#include <QtCore/qnamespace.h>
#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

#include <QtGui/QSurfaceFormat>

#ifdef __GLEW_H__
#if defined(Q_CC_GNU)
#warning qopenglfunctions.h is not compatible with GLEW, GLEW defines will be undefined
#warning To use GLEW with Qt, do not include <qopengl.h> or <QOpenGLFunctions> after glew.h
#endif
#endif

#include <QtGui/qopengl.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QOpenGLContextPrivate;
class QOpenGLContextGroupPrivate;
class QOpenGLFunctions;
class QPlatformOpenGLContext;

class QScreen;
class QSurface;

class Q_GUI_EXPORT QOpenGLContextGroup : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QOpenGLContextGroup)
public:
    ~QOpenGLContextGroup();

    QList<QOpenGLContext *> shares() const;

    static QOpenGLContextGroup *currentContextGroup();

private:
    QOpenGLContextGroup();

    friend class QOpenGLContext;
    friend class QOpenGLContextGroupResourceBase;
    friend class QOpenGLSharedResource;
    friend class QOpenGLMultiGroupSharedResource;
};

class Q_GUI_EXPORT QOpenGLContext : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QOpenGLContext)
public:
    QOpenGLContext(QObject *parent = 0);
    ~QOpenGLContext();

    void setFormat(const QSurfaceFormat &format);
    void setShareContext(QOpenGLContext *shareContext);
    void setScreen(QScreen *screen);

    bool create();
    bool isValid() const;

    QSurfaceFormat format() const;
    QOpenGLContext *shareContext() const;
    QOpenGLContextGroup *shareGroup() const;
    QScreen *screen() const;

    GLuint defaultFramebufferObject() const;

    bool makeCurrent(QSurface *surface);
    void doneCurrent();

    void swapBuffers(QSurface *surface);
    QFunctionPointer getProcAddress(const QByteArray &procName);

    QSurface *surface() const;

    static QOpenGLContext *currentContext();
    static bool areSharing(QOpenGLContext *first, QOpenGLContext *second);

    QPlatformOpenGLContext *handle() const;
    QPlatformOpenGLContext *shareHandle() const;

    QOpenGLFunctions *functions() const;

Q_SIGNALS:
    void aboutToBeDestroyed();

private:
    friend class QGLContext;
    friend class QOpenGLContextResourceBase;
    friend class QOpenGLPaintDevice;
    friend class QOpenGLGlyphTexture;
    friend class QOpenGLTextureGlyphCache;
    friend class QOpenGLEngineShaderManager;
    friend class QOpenGLFramebufferObject;
    friend class QOpenGLFramebufferObjectPrivate;
    friend class QOpenGL2PaintEngineEx;
    friend class QOpenGL2PaintEngineExPrivate;
    friend class QSGDistanceFieldGlyphCache;
    friend class QWidgetPrivate;

    void *qGLContextHandle() const;
    void setQGLContextHandle(void *handle,void (*qGLContextDeleteFunction)(void *));
    void deleteQGLContext();

    void destroy();
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_OPENGL

#endif // QGUIGLCONTEXT_H
