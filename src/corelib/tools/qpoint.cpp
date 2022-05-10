// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpoint.h"
#include "qdatastream.h"

#include <private/qdebug_p.h>
#include <QtCore/qhashfunctions.h>

QT_BEGIN_NAMESPACE

/*!
    \class QPoint
    \inmodule QtCore
    \ingroup painting
    \reentrant

    \brief The QPoint class defines a point in the plane using integer
    precision.

    A point is specified by a x coordinate and an y coordinate which
    can be accessed using the x() and y() functions. The isNull()
    function returns \c true if both x and y are set to 0. The
    coordinates can be set (or altered) using the setX() and setY()
    functions, or alternatively the rx() and ry() functions which
    return references to the coordinates (allowing direct
    manipulation).

    Given a point \e p, the following statements are all equivalent:

    \snippet code/src_corelib_tools_qpoint.cpp 0

    A QPoint object can also be used as a vector: Addition and
    subtraction are defined as for vectors (each component is added
    separately). A QPoint object can also be divided or multiplied by
    an \c int or a \c qreal.

    In addition, the QPoint class provides the manhattanLength()
    function which gives an inexpensive approximation of the length of
    the QPoint object interpreted as a vector. Finally, QPoint objects
    can be streamed as well as compared.

    \sa QPointF, QPolygon
*/


/*****************************************************************************
  QPoint member functions
 *****************************************************************************/

/*!
    \fn QPoint::QPoint()

    Constructs a null point, i.e. with coordinates (0, 0)

    \sa isNull()
*/

/*!
    \fn QPoint::QPoint(int xpos, int ypos)

    Constructs a point with the given coordinates (\a xpos, \a ypos).

    \sa setX(), setY()
*/

/*!
    \fn bool QPoint::isNull() const

    Returns \c true if both the x and y coordinates are set to 0,
    otherwise returns \c false.
*/

/*!
    \fn int QPoint::x() const

    Returns the x coordinate of this point.

    \sa setX(), rx()
*/

/*!
    \fn int QPoint::y() const

    Returns the y coordinate of this point.

    \sa setY(), ry()
*/

/*!
    \fn void QPoint::setX(int x)

    Sets the x coordinate of this point to the given \a x coordinate.

    \sa x(), setY()
*/

/*!
    \fn void QPoint::setY(int y)

    Sets the y coordinate of this point to the given \a y coordinate.

    \sa y(), setX()
*/

/*!
    \fn QPoint::transposed() const
    \since 5.14

    Returns a point with x and y coordinates exchanged:
    \code
    QPoint{1, 2}.transposed() // {2, 1}
    \endcode

    \sa x(), y(), setX(), setY()
*/

/*!
    \fn int &QPoint::rx()

    Returns a reference to the x coordinate of this point.

    Using a reference makes it possible to directly manipulate x. For example:

    \snippet code/src_corelib_tools_qpoint.cpp 1

    \sa x(), setX()
*/

/*!
    \fn int &QPoint::ry()

    Returns a reference to the y coordinate of this point.

    Using a reference makes it possible to directly manipulate y. For
    example:

    \snippet code/src_corelib_tools_qpoint.cpp 2

    \sa y(), setY()
*/


/*!
    \fn QPoint &QPoint::operator+=(const QPoint &point)

    Adds the given \a point to this point and returns a reference to
    this point. For example:

    \snippet code/src_corelib_tools_qpoint.cpp 3

    \sa operator-=()
*/

/*!
    \fn QPoint &QPoint::operator-=(const QPoint &point)

    Subtracts the given \a point from this point and returns a
    reference to this point. For example:

    \snippet code/src_corelib_tools_qpoint.cpp 4

    \sa operator+=()
*/

/*!
    \fn QPoint &QPoint::operator*=(float factor)

    Multiplies this point's coordinates by the given \a factor, and
    returns a reference to this point.

    Note that the result is rounded to the nearest integer as points are held as
    integers. Use QPointF for floating point accuracy.

    \sa operator/=()
*/

/*!
    \fn QPoint &QPoint::operator*=(double factor)

    Multiplies this point's coordinates by the given \a factor, and
    returns a reference to this point. For example:

    \snippet code/src_corelib_tools_qpoint.cpp 5

    Note that the result is rounded to the nearest integer as points are held as
    integers. Use QPointF for floating point accuracy.

    \sa operator/=()
*/

/*!
    \fn QPoint &QPoint::operator*=(int factor)

    Multiplies this point's coordinates by the given \a factor, and
    returns a reference to this point.

    \sa operator/=()
*/

