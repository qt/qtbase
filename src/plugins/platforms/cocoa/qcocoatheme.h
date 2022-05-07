/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QPLATFORMTHEME_COCOA_H
#define QPLATFORMTHEME_COCOA_H

#include <QtCore/QHash>
#include <qpa/qplatformtheme.h>

#include <QtCore/private/qcore_mac_p.h>

QT_BEGIN_NAMESPACE

class QPalette;
class QCocoaTheme : public QPlatformTheme
{
public:
    QCocoaTheme();
    ~QCocoaTheme();

    void reset();

    QPlatformMenuItem* createPlatformMenuItem() const override;
    QPlatformMenu* createPlatformMenu() const override;
    QPlatformMenuBar* createPlatformMenuBar() const override;

#ifndef QT_NO_SYSTEMTRAYICON
    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const override;
#endif

    bool usePlatformNativeDialog(DialogType dialogType) const override;
    QPlatformDialogHelper *createPlatformDialogHelper(DialogType dialogType) const override;

    const QPalette *palette(Palette type = SystemPalette) const override;
    const QFont *font(Font type = SystemFont) const override;
    QPixmap standardPixmap(StandardPixmap sp, const QSizeF &size) const override;
    QIcon fileIcon(const QFileInfo &fileInfo, QPlatformTheme::IconOptions options = {}) const override;

    QVariant themeHint(ThemeHint hint) const override;
    Appearance appearance() const override;
    QString standardButtonText(int button) const override;
    QKeySequence standardButtonShortcut(int button) const override;

    static const char *name;

    void handleSystemThemeChange();

#ifndef QT_NO_SHORTCUT
    QList<QKeySequence> keyBindings(QKeySequence::StandardKey key) const override;
#endif

private:
    mutable QPalette *m_systemPalette;
    QMacNotificationObserver m_systemColorObserver;
    mutable QHash<QPlatformTheme::Palette, QPalette*> m_palettes;
    QMacKeyValueObserver m_appearanceObserver;
};

QT_END_NAMESPACE

#endif
