// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmtheme.h"
#include <QtCore/qvariant.h>
#include <QFontDatabase>
#include <QList>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QWasmTheme::QWasmTheme()
{
    for (auto family : QFontDatabase::families())
        if (QFontDatabase::isFixedPitch(family))
            fixedFont = new QFont(family);
}

QWasmTheme::~QWasmTheme()
{
    if (fixedFont)
        delete fixedFont;
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

QT_END_NAMESPACE
