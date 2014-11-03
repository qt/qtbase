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

#include "qpepperfontengine.h"

#ifdef QT_PEPPER_USE_PEPPER_FONT_ENGINE

#include "ppapi/cpp/dev/font_dev.h"

static inline unsigned int getChar(const QChar *str, int &i, const int len)
{
    unsigned int uc = str[i].unicode();
    if (uc >= 0xd800 && uc < 0xdc00 && i < len-1) {
        uint low = str[i+1].unicode();
       if (low >= 0xdc00 && low < 0xe000) {
            uc = (uc - 0xd800)*0x400 + (low - 0xdc00) + 0x10000;
            ++i;
        }
    }
    return uc;
}

QFontEnginePepper::QFontEnginePepper(const QFontDef &fontDef)
:m_fontDef(fontDef)
{
    qDebug() << "QFontEnginePepper" << fontDef.family;
    m_fontName = fontDef.family;
    m_pepperRequestedFontDescription.set_family(PP_FONTFAMILY_DEFAULT);
    m_pepperFont = pp::Font_Dev(QtPepperMain::globalInstance()->instance(), m_pepperRequestedFontDescription);
    m_pepperFont.Describe(&m_pepperActualFontDescription, &m_pepperFontMetrics);
    qDebug() << "QFontEnginePepper done";
}

bool QFontEnginePepper::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags) const
{
    if(*nglyphs < len) {
        *nglyphs = len;
        return false;
    }
    *nglyphs = 0;

    bool mirrored = flags & QTextEngine::RightToLeft;
    for(int i = 0; i < len; i++) {
        unsigned int uc = getChar(str, i, len);
        if (mirrored)
            uc = QChar::mirroredChar(uc);
        glyphs->glyphs[*nglyphs] = uc < 0x10000 ? uc : 0;
        ++*nglyphs;
    }

    glyphs->numGlyphs = *nglyphs;

    if (flags & QTextEngine::GlyphIndicesOnly)
        return true;

    qDebug() << "recalc advances";

//    recalcAdvances(glyphs, flags);

    return true;
}


glyph_metrics_t QFontEnginePepper::boundingBox(const QGlyphLayout &glyphLayout)
{

}

glyph_metrics_t QFontEnginePepper::boundingBox(glyph_t glyph)
{

}

QFixed QFontEnginePepper::ascent() const
{
    return m_pepperFontMetrics.ascent;
}

QFixed QFontEnginePepper::descent() const
{
    return m_pepperFontMetrics.descent;
}

QFixed QFontEnginePepper::leading() const
{

}

qreal QFontEnginePepper::maxCharWidth() const
{

}

const char* QFontEnginePepper::name() const
{
    return m_fontName.toLatin1().constData();
}

bool QFontEnginePepper::canRender(const QChar*, int)
{
    return true;
}

QFontEngine::Type QFontEnginePepper::type() const
{
    return QFontEngine::TestFontEngine;
}

#endif // QT_PEPPER_USE_PEPPER_FONT_ENGINE
