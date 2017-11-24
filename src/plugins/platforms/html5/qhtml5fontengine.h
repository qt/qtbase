/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QHTML5FONTENGINE_P_H
#define QHTML5FONTENGINE_P_H

#include <private/qfontengine_p.h>

//#ifdef QT_HTML5_USE_HTML5_FONT_ENGINE

//#include <ppapi/cpp/dev/font_dev.h>

QT_BEGIN_NAMESPACE

class QFontEngineHtml5: public QFontEngine
{
public:
    QFontEngineHtml5(const QFontDef &fontDef);
//    bool stringToCMap(const QChar *, int, QGlyphLayout *, int *,
//                      QFlags<QTextEngine::ShaperFlag>) const;
    glyph_metrics_t boundingBox(const QGlyphLayout &);
    glyph_metrics_t boundingBox(glyph_t);
    QFixed ascent() const;
    QFixed descent() const;
    QFixed leading() const;
    qreal maxCharWidth() const;
    const char *name() const;
    bool canRender(const QChar *, int);
    QFontEngine::Type type() const;

private:
    QString m_fontName;
    QFontDef m_fontDef;
//    pp::FontDescription_Dev m_html5RequestedFontDescription;
//    pp::FontDescription_Dev m_html5ActualFontDescription;
//    pp::Font_Dev m_html5Font;
//    PP_FontMetrics_Dev m_html5FontMetrics;
};

QT_END_NAMESPACE

//#endif
#endif
