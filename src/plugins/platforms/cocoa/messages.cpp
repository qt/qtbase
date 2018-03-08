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

#include "messages.h"

#include <QCoreApplication>

// Translatable messages should go into this .cpp file for them to be picked up by lupdate.

QT_BEGIN_NAMESPACE

QString msgAboutQt()
{
    return QCoreApplication::translate("QCocoaMenuItem", "About Qt");
}

static const char *application_menu_strings[] =
{
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","Services"),
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","Hide %1"),
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","Hide Others"),
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","Show All"),
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","Preferences..."),
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","Quit %1"),
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU","About %1")
};

QString qt_mac_applicationmenu_string(int type)
{
    QString menuString = QString::fromLatin1(application_menu_strings[type]);
    const QString translated = QCoreApplication::translate("QMenuBar", application_menu_strings[type]);
    if (translated != menuString) {
        return translated;
    } else {
        return QCoreApplication::translate("MAC_APPLICATION_MENU", application_menu_strings[type]);
    }
}

QPlatformMenuItem::MenuRole detectMenuRole(const QString &caption)
{
    QString captionNoAmpersand(caption);
    captionNoAmpersand.remove(QLatin1Char('&'));
    const QString aboutString = QCoreApplication::translate("QCocoaMenuItem", "About");
    if (captionNoAmpersand.startsWith(aboutString, Qt::CaseInsensitive) || caption.endsWith(aboutString, Qt::CaseInsensitive))
        return QPlatformMenuItem::AboutRole;
    if (captionNoAmpersand.startsWith(QCoreApplication::translate("QCocoaMenuItem", "Config"), Qt::CaseInsensitive)
        || captionNoAmpersand.startsWith(QCoreApplication::translate("QCocoaMenuItem", "Preference"), Qt::CaseInsensitive)
        || captionNoAmpersand.startsWith(QCoreApplication::translate("QCocoaMenuItem", "Options"), Qt::CaseInsensitive)
        || captionNoAmpersand.startsWith(QCoreApplication::translate("QCocoaMenuItem", "Setting"), Qt::CaseInsensitive)
        || captionNoAmpersand.startsWith(QCoreApplication::translate("QCocoaMenuItem", "Setup"), Qt::CaseInsensitive)) {
        return QPlatformMenuItem::PreferencesRole;
    }
    if (captionNoAmpersand.startsWith(QCoreApplication::translate("QCocoaMenuItem", "Quit"), Qt::CaseInsensitive)
        || captionNoAmpersand.startsWith(QCoreApplication::translate("QCocoaMenuItem", "Exit"), Qt::CaseInsensitive)) {
        return QPlatformMenuItem::QuitRole;
    }
    if (!captionNoAmpersand.compare(QCoreApplication::translate("QCocoaMenuItem", "Cut"), Qt::CaseInsensitive))
        return QPlatformMenuItem::CutRole;
    if (!captionNoAmpersand.compare(QCoreApplication::translate("QCocoaMenuItem", "Copy"), Qt::CaseInsensitive))
        return QPlatformMenuItem::CopyRole;
    if (!captionNoAmpersand.compare(QCoreApplication::translate("QCocoaMenuItem", "Paste"), Qt::CaseInsensitive))
        return QPlatformMenuItem::PasteRole;
    if (!captionNoAmpersand.compare(QCoreApplication::translate("QCocoaMenuItem", "Select All"), Qt::CaseInsensitive))
        return QPlatformMenuItem::SelectAllRole;
    return QPlatformMenuItem::NoRole;
}

QString msgDialogButtonDiscard()
{
    return QCoreApplication::translate("QCocoaTheme", "Don't Save");
}

QT_END_NAMESPACE
