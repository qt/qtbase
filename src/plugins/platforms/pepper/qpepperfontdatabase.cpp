/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "qpepperfontdatabase.h"

#ifdef QT_PEPPER_USE_PEPPER_FONT_ENGINE
#include <QtGui/private/qfontengine_nacl_p.h>
#endif

void QPepperFontDatabase::populateFontDatabase()
{
#ifdef QT_PEPPER_USE_PEPPER_FONT_ENGINE
    QSupportedWritingSystems writingSystems;
    writingSystems.setSupported(QFontDatabase::Latin);
    registerFont("Arial", "", QFont::Normal, QFont::StyleNormal, QFont::Unstretched, true, true, 12,
                 writingSystems, 0);
#else
    // Load font file from resources. Currently
    // all fonts needs to be bundled with the nexe
    // as Qt resources.
    QStringList fontFileNames = QStringList() << QStringLiteral(":/fonts/Vera.ttf")
                                              << QStringLiteral(":/fonts/DejaVuSans.ttf");

    foreach (const QString &fontFileName, fontFileNames) {
        QFile theFont(fontFileName);
        if (!theFont.open(QIODevice::ReadOnly)) {
            qDebug() << "could not open font file" << fontFileName;
            break;
        }

        QBasicFontDatabase::addTTFile(theFont.readAll(), fontFileName.toLatin1());
    }
#endif
}

QFontEngine *QPepperFontDatabase::fontEngine(const QFontDef &fontDef, void *handle)
{
// qDebug() << "QPepperFontDatabase::fontEngine" << fontDef.family << fontDef.pixelSize <<
// fontDef.pointSize;
#ifdef QT_PEPPER_USE_PEPPER_FONT_ENGINE
    return new QFontEnginePepper(fontDef);
#else
    return QBasicFontDatabase::fontEngine(fontDef, handle);
#endif
}

QStringList QPepperFontDatabase::fallbacksForFamily(const QString &family, QFont::Style style,
                                                    QFont::StyleHint styleHint,
                                                    QChar::Script script) const
{
    // qDebug() << "QPepperFontDatabase::fallbacksForFamily" << family;
    QStringList fallbacks
        = QBasicFontDatabase::fallbacksForFamily(family, style, styleHint, script);
    // qDebug() << "Fallbacks" << fallbacks;

    // Add the vera.ttf font (loaded in populateFontDatabase above) as a falback font
    // to all other fonts (except itself).
    const QString veraFontFamily = QStringLiteral("Bitstream Vera Sans");
    if (family != veraFontFamily)
        fallbacks.append(veraFontFamily);

#ifdef QT_PEPPER_USE_PEPPER_FONT_ENGINE
    if (fallbacks.isEmpty())
        fallbacks.append("Arial");
#endif
    return fallbacks;
}

QStringList QPepperFontDatabase::addApplicationFont(const QByteArray &fontData,
                                                    const QString &fileName)
{
    // qDebug() << "QPepperFontDatabase::addApplicationFont";
    return QBasicFontDatabase::addApplicationFont(fontData, fileName);
}

void QPepperFontDatabase::releaseHandle(void *handle)
{
    // qDebug() << "QPepperFontDatabase::releaseHandle" << handle;
    QBasicFontDatabase::releaseHandle(handle);
}
