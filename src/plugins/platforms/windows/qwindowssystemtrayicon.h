/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef QWINDOWSSYSTEMTRAYICON_H
#define QWINDOWSSYSTEMTRAYICON_H

#include <QtGui/qicon.h>
#include <QtGui/qpa/qplatformsystemtrayicon.h>

#include <QtCore/qpointer.h>
#include <QtCore/qstring.h>
#include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

class QDebug;

class QWindowsPopupMenu;

class QWindowsSystemTrayIcon : public QPlatformSystemTrayIcon
{
public:
    QWindowsSystemTrayIcon();
    ~QWindowsSystemTrayIcon();

    void init() override;
    void cleanup() override;
    void updateIcon(const QIcon &icon) override;
    void updateToolTip(const QString &tooltip) override;
    void updateMenu(QPlatformMenu *) override {}
    QRect geometry() const override;
    void showMessage(const QString &title, const QString &msg,
                     const QIcon &icon, MessageIcon iconType, int msecs) override;

    bool isSystemTrayAvailable() const override { return true; }
    bool supportsMessages() const override;

    QPlatformMenu *createMenu() const override;

    bool winEvent(const MSG &message, long *result);

#ifndef QT_NO_DEBUG_STREAM
    void formatDebug(QDebug &d) const;
#endif

private:
    bool isInstalled() const { return m_hwnd != nullptr; }
    bool ensureInstalled();
    void ensureCleanup();
    bool sendTrayMessage(DWORD msg);
    HICON createIcon(const QIcon &icon);

    QIcon m_icon;
    QString m_toolTip;
    HWND m_hwnd = nullptr;
    HICON m_hIcon = nullptr;
    mutable QPointer<QWindowsPopupMenu> m_menu;
    bool m_ignoreNextMouseRelease = false;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const QWindowsSystemTrayIcon *);
#endif // !QT_NO_DEBUG_STREAM

QT_END_NAMESPACE

#endif // QWINDOWSSYSTEMTRAYICON_H
