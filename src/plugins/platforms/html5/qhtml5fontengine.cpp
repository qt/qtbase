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

#include "qhtml5fontengine.h"

//#ifdef QT_HTML5_USE_HTML5_FONT_ENGINE

//#include <ppapi/cpp/dev/font_dev.h>

static inline unsigned int getChar(const QChar *str, int &i, const int len)
{
    unsigned int uc = str[i].unicode();
    if (uc >= 0xd800 && uc < 0xdc00 && i < len - 1) {
        uint low = str[i + 1].unicode();
        if (low >= 0xdc00 && low < 0xe000) {
            uc = (uc - 0xd800) * 0x400 + (low - 0xdc00) + 0x10000;
            ++i;
        }
    }
    return uc;
}

QFontEngineHtml5::QFontEngineHtml5(const QFontDef &fontDef)
    : m_fontDef(fontDef)
{
    qDebug() << "QFontEngineHtml5" << fontDef.family;
    m_fontName = fontDef.family;
    m_html5RequestedFontDescription.set_family(PP_FONTFAMILY_DEFAULT);
    m_html5Font = pp::Font_Dev(QtHtml5Main::globalInstance()->instance(),
                                m_html5RequestedFontDescription);
    m_html5Font.Describe(&m_html5ActualFontDescription, &m_html5FontMetrics);
    qDebug() << "QFontEngineHtml5 done";
}

//bool QFontEngineHtml5::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs,
//                                     QTextEngine::ShaperFlags flags) const
//{
//    if (*nglyphs < len) {
//        *nglyphs = len;
//        return false;
//    }
//    *nglyphs = 0;

//    bool mirrored = flags & QTextEngine::RightToLeft;
//    for (int i = 0; i < len; i++) {
//        unsigned int uc = getChar(str, i, len);
//        if (mirrored)
//            uc = QChar::mirroredChar(uc);
//        glyphs->glyphs[*nglyphs] = uc < 0x10000 ? uc : 0;
//        ++*nglyphs;
//    }

//    glyphs->numGlyphs = *nglyphs;

//    if (flags & QTextEngine::GlyphIndicesOnly)
//        return true;

//    qDebug() << "recalc advances";

//    //    recalcAdvances(glyphs, flags);

//    return true;
//}

glyph_metrics_t QFontEngineHtml5::boundingBox(const QGlyphLayout &glyphLayout) {}

glyph_metrics_t QFontEngineHtml5::boundingBox(glyph_t glyph) {}

QFixed QFontEngineHtml5::ascent() const { return m_html5FontMetrics.ascent; }

QFixed QFontEngineHtml5::descent() const { return m_html5FontMetrics.descent; }

QFixed QFontEngineHtml5::leading() const {}

qreal QFontEngineHtml5::maxCharWidth() const {}

const char *QFontEngineHtml5::name() const { return m_fontName.toLatin1().constData(); }

bool QFontEngineHtml5::canRender(const QChar *, int) { return true; }

QFontEngine::Type QFontEngineHtml5::type() const { return QFontEngine::TestFontEngine; }

//#endif // QT_html5_USE_html5_FONT_ENGINE
