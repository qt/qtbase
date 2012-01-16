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

#include "qgl2pexvertexarray_p.h"

#include <private/qbezier_p.h>

QT_BEGIN_NAMESPACE

void QGL2PEXVertexArray::clear()
{
    vertexArray.reset();
    vertexArrayStops.reset();
    boundingRectDirty = true;
}


QGLRect QGL2PEXVertexArray::boundingRect() const
{
    if (boundingRectDirty)
        return QGLRect(0.0, 0.0, 0.0, 0.0);
    else
        return QGLRect(minX, minY, maxX, maxY);
}

void QGL2PEXVertexArray::addClosingLine(int index)
{
    QPointF point(vertexArray.at(index));
    if (point != QPointF(vertexArray.last()))
        vertexArray.add(point);
}

void QGL2PEXVertexArray::addCentroid(const QVectorPath &path, int subPathIndex)
{
    const QPointF *const points = reinterpret_cast<const QPointF *>(path.points());
    const QPainterPath::ElementType *const elements = path.elements();

    QPointF sum = points[subPathIndex];
    int count = 1;

    for (int i = subPathIndex + 1; i < path.elementCount() && (!elements || elements[i] != QPainterPath::MoveToElement); ++i) {
        sum += points[i];
        ++count;
    }

    const QPointF centroid = sum / qreal(count);
    vertexArray.add(centroid);
}

void QGL2PEXVertexArray::addPath(const QVectorPath &path, GLfloat curveInverseScale, bool outline)
{
    const QPointF* const points = reinterpret_cast<const QPointF*>(path.points());
    const QPainterPath::ElementType* const elements = path.elements();

    if (boundingRectDirty) {
        minX = maxX = points[0].x();
        minY = maxY = points[0].y();
        boundingRectDirty = false;
    }

    if (!outline && !path.isConvex())
        addCentroid(path, 0);

    int lastMoveTo = vertexArray.size();
    vertexArray.add(points[0]); // The first element is always a moveTo

    do {
        if (!elements) {
//             qDebug("QVectorPath has no elements");
            // If the path has a null elements pointer, the elements implicitly
            // start with a moveTo (already added) and continue with lineTos:
            for (int i=1; i<path.elementCount(); ++i)
                lineToArray(points[i].x(), points[i].y());

            break;
        }
//         qDebug("QVectorPath has element types");

        for (int i=1; i<path.elementCount(); ++i) {
            switch (elements[i]) {
            case QPainterPath::MoveToElement:
                if (!outline)
                    addClosingLine(lastMoveTo);
//                qDebug("element[%d] is a MoveToElement", i);
                vertexArrayStops.add(vertexArray.size());
                if (!outline) {
                    if (!path.isConvex()) addCentroid(path, i);
                    lastMoveTo = vertexArray.size();
                }
                lineToArray(points[i].x(), points[i].y()); // Add the moveTo as a new vertex
                break;
            case QPainterPath::LineToElement:
//                qDebug("element[%d] is a LineToElement", i);
                lineToArray(points[i].x(), points[i].y());
                break;
            case QPainterPath::CurveToElement: {
                QBezier b = QBezier::fromPoints(*(((const QPointF *) points) + i - 1),
                                                points[i],
                                                points[i+1],
                                                points[i+2]);
                QRectF bounds = b.bounds();
                // threshold based on same algorithm as in qtriangulatingstroker.cpp
                int threshold = qMin<float>(64, qMax(bounds.width(), bounds.height()) * 3.14f / (curveInverseScale * 6));
                if (threshold < 3) threshold = 3;
                qreal one_over_threshold_minus_1 = qreal(1) / (threshold - 1);
                for (int t=0; t<threshold; ++t) {
                    QPointF pt = b.pointAt(t * one_over_threshold_minus_1);
                    lineToArray(pt.x(), pt.y());
                }
                i += 2;
                break; }
            default:
                break;
            }
        }
    } while (0);

    if (!outline)
        addClosingLine(lastMoveTo);
    vertexArrayStops.add(vertexArray.size());
}

void QGL2PEXVertexArray::lineToArray(const GLfloat x, const GLfloat y)
{
    vertexArray.add(QGLPoint(x, y));

    if (x > maxX)
        maxX = x;
    else if (x < minX)
        minX = x;
    if (y > maxY)
        maxY = y;
    else if (y < minY)
        minY = y;
}

QT_END_NAMESPACE
