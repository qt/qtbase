// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    ~QWindowsSystemTrayIcon() override;

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
    bool setIconVisible(bool visible);
    bool isIconVisible() const;
    HICON createIcon(const QIcon &icon);

    QIcon m_icon;
    QString m_toolTip;
    HWND m_hwnd = nullptr;
    HICON m_hIcon = nullptr;
    HICON m_hMessageIcon = nullptr;
    mutable QPointer<QWindowsPopupMenu> m_menu;
    bool m_ignoreNextMouseRelease = false;
    bool m_visible = false;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const QWindowsSystemTrayIcon *);
#endif // !QT_NO_DEBUG_STREAM

QT_END_NAMESPACE

#endif // QWINDOWSSYSTEMTRAYICON_H
