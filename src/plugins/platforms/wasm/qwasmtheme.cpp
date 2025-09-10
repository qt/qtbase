// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmtheme.h"
#include <QtCore/qvariant.h>
#include <QFontDatabase>
#include <QList>
#include <qpa/qwindowsysteminterface.h>

#include <private/qstdweb_p.h>

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

Q_GUI_EXPORT QPalette qt_fusionPalette();

Qt::ColorScheme QWasmTheme::s_autoColorScheme = Qt::ColorScheme::Unknown;
bool QWasmTheme::s_autoPaletteIsDirty = false;

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QWasmTheme::QWasmTheme()
{
    if (emscripten::val::global("window").call<emscripten::val>(
        "matchMedia",
        std::string("(prefers-color-scheme:dark)"))["matches"].as<bool>())
        s_autoColorScheme = Qt::ColorScheme::Dark;
    else
        s_autoColorScheme = Qt::ColorScheme::Light;

    for (auto family : QFontDatabase::families())
        if (QFontDatabase::isFixedPitch(family))
            fixedFont = new QFont(family);

    m_palette = std::make_unique<QPalette>();
    m_paletteIsDirty = true; // Force update later

    const auto callback = [=](emscripten::val event) { QWasmTheme::onColorSchemeChange(event); };
    const emscripten::val window = emscripten::val::global("window");
    if (!window.isUndefined()) {
        const emscripten::val matchMedia = window.call<emscripten::val>("matchMedia", emscripten::val("(prefers-color-scheme: dark)"));
        if (!matchMedia.isUndefined()) {
            static auto changeEvent =
                std::make_unique<qstdweb::EventCallback>(matchMedia, "change", callback);
        }
    }
}

QWasmTheme::~QWasmTheme()
{
    if (fixedFont)
        delete fixedFont;
}

const QPalette *QWasmTheme::palette(Palette type) const
{
    if (type == SystemPalette) {
        if (m_paletteIsDirty || s_autoPaletteIsDirty) {
            m_paletteIsDirty = false;
            s_autoPaletteIsDirty = false;
            *m_palette = qt_fusionPalette();
        }
        return m_palette.get();
    }
    return nullptr;
}

Qt::ColorScheme QWasmTheme::colorScheme() const
{
    if (m_colorScheme != Qt::ColorScheme::Unknown)
        return m_colorScheme;
    return s_autoColorScheme;
}

void QWasmTheme::requestColorScheme(Qt::ColorScheme scheme)
{
    if (m_colorScheme != scheme) {
        m_paletteIsDirty = true;
        m_colorScheme = scheme;
        QWindowSystemInterface::handleThemeChange<QWindowSystemInterface::SynchronousDelivery>();
    }
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

void QWasmTheme::onColorSchemeChange(emscripten::val event)
{
    const emscripten::val matches = event["matches"];
    if (!matches.isUndefined()) {
        const auto oldAutoColorScheme = s_autoColorScheme;
        if (matches.as<int>())
            s_autoColorScheme = Qt::ColorScheme::Dark;
        else
            s_autoColorScheme = Qt::ColorScheme::Light;

        if (oldAutoColorScheme != s_autoColorScheme) {
            s_autoPaletteIsDirty = true;
            QWindowSystemInterface::handleThemeChange<QWindowSystemInterface::SynchronousDelivery>();
        }
    }
}


QT_END_NAMESPACE
