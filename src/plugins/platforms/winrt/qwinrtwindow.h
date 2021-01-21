/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
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
    ~QWinRTWindow() override;

    QSurfaceFormat format() const override;
    bool isActive() const override;
    bool isExposed() const override;
    void setGeometry(const QRect &rect) override;
    void setVisible(bool visible) override;
    void setWindowTitle(const QString &title) override;
    void raise() override;
    void lower() override;

    WId winId() const override;

    qreal devicePixelRatio() const override;
    void setWindowState(Qt::WindowStates state) override;

    bool setMouseGrabEnabled(bool grab) override;
    bool setKeyboardGrabEnabled(bool grab) override;

    EGLSurface eglSurface() const;
    void createEglSurface(EGLDisplay display, EGLConfig config);

private:
    QScopedPointer<QWinRTWindowPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTWindow)
};

QT_END_NAMESPACE

#endif // QWINRTWINDOW_H
