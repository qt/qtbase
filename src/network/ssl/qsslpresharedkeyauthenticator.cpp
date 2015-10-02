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

#include "qsslpresharedkeyauthenticator.h"
#include "qsslpresharedkeyauthenticator_p.h"

#include <QSharedData>

QT_BEGIN_NAMESPACE

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

    \code

    connect(socket, &QSslSocket::preSharedKeyAuthenticationRequired,
            this, &AuthManager::handlePreSharedKeyAuthentication);

    \endcode

    The signal carries a QSslPreSharedKeyAuthenticator object containing the
    identity hint the server sent to the client, and which must be filled with the
    corresponding client identity and the derived key:

    \code

    void AuthManager::handlePreSharedKeyAuthentication(QSslPreSharedKeyAuthenticator *authenticator)
    {
        authenticator->setIdentity("My Qt App");

        const QByteArray key = deriveKey(authenticator->identityHint(), passphrase);
        authenticator->setPreSharedKey(key);
    }

    \endcode

    \note PSK ciphersuites are supported only when using OpenSSL 1.0.1 (or
    greater) as the SSL backend.

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

    Move-assigns the the QSslPreSharedKeyAuthenticator object \a authenticator to this
    object, and returns a reference to the moved instance.
*/

/*!
    \fn void QSslPreSharedKeyAuthenticator::swap(QSslPreSharedKeyAuthenticator &authenticator)

    Swaps the QSslPreSharedKeyAuthenticator object \a authenticator with this object.
    This operation is very fast and never fails.
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
    \relates QSslPreSharedKeyAuthenticator
    \since 5.5

    Returns true if the authenticator object \a lhs is equal to \a rhs; false
    otherwise.

    Two authenticator objects are equal if and only if they have the same
    identity hint, identity, pre shared key, maximum length for the identity
    and maximum length for the pre shared key.

*/
bool operator==(const QSslPreSharedKeyAuthenticator &lhs, const QSslPreSharedKeyAuthenticator &rhs)
{
    return ((lhs.d == rhs.d) ||
            (lhs.d->identityHint == rhs.d->identityHint &&
             lhs.d->identity == rhs.d->identity &&
             lhs.d->maximumIdentityLength == rhs.d->maximumIdentityLength &&
             lhs.d->preSharedKey == rhs.d->preSharedKey &&
             lhs.d->maximumPreSharedKeyLength == rhs.d->maximumPreSharedKeyLength));
}

/*!
    \fn bool operator!=(const QSslPreSharedKeyAuthenticator &lhs, const QSslPreSharedKeyAuthenticator &rhs)
    \relates QSslPreSharedKeyAuthenticator
    \since 5.5

    Returns true if the authenticator object \a lhs is different than \a rhs;
    false otherwise.

*/

QT_END_NAMESPACE
