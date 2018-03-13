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

#include "qflatpaktheme.h"
#include "qflatpakfiledialog_p.h"

#include <private/qguiapplication_p.h>
#include <qpa/qplatformtheme_p.h>
#include <qpa/qplatformthemefactory_p.h>
#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

class QFlatpakThemePrivate : public QPlatformThemePrivate
{
public:
    QFlatpakThemePrivate()
        : QPlatformThemePrivate()
    { }

    ~QFlatpakThemePrivate()
    {
        delete baseTheme;
    }

    QPlatformTheme *baseTheme;
};

QFlatpakTheme::QFlatpakTheme()
    : d_ptr(new QFlatpakThemePrivate)
{
    Q_D(QFlatpakTheme);

    QStringList themeNames;
    themeNames += QGuiApplicationPrivate::platform_integration->themeNames();
    // 1) Look for a theme plugin.
    for (const QString &themeName : qAsConst(themeNames)) {
        d->baseTheme = QPlatformThemeFactory::create(themeName, nullptr);
        if (d->baseTheme)
            break;
    }

    // 2) If no theme plugin was found ask the platform integration to
    // create a theme
    if (!d->baseTheme) {
        for (const QString &themeName : qAsConst(themeNames)) {
            d->baseTheme = QGuiApplicationPrivate::platform_integration->createPlatformTheme(themeName);
            if (d->baseTheme)
                break;
        }
        // No error message; not having a theme plugin is allowed.
    }

    // 3) Fall back on the built-in "null" platform theme.
    if (!d->baseTheme)
        d->baseTheme = new QPlatformTheme;
}

QPlatformMenuItem* QFlatpakTheme::createPlatformMenuItem() const
{
    Q_D(const QFlatpakTheme);
    return d->baseTheme->createPlatformMenuItem();
}

QPlatformMenu* QFlatpakTheme::createPlatformMenu() const
{
    Q_D(const QFlatpakTheme);
    return d->baseTheme->createPlatformMenu();
}

QPlatformMenuBar* QFlatpakTheme::createPlatformMenuBar() const
{
    Q_D(const QFlatpakTheme);
    return d->baseTheme->createPlatformMenuBar();
}

void QFlatpakTheme::showPlatformMenuBar()
{
    Q_D(const QFlatpakTheme);
    return d->baseTheme->showPlatformMenuBar();
}

bool QFlatpakTheme::usePlatformNativeDialog(DialogType type) const
{
    Q_D(const QFlatpakTheme);

    if (type == FileDialog)
        return true;

    return d->baseTheme->usePlatformNativeDialog(type);
}

QPlatformDialogHelper* QFlatpakTheme::createPlatformDialogHelper(DialogType type) const
{
    Q_D(const QFlatpakTheme);

    if (type == FileDialog)
        return new QFlatpakFileDialog;

    return d->baseTheme->createPlatformDialogHelper(type);
}

#ifndef QT_NO_SYSTEMTRAYICON
QPlatformSystemTrayIcon* QFlatpakTheme::createPlatformSystemTrayIcon() const
{
    Q_D(const QFlatpakTheme);
    return d->baseTheme->createPlatformSystemTrayIcon();
}
#endif

const QPalette *QFlatpakTheme::palette(Palette type) const
{
    Q_D(const QFlatpakTheme);
    return d->baseTheme->palette(type);
}

const QFont* QFlatpakTheme::font(Font type) const
{
    Q_D(const QFlatpakTheme);
    return d->baseTheme->font(type);
}

QVariant QFlatpakTheme::themeHint(ThemeHint hint) const
{
    Q_D(const QFlatpakTheme);
    return d->baseTheme->themeHint(hint);
}

QPixmap QFlatpakTheme::standardPixmap(StandardPixmap sp, const QSizeF &size) const
{
    Q_D(const QFlatpakTheme);
    return d->baseTheme->standardPixmap(sp, size);
}

QIcon QFlatpakTheme::fileIcon(const QFileInfo &fileInfo,
                              QPlatformTheme::IconOptions iconOptions) const
{
    Q_D(const QFlatpakTheme);
    return d->baseTheme->fileIcon(fileInfo, iconOptions);
}

QIconEngine * QFlatpakTheme::createIconEngine(const QString &iconName) const
{
    Q_D(const QFlatpakTheme);
    return d->baseTheme->createIconEngine(iconName);
}

QList<QKeySequence> QFlatpakTheme::keyBindings(QKeySequence::StandardKey key) const
{
    Q_D(const QFlatpakTheme);
    return d->baseTheme->keyBindings(key);
}

QString QFlatpakTheme::standardButtonText(int button) const
{
    Q_D(const QFlatpakTheme);
    return d->baseTheme->standardButtonText(button);
}

QT_END_NAMESPACE
