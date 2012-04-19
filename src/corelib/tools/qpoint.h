/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPOINT_H
#define QPOINT_H

#include <QtCore/qnamespace.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class Q_CORE_EXPORT QPoint
{
public:
    Q_DECL_CONSTEXPR QPoint();
    Q_DECL_CONSTEXPR QPoint(int xpos, int ypos);

    Q_DECL_CONSTEXPR inline bool isNull() const;

    Q_DECL_CONSTEXPR inline int x() const;
    Q_DECL_CONSTEXPR inline int y() const;
    inline void setX(int x);
    inline void setY(int y);

    Q_DECL_CONSTEXPR inline int manhattanLength() const;

    inline int &rx();
    inline int &ry();

    inline QPoint &operator+=(const QPoint &p);
    inline QPoint &operator-=(const QPoint &p);

    inline QPoint &operator*=(float c);
    inline QPoint &operator*=(double c);
    inline QPoint &operator*=(int c);

    inline QPoint &operator/=(qreal c);

    friend Q_DECL_CONSTEXPR inline bool operator==(const QPoint &, const QPoint &);
    friend Q_DECL_CONSTEXPR inline bool operator!=(const QPoint &, const QPoint &);
    friend Q_DECL_CONSTEXPR inline const QPoint operator+(const QPoint &, const QPoint &);
    friend Q_DECL_CONSTEXPR inline const QPoint operator-(const QPoint &, const QPoint &);
    friend Q_DECL_CONSTEXPR inline const QPoint operator*(const QPoint &, float);
    friend Q_DECL_CONSTEXPR inline const QPoint operator*(float, const QPoint &);
    friend Q_DECL_CONSTEXPR inline const QPoint operator*(const QPoint &, double);
    friend Q_DECL_CONSTEXPR inline const QPoint operator*(double, const QPoint &);
    friend Q_DECL_CONSTEXPR inline const QPoint operator*(const QPoint &, int);
    friend Q_DECL_CONSTEXPR inline const QPoint operator*(int, const QPoint &);
    friend Q_DECL_CONSTEXPR inline const QPoint operator-(const QPoint &);
    friend Q_DECL_CONSTEXPR inline const QPoint operator/(const QPoint &, qreal);

private:
    friend class QTransform;
    int xp;
    int yp;
};

Q_DECLARE_TYPEINFO(QPoint, Q_MOVABLE_TYPE);

/*****************************************************************************
  QPoint stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QPoint &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QPoint &);
#endif

/*****************************************************************************
  QPoint inline functions
 *****************************************************************************/

Q_DECL_CONSTEXPR inline QPoint::QPoint() : xp(0), yp(0) {}

Q_DECL_CONSTEXPR inline QPoint::QPoint(int xpos, int ypos) : xp(xpos), yp(ypos) {}

Q_DECL_CONSTEXPR inline bool QPoint::isNull() const
{ return xp == 0 && yp == 0; }

Q_DECL_CONSTEXPR inline int QPoint::x() const
{ return xp; }

Q_DECL_CONSTEXPR inline int QPoint::y() const
{ return yp; }

inline void QPoint::setX(int xpos)
{ xp = xpos; }

inline void QPoint::setY(int ypos)
{ yp = ypos; }

inline int Q_DECL_CONSTEXPR QPoint::manhattanLength() const
{ return qAbs(x())+qAbs(y()); }

inline int &QPoint::rx()
{ return xp; }

inline int &QPoint::ry()
{ return yp; }

inline QPoint &QPoint::operator+=(const QPoint &p)
{ xp+=p.xp; yp+=p.yp; return *this; }

inline QPoint &QPoint::operator-=(const QPoint &p)
{ xp-=p.xp; yp-=p.yp; return *this; }

inline QPoint &QPoint::operator*=(float c)
{ xp = qRound(xp*c); yp = qRound(yp*c); return *this; }

inline QPoint &QPoint::operator*=(double c)
{ xp = qRound(xp*c); yp = qRound(yp*c); return *this; }

