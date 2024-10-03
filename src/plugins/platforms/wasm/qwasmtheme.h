// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMTHEME_H
#define QWASMTHEME_H

#include <qpa/qplatformtheme.h>
#include <QtGui/QFont>

QT_BEGIN_NAMESPACE

class QWasmEventTranslator;
class QWasmFontDatabase;
class QWasmWindow;
class QWasmEventDispatcher;
class QWasmScreen;
class QWasmCompositor;
class QWasmBackingStore;

class QWasmTheme : public QPlatformTheme
{
public:
    QWasmTheme();
    ~QWasmTheme();

    const QPalette *palette(Palette type = SystemPalette) const override;
    Qt::ColorScheme colorScheme() const override;
    void requestColorScheme(Qt::ColorScheme scheme) override;
    QVariant themeHint(ThemeHint hint) const override;
    const QFont *font(Font type) const override;
    QFont *fixedFont = nullptr;

    static void onColorSchemeChange(emscripten::val event);

private:
    Qt::ColorScheme m_colorScheme = Qt::ColorScheme::Unknown;
    std::unique_ptr<QPalette> m_palette;
    mutable bool m_paletteIsDirty = false;

    static Qt::ColorScheme s_autoColorScheme;
    static bool s_autoPaletteIsDirty;
};

QT_END_NAMESPACE

#endif // QWASMTHEME_H
