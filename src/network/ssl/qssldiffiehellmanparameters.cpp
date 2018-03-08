/****************************************************************************
**
** Copyright (C) 2015 Mikkel Krautz <mikkel@krautz.dk>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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


/*!
    \class QSslDiffieHellmanParameters
    \brief The QSslDiffieHellmanParameters class provides an interface for Diffie-Hellman parameters for servers.
    \since 5.8

    \reentrant
    \ingroup network
    \ingroup ssl
    \ingroup shared
    \inmodule QtNetwork

    QSslDiffieHellmanParameters provides an interface for setting Diffie-Hellman parameters to servers based on QSslSocket.

    \sa QSslSocket, QSslCipher, QSslConfiguration
*/

#include "qssldiffiehellmanparameters.h"
#include "qssldiffiehellmanparameters_p.h"
#include "qsslsocket.h"
#include "qsslsocket_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qatomic.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qbytearraymatcher.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

// The 1024-bit MODP group from RFC 2459 (Second Oakley Group)
Q_AUTOTEST_EXPORT const char *qssl_dhparams_default_base64 =
    "MIGHAoGBAP//////////yQ/aoiFowjTExmKLgNwc0SkCTgiKZ8x0Agu+pjsTmyJR"
    "Sgh5jjQE3e+VGbPNOkMbMCsKbfJfFDdP4TVtbVHCReSFtXZiXn7G9ExC6aY37WsL"
    "/1y29Aa37e44a/taiZ+lrp8kEXxLH+ZJKGZR7OZTgf//////////AgEC";

/*!
    Returns the default QSslDiffieHellmanParameters used by QSslSocket.

    This is currently the 1024-bit MODP group from RFC 2459, also
    known as the Second Oakley Group.
*/
QSslDiffieHellmanParameters QSslDiffieHellmanParameters::defaultParameters()
{
    QSslDiffieHellmanParameters def;
    def.d->derData = QByteArray::fromBase64(QByteArray(qssl_dhparams_default_base64));
    return def;
}

/*!
    Constructs an empty QSslDiffieHellmanParameters instance.

    If an empty QSslDiffieHellmanParameters instance is set on a
    QSslConfiguration object, Diffie-Hellman negotiation will
    be disabled.

    \sa isValid()
    \sa QSslConfiguration
*/
QSslDiffieHellmanParameters::QSslDiffieHellmanParameters()
    : d(new QSslDiffieHellmanParametersPrivate)
{
    d->ref.ref();
}

/*!
    Constructs a QSslDiffieHellmanParameters object using
    the byte array \a encoded in either PEM or DER form as specified by \a encoding.

    Use the isValid() method on the returned object to
    check whether the Diffie-Hellman parameters were valid and
    loaded correctly.

    \sa isValid()
    \sa QSslConfiguration
*/
QSslDiffieHellmanParameters QSslDiffieHellmanParameters::fromEncoded(const QByteArray &encoded, QSsl::EncodingFormat encoding)
{
    QSslDiffieHellmanParameters result;
    switch (encoding) {
    case QSsl::Der:
        result.d->decodeDer(encoded);
        break;
    case QSsl::Pem:
        result.d->decodePem(encoded);
        break;
    }
    return result;
}

/*!
    Constructs a QSslDiffieHellmanParameters object by
    reading from \a device in either PEM or DER form as specified by \a encoding.

    Use the isValid() method on the returned object
    to check whether the Diffie-Hellman parameters were valid
    and loaded correctly.

    In particular, if \a device is \c nullptr or not open for reading, an invalid
    object will be returned.

    \sa isValid()
    \sa QSslConfiguration
*/
QSslDiffieHellmanParameters QSslDiffieHellmanParameters::fromEncoded(QIODevice *device, QSsl::EncodingFormat encoding)
{
    if (device)
        return fromEncoded(device->readAll(), encoding);
    else
        return QSslDiffieHellmanParameters();
}

/*!
    Constructs an identical copy of \a other.
*/
QSslDiffieHellmanParameters::QSslDiffieHellmanParameters(const QSslDiffieHellmanParameters &other)
    : d(other.d)
{
    if (d)
        d->ref.ref();
}

/*!
    \fn QSslDiffieHellmanParameters::QSslDiffieHellmanParameters(QSslDiffieHellmanParameters &&other)

    Move-constructs from \a other.

    \note The moved-from object \a other is placed in a partially-formed state, in which
    the only valid operations are destruction and assignment of a new value.
*/

/*!
    Destroys the QSslDiffieHellmanParameters object.
*/
QSslDiffieHellmanParameters::~QSslDiffieHellmanParameters()
{
    if (d && !d->ref.deref())
        delete d;
}

