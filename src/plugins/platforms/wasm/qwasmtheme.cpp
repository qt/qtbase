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
****************************************************************************/

#include "qwasmtheme.h"
#include <QtCore/qvariant.h>
#include <QFontDatabase>

QT_BEGIN_NAMESPACE

QWasmTheme::QWasmTheme()
{
    QFontDatabase fdb;
    for (auto family : fdb.families())
        if (fdb.isFixedPitch(family))
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
        return QVariant(QStringList() << QLatin1String("fusion"));
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
