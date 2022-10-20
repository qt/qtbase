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

#ifndef QFIXED_P_H
#define QFIXED_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include "QtCore/qdebug.h"
#include "QtCore/qpoint.h"
#include "QtCore/qsize.h"

QT_BEGIN_NAMESPACE

struct QFixed {
private:
    constexpr QFixed(int val, int) : val(val) {} // 2nd int is just a dummy for disambiguation
public:
    constexpr QFixed() : val(0) {}
    constexpr QFixed(int i) : val(i * 64) {}
    constexpr QFixed(long i) : val(i * 64) {}
    QFixed &operator=(int i) { val = i * 64; return *this; }
    QFixed &operator=(long i) { val = i * 64; return *this; }

    constexpr static QFixed fromReal(qreal r) { return fromFixed((int)(r*qreal(64))); }
    constexpr static QFixed fromFixed(int fixed) { return QFixed(fixed,0); } // uses private ctor

    constexpr inline int value() const { return val; }
    inline void setValue(int value) { val = value; }

    constexpr inline int toInt() const { return (((val)+32) & -64)>>6; }
    constexpr inline qreal toReal() const { return ((qreal)val)/(qreal)64; }

    constexpr inline int truncate() const { return val>>6; }
    constexpr inline QFixed round() const { return fromFixed(((val)+32) & -64); }
    constexpr inline QFixed floor() const { return fromFixed((val) & -64); }
    constexpr inline QFixed ceil() const { return fromFixed((val+63) & -64); }

    constexpr inline QFixed operator+(int i) const { return fromFixed(val + i * 64); }
    constexpr inline QFixed operator+(uint i) const { return fromFixed((val + (i<<6))); }
    constexpr inline QFixed operator+(const QFixed &other) const { return fromFixed((val + other.val)); }
    inline QFixed &operator+=(int i) { val += i * 64; return *this; }
    inline QFixed &operator+=(uint i) { val += (i<<6); return *this; }
    inline QFixed &operator+=(const QFixed &other) { val += other.val; return *this; }
    constexpr inline QFixed operator-(int i) const { return fromFixed(val - i * 64); }
    constexpr inline QFixed operator-(uint i) const { return fromFixed((val - (i<<6))); }
    constexpr inline QFixed operator-(const QFixed &other) const { return fromFixed((val - other.val)); }
    inline QFixed &operator-=(int i) { val -= i * 64; return *this; }
    inline QFixed &operator-=(uint i) { val -= (i<<6); return *this; }
    inline QFixed &operator-=(const QFixed &other) { val -= other.val; return *this; }
    constexpr inline QFixed operator-() const { return fromFixed(-val); }

    constexpr inline bool operator==(const QFixed &other) const { return val == other.val; }
    constexpr inline bool operator!=(const QFixed &other) const { return val != other.val; }
    constexpr inline bool operator<(const QFixed &other) const { return val < other.val; }
    constexpr inline bool operator>(const QFixed &other) const { return val > other.val; }
    constexpr inline bool operator<=(const QFixed &other) const { return val <= other.val; }
    constexpr inline bool operator>=(const QFixed &other) const { return val >= other.val; }
    constexpr inline bool operator!() const { return !val; }

    inline QFixed &operator/=(int x) { val /= x; return *this; }
    inline QFixed &operator/=(const QFixed &o) {
        if (o.val == 0) {
            val = 0x7FFFFFFFL;
        } else {
            bool neg = false;
            qint64 a = val;
            qint64 b = o.val;
            if (a < 0) { a = -a; neg = true; }
            if (b < 0) { b = -b; neg = !neg; }

            int res = (int)(((a << 6) + (b >> 1)) / b);

            val = (neg ? -res : res);
        }
        return *this;
    }
    constexpr inline QFixed operator/(int d) const { return fromFixed(val/d); }
    inline QFixed operator/(QFixed b) const { QFixed f = *this; return (f /= b); }
    inline QFixed operator>>(int d) const { QFixed f = *this; f.val >>= d; return f; }
    inline QFixed &operator*=(int i) { val *= i; return *this; }
    inline QFixed &operator*=(uint i) { val *= i; return *this; }
    inline QFixed &operator*=(const QFixed &o) {
        bool neg = false;
        qint64 a = val;
        qint64 b = o.val;
        if (a < 0) { a = -a; neg = true; }
        if (b < 0) { b = -b; neg = !neg; }

        int res = (int)((a * b + 0x20L) >> 6);
        val = neg ? -res : res;
        return *this;
    }
    constexpr inline QFixed operator*(int i) const { return fromFixed(val * i); }
    constexpr inline QFixed operator*(uint i) const { return fromFixed(val * i); }
    inline QFixed operator*(const QFixed &o) const { QFixed f = *this; return (f *= o); }

private:
    constexpr QFixed(qreal i) : val((int)(i*qreal(64))) {}
    QFixed &operator=(qreal i) { val = (int)(i*qreal(64)); return *this; }
    constexpr inline QFixed operator+(qreal i) const { return fromFixed((val + (int)(i*qreal(64)))); }
    inline QFixed &operator+=(qreal i) { val += (int)(i*64); return *this; }
    constexpr inline QFixed operator-(qreal i) const { return fromFixed((val - (int)(i*qreal(64)))); }
    inline QFixed &operator-=(qreal i) { val -= (int)(i*64); return *this; }
    inline QFixed &operator/=(qreal r) { val = (int)(val/r); return *this; }
    constexpr inline QFixed operator/(qreal d) const { return fromFixed((int)(val/d)); }
    inline QFixed &operator*=(qreal d) { val = (int) (val*d); return *this; }
    constexpr inline QFixed operator*(qreal d) const { return fromFixed((int) (val*d)); }
    int val;
};
Q_DECLARE_TYPEINFO(QFixed, Q_PRIMITIVE_TYPE);

