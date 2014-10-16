/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpepperfontdatabase.h"
#include <QtGui>
#ifdef QT_PEPPER_USE_PEPPER_FONT_ENGINE
#include <QtGui/private/qfontengine_nacl_p.h>
#endif

void QPepperFontDatabase::populateFontDatabase()
{
#ifdef QT_PEPPER_USE_PEPPER_FONT_ENGINE
    QSupportedWritingSystems writingSystems;
    writingSystems.setSupported(QFontDatabase::Latin);
    registerFont("Arial", "",
                 QFont::Normal,
                 QFont::StyleNormal,
                 QFont::Unstretched,
                 true,
                 true,
                 12,
                 writingSystems,
                 0);
#else
    // Load font file from resources. Currently
    // all fonts needs to be bundled with the nexe
    // as Qt resources.
    QStringList fontFileNames = QStringList() << ":/fonts/Vera.ttf"
                                              << ":/fonts/DejaVuSans.ttf";

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
   // qDebug() << "QPepperFontDatabase::fontEngine" << fontDef.family << fontDef.pixelSize << fontDef.pointSize;
#ifdef QT_PEPPER_USE_PEPPER_FONT_ENGINE
    return new QFontEnginePepper(fontDef);
#else
    return QBasicFontDatabase::fontEngine(fontDef, handle);
#endif
}

QStringList QPepperFontDatabase::fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const
{
    // qDebug() << "QPepperFontDatabase::fallbacksForFamily" << family;
    QStringList fallbacks = QBasicFontDatabase::fallbacksForFamily(family, style, styleHint, script);
    // qDebug() << "Fallbacks" << fallbacks;
#ifdef QT_PEPPER_USE_PEPPER_FONT_ENGINE
    if (fallbacks.isEmpty())
        fallbacks.append("Arial");
#endif
    return fallbacks;
}

QStringList QPepperFontDatabase::addApplicationFont(const QByteArray &fontData, const QString &fileName)
{
    // qDebug() << "QPepperFontDatabase::addApplicationFont";
    return QBasicFontDatabase::addApplicationFont(fontData, fileName);
}

void QPepperFontDatabase::releaseHandle(void *handle)
{
    // qDebug() << "QPepperFontDatabase::releaseHandle" << handle;
    QBasicFontDatabase::releaseHandle(handle);
}