/*!
    Copies the contents of \a other into this QSslDiffieHellmanParameters, making the two QSslDiffieHellmanParameters
    identical.

    Returns a reference to this QSslDiffieHellmanParameters.
*/
QSslDiffieHellmanParameters &QSslDiffieHellmanParameters::operator=(const QSslDiffieHellmanParameters &other)
{
    QSslDiffieHellmanParameters copy(other);
    swap(copy);
    return *this;
}

/*!
    \fn QSslDiffieHellmanParameters &QSslDiffieHellmanParameters::operator=(QSslDiffieHellmanParameters &&other)

    Move-assigns \a other to this QSslDiffieHellmanParameters instance.

    \note The moved-from object \a other is placed in a partially-formed state, in which
    the only valid operations are destruction and assignment of a new value.
*/

/*!
    \fn void QSslDiffieHellmanParameters::swap(QSslDiffieHellmanParameters &other)

    Swaps this QSslDiffieHellmanParameters with \a other. This function is very fast and
    never fails.
*/

/*!
    Returns \c true if this is a an empty QSslDiffieHellmanParameters instance.

    Setting an empty QSslDiffieHellmanParameters instance on a QSslSocket-based
    server will disable Diffie-Hellman key exchange.
*/
bool QSslDiffieHellmanParameters::isEmpty() const Q_DECL_NOTHROW
{
    return d->derData.isNull() && d->error == QSslDiffieHellmanParameters::NoError;
}

/*!
    Returns \c true if this is a valid QSslDiffieHellmanParameters; otherwise false.

    This method should be used after constructing a QSslDiffieHellmanParameters
    object to determine its validity.

    If a QSslDiffieHellmanParameters object is not valid, you can use the error()
    method to determine what error prevented the object from being constructed.

    \sa error()
*/
bool QSslDiffieHellmanParameters::isValid() const Q_DECL_NOTHROW
{
    return d->error == QSslDiffieHellmanParameters::NoError;
}

/*!
    \enum QSslDiffieHellmanParameters::Error

    Describes a QSslDiffieHellmanParameters error.

    \value NoError               No error occurred.

    \value InvalidInputDataError The given input data could not be used to
                                 construct a QSslDiffieHellmanParameters
                                 object.

    \value UnsafeParametersError The Diffie-Hellman parameters are unsafe
                                 and should not be used.
*/

/*!
    Returns the error that caused the QSslDiffieHellmanParameters object
    to be invalid.
*/
QSslDiffieHellmanParameters::Error QSslDiffieHellmanParameters::error() const Q_DECL_NOTHROW
{
    return d->error;
}

/*!
    Returns a human-readable description of the error that caused the
    QSslDiffieHellmanParameters object to be invalid.
*/
QString QSslDiffieHellmanParameters::errorString() const Q_DECL_NOTHROW
{
    switch (d->error) {
    case QSslDiffieHellmanParameters::NoError:
        return QCoreApplication::translate("QSslDiffieHellmanParameter", "No error");
    case QSslDiffieHellmanParameters::InvalidInputDataError:
        return QCoreApplication::translate("QSslDiffieHellmanParameter", "Invalid input data");
    case QSslDiffieHellmanParameters::UnsafeParametersError:
        return QCoreApplication::translate("QSslDiffieHellmanParameter", "The given Diffie-Hellman parameters are deemed unsafe");
    }

    Q_UNREACHABLE();
    return QString();
}

/*!
    \since 5.8
    \relates QSslDiffieHellmanParameters

    Returns \c true if \a lhs is equal to \a rhs; otherwise returns \c false.
*/
bool operator==(const QSslDiffieHellmanParameters &lhs, const QSslDiffieHellmanParameters &rhs) Q_DECL_NOTHROW
{
    return lhs.d->derData == rhs.d->derData;
}

#ifndef QT_NO_DEBUG_STREAM
/*!
    \since 5.8
    \relates QSslDiffieHellmanParameters

    Writes the set of Diffie-Hellman parameters in \a dhparam into the debug object \a debug for
    debugging purposes.

    The Diffie-Hellman parameters will be represented in Base64-encoded DER form.

    \sa {Debugging Techniques}
*/
QDebug operator<<(QDebug debug, const QSslDiffieHellmanParameters &dhparam)
{
    QDebugStateSaver saver(debug);
    debug.resetFormat().nospace();
    debug << "QSslDiffieHellmanParameters(" << dhparam.d->derData.toBase64() << ')';
    return debug;
}
#endif

/*!
    \since 5.8
    \relates QSslDiffieHellmanParameters

    Returns an hash value for \a dhparam, using \a seed to seed
    the calculation.
*/
uint qHash(const QSslDiffieHellmanParameters &dhparam, uint seed) Q_DECL_NOTHROW
{
    return qHash(dhparam.d->derData, seed);
}

QT_END_NAMESPACE
