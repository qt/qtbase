/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPLATFORMTHEME_COCOA_H
#define QPLATFORMTHEME_COCOA_H

#include <QtCore/QHash>
#include <qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

class QPalette;
class QCocoaTheme : public QPlatformTheme
{
public:
    QCocoaTheme();
    ~QCocoaTheme();

    QPlatformMenuItem* createPlatformMenuItem() const Q_DECL_OVERRIDE;
    QPlatformMenu* createPlatformMenu() const Q_DECL_OVERRIDE;
    QPlatformMenuBar* createPlatformMenuBar() const Q_DECL_OVERRIDE;

#ifndef QT_NO_SYSTEMTRAYICON
    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const Q_DECL_OVERRIDE;
#endif

    bool usePlatformNativeDialog(DialogType dialogType) const Q_DECL_OVERRIDE;
    QPlatformDialogHelper *createPlatformDialogHelper(DialogType dialogType) const Q_DECL_OVERRIDE;

    const QPalette *palette(Palette type = SystemPalette) const Q_DECL_OVERRIDE;
    const QFont *font(Font type = SystemFont) const Q_DECL_OVERRIDE;
    QPixmap standardPixmap(StandardPixmap sp, const QSizeF &size) const Q_DECL_OVERRIDE;
    QPixmap fileIconPixmap(const QFileInfo &fileInfo,
                           const QSizeF &size,
                           QPlatformTheme::IconOptions options = 0) const Q_DECL_OVERRIDE;

    QVariant themeHint(ThemeHint hint) const Q_DECL_OVERRIDE;
    QString standardButtonText(int button) const Q_DECL_OVERRIDE;

    static const char *name;

private:
    mutable QPalette *m_systemPalette;
    mutable QHash<QPlatformTheme::Palette, QPalette*> m_palettes;
    mutable QHash<QPlatformTheme::Font, QFont*> m_fonts;
};

QT_END_NAMESPACE

#endif
