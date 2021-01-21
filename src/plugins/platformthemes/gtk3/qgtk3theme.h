/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QGTK3THEME_H
#define QGTK3THEME_H

#include <private/qgenericunixthemes_p.h>

QT_BEGIN_NAMESPACE

class QGtk3Theme : public QGnomeTheme
{
public:
    QGtk3Theme();

    virtual QVariant themeHint(ThemeHint hint) const override;
    virtual QString gtkFontName() const override;

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
