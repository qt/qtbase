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

#include "qiostheme.h"

#include <QtCore/QStringList>
#include <QtCore/QVariant>

#include <QtGui/QFont>

#include <QtFontDatabaseSupport/private/qcoretextfontdatabase_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

#include <UIKit/UIFont.h>
#include <UIKit/UIInterface.h>

#ifndef Q_OS_TVOS
#include "qiosmenu.h"
#include "qiosfiledialog.h"
#include "qiosmessagedialog.h"
#endif

QT_BEGIN_NAMESPACE

const char *QIOSTheme::name = "ios";

QIOSTheme::QIOSTheme()
    : m_systemPalette(*QPlatformTheme::palette(QPlatformTheme::SystemPalette))
{
    m_systemPalette.setBrush(QPalette::Highlight, QColor(204, 221, 237));
    m_systemPalette.setBrush(QPalette::HighlightedText, Qt::black);
}

QIOSTheme::~QIOSTheme()
{
    qDeleteAll(m_fonts);
}

const QPalette *QIOSTheme::palette(QPlatformTheme::Palette type) const
{
    if (type == QPlatformTheme::SystemPalette)
        return &m_systemPalette;
    return 0;
}

QPlatformMenuItem* QIOSTheme::createPlatformMenuItem() const
{
#ifdef Q_OS_TVOS
    return 0;
#else
    return new QIOSMenuItem();
#endif
}

QPlatformMenu* QIOSTheme::createPlatformMenu() const
{
#ifdef Q_OS_TVOS
    return 0;
#else
    return new QIOSMenu();
#endif
}

bool QIOSTheme::usePlatformNativeDialog(QPlatformTheme::DialogType type) const
{
    switch (type) {
    case FileDialog:
    case MessageDialog:
        return true;
    default:
        return false;
    }
}

QPlatformDialogHelper *QIOSTheme::createPlatformDialogHelper(QPlatformTheme::DialogType type) const
{
    switch (type) {
#ifndef Q_OS_TVOS
    case FileDialog:
        return new QIOSFileDialog();
        break;
    case MessageDialog:
        return new QIOSMessageDialog();
        break;
#endif
    default:
        return 0;
    }
}

QVariant QIOSTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case QPlatformTheme::StyleNames:
        return QStringList(QStringLiteral("fusion"));
    case KeyboardScheme:
        return QVariant(int(MacKeyboardScheme));
    default:
        return QPlatformTheme::themeHint(hint);
    }
}

const QFont *QIOSTheme::font(Font type) const
{
    if (m_fonts.isEmpty()) {
        QCoreTextFontDatabase *ctfd = static_cast<QCoreTextFontDatabase *>(QGuiApplicationPrivate::platformIntegration()->fontDatabase());
        m_fonts = ctfd->themeFonts();
    }

    return m_fonts.value(type, 0);
}

QT_END_NAMESPACE
