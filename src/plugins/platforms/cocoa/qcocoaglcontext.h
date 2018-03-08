/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOCOAGLCONTEXT_H
#define QCOCOAGLCONTEXT_H

#include <QtCore/QPointer>
#include <qpa/qplatformopenglcontext.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QWindow>

#include <AppKit/AppKit.h>

QT_BEGIN_NAMESPACE

class QCocoaGLContext : public QPlatformOpenGLContext
{
public:
    QCocoaGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, const QVariant &nativeHandle);
    ~QCocoaGLContext();

    QSurfaceFormat format() const override;

    void swapBuffers(QPlatformSurface *surface) override;

    bool makeCurrent(QPlatformSurface *surface) override;
    void doneCurrent() override;

    QFunctionPointer getProcAddress(const char *procName) override;

    void update();

    static NSOpenGLPixelFormat *createNSOpenGLPixelFormat(const QSurfaceFormat &format);
    NSOpenGLContext *nsOpenGLContext() const;

    bool isSharing() const override;
    bool isValid() const override;

    void windowWasHidden();

    QVariant nativeHandle() const;

private:
    void setActiveWindow(QWindow *window);
    void updateSurfaceFormat();

    NSOpenGLContext *m_context;
    NSOpenGLContext *m_shareContext;
    QSurfaceFormat m_format;
    QPointer<QWindow> m_currentWindow;
    bool m_didCheckForSoftwareContext;
};

QT_END_NAMESPACE

#endif // QCOCOAGLCONTEXT_H
