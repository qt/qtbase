// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmtheme.h"
#include "qwasmfiledialoghelper.h"
#include <QtCore/qvariant.h>
#include <QFontDatabase>
#include <QList>
#include <qloggingcategory.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpalette.h>

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

Q_GUI_EXPORT QPalette qt_fusionPalette();

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcQpaThemeWasm, "qt.qpa.theme.wasm")

namespace {
bool matchMedia(std::string mediaQueryString)
{
    return emscripten::val::global("window")
            .call<emscripten::val>("matchMedia", mediaQueryString)["matches"]
            .as<bool>();
}

Qt::ColorScheme getColorSchemeFromMedia()
{
    if (matchMedia(colorSchemePreferenceDark)) {
        return Qt::ColorScheme::Dark;
    } else {
        return Qt::ColorScheme::Light;
    }
}

Qt::ContrastPreference getContrastPreferenceFromMedia()
{
    if (matchMedia(contrastPreferenceMore)) {
        return Qt::ContrastPreference::HighContrast;
    } else {
        return Qt::ContrastPreference::NoPreference;
    }
}
} // namespace

using namespace Qt::StringLiterals;

QWasmTheme::QWasmTheme()
{
    m_colorScheme = getColorSchemeFromMedia();
    qCDebug(lcQpaThemeWasm) << "Initializing Wasm theme. Color scheme: " << m_colorScheme;
    m_contrastPreference = getContrastPreferenceFromMedia();
    qCDebug(lcQpaThemeWasm) << "Initializing Wasm theme. Contrast preference: " << m_contrastPreference;

    for (auto family : QFontDatabase::families())
        if (QFontDatabase::isFixedPitch(family))
            fixedFont = new QFont(family);

    m_palette = std::make_unique<QPalette>();
    m_paletteIsDirty = true; // Force update later

    registerCallbacks(
            { colorSchemePreferenceDark },
            [this](emscripten::val) { QWasmTheme::onColorSchemeChange(); },
            m_colorSchemeChangeCallback);
    registerCallbacks(
            { contrastPreferenceNoPreference, contrastPreferenceMore, contrastPreferenceLess,
              contrastPreferenceCustom },
            [this](emscripten::val) { QWasmTheme::onContrastPreferenceChange(); },
            m_contrastPreferenceChangeCallbacks);
}

QWasmTheme::~QWasmTheme()
{
    if (fixedFont)
        delete fixedFont;
}

const QPalette *QWasmTheme::palette(Palette type) const
{
    if (type == SystemPalette) {
        if (m_paletteIsDirty) {
            m_paletteIsDirty = false;
            *m_palette = qt_fusionPalette();
        }
        return m_palette.get();
    }
    return nullptr;
}

Qt::ColorScheme QWasmTheme::colorScheme() const
{
    return m_colorScheme;
}

void QWasmTheme::requestColorScheme(Qt::ColorScheme scheme)
{
    if (m_colorScheme != scheme) {
        m_paletteIsDirty = true;
        m_colorScheme = scheme;
        QWindowSystemInterface::handleThemeChange<QWindowSystemInterface::SynchronousDelivery>();
    }
}

Qt::ContrastPreference QWasmTheme::contrastPreference() const
{
    return m_contrastPreference;
}

QVariant QWasmTheme::themeHint(ThemeHint hint) const
{
    if (hint == QPlatformTheme::StyleNames)
        return QVariant(QStringList() << "Fusion"_L1);
    if (hint == QPlatformTheme::UiEffects)
        return QVariant(int(HoverEffect));
    return QPlatformTheme::themeHint(hint);
}

const QFont *QWasmTheme::font(Font type) const
{
    if (type == QPlatformTheme::FixedFont) {
        return fixedFont;
    }
    return nullptr;
}

bool QWasmTheme::usePlatformNativeDialog(DialogType type) const
{
    return (type == DialogType::FileDialog);
}

QPlatformDialogHelper *QWasmTheme::createPlatformDialogHelper(DialogType type) const
{
    if (type == DialogType::FileDialog)
        return new QWasmFileDialogHelper();
    return nullptr;
}

void QWasmTheme::onColorSchemeChange()
{
    auto colorScheme = getColorSchemeFromMedia();
    if (m_colorScheme != colorScheme) {
        qCDebug(lcQpaThemeWasm) << "Color scheme changed to " << colorScheme;
        m_colorScheme = colorScheme;
        m_paletteIsDirty = true;
        QWindowSystemInterface::handleThemeChange<QWindowSystemInterface::SynchronousDelivery>();
    }
}

void QWasmTheme::onContrastPreferenceChange()
{
    auto contrastPreference = getContrastPreferenceFromMedia();
    if (m_contrastPreference != contrastPreference) {
        qCDebug(lcQpaThemeWasm) << "Contrast preference changed to " << contrastPreference;
        m_contrastPreference = contrastPreference;
        m_paletteIsDirty = true;
        QWindowSystemInterface::handleThemeChange<QWindowSystemInterface::SynchronousDelivery>();
    }
}


QT_END_NAMESPACE
