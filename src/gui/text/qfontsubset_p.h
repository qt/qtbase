/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QFONTSUBSET_P_H
#define QFONTSUBSET_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include "private/qfontengine_p.h"

QT_BEGIN_NAMESPACE

class QFontSubset
{
public:
    explicit QFontSubset(QFontEngine *fe, int obj_id = 0)
        : object_id(obj_id), noEmbed(false), fontEngine(fe), downloaded_glyphs(0), standard_font(false)
    {
        fontEngine->ref.ref();
#ifndef QT_NO_PDF
        addGlyph(0);
#endif
    }
    ~QFontSubset() {
        if (!fontEngine->ref.deref())
            delete fontEngine;
    }

    QByteArray toTruetype() const;
#ifndef QT_NO_PDF
    QByteArray widthArray() const;
    QByteArray createToUnicodeMap() const;
    QVector<int> getReverseMap() const;
    QByteArray glyphName(unsigned int glyph, const QVector<int> &reverseMap) const;

    static QByteArray glyphName(unsigned short unicode, bool symbol);

    int addGlyph(int index);
#endif
    const int object_id;
    bool noEmbed;
    QFontEngine *fontEngine;
    QVector<int> glyph_indices;
    mutable int downloaded_glyphs;
    mutable bool standard_font;
    int nGlyphs() const { return glyph_indices.size(); }
    mutable QFixed emSquare;
    mutable QVector<QFixed> widths;
};

QT_END_NAMESPACE

#endif // QFONTSUBSET_P_H
