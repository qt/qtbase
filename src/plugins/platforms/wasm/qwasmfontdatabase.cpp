// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmfontdatabase.h"

#include <QtCore/qfile.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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

void QWasmFontDatabase::releaseHandle(void *handle)
{
    QFreeTypeFontDatabase::releaseHandle(handle);
}

QFont QWasmFontDatabase::defaultFont() const
{
    return QFont("Bitstream Vera Sans"_L1);
}

QT_END_NAMESPACE
