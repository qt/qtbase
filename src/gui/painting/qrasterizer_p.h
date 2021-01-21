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

#ifndef QRASTERIZER_P_H
#define QRASTERIZER_P_H

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
#include "QtGui/qpainter.h"

#include <private/qdrawhelper_p.h>
#include <private/qrasterdefs_p.h>

QT_BEGIN_NAMESPACE

struct QSpanData;
class QRasterBuffer;
class QRasterizerPrivate;

class
QRasterizer
{
public:
    QRasterizer();
    ~QRasterizer();

    void setAntialiased(bool antialiased);
    void setClipRect(const QRect &clipRect);
    void setLegacyRoundingEnabled(bool legacyRoundingEnabled);

    void initialize(ProcessSpans blend, void *data);

    void rasterize(const QT_FT_Outline *outline, Qt::FillRule fillRule);
    void rasterize(const QPainterPath &path, Qt::FillRule fillRule);

    // width should be in units of |a-b|
    void rasterizeLine(const QPointF &a, const QPointF &b, qreal width, bool squareCap = false);

private:
    QRasterizerPrivate *d;
};

QT_END_NAMESPACE

#endif
