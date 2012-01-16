/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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

#ifndef QTRIANGULATINGSTROKER_P_H
#define QTRIANGULATINGSTROKER_P_H

#include <private/qdatabuffer_p.h>
#include <qvarlengtharray.h>
#include <private/qvectorpath_p.h>
#include <private/qbezier_p.h>
#include <private/qnumeric_p.h>
#include <private/qmath_p.h>

QT_BEGIN_NAMESPACE

class QTriangulatingStroker
{
public:
    QTriangulatingStroker() : m_vertices(0) {}
    void process(const QVectorPath &path, const QPen &pen, const QRectF &clip);

    inline int vertexCount() const { return m_vertices.size(); }
    inline const float *vertices() const { return m_vertices.data(); }

    inline void setInvScale(qreal invScale) { m_inv_scale = invScale; }

private:
    inline void emitLineSegment(float x, float y, float nx, float ny);
    void moveTo(const qreal *pts);
    inline void lineTo(const qreal *pts);
    void cubicTo(const qreal *pts);
    void join(const qreal *pts);
    inline void normalVector(float x1, float y1, float x2, float y2, float *nx, float *ny);
    void endCap(const qreal *pts);
    void arcPoints(float cx, float cy, float fromX, float fromY, float toX, float toY, QVarLengthArray<float> &points);
    void endCapOrJoinClosed(const qreal *start, const qreal *cur, bool implicitClose, bool endsAtStart);


    QDataBuffer<float> m_vertices;

    float m_cx, m_cy;           // current points
    float m_nvx, m_nvy;         // normal vector...
    float m_width;
    qreal m_miter_limit;

    int m_roundness;            // Number of line segments in a round join
    qreal m_sin_theta;          // sin(m_roundness / 360);
    qreal m_cos_theta;          // cos(m_roundness / 360);
    qreal m_inv_scale;
    float m_curvyness_mul;
    float m_curvyness_add;

    Qt::PenJoinStyle m_join_style;
    Qt::PenCapStyle m_cap_style;
};

class QDashedStrokeProcessor
{
public:
    QDashedStrokeProcessor();

    void process(const QVectorPath &path, const QPen &pen, const QRectF &clip);

    inline void addElement(QPainterPath::ElementType type, qreal x, qreal y) {
        m_points.add(x);
        m_points.add(y);
        m_types.add(type);
    }

    inline int elementCount() const { return m_types.size(); }
    inline qreal *points() const { return m_points.data(); }
    inline QPainterPath::ElementType *elementTypes() const { return m_types.data(); }

    inline void setInvScale(qreal invScale) { m_inv_scale = invScale; }

private:
    QDataBuffer<qreal> m_points;
    QDataBuffer<QPainterPath::ElementType> m_types;
    QDashStroker m_dash_stroker;
    qreal m_inv_scale;
};

inline void QTriangulatingStroker::normalVector(float x1, float y1, float x2, float y2,
                                                float *nx, float *ny)
{
    float dx = x2 - x1;
    float dy = y2 - y1;

    float pw;

    if (dx == 0)
        pw = m_width / qAbs(dy);
    else if (dy == 0)
        pw = m_width / qAbs(dx);
    else
        pw = m_width / sqrt(dx*dx + dy*dy);

    *nx = -dy * pw;
    *ny = dx * pw;
}

inline void QTriangulatingStroker::emitLineSegment(float x, float y, float vx, float vy)
{
    m_vertices.add(x + vx);
    m_vertices.add(y + vy);
    m_vertices.add(x - vx);
    m_vertices.add(y - vy);
}

void QTriangulatingStroker::lineTo(const qreal *pts)
{
    emitLineSegment(pts[0], pts[1], m_nvx, m_nvy);
    m_cx = pts[0];
    m_cy = pts[1];
}

QT_END_NAMESPACE

#endif
