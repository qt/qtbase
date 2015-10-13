/****************************************************************************
**
** Copyright (C) 2014 Governikus GmbH & Co. KG.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include "qsslellipticcurve.h"

#ifndef QT_NO_DEBUG_STREAM
#include <QDebug>
#endif

QT_BEGIN_NAMESPACE

/*!
    \class QSslEllipticCurve
    \since 5.5

    \brief Represents an elliptic curve for use by elliptic-curve cipher algorithms.

    \reentrant
    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork

    The class QSslEllipticCurve represents an elliptic curve for use by
    elliptic-curve cipher algorithms.

    Elliptic curves can be constructed from a "short name" (SN) (fromShortName()),
    and by a call to QSslConfiguration::supportedEllipticCurves().

    QSslEllipticCurve instances can be compared for equality and can be used as keys
    in QHash and QSet. They cannot be used as key in a QMap.
*/

/*!
    \fn QSslEllipticCurve::QSslEllipticCurve()

    Constructs an invalid elliptic curve.

    \sa isValid(), QSslConfiguration::supportedEllipticCurves()
*/

/*!
    \fn QSslEllipticCurve QSslEllipticCurve::fromShortName(const QString &name)

    Returns an QSslEllipticCurve instance representing the
    named curve \a name. The \a name is the conventional short
    name for the curve, as represented by RFC 4492 (for instance \c{secp521r1}),
    or as NIST short names (for instance \c{P-256}). The actual set of
    recognized names depends on the SSL implementation.

    If the given \a name is not supported, returns an invalid QSslEllipticCurve instance.

    \note The OpenSSL implementation of this function treats the name case-sensitively.

    \sa shortName()
*/

/*!
    \fn QSslEllipticCurve QSslEllipticCurve::fromLongName(const QString &name)

    Returns an QSslEllipticCurve instance representing the named curve \a name.
    The \a name is a long name for the curve, whose exact spelling depends on the
    SSL implementation.

    If the given \a name is not supported, returns an invalid QSslEllipticCurve instance.

    \note The OpenSSL implementation of this function treats the name case-sensitively.

    \sa longName()
*/

/*!
    \fn QString QSslEllipticCurve::shortName() const

    Returns the conventional short name for this curve. If this
    curve is invalid, returns an empty string.

    \sa longName()
*/

/*!
    \fn QString QSslEllipticCurve::longName() const

    Returns the conventional long name for this curve. If this
    curve is invalid, returns an empty string.

    \sa shortName()
*/

/*!
    \fn bool QSslEllipticCurve::isValid() const

    Returns true if this elliptic curve is a valid curve, false otherwise.
*/

/*!
    \fn bool QSslEllipticCurve::isTlsNamedCurve() const

    Returns true if this elliptic curve is one of the named curves that can be
    used in the key exchange when using an elliptic curve cipher with TLS;
    false otherwise.
*/

/*!
    \fn bool operator==(QSslEllipticCurve lhs, QSslEllipticCurve rhs)
    \since 5.5
    \relates QSslEllipticCurve

    Returns true if the curve \a lhs represents the same curve of \a rhs;
*/

/*!
    \fn bool operator!=(QSslEllipticCurve lhs, QSslEllipticCurve rhs)
    \since 5.5
    \relates QSslEllipticCurve

    Returns true if the curve \a lhs represents a different curve than \a rhs;
    false otherwise.
*/

/*!
    \fn uint qHash(QSslEllipticCurve curve, uint seed)
    \since 5.5
    \relates QHash

    Returns an hash value for the curve \a curve, using \a seed to seed
    the calculation.
*/

#ifndef QT_NO_DEBUG_STREAM
/*!
    \relates QSslEllipticCurve
    \since 5.5

    Writes the elliptic curve \a curve into the debug object \a debug for
    debugging purposes.

    \sa {Debugging Techniques}
*/
QDebug operator<<(QDebug debug, QSslEllipticCurve curve)
{
    QDebugStateSaver saver(debug);
    debug.resetFormat().nospace();
    debug << "QSslEllipticCurve(" << curve.shortName() << ')';
    return debug;
}
#endif

QT_END_NAMESPACE
