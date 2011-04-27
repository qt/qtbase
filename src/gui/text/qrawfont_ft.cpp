/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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
#include "qfontengine_ft_p.h"

#if defined(Q_WS_X11)
#  include "qfontengine_x11_p.h"
#endif

QT_BEGIN_NAMESPACE

class QFontEngineFTRawFont

#if defined(Q_WS_X11)
        : public QFontEngineX11FT
#else
        : public QFontEngineFT
#endif

{
public:
    QFontEngineFTRawFont(const QFontDef &fontDef)
#if defined(Q_WS_X11)
        : QFontEngineX11FT(fontDef)
#else
        : QFontEngineFT(fontDef)
#endif
    {
    }

    void updateFamilyNameAndStyle()
    {
        fontDef.family = QString::fromAscii(freetype->face->family_name);

        if (freetype->face->style_flags & FT_STYLE_FLAG_ITALIC)
            fontDef.style = QFont::StyleItalic;

        if (freetype->face->style_flags & FT_STYLE_FLAG_BOLD)
            fontDef.weight = QFont::Bold;
    }

    bool initFromData(const QByteArray &fontData)
    {
        FaceId faceId;
        faceId.filename = "";
        faceId.index = 0;

        return init(faceId, true, Format_None, fontData);
    }

    bool initFromFontEngine(QFontEngine *oldFontEngine)
    {
        QFontEngineFT *fe = static_cast<QFontEngineFT *>(oldFontEngine);

        // Increase the reference of this QFreetypeFace since one more QFontEngineFT
        // will be using it
        fe->freetype->ref.ref();
        if (!init(fe->faceId(), fe->antialias, fe->defaultFormat, fe->freetype))
            return false;

        default_load_flags = fe->default_load_flags;
        default_hint_style = fe->default_hint_style;
        antialias = fe->antialias;
        transform = fe->transform;
        embolden = fe->embolden;
        subpixelType = fe->subpixelType;
        lcdFilterType = fe->lcdFilterType;
        canUploadGlyphsToServer = fe->canUploadGlyphsToServer;
        embeddedbitmap = fe->embeddedbitmap;

#if defined(Q_WS_X11)
        xglyph_format = static_cast<QFontEngineX11FT *>(fe)->xglyph_format;
#endif
        return true;
    }
};


void QRawFontPrivate::platformCleanUp()
{
    // Font engine handles all resources
}

void QRawFontPrivate::platformLoadFromData(const QByteArray &fontData, int pixelSize,
                                           QFont::HintingPreference hintingPreference)
{
    Q_ASSERT(fontEngine == 0);

    QFontDef fontDef;
    fontDef.pixelSize = pixelSize;

    QFontEngineFTRawFont *fe = new QFontEngineFTRawFont(fontDef);
    if (!fe->initFromData(fontData)) {
        delete fe;
        return;
    }

    fe->updateFamilyNameAndStyle();

    switch (hintingPreference) {
    case QFont::PreferNoHinting:
        fe->setDefaultHintStyle(QFontEngineFT::HintNone);
        break;
    case QFont::PreferFullHinting:
        fe->setDefaultHintStyle(QFontEngineFT::HintFull);
        break;
    case QFont::PreferVerticalHinting:
        fe->setDefaultHintStyle(QFontEngineFT::HintLight);
        break;
    default:
        // Leave it as it is
        break;
    }

    fontEngine = fe;
    fontEngine->ref.ref();
}

void QRawFontPrivate::platformSetPixelSize(int pixelSize)
{
    if (fontEngine == NULL)
        return;

    QFontEngine *oldFontEngine = fontEngine;

    QFontDef fontDef;
    fontDef.pixelSize = pixelSize;
    QFontEngineFTRawFont *fe = new QFontEngineFTRawFont(fontDef);
    if (!fe->initFromFontEngine(oldFontEngine)) {
        delete fe;
        return;
    }

    fontEngine = fe;
    fontEngine->fontDef = oldFontEngine->fontDef;
    fontEngine->fontDef.pixelSize = pixelSize;
    fontEngine->ref.ref();
    Q_ASSERT(fontEngine != oldFontEngine);
    oldFontEngine->ref.deref();
    if (oldFontEngine->cache_count == 0 && oldFontEngine->ref == 0)
        delete oldFontEngine;
}

QT_END_NAMESPACE

#endif // QT_NO_RAWFONT
