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

#ifndef QPEPPERFONTENGINE_P_H
#define QPEPPERFONTENGINE_P_H

#include <private/qfontengine_p.h>

#ifdef QT_PEPPER_USE_PEPPER_FONT_ENGINE

#include <ppapi/cpp/dev/font_dev.h>

QT_BEGIN_NAMESPACE

class QFontEnginePepper : public QFontEngine
{
public:
    QFontEnginePepper(const QFontDef &fontDef);
    bool stringToCMap(const QChar *, int, QGlyphLayout *, int *,
                      QFlags<QTextEngine::ShaperFlag>) const;
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
    pp::FontDescription_Dev m_pepperRequestedFontDescription;
    pp::FontDescription_Dev m_pepperActualFontDescription;
    pp::Font_Dev m_pepperFont;
    PP_FontMetrics_Dev m_pepperFontMetrics;
};

QT_END_NAMESPACE

#endif
#endif
