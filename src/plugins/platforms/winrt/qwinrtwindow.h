/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINRTWINDOW_H
#define QWINRTWINDOW_H

#include <qpa/qplatformwindow.h>
#include <qpa/qwindowsysteminterface.h>
#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

class QWinRTWindowPrivate;
class QWinRTWindow : public QPlatformWindow
{
public:
    QWinRTWindow(QWindow *window);
    ~QWinRTWindow();

    QSurfaceFormat format() const;
    bool isActive() const;
    bool isExposed() const;
    void setGeometry(const QRect &rect);
    void setVisible(bool visible);
    void setWindowTitle(const QString &title);
    void raise();
    void lower();

    WId winId() const Q_DECL_OVERRIDE;

    qreal devicePixelRatio() const Q_DECL_OVERRIDE;
    void setWindowState(Qt::WindowState state) Q_DECL_OVERRIDE;

    EGLSurface eglSurface() const;
    void createEglSurface(EGLDisplay display, EGLConfig config);

private:
    QScopedPointer<QWinRTWindowPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTWindow)
};

QT_END_NAMESPACE

#endif // QWINRTWINDOW_H
