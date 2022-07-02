// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGTK3THEME_H
#define QGTK3THEME_H

#include <private/qtguiglobal_p.h>
#include <private/qgenericunixthemes_p.h>

QT_BEGIN_NAMESPACE

class QGtk3Theme : public QGnomeTheme
{
public:
    QGtk3Theme();

    virtual QVariant themeHint(ThemeHint hint) const override;
    virtual QString gtkFontName() const override;

    Appearance appearance() const override;

    bool usePlatformNativeDialog(DialogType type) const override;
    QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const override;

    QPlatformMenu* createPlatformMenu() const override;
    QPlatformMenuItem* createPlatformMenuItem() const override;

    static const char *name;
private:
    static bool useNativeFileDialog();
};

QT_END_NAMESPACE

#endif // QGTK3THEME_H
