// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMTHEME_H
#define QWASMTHEME_H

#include <qpa/qplatformtheme.h>
#include <QtGui/QFont>

#include <memory>

#include <private/qstdweb_p.h>

QT_BEGIN_NAMESPACE

class QWasmEventTranslator;
class QWasmFontDatabase;
class QWasmWindow;
class QWasmEventDispatcher;
class QWasmScreen;
class QWasmCompositor;
class QWasmBackingStore;

// this reflects @media/prefers-contrast
constexpr auto colorSchemePreferenceDark =  "(prefers-color-scheme: dark)";
constexpr auto contrastPreferenceNoPreference =  "(prefers-contrast: no-preference)";
constexpr auto contrastPreferenceMore =  "(prefers-contrast: more)";
constexpr auto contrastPreferenceLess =  "(prefers-contrast: less)";
constexpr auto contrastPreferenceCustom =  "(prefers-contrast: custom)";

template <typename MediaName, typename CallbackFn, typename Container>
void registerCallbacks(std::initializer_list<MediaName> mediaNames, CallbackFn callback, Container &callbacksContainer)
{
    emscripten::val window = emscripten::val::global("window");
    if (!window.isUndefined()) {
        for (auto &&mediaName : mediaNames) {
            auto media = window.call<emscripten::val>("matchMedia", emscripten::val(mediaName));
            if constexpr (std::is_same_v<Container, std::vector<QWasmEventHandler>>) {
                callbacksContainer.emplace_back(media, "change", callback);
            } else {
                Q_ASSERT(mediaNames.size() == 1);
                callbacksContainer = QWasmEventHandler(media, "change", callback);
            }
        }
    }
}

class QWasmTheme : public QPlatformTheme
{
public:
    QWasmTheme();
    ~QWasmTheme();

    const QPalette *palette(Palette type = SystemPalette) const override;
    Qt::ColorScheme colorScheme() const override;
    void requestColorScheme(Qt::ColorScheme scheme) override;
    Qt::ContrastPreference contrastPreference() const override;
    QVariant themeHint(ThemeHint hint) const override;
    const QFont *font(Font type) const override;
    bool usePlatformNativeDialog(DialogType type) const override;
    QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const override;

    QFont *fixedFont = nullptr;

    void onColorSchemeChange();
    void onContrastPreferenceChange();

private:
    Qt::ColorScheme m_colorScheme = Qt::ColorScheme::Unknown;
    QWasmEventHandler m_colorSchemeChangeCallback;
    std::unique_ptr<QPalette> m_palette;
    mutable bool m_paletteIsDirty = false;
    Qt::ContrastPreference m_contrastPreference;
    std::vector<QWasmEventHandler> m_contrastPreferenceChangeCallbacks;
};

QT_END_NAMESPACE

#endif // QWASMTHEME_H
