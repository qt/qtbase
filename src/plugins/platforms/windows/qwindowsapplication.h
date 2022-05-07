/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QWINDOWSAPPLICATION_H
#define QWINDOWSAPPLICATION_H

#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

class QWindowsApplication : public QNativeInterface::Private::QWindowsApplication
{
public:
    void setTouchWindowTouchType(TouchWindowTouchTypes type) override;
    TouchWindowTouchTypes touchWindowTouchType() const override;

    WindowActivationBehavior windowActivationBehavior() const override;
    void setWindowActivationBehavior(WindowActivationBehavior behavior) override;

    bool isTabletMode() const override;

    bool isWinTabEnabled() const override;
    bool setWinTabEnabled(bool enabled) override;

    bool isDarkMode() const override;
    DarkModeHandling darkModeHandling() const override;
    void setDarkModeHandling(DarkModeHandling handling) override;

    void registerMime(QNativeInterface::Private::QWindowsMime *mime) override;
    void unregisterMime(QNativeInterface::Private::QWindowsMime *mime) override;

    int registerMimeType(const QString &mime) override;

    HWND createMessageWindow(const QString &classNameTemplate,
                             const QString &windowName,
                             QFunctionPointer eventProc = nullptr) const override;

    bool asyncExpose() const override;
    void setAsyncExpose(bool value) override;

    QVariant gpu() const override;
    QVariant gpuList() const override;

private:
    WindowActivationBehavior m_windowActivationBehavior = DefaultActivateWindow;
    TouchWindowTouchTypes m_touchWindowTouchTypes = NormalTouch;
    DarkModeHandling m_darkModeHandling;
};

QT_END_NAMESPACE

#endif // QWINDOWSAPPLICATION_H
