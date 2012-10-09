/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#import <Cocoa/Cocoa.h>

#include "qcocoatheme.h"

#include <QtCore/QVariant>

#include "qcocoacolordialoghelper.h"
#include "qcocoafiledialoghelper.h"
#include "qcocoafontdialoghelper.h"
#include "qcocoasystemsettings.h"
#include "qcocoasystemtrayicon.h"
#include "qcocoamenuitem.h"
#include "qcocoamenu.h"
#include "qcocoamenubar.h"

#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

const char *QCocoaTheme::name = "cocoa";

QCocoaTheme::QCocoaTheme()
    :m_systemPalette(0)
{

}

QCocoaTheme::~QCocoaTheme()
{
    delete m_systemPalette;
}

bool QCocoaTheme::usePlatformNativeDialog(DialogType dialogType) const
{
    if (dialogType == QPlatformTheme::FileDialog)
        return true;
#ifndef QT_NO_COLORDIALOG
    if (dialogType == QPlatformTheme::ColorDialog)
        return true;
#endif
#ifndef QT_NO_FONTDIALOG
    if (dialogType == QPlatformTheme::FontDialog)
        return true;
#endif
    return false;
}

QPlatformDialogHelper * QCocoaTheme::createPlatformDialogHelper(DialogType dialogType) const
{
    switch (dialogType) {
    case QPlatformTheme::FileDialog:
        return new QCocoaFileDialogHelper();
#ifndef QT_NO_COLORDIALOG
    case QPlatformTheme::ColorDialog:
        return new QCocoaColorDialogHelper();
#endif
#ifndef QT_NO_FONTDIALOG
    case QPlatformTheme::FontDialog:
        return new QCocoaFontDialogHelper();
#endif
    default:
        return 0;
    }
}

#ifndef QT_NO_SYSTEMTRAYICON
QPlatformSystemTrayIcon *QCocoaTheme::createPlatformSystemTrayIcon() const
{
    return new QCocoaSystemTrayIcon;
}
#endif

const QPalette *QCocoaTheme::palette(Palette type) const
{
    if (type == SystemPalette) {
        if (!m_systemPalette)
            m_systemPalette = qt_mac_createSystemPalette();
        return m_systemPalette;
    } else {
        if (m_palettes.isEmpty())
            m_palettes = qt_mac_createRolePalettes();
        return m_palettes.value(type, 0);
    }
    return 0;
}

const QFont *QCocoaTheme::font(Font type) const
{
    if (m_fonts.isEmpty()) {
        m_fonts = qt_mac_createRoleFonts();
    }
    return m_fonts.value(type, 0);
}

QVariant QCocoaTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case QPlatformTheme::StyleNames:
        return QStringList(QStringLiteral("macintosh"));
    case QPlatformTheme::DialogButtonBoxLayout:
        return QVariant(1); // QDialogButtonBox::MacLayout
    case KeyboardScheme:
        return QVariant(int(MacKeyboardScheme));
    case TabAllWidgets:
        return QVariant(bool([[NSApplication sharedApplication] isFullKeyboardAccessEnabled]));
    default:
        break;
    }
    return QPlatformTheme::themeHint(hint);
}

QPlatformMenuItem *QCocoaTheme::createPlatformMenuItem() const
{
    return new QCocoaMenuItem();
}

QPlatformMenu *QCocoaTheme::createPlatformMenu() const
{
    return new QCocoaMenu();
}

QPlatformMenuBar *QCocoaTheme::createPlatformMenuBar() const
{
    static bool haveMenubar = false;
    if (!haveMenubar) {
        haveMenubar = true;
        QObject::connect(qGuiApp, SIGNAL(focusWindowChanged(QWindow*)),
            QGuiApplicationPrivate::platformIntegration()->nativeInterface(),
                SLOT(onAppFocusWindowChanged(QWindow*)));
    }

    return new QCocoaMenuBar();
}

QT_END_NAMESPACE
