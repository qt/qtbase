/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qplatformtheme.h"

#include "qplatformtheme_p.h"

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/qfileinfo.h>
#include <qpalette.h>
#include <qtextformat.h>
#include <qiconloader_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformTheme
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa
    \brief The QPlatformTheme class allows customizing the UI based on themes.

*/

/*!
    \enum QPlatformTheme::ThemeHint

    This enum describes the available theme hints.

    \value CursorFlashTime (int) Cursor flash time in ms, overriding
                                 QPlatformIntegration::styleHint.

    \value KeyboardInputInterval (int) Keyboard input interval in ms, overriding
                                 QPlatformIntegration::styleHint.

    \value MouseDoubleClickInterval (int) Mouse double click interval in ms,
                                    overriding QPlatformIntegration::styleHint.

    \value StartDragDistance (int) Start drag distance,
                             overriding QPlatformIntegration::styleHint.

    \value StartDragTime (int) Start drag time in ms,
                               overriding QPlatformIntegration::styleHint.

    \value KeyboardAutoRepeatRate (int) Keyboard auto repeat rate,
                                  overriding QPlatformIntegration::styleHint.

    \value PasswordMaskDelay (int) Pass word mask delay in ms,
                                   overriding QPlatformIntegration::styleHint.

    \value StartDragVelocity (int) Velocity of a drag,
                                   overriding QPlatformIntegration::styleHint.

    \value TextCursorWidth  (int) Determines the width of the text cursor.

    \value DropShadow       (bool) Determines whether the drop shadow effect for
                            tooltips or whatsthis is enabled.

    \value MaximumScrollBarDragDistance (int) Determines the value returned by
                            QStyle::pixelMetric(PM_MaximumDragDistance)

    \value ToolButtonStyle (int) A value representing a Qt::ToolButtonStyle.

    \value ToolBarIconSize Icon size for tool bars.

    \value SystemIconThemeName (QString) Name of the icon theme.

    \value SystemIconFallbackThemeName (QString) Name of the fallback icon theme.

    \value IconThemeSearchPaths (QStringList) Search paths for icons.

    \value ItemViewActivateItemOnSingleClick (bool) Activate items by single click.

    \value StyleNames (QStringList) A list of preferred style names.

    \value WindowAutoPlacement (bool) A boolean value indicating whether Windows
                               (particularly dialogs) are placed by the system
                               (see _NET_WM_FULL_PLACEMENT in X11).

    \value DialogButtonBoxLayout (int) An integer representing a
                                 QDialogButtonBox::ButtonLayout value.

    \value DialogButtonBoxButtonsHaveIcons (bool) A boolean value indicating whether
                                            the buttons of a QDialogButtonBox should have icons.

    \value UseFullScreenForPopupMenu (bool) Pop menus can cover the full screen including task bar.

    \value KeyboardScheme (int) An integer value (enum KeyboardSchemes) specifying the
                           keyboard scheme.

    \value UiEffects (int) A flag value consisting of UiEffect values specifying the enabled UI animations.

    \value SpellCheckUnderlineStyle (int) A QTextCharFormat::UnderlineStyle specifying
                                    the underline style used misspelled words when spell checking.

    \value TabAllWidgets (bool) Whether tab navigation should go through all the widgets or components,
                         or just through text boxes and list views. This is mostly a Mac feature.

    \sa themeHint(), QStyle::pixelMetric()
*/

QPlatformThemePrivate::QPlatformThemePrivate()
        : systemPalette(0)
{ }

QPlatformThemePrivate::~QPlatformThemePrivate()
{
    delete systemPalette;
}

Q_GUI_EXPORT QPalette qt_fusionPalette();

void QPlatformThemePrivate::initializeSystemPalette()
{
    Q_ASSERT(!systemPalette);
    systemPalette = new QPalette(qt_fusionPalette());
}

QPlatformTheme::QPlatformTheme()
    : d_ptr(new QPlatformThemePrivate)
{

}

QPlatformTheme::QPlatformTheme(QPlatformThemePrivate *priv)
    : d_ptr(priv)
{ }

QPlatformTheme::~QPlatformTheme()
{

}

bool QPlatformTheme::usePlatformNativeDialog(DialogType type) const
{
    Q_UNUSED(type);
    return false;
}

QPlatformDialogHelper *QPlatformTheme::createPlatformDialogHelper(DialogType type) const
{
    Q_UNUSED(type);
    return 0;
}

const QPalette *QPlatformTheme::palette(Palette type) const
{
    Q_D(const QPlatformTheme);
    if (type == QPlatformTheme::SystemPalette) {
        if (!d->systemPalette)
            const_cast<QPlatformTheme *>(this)->d_ptr->initializeSystemPalette();
        return d->systemPalette;
    }
    return 0;
}

