// Copyright (C) 2014 Governikus GmbH & Co. KG.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsslellipticcurve.h"
#include "qtlsbackend_p.h"
#include "qsslsocket_p.h"

#ifndef QT_NO_DEBUG_STREAM
#include <QDebug>
#endif

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN(QSslEllipticCurve)

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

    \note This class is currently only supported in OpenSSL.
*/

/*!
    \fn QSslEllipticCurve::QSslEllipticCurve()

    Constructs an invalid elliptic curve.

    \sa isValid(), QSslConfiguration::supportedEllipticCurves()
*/

/*!
    Returns an QSslEllipticCurve instance representing the
    named curve \a name. The \a name is the conventional short
    name for the curve, as represented by RFC 4492 (for instance \c{secp521r1}),
    or as NIST short names (for instance \c{P-256}). The actual set of
    recognized names depends on the SSL implementation.

    If the given \a name is not supported, returns an invalid QSslEllipticCurve instance.

    \note The OpenSSL implementation of this function treats the name case-sensitively.

    \sa shortName()
*/
QSslEllipticCurve QSslEllipticCurve::fromShortName(const QString &name)
{
    QSslEllipticCurve result;
    if (name.isEmpty())
        return result;

    if (const auto *tlsBackend = QSslSocketPrivate::tlsBackendInUse())
        result.id = tlsBackend->curveIdFromShortName(name);

    return result;
}

/*!
    Returns an QSslEllipticCurve instance representing the named curve \a name.
    The \a name is a long name for the curve, whose exact spelling depends on the
    SSL implementation.

    If the given \a name is not supported, returns an invalid QSslEllipticCurve instance.

    \note The OpenSSL implementation of this function treats the name case-sensitively.

    \sa longName()
*/
QSslEllipticCurve QSslEllipticCurve::fromLongName(const QString &name)
{
    QSslEllipticCurve result;
    if (name.isEmpty())
        return result;

    if (const auto *tlsBackend = QSslSocketPrivate::tlsBackendInUse())
        result.id = tlsBackend->curveIdFromLongName(name);

    return result;
}

/*!
    Returns the conventional short name for this curve. If this
    curve is invalid, returns an empty string.

    \sa longName()
*/
QString QSslEllipticCurve::shortName() const
{
    QString name;

    if (const auto *tlsBackend = QSslSocketPrivate::tlsBackendInUse())
        name = tlsBackend->shortNameForId(id);

    return name;
}

/*!
    Returns the conventional long name for this curve. If this
    curve is invalid, returns an empty string.

    \sa shortName()
*/
QString QSslEllipticCurve::longName() const
{
    QString name;

    if (const auto *tlsBackend = QSslSocketPrivate::tlsBackendInUse())
        name = tlsBackend->longNameForId(id);

    return name;
}

/*!
    \fn bool QSslEllipticCurve::isValid() const

    Returns true if this elliptic curve is a valid curve, false otherwise.
*/

/*!
    Returns true if this elliptic curve is one of the named curves that can be
    used in the key exchange when using an elliptic curve cipher with TLS;
    false otherwise.
*/
bool QSslEllipticCurve::isTlsNamedCurve() const noexcept
{
    if (const auto *tlsBackend = QSslSocketPrivate::tlsBackendInUse())
        return tlsBackend->isTlsNamedCurve(id);

    return false;
}


/*!
    \fn bool QSslEllipticCurve::operator==(QSslEllipticCurve lhs, QSslEllipticCurve rhs)
    \since 5.5

    Returns true if the curve \a lhs represents the same curve of \a rhs;
*/

/*!
    \fn bool QSslEllipticCurve::operator!=(QSslEllipticCurve lhs, QSslEllipticCurve rhs)
    \since 5.5

    Returns true if the curve \a lhs represents a different curve than \a rhs;
    false otherwise.
*/

/*!
    \fn size_t qHash(QSslEllipticCurve curve, size_t seed = 0)
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