/*!
    \fn static int QPoint::dotProduct(const QPoint &p1, const QPoint &p2)
    \since 5.1

    \snippet code/src_corelib_tools_qpoint.cpp 16

    Returns the dot product of \a p1 and \a p2.
*/

/*!
    \fn bool QPoint::operator==(const QPoint &p1, const QPoint &p2)

    Returns \c true if \a p1 and \a p2 are equal; otherwise returns
    false.
*/

/*!
    \fn bool QPoint::operator!=(const QPoint &p1, const QPoint &p2)

    Returns \c true if \a p1 and \a p2 are not equal; otherwise returns \c false.
*/

/*!
    \fn QPoint QPoint::operator+(const QPoint &p1, const QPoint &p2)

    Returns a QPoint object that is the sum of the given points, \a p1
    and \a p2; each component is added separately.

    \sa QPoint::operator+=()
*/

/*!
    \fn Point QPoint::operator-(const QPoint &p1, const QPoint &p2)

    Returns a QPoint object that is formed by subtracting \a p2 from
    \a p1; each component is subtracted separately.

    \sa QPoint::operator-=()
*/

/*!
    \fn QPoint QPoint::operator*(const QPoint &point, float factor)

    Returns a copy of the given \a point multiplied by the given \a factor.

    Note that the result is rounded to the nearest integer as points
    are held as integers. Use QPointF for floating point accuracy.

    \sa QPoint::operator*=()
*/

/*!
    \fn QPoint QPoint::operator*(const QPoint &point, double factor)

    Returns a copy of the given \a point multiplied by the given \a factor.

    Note that the result is rounded to the nearest integer as points
    are held as integers. Use QPointF for floating point accuracy.

    \sa QPoint::operator*=()
*/

/*!
    \fn QPoint QPoint::operator*(const QPoint &point, int factor)

    Returns a copy of the given \a point multiplied by the given \a factor.

    \sa QPoint::operator*=()
*/

/*!
    \fn QPoint QPoint::operator*(float factor, const QPoint &point)
    \overload

    Returns a copy of the given \a point multiplied by the given \a factor.

    Note that the result is rounded to the nearest integer as points
    are held as integers. Use QPointF for floating point accuracy.

    \sa QPoint::operator*=()
*/

/*!
    \fn QPoint QPoint::operator*(double factor, const QPoint &point)
    \overload

    Returns a copy of the given \a point multiplied by the given \a factor.

    Note that the result is rounded to the nearest integer as points
    are held as integers. Use QPointF for floating point accuracy.

    \sa QPoint::operator*=()
*/

/*!
    \fn QPoint QPoint::operator*(int factor, const QPoint &point)
    \overload

    Returns a copy of the given \a point multiplied by the given \a factor.

    \sa QPoint::operator*=()
*/

/*!
    \fn QPoint QPoint::operator+(const QPoint &point)
    \since 5.0

    Returns \a point unmodified.
*/

/*!
    \fn QPoint QPoint::operator-(const QPoint &point)
    \overload

    Returns a QPoint object that is formed by changing the sign of
    both components of the given \a point.

    Equivalent to \c{QPoint(0,0) - point}.
*/

/*!
    \fn QPoint &QPoint::operator/=(qreal divisor)
    \overload

    Divides both x and y by the given \a divisor, and returns a reference to this
    point. For example:

    \snippet code/src_corelib_tools_qpoint.cpp 6

    Note that the result is rounded to the nearest integer as points are held as
    integers. Use QPointF for floating point accuracy.

    \sa operator*=()
*/

/*!
    \fn const QPoint QPoint::operator/(const QPoint &point, qreal divisor)

    Returns the QPoint formed by dividing both components of the given \a point
    by the given \a divisor.

    Note that the result is rounded to the nearest integer as points are held as
    integers. Use QPointF for floating point accuracy.

    \sa QPoint::operator/=()
*/

/*!
    \fn QPoint::toPointF() const
    \since 6.4

    Returns this point as a point with floating point accuracy.

    \sa QPointF::toPoint()
*/

