// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPLATFORMTHEME_H
#define QOHOSPLATFORMTHEME_H

#include <QtCore/qhash.h>
#include <QtCore/qmap.h>
#include <QtGui/qpalette.h>
#include <qohosplugincore.h>
#include <qpa/qplatformtheme.h>
#include <qpa/qplatformdialoghelper.h>


QT_BEGIN_NAMESPACE

template<typename T>
using QOhosSupplier = std::function<T()>;

template <typename T>
class QOhosOptional;

class QOhosPlatformTheme : public QPlatformTheme
{
public:

    static constexpr int defaultWheelScrollLines = 3;

    QOhosPlatformTheme();

    void requestColorScheme(Qt::ColorScheme scheme) override;
    Qt::ColorScheme colorScheme() const override;

    QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const override;
    bool usePlatformNativeDialog(DialogType type) const override;
    QVariant themeHint(ThemeHint hint) const override;

    const QPalette *palette(Palette type) const override;
    const QFont *font(Font type) const override;
    void setWheelScrollLines(int wheelScrollLines);

    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const override;

private:
    QMap<Qt::ColorScheme, QHash<Palette, QPalette>> m_themesPalettes;
    QHash<Font, QFont> m_fonts;
    int m_wheelScrollLines;
    QOhosSupplier<QOhosOptional<bool>> m_ohosConfigDarkModeFlagSupplier;
};

QT_END_NAMESPACE

#endif // QOHOSPLATFORMTHEME_H
