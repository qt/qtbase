// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

    void setHasBorderInFullScreenDefault(bool border) override;

    bool isTabletMode() const override;

    bool isWinTabEnabled() const override;
    bool setWinTabEnabled(bool enabled) override;

    DarkModeHandling darkModeHandling() const override;
    void setDarkModeHandling(DarkModeHandling handling) override;

    void registerMime(QWindowsMimeConverter *mime) override;
    void unregisterMime(QWindowsMimeConverter *mime) override;

    int registerMimeType(const QString &mime) override;

    HWND createMessageWindow(const QString &classNameTemplate,
                             const QString &windowName,
                             QFunctionPointer eventProc = nullptr) const override;

    bool asyncExpose() const override;
    void setAsyncExpose(bool value) override;

    QVariant gpu() const override;
    QVariant gpuList() const override;

    void populateLightSystemPalette(QPalette &palette) const override;

private:
    WindowActivationBehavior m_windowActivationBehavior = DefaultActivateWindow;
    TouchWindowTouchTypes m_touchWindowTouchTypes = NormalTouch;
    DarkModeHandling m_darkModeHandling;
};

QT_END_NAMESPACE

#endif // QWINDOWSAPPLICATION_H