#define QFIXED_MAX (INT_MAX/256)

constexpr inline int qRound(const QFixed &f) { return f.toInt(); }
constexpr inline int qFloor(const QFixed &f) { return f.floor().truncate(); }

constexpr inline QFixed operator*(int i, const QFixed &d) { return d*i; }
constexpr inline QFixed operator+(int i, const QFixed &d) { return d+i; }
constexpr inline QFixed operator-(int i, const QFixed &d) { return -(d-i); }
constexpr inline QFixed operator*(uint i, const QFixed &d) { return d*i; }
constexpr inline QFixed operator+(uint i, const QFixed &d) { return d+i; }
constexpr inline QFixed operator-(uint i, const QFixed &d) { return -(d-i); }
// constexpr inline QFixed operator*(qreal d, const QFixed &d2) { return d2*d; }

constexpr inline bool operator==(const QFixed &f, int i) { return f.value() == i * 64; }
constexpr inline bool operator==(int i, const QFixed &f) { return f.value() == i * 64; }
constexpr inline bool operator!=(const QFixed &f, int i) { return f.value() != i * 64; }
constexpr inline bool operator!=(int i, const QFixed &f) { return f.value() != i * 64; }
constexpr inline bool operator<=(const QFixed &f, int i) { return f.value() <= i * 64; }
constexpr inline bool operator<=(int i, const QFixed &f) { return i * 64 <= f.value(); }
constexpr inline bool operator>=(const QFixed &f, int i) { return f.value() >= i * 64; }
constexpr inline bool operator>=(int i, const QFixed &f) { return i * 64 >= f.value(); }
constexpr inline bool operator<(const QFixed &f, int i) { return f.value() < i * 64; }
constexpr inline bool operator<(int i, const QFixed &f) { return i * 64 < f.value(); }
constexpr inline bool operator>(const QFixed &f, int i) { return f.value() > i * 64; }
constexpr inline bool operator>(int i, const QFixed &f) { return i * 64 > f.value(); }

#ifndef QT_NO_DEBUG_STREAM
inline QDebug &operator<<(QDebug &dbg, const QFixed &f)
{ return dbg << f.toReal(); }
#endif

struct QFixedPoint {
    QFixed x;
    QFixed y;
    constexpr inline QFixedPoint() {}
    constexpr inline QFixedPoint(const QFixed &_x, const QFixed &_y) : x(_x), y(_y) {}
    constexpr QPointF toPointF() const { return QPointF(x.toReal(), y.toReal()); }
    constexpr static QFixedPoint fromPointF(const QPointF &p) {
        return QFixedPoint(QFixed::fromReal(p.x()), QFixed::fromReal(p.y()));
    }
    constexpr inline bool operator==(const QFixedPoint &other) const
    {
        return x == other.x && y == other.y;
    }
};
Q_DECLARE_TYPEINFO(QFixedPoint, Q_PRIMITIVE_TYPE);

constexpr inline QFixedPoint operator-(const QFixedPoint &p1, const QFixedPoint &p2)
{ return QFixedPoint(p1.x - p2.x, p1.y - p2.y); }
constexpr inline QFixedPoint operator+(const QFixedPoint &p1, const QFixedPoint &p2)
{ return QFixedPoint(p1.x + p2.x, p1.y + p2.y); }

struct QFixedSize {
    QFixed width;
    QFixed height;
    constexpr QFixedSize() {}
    constexpr QFixedSize(QFixed _width, QFixed _height) : width(_width), height(_height) {}
    constexpr QSizeF toSizeF() const { return QSizeF(width.toReal(), height.toReal()); }
    constexpr static QFixedSize fromSizeF(const QSizeF &s) {
        return QFixedSize(QFixed::fromReal(s.width()), QFixed::fromReal(s.height()));
    }
};
Q_DECLARE_TYPEINFO(QFixedSize, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif // QTEXTENGINE_P_H
