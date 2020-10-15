/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPOLYGON_H
#define QPOLYGON_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qlist.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>

QT_BEGIN_NAMESPACE

class QTransform;
class QRect;
class QVariant;

// We export each out-of-line method invidually to prevent MSVC from
// exporting the whole QList class.
class QPolygon : public QList<QPoint>
{
public:
    using QList<QPoint>::QList;
    QPolygon() = default;
    Q_IMPLICIT QPolygon(const QList<QPoint> &v) : QList<QPoint>(v) { }
    Q_IMPLICIT QPolygon(QList<QPoint> &&v) noexcept : QList<QPoint>(std::move(v)) { }
    Q_IMPLICIT Q_GUI_EXPORT QPolygon(const QRect &r, bool closed=false);
    Q_GUI_EXPORT QPolygon(int nPoints, const int *points);
    void swap(QPolygon &other) noexcept { QList<QPoint>::swap(other); } // prevent QList<QPoint><->QPolygon swaps

    Q_GUI_EXPORT operator QVariant() const;

    Q_GUI_EXPORT void translate(int dx, int dy);
    void translate(const QPoint &offset);

    [[nodiscard]] Q_GUI_EXPORT QPolygon translated(int dx, int dy) const;
    [[nodiscard]] inline QPolygon translated(const QPoint &offset) const;

    Q_GUI_EXPORT QRect boundingRect() const;

    Q_GUI_EXPORT void point(int i, int *x, int *y) const;
    QPoint point(int i) const;
    void setPoint(int index, int x, int y);
    void setPoint(int index, const QPoint &p);
    Q_GUI_EXPORT void setPoints(int nPoints, const int *points);
    Q_GUI_EXPORT void setPoints(int nPoints, int firstx, int firsty, ...);
    Q_GUI_EXPORT void putPoints(int index, int nPoints, const int *points);
    Q_GUI_EXPORT void putPoints(int index, int nPoints, int firstx, int firsty, ...);
    Q_GUI_EXPORT void putPoints(int index, int nPoints, const QPolygon & from, int fromIndex=0);

    Q_GUI_EXPORT bool containsPoint(const QPoint &pt, Qt::FillRule fillRule) const;

    [[nodiscard]] Q_GUI_EXPORT QPolygon united(const QPolygon &r) const;
    [[nodiscard]] Q_GUI_EXPORT QPolygon intersected(const QPolygon &r) const;
    [[nodiscard]] Q_GUI_EXPORT QPolygon subtracted(const QPolygon &r) const;

    Q_GUI_EXPORT bool intersects(const QPolygon &r) const;
};
Q_DECLARE_SHARED(QPolygon)

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QPolygon &);
#endif

/*****************************************************************************
  QPolygon stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &stream, const QPolygon &polygon);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &stream, QPolygon &polygon);
#endif

/*****************************************************************************
  Misc. QPolygon functions
 *****************************************************************************/

inline void QPolygon::setPoint(int index, const QPoint &pt)
{ (*this)[index] = pt; }

inline void QPolygon::setPoint(int index, int x, int y)
{ (*this)[index] = QPoint(x, y); }

inline QPoint QPolygon::point(int index) const
{ return at(index); }

inline void QPolygon::translate(const QPoint &offset)
{ translate(offset.x(), offset.y()); }

inline QPolygon QPolygon::translated(const QPoint &offset) const
{ return translated(offset.x(), offset.y()); }

class QRectF;

class QPolygonF : public QList<QPointF>
{
public:
    using QList<QPointF>::QList;
    QPolygonF() = default;
    Q_IMPLICIT QPolygonF(const QList<QPointF> &v) : QList<QPointF>(v) { }
    Q_IMPLICIT QPolygonF(QList<QPointF> &&v) noexcept : QList<QPointF>(std::move(v)) { }
    Q_IMPLICIT Q_GUI_EXPORT QPolygonF(const QRectF &r);
    Q_IMPLICIT Q_GUI_EXPORT QPolygonF(const QPolygon &a);
    inline void swap(QPolygonF &other) { QList<QPointF>::swap(other); } // prevent QList<QPointF><->QPolygonF swaps

    Q_GUI_EXPORT operator QVariant() const;

    inline void translate(qreal dx, qreal dy);
    void Q_GUI_EXPORT translate(const QPointF &offset);

    inline QPolygonF translated(qreal dx, qreal dy) const;
    [[nodiscard]] Q_GUI_EXPORT QPolygonF translated(const QPointF &offset) const;

    QPolygon Q_GUI_EXPORT toPolygon() const;

    bool isClosed() const { return !isEmpty() && first() == last(); }

    QRectF Q_GUI_EXPORT boundingRect() const;

    Q_GUI_EXPORT bool containsPoint(const QPointF &pt, Qt::FillRule fillRule) const;

    [[nodiscard]] Q_GUI_EXPORT QPolygonF united(const QPolygonF &r) const;
    [[nodiscard]] Q_GUI_EXPORT QPolygonF intersected(const QPolygonF &r) const;
    [[nodiscard]] Q_GUI_EXPORT QPolygonF subtracted(const QPolygonF &r) const;

    Q_GUI_EXPORT bool intersects(const QPolygonF &r) const;
};
Q_DECLARE_SHARED(QPolygonF)

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QPolygonF &);
#endif

/*****************************************************************************
  QPolygonF stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &stream, const QPolygonF &array);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &stream, QPolygonF &array);
#endif

inline void QPolygonF::translate(qreal dx, qreal dy)
{ translate(QPointF(dx, dy)); }

inline QPolygonF QPolygonF::translated(qreal dx, qreal dy) const
{ return translated(QPointF(dx, dy)); }

QT_END_NAMESPACE

#endif // QPOLYGON_H
