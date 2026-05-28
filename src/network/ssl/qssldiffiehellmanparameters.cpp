// Copyright (C) 2015 Mikkel Krautz <mikkel@krautz.dk>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


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
#include "qtlsbackend_p.h"
#include "qsslsocket.h"
#include "qsslsocket_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qatomic.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qbytearraymatcher.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

// The 2048-bit MODP group from RFC 3526
Q_AUTOTEST_EXPORT extern const char qssl_dhparams_default_base64[] =
    "MIIBCAKCAQEA///////////JD9qiIWjCNMTGYouA3BzRKQJOCIpnzHQCC76mOxObIlFKCHmO"
    "NATd75UZs806QxswKwpt8l8UN0/hNW1tUcJF5IW1dmJefsb0TELppjftawv/XLb0Brft7jhr"
    "+1qJn6WunyQRfEsf5kkoZlHs5Fs9wgB8uKFjvwWY2kg2HFXTmmkWP6j9JM9fg2VdI9yjrZYc"
    "YvNWIIVSu57VKQdwlpZtZww1Tkq8mATxdGwIyhghfDKQXkYuNs474553LBgOhgObJ4Oi7Aei"
    "j7XFXfBvTFLJ3ivL9pVYFxg5lUl86pVq5RXSJhiY+gUQFXKOWoqsqmj//////////wIBAg==";

/*!
    Returns the default QSslDiffieHellmanParameters used by QSslSocket.

    This is currently the 2048-bit MODP group from RFC 3526.
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
    const auto *tlsBackend = QSslSocketPrivate::tlsBackendInUse();
    if (!tlsBackend)
        return result;
    switch (encoding) {
    case QSsl::Der:
        result.d->initFromDer(encoded);
        break;
    case QSsl::Pem:
        result.d->initFromPem(encoded);
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

    In particular, if \a device is \nullptr or not open for reading, an invalid
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
    \memberswap{QSslDiffieHellmanParameters}
*/

/*!
    Returns \c true if this is a an empty QSslDiffieHellmanParameters instance.

    Setting an empty QSslDiffieHellmanParameters instance on a QSslSocket-based
    server will disable Diffie-Hellman key exchange.
*/
bool QSslDiffieHellmanParameters::isEmpty() const noexcept
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
bool QSslDiffieHellmanParameters::isValid() const noexcept
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
QSslDiffieHellmanParameters::Error QSslDiffieHellmanParameters::error() const noexcept
{
    return d->error;
}

/*!
    Returns a human-readable description of the error that caused the
    QSslDiffieHellmanParameters object to be invalid.
*/
QString QSslDiffieHellmanParameters::errorString() const noexcept
{
    switch (d->error) {
    case QSslDiffieHellmanParameters::NoError:
        return QCoreApplication::translate("QSslDiffieHellmanParameter", "No error");
    case QSslDiffieHellmanParameters::InvalidInputDataError:
        return QCoreApplication::translate("QSslDiffieHellmanParameter", "Invalid input data");
    case QSslDiffieHellmanParameters::UnsafeParametersError:
        return QCoreApplication::translate("QSslDiffieHellmanParameter", "The given Diffie-Hellman parameters are deemed unsafe");
    }

    Q_UNREACHABLE_RETURN(QString());
}

/*!
    \fn bool QSslDiffieHellmanParameters::operator==(const QSslDiffieHellmanParameters &lhs, const QSslDiffieHellmanParameters &rhs) noexcept
    \since 5.8

    Returns \c true if \a lhs is equal to \a rhs; otherwise returns \c false.
*/

/*!
    \fn bool QSslDiffieHellmanParameters::operator!=(const QSslDiffieHellmanParameters &lhs, const QSslDiffieHellmanParameters &rhs) noexcept
    \since 5.8

    Returns \c true if \a lhs is not equal to \a rhs; otherwise returns \c false.
*/

/*!
    \internal
*/
bool QSslDiffieHellmanParameters::isEqual(const QSslDiffieHellmanParameters &other) const noexcept
{
    return d->derData == other.d->derData;
}

/*!
    \internal
*/
void QSslDiffieHellmanParametersPrivate::initFromDer(const QByteArray &der)
{
    if (const auto *tlsBackend = QSslSocketPrivate::tlsBackendInUse())
        error = QSslDiffieHellmanParameters::Error(tlsBackend->dhParametersFromDer(der, &derData));
}

/*!
    \internal
*/
void QSslDiffieHellmanParametersPrivate::initFromPem(const QByteArray &pem)
{
    if (const auto *tlsBackend = QSslSocketPrivate::tlsBackendInUse())
        error = QSslDiffieHellmanParameters::Error(tlsBackend->dhParametersFromPem(pem, &derData));
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
    \fn size_t qHash(const QSslDiffieHellmanParameters &key, size_t seed)
    \since 5.8
    \qhashold{QSslDiffieHellmanParameters}
*/
size_t qHash(const QSslDiffieHellmanParameters &dhparam, size_t seed) noexcept
{
    return qHash(dhparam.d->derData, seed);
}

QT_END_NAMESPACE
