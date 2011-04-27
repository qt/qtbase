/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qglobal.h>

#if !defined(QT_NO_RAWFONT)

#include "qrawfont_p.h"
#include "qfontengine_coretext_p.h"

QT_BEGIN_NAMESPACE

void QRawFontPrivate::platformCleanUp()
{
}

extern int qt_defaultDpi();

void QRawFontPrivate::platformLoadFromData(const QByteArray &fontData,
                                           int pixelSize,
                                           QFont::HintingPreference hintingPreference)
{
    // Mac OS X ignores it
    Q_UNUSED(hintingPreference);

    QCFType<CGDataProviderRef> dataProvider = CGDataProviderCreateWithData(NULL,
            fontData.constData(), fontData.size(), NULL);

    CGFontRef cgFont = CGFontCreateWithDataProvider(dataProvider);

    if (cgFont == NULL) {
        qWarning("QRawFont::platformLoadFromData: CGFontCreateWithDataProvider failed");
    } else {
        QFontDef def;
        def.pixelSize = pixelSize;
        def.pointSize = pixelSize * 72.0 / qt_defaultDpi();
        fontEngine = new QCoreTextFontEngine(cgFont, def);
        CFRelease(cgFont);
        fontEngine->ref.ref();
    }
}

void QRawFontPrivate::platformSetPixelSize(int pixelSize)
{
    if (fontEngine == NULL)
        return;

    QFontEngine *oldFontEngine = fontEngine;

    QFontDef fontDef = oldFontEngine->fontDef;
    fontDef.pixelSize = pixelSize;
    fontDef.pointSize = pixelSize * 72.0 / qt_defaultDpi();

    QCoreTextFontEngine *ctFontEngine = static_cast<QCoreTextFontEngine *>(oldFontEngine);
    Q_ASSERT(ctFontEngine->cgFont);

    fontEngine = new QCoreTextFontEngine(ctFontEngine->cgFont, fontDef);
    fontEngine->ref.ref();
    Q_ASSERT(fontEngine != oldFontEngine);
    oldFontEngine->ref.deref();
    if (oldFontEngine->cache_count == 0 && oldFontEngine->ref == 0)
        delete oldFontEngine;
}

QT_END_NAMESPACE

#endif // QT_NO_RAWFONT
