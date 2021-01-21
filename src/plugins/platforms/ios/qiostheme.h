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

#ifndef QIOSTHEME_H
#define QIOSTHEME_H

#include <QtCore/QHash>
#include <QtGui/QPalette>
#include <qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

class QIOSTheme : public QPlatformTheme
{
public:
    QIOSTheme();
    ~QIOSTheme();

    const QPalette *palette(Palette type = SystemPalette) const override;
    QVariant themeHint(ThemeHint hint) const override;

    QPlatformMenuItem* createPlatformMenuItem() const override;
    QPlatformMenu* createPlatformMenu() const override;

    bool usePlatformNativeDialog(DialogType type) const override;
    QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const override;

    const QFont *font(Font type = SystemFont) const override;

    static const char *name;

    static void initializeSystemPalette();

private:
    mutable QHash<QPlatformTheme::Font, QFont *> m_fonts;

    static QPalette s_systemPalette;
};

QT_END_NAMESPACE

#endif
