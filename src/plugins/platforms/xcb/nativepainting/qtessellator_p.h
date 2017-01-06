/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
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
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTESSELATOR_P_H
#define QTESSELATOR_P_H

#include <QPoint>
#include <QRect>

QT_BEGIN_NAMESPACE

class QTessellatorPrivate;

typedef int Q27Dot5;
#define Q27Dot5ToDouble(i) ((i)/32.)
#define FloatToQ27Dot5(i) (int)((i) * 32)
#define IntToQ27Dot5(i) ((i) << 5)
#define Q27Dot5ToXFixed(i) ((i) << 11)
#define Q27Dot5Factor 32

class QTessellator {
public:
    QTessellator();
    virtual ~QTessellator();

    QRectF tessellate(const QPointF *points, int nPoints);
    void tessellateConvex(const QPointF *points, int nPoints);
    void tessellateRect(const QPointF &a, const QPointF &b, qreal width);

    void setWinding(bool w);

    struct Vertex {
        Q27Dot5 x;
        Q27Dot5 y;
    };
    struct Trapezoid {
        Q27Dot5 top;
        Q27Dot5 bottom;
        const Vertex *topLeft;
        const Vertex *bottomLeft;
        const Vertex *topRight;
        const Vertex *bottomRight;
    };
    virtual void addTrap(const Trapezoid &trap) = 0;

private:
    friend class QTessellatorPrivate;
    QTessellatorPrivate *d;
};

QT_END_NAMESPACE

#endif
