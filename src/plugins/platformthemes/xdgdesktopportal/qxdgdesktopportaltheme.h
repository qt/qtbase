// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QXDGDESKTOPPORTALTHEME_H
#define QXDGDESKTOPPORTALTHEME_H

#include <qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

class QXdgDesktopPortalThemePrivate;

class QXdgDesktopPortalTheme : public QPlatformTheme
{
    Q_DECLARE_PRIVATE(QXdgDesktopPortalTheme)
public:
    QXdgDesktopPortalTheme();

    QPlatformMenuItem *createPlatformMenuItem() const override;
    QPlatformMenu *createPlatformMenu() const override;
    QPlatformMenuBar *createPlatformMenuBar() const override;
    void showPlatformMenuBar() override;

    bool usePlatformNativeDialog(DialogType type) const override;
    QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const override;

#ifndef QT_NO_SYSTEMTRAYICON
    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const override;
#endif

    const QPalette *palette(Palette type = SystemPalette) const override;

    const QFont *font(Font type = SystemFont) const override;

    QVariant themeHint(ThemeHint hint) const override;

    Qt::ColorScheme colorScheme() const override;
    Qt::ContrastPreference contrastPreference() const override;

    QPixmap standardPixmap(StandardPixmap sp, const QSizeF &size) const override;
    QIcon fileIcon(const QFileInfo &fileInfo,
                   QPlatformTheme::IconOptions iconOptions = { }) const override;

    QIconEngine *createIconEngine(const QString &iconName) const override;

#if QT_CONFIG(shortcut)
    QList<QKeySequence> keyBindings(QKeySequence::StandardKey key) const override;
#endif

    QString standardButtonText(int button) const override;

    static bool isXdgPlugin(const QString &key);

private:
    QScopedPointer<QXdgDesktopPortalThemePrivate> d_ptr;
    Q_DISABLE_COPY_MOVE(QXdgDesktopPortalTheme)
};

QT_END_NAMESPACE

#endif // QXDGDESKTOPPORTALTHEME_H