/*****************************************************************************
  QPoint stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QPoint &point)
    \relates QPoint

    Writes the given \a point to the given \a stream and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &s, const QPoint &p)
{
    if (s.version() == 1)
        s << (qint16)p.x() << (qint16)p.y();
    else
        s << (qint32)p.x() << (qint32)p.y();
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QPoint &point)
    \relates QPoint

    Reads a point from the given \a stream into the given \a point
    and returns a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &s, QPoint &p)
{
    if (s.version() == 1) {
        qint16 x, y;
        s >> x;  p.rx() = x;
        s >> y;  p.ry() = y;
    }
    else {
        qint32 x, y;
        s >> x;  p.rx() = x;
        s >> y;  p.ry() = y;
    }
    return s;
}

#endif // QT_NO_DATASTREAM
/*!
    \fn int QPoint::manhattanLength() const

    Returns the sum of the absolute values of x() and y(),
    traditionally known as the "Manhattan length" of the vector from
    the origin to the point. For example:

    \snippet code/src_corelib_tools_qpoint.cpp 7

    This is a useful, and quick to calculate, approximation to the
    true length:

    \snippet code/src_corelib_tools_qpoint.cpp 8

    The tradition of "Manhattan length" arises because such distances
    apply to travelers who can only travel on a rectangular grid, like
    the streets of Manhattan.
*/

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QPoint &p)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QPoint" << '(';
    QtDebugUtils::formatQPoint(dbg, p);
    dbg << ')';
    return dbg;
}

QDebug operator<<(QDebug dbg, const QPointF &p)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QPointF" << '(';
    QtDebugUtils::formatQPoint(dbg, p);
    dbg << ')';
    return dbg;
}
#endif

/*!
    \fn size_t qHash(QPoint key, size_t seed = 0)
    \relates QHash
    \since 6.0

    Returns the hash value for the \a key, using \a seed to seed the
    calculation.
*/
size_t qHash(QPoint key, size_t seed) noexcept
{
    return qHashMulti(seed, key.x(), key.y());
}

/*!
    \class QPointF
    \inmodule QtCore
    \ingroup painting
    \reentrant

    \brief The QPointF class defines a point in the plane using
    floating point precision.

    A point is specified by a x coordinate and an y coordinate which
    can be accessed using the x() and y() functions. The coordinates
    of the point are specified using finite floating point numbers for
    accuracy. The isNull() function returns \c true if both x and y are
    set to 0.0. The coordinates can be set (or altered) using the setX()
    and setY() functions, or alternatively the rx() and ry() functions which
    return references to the coordinates (allowing direct
    manipulation).

    Given a point \e p, the following statements are all equivalent:

    \snippet code/src_corelib_tools_qpoint.cpp 9

    A QPointF object can also be used as a vector: Addition and
    subtraction are defined as for vectors (each component is added
    separately). A QPointF object can also be divided or multiplied by
    an \c int or a \c qreal.

    In addition, the QPointF class provides a constructor converting a
    QPoint object into a QPointF object, and a corresponding toPoint()
    function which returns a QPoint copy of \e this point. Finally,
    QPointF objects can be streamed as well as compared.

    \sa QPoint, QPolygonF
*/

/*!
    \fn QPointF::QPointF()

    Constructs a null point, i.e. with coordinates (0.0, 0.0)

    \sa isNull()
*/

/*!
    \fn QPointF::QPointF(const QPoint &point)

    Constructs a copy of the given \a point.

    \sa toPoint(), QPoint::toPointF()
*/

/*!
    \fn QPointF::QPointF(qreal xpos, qreal ypos)

    Constructs a point with the given coordinates (\a xpos, \a ypos).

    \sa setX(), setY()
*/

/*!
    \fn bool QPointF::isNull() const

    Returns \c true if both the x and y coordinates are set to 0.0 (ignoring
    the sign); otherwise returns \c false.
*/


/*!
    \fn qreal QPointF::manhattanLength() const
    \since 4.6

    Returns the sum of the absolute values of x() and y(),
    traditionally known as the "Manhattan length" of the vector from
    the origin to the point.

    \sa QPoint::manhattanLength()
*/

/*!
    \fn qreal QPointF::x() const

    Returns the x coordinate of this point.

    \sa setX(), rx()
*/

/*!
    \fn qreal QPointF::y() const

    Returns the y coordinate of this point.

    \sa setY(), ry()
*/

/*!
    \fn void QPointF::setX(qreal x)

    Sets the x coordinate of this point to the given finite \a x coordinate.

    \sa x(), setY()
*/

/*!
    \fn void QPointF::setY(qreal y)

    Sets the y coordinate of this point to the given finite \a y coordinate.

    \sa y(), setX()
*/

/*!
    \fn QPointF::transposed() const
    \since 5.14

    Returns a point with x and y coordinates exchanged:
    \code
    QPointF{1.0, 2.0}.transposed() // {2.0, 1.0}
    \endcode

    \sa x(), y(), setX(), setY()
*/

/*!
    \fn qreal& QPointF::rx()

    Returns a reference to the x coordinate of this point.

    Using a reference makes it possible to directly manipulate x. For example:

    \snippet code/src_corelib_tools_qpoint.cpp 10

    \sa x(), setX()
*/

