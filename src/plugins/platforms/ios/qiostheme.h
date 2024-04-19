// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIOSTHEME_H
#define QIOSTHEME_H

#import <UIKit/UIKit.h>

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
    void requestColorScheme(Qt::ColorScheme scheme) override;

#if !defined(Q_OS_TVOS) && !defined(Q_OS_VISIONOS)
    QPlatformMenuItem* createPlatformMenuItem() const override;
    QPlatformMenu* createPlatformMenu() const override;
#endif

    bool usePlatformNativeDialog(DialogType type) const override;
    QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const override;

    const QFont *font(Font type = SystemFont) const override;
    QIconEngine *createIconEngine(const QString &iconName) const override;

    static const char *name;

    static void initializeSystemPalette();
    static void applyTheme(UIWindow *window);

private:
    static QPalette s_systemPalette;
    static inline Qt::ColorScheme s_colorSchemeOverride = Qt::ColorScheme::Unknown;
    QMacNotificationObserver m_contentSizeCategoryObserver;
};

QT_END_NAMESPACE

#endif