inline QPoint &QPoint::operator*=(int c)
{ xp = xp*c; yp = yp*c; return *this; }

Q_DECL_CONSTEXPR inline bool operator==(const QPoint &p1, const QPoint &p2)
{ return p1.xp == p2.xp && p1.yp == p2.yp; }

Q_DECL_CONSTEXPR inline bool operator!=(const QPoint &p1, const QPoint &p2)
{ return p1.xp != p2.xp || p1.yp != p2.yp; }

Q_DECL_CONSTEXPR inline const QPoint operator+(const QPoint &p1, const QPoint &p2)
{ return QPoint(p1.xp+p2.xp, p1.yp+p2.yp); }

Q_DECL_CONSTEXPR inline const QPoint operator-(const QPoint &p1, const QPoint &p2)
{ return QPoint(p1.xp-p2.xp, p1.yp-p2.yp); }

Q_DECL_CONSTEXPR inline const QPoint operator*(const QPoint &p, float c)
{ return QPoint(qRound(p.xp*c), qRound(p.yp*c)); }

Q_DECL_CONSTEXPR inline const QPoint operator*(const QPoint &p, double c)
{ return QPoint(qRound(p.xp*c), qRound(p.yp*c)); }

Q_DECL_CONSTEXPR inline const QPoint operator*(const QPoint &p, int c)
{ return QPoint(p.xp*c, p.yp*c); }

Q_DECL_CONSTEXPR inline const QPoint operator*(float c, const QPoint &p)
{ return QPoint(qRound(p.xp*c), qRound(p.yp*c)); }

Q_DECL_CONSTEXPR inline const QPoint operator*(double c, const QPoint &p)
{ return QPoint(qRound(p.xp*c), qRound(p.yp*c)); }

Q_DECL_CONSTEXPR inline const QPoint operator*(int c, const QPoint &p)
{ return QPoint(p.xp*c, p.yp*c); }

Q_DECL_CONSTEXPR inline const QPoint operator-(const QPoint &p)
{ return QPoint(-p.xp, -p.yp); }

inline QPoint &QPoint::operator/=(qreal c)
{
    xp = qRound(xp/c);
    yp = qRound(yp/c);
    return *this;
}

Q_DECL_CONSTEXPR inline const QPoint operator/(const QPoint &p, qreal c)
{
    return QPoint(qRound(p.xp/c), qRound(p.yp/c));
}

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QPoint &);
#endif





class Q_CORE_EXPORT QPointF
{
public:
    Q_DECL_CONSTEXPR QPointF();
    Q_DECL_CONSTEXPR QPointF(const QPoint &p);
    Q_DECL_CONSTEXPR QPointF(qreal xpos, qreal ypos);

    Q_DECL_CONSTEXPR inline qreal manhattanLength() const;

    inline bool isNull() const;

    Q_DECL_CONSTEXPR inline qreal x() const;
    Q_DECL_CONSTEXPR inline qreal y() const;
    inline void setX(qreal x);
    inline void setY(qreal y);

    inline qreal &rx();
    inline qreal &ry();

    inline QPointF &operator+=(const QPointF &p);
    inline QPointF &operator-=(const QPointF &p);
    inline QPointF &operator*=(qreal c);
    inline QPointF &operator/=(qreal c);

    friend Q_DECL_CONSTEXPR inline bool operator==(const QPointF &, const QPointF &);
    friend Q_DECL_CONSTEXPR inline bool operator!=(const QPointF &, const QPointF &);
    friend Q_DECL_CONSTEXPR inline const QPointF operator+(const QPointF &, const QPointF &);
    friend Q_DECL_CONSTEXPR inline const QPointF operator-(const QPointF &, const QPointF &);
    friend Q_DECL_CONSTEXPR inline const QPointF operator*(qreal, const QPointF &);
    friend Q_DECL_CONSTEXPR inline const QPointF operator*(const QPointF &, qreal);
    friend Q_DECL_CONSTEXPR inline const QPointF operator-(const QPointF &);
    friend Q_DECL_CONSTEXPR inline const QPointF operator/(const QPointF &, qreal);

