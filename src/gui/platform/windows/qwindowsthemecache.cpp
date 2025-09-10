// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsthemecache_p.h"
#include <QtCore/qdebug.h>
#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE

// Theme names matching the QWindowsVistaStylePrivate::Theme enumeration.
constexpr const wchar_t *themeNames[] = {
    L"BUTTON",   L"COMBOBOX",   L"EDIT",    L"HEADER",    L"LISTVIEW",
    L"MENU",     L"PROGRESS",   L"REBAR",   L"SCROLLBAR", L"SPIN",
    L"TAB",      L"TASKDIALOG", L"TOOLBAR", L"TOOLTIP",   L"TRACKBAR",
    L"WINDOW",   L"STATUS",     L"TREEVIEW"
};

typedef std::array<HTHEME, std::size(themeNames)> ThemeArray;
typedef QHash<HWND, ThemeArray> ThemesCache;
Q_GLOBAL_STATIC(ThemesCache, themesCache);

QString QWindowsThemeCache::themeName(int theme)
{
    return theme >= 0 && theme < int(std::size(themeNames))
        ? QString::fromWCharArray(themeNames[theme]) : QString();
}

HTHEME QWindowsThemeCache::createTheme(int theme, HWND hwnd)
{
    if (Q_UNLIKELY(theme < 0 || theme >= int(std::size(themeNames)) || !hwnd)) {
        qWarning("Invalid parameters #%d, %p", theme, hwnd);
        return nullptr;
    }

    // Get or create themes array for this window.
    ThemesCache *cache = themesCache();
    auto it = cache->find(hwnd);
    if (it == cache->end())
        it = cache->insert(hwnd, ThemeArray {});

    // Get or create theme data
    ThemeArray &themes = *it;
    if (!themes[theme]) {
        const wchar_t *name = themeNames[theme];
        themes[theme] = OpenThemeData(hwnd, name);
        if (Q_UNLIKELY(!themes[theme]))
            qErrnoWarning("OpenThemeData() failed for theme %d (%s).",
                          theme, qPrintable(themeName(theme)));
    }
    return themes[theme];
}

static void clearThemes(ThemeArray &themes)
{
    for (auto &theme : themes) {
        if (theme) {
            CloseThemeData(theme);
            theme = nullptr;
        }
    }
}

void QWindowsThemeCache::clearThemeCache(HWND hwnd)
{
    ThemesCache *cache = themesCache();
    auto it = cache->find(hwnd);
    if (it == cache->end())
        return;
    clearThemes(*it);
}

void QWindowsThemeCache::clearAllThemeCaches()
{
    ThemesCache *cache = themesCache();
    for (auto &themeArray : *cache)
        clearThemes(themeArray);
}

QT_END_NAMESPACE
