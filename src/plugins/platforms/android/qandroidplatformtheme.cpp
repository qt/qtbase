/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qandroidplatformtheme.h"
#include "qandroidplatformmenubar.h"
#include "qandroidplatformmenu.h"
#include "qandroidplatformmenuitem.h"
#include "qandroidplatformdialoghelpers.h"
#include <QVariant>
#include <QFileInfo>
#include <QCoreApplication>
#include <qandroidplatformintegration.h>

QAndroidPlatformTheme::QAndroidPlatformTheme(QAndroidPlatformNativeInterface *androidPlatformNativeInterface)
{
    m_androidPlatformNativeInterface = androidPlatformNativeInterface;
}

QPlatformMenuBar *QAndroidPlatformTheme::createPlatformMenuBar() const
{
    return new QAndroidPlatformMenuBar;
}

QPlatformMenu *QAndroidPlatformTheme::createPlatformMenu() const
{
    return new QAndroidPlatformMenu;
}

QPlatformMenuItem *QAndroidPlatformTheme::createPlatformMenuItem() const
{
    return new QAndroidPlatformMenuItem;
}

static inline int paletteType(QPlatformTheme::Palette type)
{
    switch (type) {
    case QPlatformTheme::ToolButtonPalette:
    case QPlatformTheme::ButtonPalette:
        return QPlatformTheme::ButtonPalette;

    case QPlatformTheme::CheckBoxPalette:
        return QPlatformTheme::CheckBoxPalette;

    case QPlatformTheme::RadioButtonPalette:
        return QPlatformTheme::RadioButtonPalette;

    case QPlatformTheme::ComboBoxPalette:
        return QPlatformTheme::ComboBoxPalette;

    case QPlatformTheme::TextEditPalette:
    case QPlatformTheme::TextLineEditPalette:
        return QPlatformTheme::TextLineEditPalette;

    case QPlatformTheme::ItemViewPalette:
        return QPlatformTheme::ItemViewPalette;

    default:
        return QPlatformTheme::SystemPalette;
    }
}

const QPalette *QAndroidPlatformTheme::palette(Palette type) const
{
    QHash<int, QPalette>::const_iterator it = m_androidPlatformNativeInterface->m_palettes.find(paletteType(type));
    if (it != m_androidPlatformNativeInterface->m_palettes.end())
        return &(it.value());
    return 0;
}

static inline int fontType(QPlatformTheme::Font type)
{
    switch (type) {
    case QPlatformTheme::LabelFont:
        return QPlatformTheme::SystemFont;

    case QPlatformTheme::ToolButtonFont:
        return QPlatformTheme::PushButtonFont;

    default:
        return type;
    }
}

const QFont *QAndroidPlatformTheme::font(Font type) const
{
    QHash<int, QFont>::const_iterator it = m_androidPlatformNativeInterface->m_fonts.find(fontType(type));
    if (it != m_androidPlatformNativeInterface->m_fonts.end())
        return &(it.value());

    // default in case the style has not set a font
    static QFont systemFont("Roboto", 14.0 * 100 / 72); // keep default size the same after changing from 100 dpi to 72 dpi
    if (type == QPlatformTheme::SystemFont)
        return &systemFont;
    return 0;
}

static const QLatin1String STYLES_PATH("/data/data/org.kde.necessitas.ministro/files/dl/style/");
static const QLatin1String STYLE_FILE("/style.json");

QVariant QAndroidPlatformTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case StyleNames:
        if (qgetenv("QT_USE_ANDROID_NATIVE_STYLE").toInt()
                && (!qgetenv("MINISTRO_ANDROID_STYLE_PATH").isEmpty()
                    || QFileInfo(STYLES_PATH
                                 + QLatin1String(qgetenv("QT_ANDROID_THEME_DISPLAY_DPI"))
                                 + STYLE_FILE).exists())) {
            return QStringList("android");
        }
        return QStringList("fusion");
        break;
    default:
        return QPlatformTheme::themeHint(hint);
    }
}

QString QAndroidPlatformTheme::standardButtonText(int button) const
{
    switch (button) {
    case QMessageDialogOptions::Yes:
        return QCoreApplication::translate("QAndroidPlatformTheme", "Yes");
    case QMessageDialogOptions::YesToAll:
        return QCoreApplication::translate("QAndroidPlatformTheme", "Yes to All");
    case QMessageDialogOptions::No:
        return QCoreApplication::translate("QAndroidPlatformTheme", "No");
    case QMessageDialogOptions::NoToAll:
        return QCoreApplication::translate("QAndroidPlatformTheme", "No to All");
    }
    return QPlatformTheme::standardButtonText(button);
}

bool QAndroidPlatformTheme::usePlatformNativeDialog(QPlatformTheme::DialogType type) const
{
    if (type == MessageDialog)
        return qgetenv("QT_USE_ANDROID_NATIVE_DIALOGS").toInt() == 1;
    return false;
}

QPlatformDialogHelper *QAndroidPlatformTheme::createPlatformDialogHelper(QPlatformTheme::DialogType type) const
{
    switch (type) {
    case MessageDialog:
        return new QtAndroidDialogHelpers::QAndroidPlatformMessageDialogHelper;
    default:
        return 0;
    }
}
