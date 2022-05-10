// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


/*!
    \class QSslKey
    \brief The QSslKey class provides an interface for private and public keys.
    \since 4.3

    \reentrant
    \ingroup network
    \ingroup ssl
    \ingroup shared
    \inmodule QtNetwork

    QSslKey provides a simple API for managing keys.

    \sa QSslSocket, QSslCertificate, QSslCipher
*/

#include "qssl_p.h"
#include "qsslkey.h"
#include "qsslkey_p.h"
#include "qsslsocket.h"
#include "qsslsocket_p.h"
#include "qtlsbackend_p.h"

#include <QtCore/qatomic.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qiodevice.h>
#ifndef QT_NO_DEBUG_STREAM
#include <QtCore/qdebug.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \fn void QSslKeyPrivate::decodePem(const QByteArray &pem, const QByteArray &passPhrase,
                               bool deepClear)
    \internal

    Allocates a new rsa or dsa struct and decodes \a pem into it
    according to the current algorithm and type.

    If \a deepClear is true, the rsa/dsa struct is freed if it is was
    already allocated, otherwise we "leak" memory (which is exactly
    what we want for copy construction).

    If \a passPhrase is non-empty, it will be used for decrypting
    \a pem.
*/

/*!
    \internal
*/
QSslKeyPrivate::QSslKeyPrivate()
{
    const auto *tlsBackend = QSslSocketPrivate::tlsBackendInUse();
    if (!tlsBackend)
        return;
    backend.reset(tlsBackend->createKey());
    if (backend.get())
        backend->clear(false /*not deep clear*/);
    else
        qCWarning(lcSsl, "Active TLS backend does not support key creation");
}

/*!
    \internal
*/
QSslKeyPrivate::~QSslKeyPrivate()
{
    if (backend.get())
        backend->clear(true /*deep clear*/);
}

QByteArray QSslKeyPrivate::decrypt(Cipher cipher, const QByteArray &data, const QByteArray &key, const QByteArray &iv)
{
    if (const auto *tlsBackend = QSslSocketPrivate::tlsBackendInUse()) {
        const std::unique_ptr<QTlsPrivate::TlsKey> cryptor(tlsBackend->createKey());
        return cryptor->decrypt(cipher, data, key, iv);
    }

    return {};
}

QByteArray QSslKeyPrivate::encrypt(Cipher cipher, const QByteArray &data, const QByteArray &key, const QByteArray &iv)
{
    if (const auto *tlsBackend = QSslSocketPrivate::tlsBackendInUse()) {
        const std::unique_ptr<QTlsPrivate::TlsKey> cryptor(tlsBackend->createKey());
        return cryptor->encrypt(cipher, data, key, iv);
    }

    return {};
}

/*!
    Constructs a null key.

    \sa isNull()
*/
QSslKey::QSslKey()
    : d(new QSslKeyPrivate)
{
}

/*!
    Constructs a QSslKey by decoding the string in the byte array
    \a encoded using a specified \a algorithm and \a encoding format.
    \a type specifies whether the key is public or private.

    If the key is encrypted then \a passPhrase is used to decrypt it.

    After construction, use isNull() to check if \a encoded contained
    a valid key.
*/
QSslKey::QSslKey(const QByteArray &encoded, QSsl::KeyAlgorithm algorithm,
                 QSsl::EncodingFormat encoding, QSsl::KeyType type, const QByteArray &passPhrase)
    : d(new QSslKeyPrivate)
{
    if (auto *tlsKey = d->backend.get()) {
        if (encoding == QSsl::Der)
            tlsKey->decodeDer(type, algorithm, encoded, passPhrase, true /*deep clear*/);
        else
            tlsKey->decodePem(type, algorithm, encoded, passPhrase, true /*deep clear*/);
    }
}

/*!
    Constructs a QSslKey by reading and decoding data from a
    \a device using a specified \a algorithm and \a encoding format.
    \a type specifies whether the key is public or private.

    If the key is encrypted then \a passPhrase is used to decrypt it.

    After construction, use isNull() to check if \a device provided
    a valid key.
*/
QSslKey::QSslKey(QIODevice *device, QSsl::KeyAlgorithm algorithm, QSsl::EncodingFormat encoding,
                 QSsl::KeyType type, const QByteArray &passPhrase)
    : d(new QSslKeyPrivate)
{
    QByteArray encoded;
    if (device)
        encoded = device->readAll();

    if (auto *tlsKey = d->backend.get()) {
        if (encoding == QSsl::Der)
            tlsKey->decodeDer(type, algorithm, encoded, passPhrase, true /*deep clear*/);
        else
            tlsKey->decodePem(type, algorithm, encoded, passPhrase, true /*deep clear*/);
    }
}

/*!
    \since 5.0
    Constructs a QSslKey from a valid native key \a handle.
    \a type specifies whether the key is public or private.

    QSslKey will take ownership for this key and you must not
    free the key using the native library.
*/
QSslKey::QSslKey(Qt::HANDLE handle, QSsl::KeyType type)
    : d(new QSslKeyPrivate)
{
    if (auto *tlsKey = d->backend.get())
        tlsKey->fromHandle(handle, type);
}