const QFont *QPlatformTheme::font(Font type) const
{
    Q_UNUSED(type)
    return 0;
}

QPixmap QPlatformTheme::standardPixmap(StandardPixmap sp, const QSizeF &size) const
{
    Q_UNUSED(sp);
    Q_UNUSED(size);
    // TODO Should return QCommonStyle pixmaps?
    return QPixmap();
}

QPixmap QPlatformTheme::fileIconPixmap(const QFileInfo &fileInfo, const QSizeF &size) const
{
    Q_UNUSED(fileInfo);
    Q_UNUSED(size);
    // TODO Should return QCommonStyle pixmaps?
    return QPixmap();
}

QVariant QPlatformTheme::themeHint(ThemeHint hint) const
{
    return QPlatformTheme::defaultThemeHint(hint);
}

QVariant QPlatformTheme::defaultThemeHint(ThemeHint hint)
{
    switch (hint) {
    case QPlatformTheme::CursorFlashTime:
        return QVariant(1000);
    case QPlatformTheme::KeyboardInputInterval:
        return QVariant(400);
    case QPlatformTheme::KeyboardAutoRepeatRate:
        return QVariant(30);
    case QPlatformTheme::MouseDoubleClickInterval:
        return QVariant(400);
    case QPlatformTheme::StartDragDistance:
        return QVariant(10);
    case QPlatformTheme::StartDragTime:
        return QVariant(500);
    case QPlatformTheme::PasswordMaskDelay:
        return QVariant(int(0));
    case QPlatformTheme::PasswordMaskCharacter:
        return QVariant(QChar(0x25CF));
    case QPlatformTheme::StartDragVelocity:
        return QVariant(int(0)); // no limit
    case QPlatformTheme::UseFullScreenForPopupMenu:
        return QVariant(false);
    case QPlatformTheme::WindowAutoPlacement:
        return QVariant(false);
    case QPlatformTheme::DialogButtonBoxLayout:
        return QVariant(int(0));
    case QPlatformTheme::DialogButtonBoxButtonsHaveIcons:
        return QVariant(false);
    case QPlatformTheme::ItemViewActivateItemOnSingleClick:
        return QVariant(false);
    case QPlatformTheme::ToolButtonStyle:
        return QVariant(int(Qt::ToolButtonIconOnly));
    case QPlatformTheme::ToolBarIconSize:
        return QVariant(int(0));
    case QPlatformTheme::SystemIconThemeName:
    case QPlatformTheme::SystemIconFallbackThemeName:
        return QVariant(QString());
    case QPlatformTheme::IconThemeSearchPaths:
        return QVariant(QStringList());
    case QPlatformTheme::StyleNames:
        return QVariant(QStringList());
    case TextCursorWidth:
        return QVariant(1);
    case DropShadow:
        return QVariant(false);
    case MaximumScrollBarDragDistance:
        return QVariant(-1);
    case KeyboardScheme:
        return QVariant(int(WindowsKeyboardScheme));
    case UiEffects:
        return QVariant(int(0));
    case SpellCheckUnderlineStyle:
        return QVariant(int(QTextCharFormat::SpellCheckUnderline));
    case TabAllWidgets:
        return QVariant(true);
    case IconPixmapSizes:
        return QVariant::fromValue(QList<int>());
    }
    return QVariant();
}

QPlatformMenuItem *QPlatformTheme::createPlatformMenuItem() const
{
    return 0;
}

QPlatformMenu *QPlatformTheme::createPlatformMenu() const
{
    return 0;
}

QPlatformMenuBar *QPlatformTheme::createPlatformMenuBar() const
{
    return 0;
}

#ifndef QT_NO_SYSTEMTRAYICON
/*!
   Factory function for QSystemTrayIcon. This function will return 0 if the platform
   integration does not support creating any system tray icon.
*/
QPlatformSystemTrayIcon *QPlatformTheme::createPlatformSystemTrayIcon() const
{
    return 0;
}
#endif

/*!
   Factory function for the QIconEngine used by QIcon::fromTheme(). By default this
   function returns a QIconLoaderEngine, but subclasses can reimplement it to
   provide their own.

   It is especially useful to benefit from some platform specific facilities or
   optimizations like an inter-process cache in systems mostly built with Qt.

   \since 5.1
*/
QIconEngine *QPlatformTheme::createIconEngine(const QString &iconName) const
{
    return new QIconLoaderEngine(iconName);
}

QT_END_NAMESPACE
