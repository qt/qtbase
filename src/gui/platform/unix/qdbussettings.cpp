// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qdbussettings_p.h"
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

namespace QDBusSettings::XdgSettings {
// https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Settings.html
enum class ColorScheme : uint { NoPreference, PreferDark, PreferLight };
} // namespace QDBusSettings::XdgSettings

Qt::ContrastPreference QDBusSettings::XdgSettings::convertContrastPreference(const QVariant &value)
{
    // XDG portal provides the contrast preference value as uint:
    // 0 for no-preference, and, 1 for high-contrast.
    if (!value.isValid())
        return Qt::ContrastPreference::NoPreference;
    return static_cast<Qt::ContrastPreference>(value.toUInt());
}

Qt::ColorScheme QDBusSettings::XdgSettings::convertColorScheme(const QVariant &value)
{
    switch (ColorScheme{ value.toUInt() }) {
    case ColorScheme::NoPreference:
        return Qt::ColorScheme::Unknown;
    case ColorScheme::PreferDark:
        return Qt::ColorScheme::Dark;
    case ColorScheme::PreferLight:
        return Qt::ColorScheme::Light;
    }
    Q_UNREACHABLE_RETURN(Qt::ColorScheme::Unknown);
}

Qt::ContrastPreference
QDBusSettings::GnomeSettings::convertContrastPreference(const QVariant &value)
{
    // GSetting provides the contrast value as boolean:
    // true for enabled high-contrast, and, false for disabled high-contrast.
    if (!value.isValid())
        return Qt::ContrastPreference::NoPreference;
    return value.toBool() ? Qt::ContrastPreference::HighContrast
                          : Qt::ContrastPreference::NoPreference;
}

QT_END_NAMESPACE