    Q_DECL_CONSTEXPR QPoint toPoint() const;

private:
    friend class QMatrix;
    friend class QTransform;

    qreal xp;
    qreal yp;
};

Q_DECLARE_TYPEINFO(QPointF, Q_MOVABLE_TYPE);

/*****************************************************************************
  QPointF stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QPointF &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QPointF &);
#endif

/*****************************************************************************
  QPointF inline functions
 *****************************************************************************/

Q_DECL_CONSTEXPR inline QPointF::QPointF() : xp(0), yp(0) { }

Q_DECL_CONSTEXPR inline QPointF::QPointF(qreal xpos, qreal ypos) : xp(xpos), yp(ypos) { }

Q_DECL_CONSTEXPR inline QPointF::QPointF(const QPoint &p) : xp(p.x()), yp(p.y()) { }

Q_DECL_CONSTEXPR inline qreal QPointF::manhattanLength() const
{
    return qAbs(x())+qAbs(y());
}

inline bool QPointF::isNull() const
{
    return qIsNull(xp) && qIsNull(yp);
}

Q_DECL_CONSTEXPR inline qreal QPointF::x() const
{
    return xp;
}

Q_DECL_CONSTEXPR inline qreal QPointF::y() const
{
    return yp;
}

inline void QPointF::setX(qreal xpos)
{
    xp = xpos;
}

inline void QPointF::setY(qreal ypos)
{
    yp = ypos;
}

inline qreal &QPointF::rx()
{
    return xp;
}

inline qreal &QPointF::ry()
{
    return yp;
}

inline QPointF &QPointF::operator+=(const QPointF &p)
{
    xp+=p.xp;
    yp+=p.yp;
    return *this;
}

inline QPointF &QPointF::operator-=(const QPointF &p)
{
    xp-=p.xp; yp-=p.yp; return *this;
}

inline QPointF &QPointF::operator*=(qreal c)
{
    xp*=c; yp*=c; return *this;
}

Q_DECL_CONSTEXPR inline bool operator==(const QPointF &p1, const QPointF &p2)
{
    return qFuzzyIsNull(p1.xp - p2.xp) && qFuzzyIsNull(p1.yp - p2.yp);
}

Q_DECL_CONSTEXPR inline bool operator!=(const QPointF &p1, const QPointF &p2)
{
    return !qFuzzyIsNull(p1.xp - p2.xp) || !qFuzzyIsNull(p1.yp - p2.yp);
}

Q_DECL_CONSTEXPR inline const QPointF operator+(const QPointF &p1, const QPointF &p2)
{
    return QPointF(p1.xp+p2.xp, p1.yp+p2.yp);
}

Q_DECL_CONSTEXPR inline const QPointF operator-(const QPointF &p1, const QPointF &p2)
{
    return QPointF(p1.xp-p2.xp, p1.yp-p2.yp);
}

Q_DECL_CONSTEXPR inline const QPointF operator*(const QPointF &p, qreal c)
{
    return QPointF(p.xp*c, p.yp*c);
}

Q_DECL_CONSTEXPR inline const QPointF operator*(qreal c, const QPointF &p)
{
    return QPointF(p.xp*c, p.yp*c);
}

Q_DECL_CONSTEXPR inline const QPointF operator-(const QPointF &p)
{
    return QPointF(-p.xp, -p.yp);
}

inline QPointF &QPointF::operator/=(qreal c)
{
    xp/=c;
    yp/=c;
    return *this;
}

Q_DECL_CONSTEXPR inline const QPointF operator/(const QPointF &p, qreal c)
{
    return QPointF(p.xp/c, p.yp/c);
}

Q_DECL_CONSTEXPR inline QPoint QPointF::toPoint() const
{
    return QPoint(qRound(xp), qRound(yp));
}

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug d, const QPointF &p);
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPOINT_H