/*!
    Constructs an identical copy of \a other.
*/
QSslKey::QSslKey(const QSslKey &other) : d(other.d)
{
}

QSslKey::QSslKey(QSslKey &&other) noexcept
    : d(nullptr)
{
    qSwap(d, other.d);
}

QSslKey &QSslKey::operator=(QSslKey &&other) noexcept
{
    if (this == &other)
        return *this;

    // If no one else is referencing the key data we want to make sure
    // before we swap the d-ptr that it is not left in memory.
    d.reset();
    qSwap(d, other.d);
    return *this;
}

/*!
    Destroys the QSslKey object.
*/
QSslKey::~QSslKey()
{
}

/*!
    Copies the contents of \a other into this key, making the two keys
    identical.

    Returns a reference to this QSslKey.
*/
QSslKey &QSslKey::operator=(const QSslKey &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn void QSslKey::swap(QSslKey &other)
    \since 5.0

    Swaps this ssl key with \a other. This function is very fast and
    never fails.
*/

/*!
    Returns \c true if this is a null key; otherwise false.

    \sa clear()
*/
bool QSslKey::isNull() const
{
    if (const auto *tlsKey = d->backend.get())
        return tlsKey->isNull();

    return true;
}

/*!
    Clears the contents of this key, making it a null key.

    \sa isNull()
*/
void QSslKey::clear()
{
    d = new QSslKeyPrivate;
}

/*!
    Returns the length of the key in bits, or -1 if the key is null.
*/
int QSslKey::length() const
{
    if (const auto *tlsKey = d->backend.get())
        return tlsKey->length();

    return -1;
}

/*!
    Returns the type of the key (i.e., PublicKey or PrivateKey).
*/
QSsl::KeyType QSslKey::type() const
{
    if (const auto *tlsKey = d->backend.get())
        return tlsKey->type();

    return QSsl::PublicKey;
}

/*!
    Returns the key algorithm.
*/
QSsl::KeyAlgorithm QSslKey::algorithm() const
{
    if (const auto *tlsKey = d->backend.get())
        return tlsKey->algorithm();

    return QSsl::Opaque;
}

/*!
  Returns the key in DER encoding.

  The \a passPhrase argument should be omitted as DER cannot be
  encrypted. It will be removed in a future version of Qt.
*/
QByteArray QSslKey::toDer(const QByteArray &passPhrase) const
{
    if (isNull() || algorithm() == QSsl::Opaque)
        return {};

    // Encrypted DER is nonsense, see QTBUG-41038.
    if (type() == QSsl::PrivateKey && !passPhrase.isEmpty())
        return {};

    QMap<QByteArray, QByteArray> headers;
    if (const auto *tlsKey = d->backend.get())
        return tlsKey->derFromPem(toPem(passPhrase), &headers);

    return {};
}

/*!
  Returns the key in PEM encoding. The result is encrypted with
  \a passPhrase if the key is a private key and \a passPhrase is
  non-empty.
*/
QByteArray QSslKey::toPem(const QByteArray &passPhrase) const
{
    if (const auto *tlsKey = d->backend.get())
        return tlsKey->toPem(passPhrase);

    return {};
}

/*!
    Returns a pointer to the native key handle, if there is
    one, else \nullptr.

    You can use this handle together with the native API to access
    extended information about the key.

    \warning Use of this function has a high probability of being
    non-portable, and its return value may vary across platforms, and
    between minor Qt releases.
*/
Qt::HANDLE QSslKey::handle() const
{
    if (d->backend.get())
        return d->backend->handle();

    return nullptr;
}

/*!
    Returns \c true if this key is equal to \a other; otherwise returns \c false.
*/
bool QSslKey::operator==(const QSslKey &other) const
{
    if (isNull())
        return other.isNull();
    if (other.isNull())
        return isNull();
    if (algorithm() != other.algorithm())
        return false;
    if (type() != other.type())
        return false;
    if (length() != other.length())
        return false;
    if (algorithm() == QSsl::Opaque)
        return handle() == other.handle();
    return toDer() == other.toDer();
}

/*! \fn bool QSslKey::operator!=(const QSslKey &other) const

  Returns \c true if this key is not equal to key \a other; otherwise
  returns \c false.
*/

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QSslKey &key)
{
    QDebugStateSaver saver(debug);
    debug.resetFormat().nospace();
    debug << "QSslKey("
          << (key.type() == QSsl::PublicKey ? "PublicKey" : "PrivateKey")
          << ", " << (key.algorithm() == QSsl::Opaque ? "OPAQUE" :
                     (key.algorithm() == QSsl::Rsa ? "RSA" :
                     (key.algorithm() == QSsl::Dsa ? "DSA" :
                     (key.algorithm() == QSsl::Dh ? "DH" : "EC"))))
          << ", " << key.length()
          << ')';
    return debug;
}
#endif

QT_END_NAMESPACE
