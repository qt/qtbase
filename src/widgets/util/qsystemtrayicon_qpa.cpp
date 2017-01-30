/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qsystemtrayicon_p.h"

#include <QtGui/qpa/qplatformsystemtrayicon.h>
#include <qpa/qplatformtheme.h>
#include <private/qguiapplication_p.h>

#include <QApplication>
#include <QStyle>

#ifndef QT_NO_SYSTEMTRAYICON

QT_BEGIN_NAMESPACE

QSystemTrayIconPrivate::QSystemTrayIconPrivate()
    : qpa_sys(QGuiApplicationPrivate::platformTheme()->createPlatformSystemTrayIcon())
    , visible(false)
{
}

QSystemTrayIconPrivate::~QSystemTrayIconPrivate()
{
    delete qpa_sys;
}

void QSystemTrayIconPrivate::install_sys()
{
    if (qpa_sys)
        install_sys_qpa();
}

void QSystemTrayIconPrivate::remove_sys()
{
    if (qpa_sys)
        remove_sys_qpa();
}

QRect QSystemTrayIconPrivate::geometry_sys() const
{
    if (qpa_sys)
        return qpa_sys->geometry();
    else
        return QRect();
}

void QSystemTrayIconPrivate::updateIcon_sys()
{
    if (qpa_sys)
        qpa_sys->updateIcon(icon);
}

void QSystemTrayIconPrivate::updateMenu_sys()
{
#if QT_CONFIG(menu)
    if (qpa_sys && menu) {
        addPlatformMenu(menu);
        qpa_sys->updateMenu(menu->platformMenu());
    }
#endif
}

void QSystemTrayIconPrivate::updateToolTip_sys()
{
    if (qpa_sys)
        qpa_sys->updateToolTip(toolTip);
}

bool QSystemTrayIconPrivate::isSystemTrayAvailable_sys()
{
    QScopedPointer<QPlatformSystemTrayIcon> sys(QGuiApplicationPrivate::platformTheme()->createPlatformSystemTrayIcon());
    if (sys)
        return sys->isSystemTrayAvailable();
    else
        return false;
}

bool QSystemTrayIconPrivate::supportsMessages_sys()
{
    QScopedPointer<QPlatformSystemTrayIcon> sys(QGuiApplicationPrivate::platformTheme()->createPlatformSystemTrayIcon());
    if (sys)
        return sys->supportsMessages();
    else
        return false;
}

void QSystemTrayIconPrivate::showMessage_sys(const QString &title, const QString &message,
                                             const QIcon &icon, QSystemTrayIcon::MessageIcon msgIcon, int msecs)
{
    if (qpa_sys)
        qpa_sys->showMessage(title, message, icon,
                        static_cast<QPlatformSystemTrayIcon::MessageIcon>(msgIcon), msecs);
}

QT_END_NAMESPACE

#endif // QT_NO_SYSTEMTRAYICON
