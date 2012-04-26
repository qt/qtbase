/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QCOCOAGLCONTEXT_H
#define QCOCOAGLCONTEXT_H

#include <QtCore/QWeakPointer>
#include <qpa/qplatformopenglcontext.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QWindow>

#undef slots
#include <Cocoa/Cocoa.h>

QT_BEGIN_NAMESPACE

class QCocoaGLContext : public QPlatformOpenGLContext
{
public:
    QCocoaGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share);

    QSurfaceFormat format() const;

    void swapBuffers(QPlatformSurface *surface);

    bool makeCurrent(QPlatformSurface *surface);
    void doneCurrent();

    void (*getProcAddress(const QByteArray &procName)) ();

    void update();

    static NSOpenGLPixelFormat *createNSOpenGLPixelFormat(const QSurfaceFormat &format);
    NSOpenGLContext *nsOpenGLContext() const;

    bool isSharing() const;
    bool isValid() const;

private:
    void setActiveWindow(QWindow *window);

    NSOpenGLContext *m_context;
    NSOpenGLContext *m_shareContext;
    QSurfaceFormat m_format;
    QWeakPointer<QWindow> m_currentWindow;
};

QT_END_NAMESPACE

#endif // QCOCOAGLCONTEXT_H
