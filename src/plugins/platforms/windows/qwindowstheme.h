// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSTHEME_H
#define QWINDOWSTHEME_H

#include <qpa/qplatformtheme.h>

#include <QtCore/qsharedpointer.h>
#include <QtCore/qvariant.h>
#include <QtCore/qlist.h>
#include <QtCore/qsize.h>

QT_BEGIN_NAMESPACE

class QWindow;

class QWindowsTheme : public QPlatformTheme
{
    Q_DISABLE_COPY_MOVE(QWindowsTheme)
public:
    QWindowsTheme();
    ~QWindowsTheme() override;

    static QWindowsTheme *instance() { return m_instance; }

    bool usePlatformNativeDialog(DialogType type) const override;
    QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const override;
#if QT_CONFIG(systemtrayicon)
    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const override;
#endif
    QVariant themeHint(ThemeHint) const override;

    Qt::ColorScheme colorScheme() const override;

    const QPalette *palette(Palette type = SystemPalette) const override
        { return m_palettes[type]; }
    const QFont *font(Font type = SystemFont) const override
        { return m_fonts[type]; }

    QPixmap standardPixmap(StandardPixmap sp, const QSizeF &size) const override;

    QIcon fileIcon(const QFileInfo &fileInfo, QPlatformTheme::IconOptions iconOptions = {}) const override;

    void windowsThemeChanged(QWindow *window);
    void displayChanged() { refreshIconPixmapSizes(); }

    QList<QSize> availableFileIconSizes() const { return m_fileIconSizes; }

    QPlatformMenuItem *createPlatformMenuItem() const override;
    QPlatformMenu *createPlatformMenu() const override;
    QPlatformMenuBar *createPlatformMenuBar() const override;
    void showPlatformMenuBar() override;

    static bool useNativeMenus();
    static bool queryDarkMode();
    static bool queryHighContrast();

    void refreshFonts();
    void refresh();

    static const char *name;

    static QPalette systemPalette(Qt::ColorScheme);

private:
    void clearPalettes();
    void refreshPalettes();
    void clearFonts();
    void refreshIconPixmapSizes();

    static QWindowsTheme *m_instance;
    QPalette *m_palettes[NPalettes];
    QFont *m_fonts[NFonts];
    QList<QSize> m_fileIconSizes;
};

QT_END_NAMESPACE

#endif // QWINDOWSTHEME_H
