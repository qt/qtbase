// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGLYPHRUN_P_H
#define QGLYPHRUN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include "qglyphrun.h"
#include "qrawfont.h"

#include <qfont.h>

#if !defined(QT_NO_RAWFONT)

QT_BEGIN_NAMESPACE

class QGlyphRunPrivate: public QSharedData
{
public:
    QGlyphRunPrivate()
        : glyphIndexData(glyphIndexes.constData())
        , glyphIndexDataSize(0)
        , glyphPositionData(glyphPositions.constData())
        , glyphPositionDataSize(0)
        , textRangeStart(-1)
        , textRangeEnd(-1)
    {
    }

    QGlyphRunPrivate(const QGlyphRunPrivate &other)
      : QSharedData(other)
      , glyphIndexes(other.glyphIndexes)
      , glyphPositions(other.glyphPositions)
      , rawFont(other.rawFont)
      , boundingRect(other.boundingRect)
      , flags(other.flags)
      , glyphIndexData(other.glyphIndexData)
      , glyphIndexDataSize(other.glyphIndexDataSize)
      , glyphPositionData(other.glyphPositionData)
      , glyphPositionDataSize(other.glyphPositionDataSize)
      , textRangeStart(other.textRangeStart)
      , textRangeEnd(other.textRangeEnd)
    {
    }

    QList<quint32> glyphIndexes;
    QList<QPointF> glyphPositions;
    QRawFont rawFont;
    QRectF boundingRect;

    QGlyphRun::GlyphRunFlags flags;

    const quint32 *glyphIndexData;
    int glyphIndexDataSize;

    const QPointF *glyphPositionData;
    int glyphPositionDataSize;

    int textRangeStart;
    int textRangeEnd;

    static QGlyphRunPrivate *get(const QGlyphRun &glyphRun)
    {
        return glyphRun.d.data();
    }
};

QT_END_NAMESPACE

#endif // QT_NO_RAWFONT

#endif // QGLYPHRUN_P_H
