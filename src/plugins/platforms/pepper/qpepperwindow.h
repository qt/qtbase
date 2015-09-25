/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt for Native Client platform plugin.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPEPPERWINDOW_H
#define QPEPPERWINDOW_H

#include <QtCore/QLoggingCategory>
#include <qpa/qplatformwindow.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_WINDOW)

class QPepperIntegration;
class QPepperCompositor;

class QPepperWindow : public QPlatformWindow
{
public:
    QPepperWindow(QWindow *window);
    ~QPepperWindow();

    void setGeometry(const QRect &rect) Q_DECL_OVERRIDE;
    void setVisible(bool visible) Q_DECL_OVERRIDE;
    void setWindowState(Qt::WindowState state) Q_DECL_OVERRIDE;
    WId winId() const Q_DECL_OVERRIDE;
    void setParent(const QPlatformWindow *window) Q_DECL_OVERRIDE;
    void raise() Q_DECL_OVERRIDE;
    void lower() Q_DECL_OVERRIDE;

    qreal devicePixelRatio() const Q_DECL_OVERRIDE;
    bool setKeyboardGrabEnabled(bool grab) Q_DECL_OVERRIDE;
    bool setMouseGrabEnabled(bool grab) Q_DECL_OVERRIDE;
#ifdef Q_OS_NACL_EMSCRIPTEN
    void requestUpdate() Q_DECL_OVERRIDE;
#endif

private:
    QPepperCompositor *m_compositor;
};

QT_END_NAMESPACE

#endif
