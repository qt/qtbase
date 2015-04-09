/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt for Native Client platform plugin.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpepperfontdatabase.h"

#include <QtCore/QFile>

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
