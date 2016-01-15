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

#ifndef QPLATFORMTHEME_COCOA_H
#define QPLATFORMTHEME_COCOA_H

#include <QtCore/QHash>
#include <qpa/qplatformtheme.h>

Q_FORWARD_DECLARE_OBJC_CLASS(QT_MANGLE_NAMESPACE(QCocoaThemeNotificationReceiver));

QT_BEGIN_NAMESPACE

class QPalette;
class QCocoaTheme : public QPlatformTheme
{
public:
    QCocoaTheme();
    ~QCocoaTheme();

    void reset();

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
    mutable QT_MANGLE_NAMESPACE(QCocoaThemeNotificationReceiver) *m_notificationReceiver;
};

QT_END_NAMESPACE

#endif
