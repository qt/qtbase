// Copyright (C) 2014 Governikus GmbH & Co. KG.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qsslpresharedkeyauthenticator.h"
#include "qsslpresharedkeyauthenticator_p.h"

#include <QSharedData>

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN(QSslPreSharedKeyAuthenticator)
QT_IMPL_METATYPE_EXTERN_TAGGED(QSslPreSharedKeyAuthenticator*, QSslPreSharedKeyAuthenticator_ptr)

/*!
    \internal
*/
QSslPreSharedKeyAuthenticatorPrivate::QSslPreSharedKeyAuthenticatorPrivate()
    : maximumIdentityLength(0),
      maximumPreSharedKeyLength(0)
{
}

/*!
    \class QSslPreSharedKeyAuthenticator

    \brief The QSslPreSharedKeyAuthenticator class provides authentication data for pre
    shared keys (PSK) ciphersuites.

    \inmodule QtNetwork

    \reentrant

    \ingroup network
    \ingroup ssl
    \ingroup shared

    \since 5.5

    The QSslPreSharedKeyAuthenticator class is used by an SSL socket to provide
    the required authentication data in a pre shared key (PSK) ciphersuite.

    In a PSK handshake, the client must derive a key, which must match the key
    set on the server. The exact algorithm of deriving the key depends on the
    application; however, for this purpose, the server may send an \e{identity
    hint} to the client. This hint, combined with other information (for
    instance a passphrase), is then used by the client to construct the shared
    key.

    The QSslPreSharedKeyAuthenticator provides means to client applications for
    completing the PSK handshake. The client application needs to connect a
    slot to the QSslSocket::preSharedKeyAuthenticationRequired() signal:

    \snippet code/src_network_ssl_qsslpresharedkeyauthenticator.cpp 0

    The signal carries a QSslPreSharedKeyAuthenticator object containing the
    identity hint the server sent to the client, and which must be filled with the
    corresponding client identity and the derived key:

    \snippet code/src_network_ssl_qsslpresharedkeyauthenticator.cpp 1

    \note PSK ciphersuites are supported only when using OpenSSL 1.0.1 (or
    greater) as the SSL backend.

    \note PSK is currently only supported in OpenSSL.

    \sa QSslSocket
*/

/*!
    Constructs a default QSslPreSharedKeyAuthenticator object.

    The identity hint, the identity and the key will be initialized to empty
    byte arrays; the maximum length for both the identity and the key will be
    initialized to 0.
*/
QSslPreSharedKeyAuthenticator::QSslPreSharedKeyAuthenticator()
    : d(new QSslPreSharedKeyAuthenticatorPrivate)
{
}

/*!
    Destroys the QSslPreSharedKeyAuthenticator object.
*/
QSslPreSharedKeyAuthenticator::~QSslPreSharedKeyAuthenticator()
{
}

/*!
    Constructs a QSslPreSharedKeyAuthenticator object as a copy of \a authenticator.

    \sa operator=()
*/
QSslPreSharedKeyAuthenticator::QSslPreSharedKeyAuthenticator(const QSslPreSharedKeyAuthenticator &authenticator)
    : d(authenticator.d)
{
}

/*!
    Assigns the QSslPreSharedKeyAuthenticator object \a authenticator to this object,
    and returns a reference to the copy.
*/
QSslPreSharedKeyAuthenticator &QSslPreSharedKeyAuthenticator::operator=(const QSslPreSharedKeyAuthenticator &authenticator)
{
    d = authenticator.d;
    return *this;
}

/*!
    \fn QSslPreSharedKeyAuthenticator &QSslPreSharedKeyAuthenticator::operator=(QSslPreSharedKeyAuthenticator &&authenticator)

    Move-assigns the QSslPreSharedKeyAuthenticator object \a authenticator to this
    object, and returns a reference to the moved instance.
*/

/*!
    \fn void QSslPreSharedKeyAuthenticator::swap(QSslPreSharedKeyAuthenticator &other)
    \memberswap{authenticator}
*/

/*!
    Returns the PSK identity hint as provided by the server. The interpretation
    of this hint is left to the application.
*/
QByteArray QSslPreSharedKeyAuthenticator::identityHint() const
{
    return d->identityHint;
}

/*!
    Sets the PSK client identity (to be advised to the server) to \a identity.

    \note it is possible to set an identity whose length is greater than
    maximumIdentityLength(); in this case, only the first maximumIdentityLength()
    bytes will be actually sent to the server.

    \sa identity(), maximumIdentityLength()
*/
void QSslPreSharedKeyAuthenticator::setIdentity(const QByteArray &identity)
{
    d->identity = identity;
}

/*!
    Returns the PSK client identity.

    \sa setIdentity()
*/
QByteArray QSslPreSharedKeyAuthenticator::identity() const
{
    return d->identity;
}


/*!
    Returns the maximum length, in bytes, of the PSK client identity.

    \note it is possible to set an identity whose length is greater than
    maximumIdentityLength(); in this case, only the first maximumIdentityLength()
    bytes will be actually sent to the server.

    \sa setIdentity()
*/
int QSslPreSharedKeyAuthenticator::maximumIdentityLength() const
{
    return d->maximumIdentityLength;
}


/*!
    Sets the pre shared key to \a preSharedKey.

    \note it is possible to set a key whose length is greater than the
    maximumPreSharedKeyLength(); in this case, only the first
    maximumPreSharedKeyLength() bytes will be actually sent to the server.

    \sa preSharedKey(), maximumPreSharedKeyLength(), QByteArray::fromHex()
*/
void QSslPreSharedKeyAuthenticator::setPreSharedKey(const QByteArray &preSharedKey)
{
    d->preSharedKey = preSharedKey;
}

/*!
    Returns the pre shared key.

    \sa setPreSharedKey()
*/
QByteArray QSslPreSharedKeyAuthenticator::preSharedKey() const
{
    return d->preSharedKey;
}

/*!
    Returns the maximum length, in bytes, of the pre shared key.

    \note it is possible to set a key whose length is greater than the
    maximumPreSharedKeyLength(); in this case, only the first
    maximumPreSharedKeyLength() bytes will be actually sent to the server.

    \sa setPreSharedKey()
*/
int QSslPreSharedKeyAuthenticator::maximumPreSharedKeyLength() const
{
    return d->maximumPreSharedKeyLength;
}

/*!
    \fn bool QSslPreSharedKeyAuthenticator::operator==(const QSslPreSharedKeyAuthenticator &lhs, const QSslPreSharedKeyAuthenticator &rhs)
    \since 5.5

    Returns \c true if the authenticator object \a lhs is equal to \a rhs;
    \c false otherwise.

    Two authenticator objects are equal if and only if they have the same
    identity hint, identity, pre shared key, maximum length for the identity
    and maximum length for the pre shared key.
*/

/*!
    \fn bool QSslPreSharedKeyAuthenticator::operator!=(const QSslPreSharedKeyAuthenticator &lhs, const QSslPreSharedKeyAuthenticator &rhs)
    \since 5.5

    Returns \c true if the authenticator object \a lhs is not equal to \a rhs;
    \c false otherwise.
*/

/*!
    \internal
*/
bool QSslPreSharedKeyAuthenticator::isEqual(const QSslPreSharedKeyAuthenticator &other) const
{
    return ((d == other.d) ||
            (d->identityHint == other.d->identityHint &&
             d->identity == other.d->identity &&
             d->maximumIdentityLength == other.d->maximumIdentityLength &&
             d->preSharedKey == other.d->preSharedKey &&
             d->maximumPreSharedKeyLength == other.d->maximumPreSharedKeyLength));
}

QT_END_NAMESPACE
