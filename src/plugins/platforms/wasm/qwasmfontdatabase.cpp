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

#include "qwasmfontdatabase.h"

#include <QtCore/qfile.h>

QT_BEGIN_NAMESPACE

void QWasmFontDatabase::populateFontDatabase()
{
    // Load font file from resources. Currently
    // all fonts needs to be bundled with the nexe
    // as Qt resources.

    const QString fontFileNames[] = {
        QStringLiteral(":/fonts/DejaVuSansMono.ttf"),
        QStringLiteral(":/fonts/Vera.ttf"),
        QStringLiteral(":/fonts/DejaVuSans.ttf"),
    };
    for (const QString &fontFileName : fontFileNames) {
        QFile theFont(fontFileName);
        if (!theFont.open(QIODevice::ReadOnly))
            break;

        QFreeTypeFontDatabase::addTTFile(theFont.readAll(), fontFileName.toLatin1());
    }
}

QFontEngine *QWasmFontDatabase::fontEngine(const QFontDef &fontDef, void *handle)
{
    return QFreeTypeFontDatabase::fontEngine(fontDef, handle);
}

QStringList QWasmFontDatabase::fallbacksForFamily(const QString &family, QFont::Style style,
                                                    QFont::StyleHint styleHint,
                                                    QChar::Script script) const
{
    QStringList fallbacks
        = QFreeTypeFontDatabase::fallbacksForFamily(family, style, styleHint, script);

    // Add the vera.ttf font (loaded in populateFontDatabase above) as a falback font
    // to all other fonts (except itself).
    const QString veraFontFamily = QStringLiteral("Bitstream Vera Sans");
    if (family != veraFontFamily)
        fallbacks.append(veraFontFamily);

    return fallbacks;
}

QStringList QWasmFontDatabase::addApplicationFont(const QByteArray &fontData,
                                                    const QString &fileName)
{
    return QFreeTypeFontDatabase::addApplicationFont(fontData, fileName);
}

void QWasmFontDatabase::releaseHandle(void *handle)
{
    QFreeTypeFontDatabase::releaseHandle(handle);
}

QFont QWasmFontDatabase::defaultFont() const
{
    return QFont(QLatin1String("Bitstream Vera Sans"));
}

QT_END_NAMESPACE
