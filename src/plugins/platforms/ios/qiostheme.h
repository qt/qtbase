// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIOSTHEME_H
#define QIOSTHEME_H

#include <QtCore/QHash>
#include <QtGui/QPalette>
#include <qpa/qplatformtheme.h>

#include <QtCore/private/qcore_mac_p.h>

QT_BEGIN_NAMESPACE

class QIOSTheme : public QPlatformTheme
{
public:
    QIOSTheme();
    ~QIOSTheme();

    const QPalette *palette(Palette type = SystemPalette) const override;
    QVariant themeHint(ThemeHint hint) const override;

    Qt::ColorScheme colorScheme() const override;

    QPlatformMenuItem* createPlatformMenuItem() const override;
    QPlatformMenu* createPlatformMenu() const override;

    bool usePlatformNativeDialog(DialogType type) const override;
    QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const override;

    const QFont *font(Font type = SystemFont) const override;

    static const char *name;

    static void initializeSystemPalette();

private:
    static QPalette s_systemPalette;
    QMacNotificationObserver m_contentSizeCategoryObserver;
};

QT_END_NAMESPACE

#endif
