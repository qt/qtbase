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

#ifndef QWINDOWSNATIVEINTERFACE_H
#define QWINDOWSNATIVEINTERFACE_H

#include <QtGui/qfont.h>
#include <QtGui/qpa/qplatformnativeinterface.h>
#include <QtPlatformHeaders/qwindowswindowfunctions.h>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsNativeInterface
    \brief Provides access to native handles.

    Currently implemented keys
    \list
    \li handle (HWND)
    \li getDC (DC)
    \li releaseDC Releases the previously acquired DC and returns 0.
    \endlist

    \internal
*/

class QWindowsNativeInterface : public QPlatformNativeInterface
{
    Q_OBJECT
    Q_PROPERTY(bool asyncExpose READ asyncExpose WRITE setAsyncExpose)
    Q_PROPERTY(bool darkMode READ isDarkMode STORED false NOTIFY darkModeChanged)
    Q_PROPERTY(bool darkModeStyle READ isDarkModeStyle STORED false)
    Q_PROPERTY(QVariant gpu READ gpu STORED false)
    Q_PROPERTY(QVariant gpuList READ gpuList STORED false)

public:
    void *nativeResourceForIntegration(const QByteArray &resource) override;
#ifndef QT_NO_OPENGL
    void *nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context) override;
#endif
    void *nativeResourceForWindow(const QByteArray &resource, QWindow *window) override;
    void *nativeResourceForScreen(const QByteArray &resource, QScreen *screen) override;
#ifndef QT_NO_CURSOR
    void *nativeResourceForCursor(const QByteArray &resource, const QCursor &cursor) override;
#endif
    Q_INVOKABLE void *createMessageWindow(const QString &classNameTemplate,
                                          const QString &windowName,
                                          void *eventProc) const;

    Q_INVOKABLE QString registerWindowClass(const QString &classNameIn, void *eventProc) const;

    Q_INVOKABLE void registerWindowsMime(void *mimeIn);
    Q_INVOKABLE void unregisterWindowsMime(void *mime);
    Q_INVOKABLE int registerMimeType(const QString &mimeType);
    Q_INVOKABLE QFont logFontToQFont(const void *logFont, int verticalDpi);

    bool asyncExpose() const;
    void setAsyncExpose(bool value);

    bool isDarkMode() const;
    bool isDarkModeStyle() const;

    QVariant gpu() const;
    QVariant gpuList() const;

    QVariantMap windowProperties(QPlatformWindow *window) const override;
    QVariant windowProperty(QPlatformWindow *window, const QString &name) const override;
    QVariant windowProperty(QPlatformWindow *window, const QString &name, const QVariant &defaultValue) const override;
    void setWindowProperty(QPlatformWindow *window, const QString &name, const QVariant &value) override;

    static QWindowsWindowFunctions::WindowActivationBehavior windowActivationBehavior()
        { return QWindowsNativeInterface::m_windowActivationBehavior; }
    static void setWindowActivationBehavior(QWindowsWindowFunctions::WindowActivationBehavior b)
        { QWindowsNativeInterface::m_windowActivationBehavior = b; }

    static bool isTabletMode();

    QFunctionPointer platformFunction(const QByteArray &function) const override;

Q_SIGNALS:
    void darkModeChanged(bool);

private:
    static QWindowsWindowFunctions::WindowActivationBehavior m_windowActivationBehavior;
};

QT_END_NAMESPACE

#endif // QWINDOWSNATIVEINTERFACE_H
