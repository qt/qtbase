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

#ifndef QWINRTWINDOW_H
#define QWINRTWINDOW_H

#include <QtCore/QLoggingCategory>
#include <qpa/qplatformwindow.h>
#include <qpa/qwindowsysteminterface.h>
#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaWindows)

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