/*!
    \fn qreal& QPointF::ry()

    Returns a reference to the y coordinate of this point.

    Using a reference makes it possible to directly manipulate y. For example:

    \snippet code/src_corelib_tools_qpoint.cpp 11

    \sa y(), setY()
*/

/*!
    \fn QPointF& QPointF::operator+=(const QPointF &point)

    Adds the given \a point to this point and returns a reference to
    this point. For example:

    \snippet code/src_corelib_tools_qpoint.cpp 12

    \sa operator-=()
*/

/*!
    \fn QPointF& QPointF::operator-=(const QPointF &point)

    Subtracts the given \a point from this point and returns a reference
    to this point. For example:

    \snippet code/src_corelib_tools_qpoint.cpp 13

    \sa operator+=()
*/

/*!
    \fn QPointF& QPointF::operator*=(qreal factor)

    Multiplies this point's coordinates by the given finite \a factor, and
    returns a reference to this point. For example:

    \snippet code/src_corelib_tools_qpoint.cpp 14

    \sa operator/=()
*/

/*!
    \fn QPointF& QPointF::operator/=(qreal divisor)

    Divides both x and y by the given \a divisor, and returns a reference
    to this point. For example:

    \snippet code/src_corelib_tools_qpoint.cpp 15

    The \a divisor must not be zero or NaN.

    \sa operator*=()
*/

/*!
    \fn QPointF QPointF::operator+(const QPointF &p1, const QPointF &p2)

    Returns a QPointF object that is the sum of the given points, \a p1
    and \a p2; each component is added separately.

    \sa QPointF::operator+=()
*/

/*!
    \fn QPointF QPointF::operator-(const QPointF &p1, const QPointF &p2)

    Returns a QPointF object that is formed by subtracting \a p2 from \a p1;
    each component is subtracted separately.

    \sa QPointF::operator-=()
*/

/*!
    \fn QPointF QPointF::operator*(const QPointF &point, qreal factor)

    Returns a copy of the given \a point, multiplied by the given finite \a factor.

    \sa QPointF::operator*=()
*/

/*!
    \fn QPointF QPointF::operator*(qreal factor, const QPointF &point)

    \overload

    Returns a copy of the given \a point, multiplied by the given finite \a factor.
*/

/*!
    \fn QPointF QPointF::operator+(const QPointF &point)
    \since 5.0

    Returns \a point unmodified.
*/

/*!
    \fn QPointF QPointF::operator-(const QPointF &point)
    \overload

    Returns a QPointF object that is formed by changing the sign of
    each component of the given \a point.

    Equivalent to \c {QPointF(0,0) - point}.
*/

/*!
    \fn QPointF QPointF::operator/(const QPointF &point, qreal divisor)

    Returns the QPointF object formed by dividing each component of
    the given \a point by the given \a divisor.

    The \a divisor must not be zero or NaN.

    \sa QPointF::operator/=()
*/

/*!
    \fn QPoint QPointF::toPoint() const

    Rounds the coordinates of this point to the nearest integer, and
    returns a QPoint object with the rounded coordinates.

    \sa QPointF(), QPoint::toPointF()
*/

/*!
    \fn static qreal QPointF::dotProduct(const QPointF &p1, const QPointF &p2)
    \since 5.1

    \snippet code/src_corelib_tools_qpoint.cpp 17

    Returns the dot product of \a p1 and \a p2.
*/

/*!
    \fn bool QPointF::operator==(const QPointF &p1, const QPointF &p2)

    Returns \c true if \a p1 is approximately equal to \a p2; otherwise
    returns \c false.

    \warning This function does not check for strict equality; instead,
    it uses a fuzzy comparison to compare the points' coordinates.

    \sa qFuzzyCompare
*/

/*!
    \fn bool QPointF::operator!=(const QPointF &p1, const QPointF &p2);

    Returns \c true if \a p1 is sufficiently different from \a p2;
    otherwise returns \c false.

    \warning This function does not check for strict inequality; instead,
    it uses a fuzzy comparison to compare the points' coordinates.

    \sa qFuzzyCompare
*/

#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QPointF &point)
    \relates QPointF

    Writes the given \a point to the given \a stream and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &s, const QPointF &p)
{
    s << double(p.x()) << double(p.y());
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QPointF &point)
    \relates QPointF

    Reads a point from the given \a stream into the given \a point
    and returns a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &s, QPointF &p)
{
    double x, y;
    s >> x;
    s >> y;
    p.setX(qreal(x));
    p.setY(qreal(y));
    return s;
}
#endif // QT_NO_DATASTREAM

QT_END_NAMESPACE
