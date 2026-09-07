// Copyright (C) 2026 Governikus GmbH & Co. KG.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qsslkeyingmaterial.h"

#ifndef QT_NO_DEBUG_STREAM
#include <QtCore/qdebug.h>
#endif
#include <QtCore/qhashfunctions.h>

QT_BEGIN_NAMESPACE

/*!
    \class QSslKeyingMaterial
    \since 6.12
    \preliminary

    \brief Describes exported keying material derived from a TLS session.

    \reentrant
    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork
    \compares equality

    QSslKeyingMaterial represents a request for keying material derived
    from an established TLS connection using the TLS exporter mechanism.

    The exporter mechanism is defined in RFC 5705 for TLS 1.2 and earlier
    and in RFC 8446 for TLS 1.3. It allows applications to derive
    cryptographically separate keying material from the TLS session
    without exposing the session's traffic keys.

    Each QSslKeyingMaterial object specifies:
    \list
        \li an exporter label identifying the purpose of the derived
            keying material
        \li an optional context value binding the keying material to
            application-specific data
        \li the desired size of the exported keying material
    \endlist

    The actual keying material is derived by the TLS backend after a
    successful handshake and can be retrieved via value().

    QSslKeyingMaterial objects are typically configured via
    QSslConfiguration::setKeyingMaterial() before initiating a TLS
    connection.

    Example: Deterministic export on client and server
    \code
    // Both client and server configure the same label and optional context
    QSslKeyingMaterial keying("session-label", 32, "app-specific-context");

    // After the TLS handshake completes get data from QSslConfiguration.
    QByteArray derived = sslConfiguration().keyingMaterial(keying)->value();

    // Both client and server will obtain the same 'derived' bytes
    // even though they each performed the derivation independently.
    use(derived);
    \endcode

    \section1 Security Considerations

    Exported keying material is a secret. QByteArray is a copy-on-write
    container, so every QSslKeyingMaterial object holding a value shares one
    buffer, and the secret remains in memory for as long as any of those
    objects lives. An application that needs to guarantee it holds the only
    remaining reference must release the Qt-internal ones explicitly.

    After a successful handshake the value lives in exactly one place inside
    Qt: the QSslKeyingMaterial entry in the socket's internal
    QSslConfiguration. Each QSslConfiguration returned by
    QSslSocket::sslConfiguration() is an independent copy of that
    configuration, sharing the value's buffer with it.

    Both QSslConfiguration::takeKeyingMaterial() overloads hand the values over
    instead of sharing them: what they return holds the only reference to the
    value, and the entries they leave behind in the configuration they were
    called on are valueless \l{clone()}{clones}. Copy the value out of the
    returned object with \l value() and let the object itself go out of scope,
    then write the configuration back to the socket: that overwrites the
    socket's entry with the valueless one, dropping the last reference Qt
    holds.

    \code
    QSslConfiguration config = socket->sslConfiguration();

    // Copy the value out of the temporary that owns it, and let it die:
    QByteArray secret = config.takeKeyingMaterial(request)->value();

    // Overwrite the socket's copy with the entry left behind, which has no value:
    socket->setSslConfiguration(config);
    \endcode

    The following copies are outside the socket's control and must be dealt
    with separately:
    \list
        \li Any other QSslConfiguration copy the application still holds,
            including one stored in a QNetworkRequest or installed with
            QSslConfiguration::setDefaultConfiguration(). Taking the value
            from one copy does not affect the others.
        \li A configuration that was never written back to the socket. Taking
            the values out of a QSslConfiguration only strips that copy of the
            configuration; the socket keeps its own until it is given the
            valueless entries.
        \li Any QSslKeyingMaterial copy the application made itself. Use
            \l clone() when a copy of a request is needed without its value.
    \endlist

    The socket also drops the values when it starts a new handshake, because
    its entries are reset to valueless clones then, and when it is destroyed.
    Neither replaces the explicit step above for a socket that stays alive.
*/

/*!
    Default-constructs an instance of QSslKeyingMaterial.

    A default instance is never valid.

    \sa isValid()
*/

QSslKeyingMaterial::QSslKeyingMaterial()
    = default;

/*!
    \fn explicit QSslKeyingMaterial::QSslKeyingMaterial(const QByteArray &label, qsizetype size)
    \fn explicit QSslKeyingMaterial::QSslKeyingMaterial(const QByteArray &label, qsizetype size, const QByteArray &context)

    Constructs a QSslKeyingMaterial object with the given exporter
    \a label, output \a size, and optional \a context.

    The \a label identifies the purpose of the exported keying material
    and must be non-empty. The \a size specifies the number of bytes
    to be derived from the TLS exporter.

    The optional \a context is application-defined data that is mixed
    into the key derivation process to provide domain separation.

    The keying material itself is not generated until a TLS handshake
    has completed successfully.

    \note Under TLS 1.2 (RFC 5705), a null context and an empty (non-null)
    context produce different keying material: the context length field is
    omitted entirely when no context is present, yielding a different PRF
    input. Under TLS 1.3 (RFC 8446), an absent context and an empty context
    are defined to be equivalent and produce the same keying material.
    Use \l{QByteArray::isNull()} to distinguish them.

    \sa isValid(), label(), context(), value()
*/

QSslKeyingMaterial::QSslKeyingMaterial(const QByteArray &label, qsizetype size)
    : m_label(label),
      m_requestedSize(size)
{
}

QSslKeyingMaterial::QSslKeyingMaterial(const QByteArray &label, qsizetype size, const QByteArray &context)
    : m_label(label),
      m_context(context),
      m_requestedSize(size)
{
}

QSslKeyingMaterial::QSslKeyingMaterial(const QSslKeyingMaterial &other)
    = default;

QSslKeyingMaterial &QSslKeyingMaterial::operator=(const QSslKeyingMaterial &other)
    = default;

QSslKeyingMaterial::~QSslKeyingMaterial()
    = default;

/*!
    Returns true if this QSslKeyingMaterial object describes a valid
    exporter request.

    A QSslKeyingMaterial object is considered valid if it has a
    non-empty exporter label and a positive output size.

    \sa label(), value()
*/

bool QSslKeyingMaterial::isValid() const noexcept
{
    return !m_label.isEmpty() && requestedSize() > 0;
}

/*!
    \fn QByteArray QSslKeyingMaterial::label() const

    Returns the exporter label used for deriving the keying material.

    The label identifies the purpose of the exported keying material
    and is included verbatim in the TLS exporter derivation.

    \sa context(), value()
*/

/*!
    \fn QByteArray QSslKeyingMaterial::context() const

    Returns the optional context value used for deriving the keying material.

    The context value binds the exported keying material to
    application-specific data and helps prevent accidental reuse of
    identical keys across different purposes.

    If no context was specified, a null/empty QByteArray is returned (see
   \l{QSslKeyingMaterial::QSslKeyingMaterial()}).

    \sa label(), value()
*/

/*!
    \fn QByteArray QSslKeyingMaterial::value() const

    Returns the exported keying material.

    The returned QByteArray contains the keying material derived from
    the TLS session using the configured exporter label and context.

    If the TLS handshake has not completed successfully or if the TLS
    backend does not support key exporters, this function returns an
    empty value.

    \note The contents of the returned keying material are
          security-sensitive and must be handled with care. See
          \l{QSslKeyingMaterial#Security Considerations}{Security
          Considerations} for how to keep the returned QByteArray the
          only copy of it.

    \sa label(), context(), requestedSize()
*/

/*!
    \fn qsizetype QSslKeyingMaterial::requestedSize() const noexcept

    The desired size of the keying material.

    The desired size is the number of bytes the handshake protocol
    is asked to generate for the purpose described by the \l label()
    and \l context() of the requested keying material.

    \sa value()
*/

/*!
    \fn void QSslKeyingMaterial::swap(QSslKeyingMaterial &other) noexcept
    \memberswap{keying material}
*/

/*!
    \fn size_t qHash(const QSslKeyingMaterial &key) noexcept
    \fn size_t qHash(const QSslKeyingMaterial &key, size_t seed) noexcept
    \qhashold{QHash}
*/
size_t qHash(const QSslKeyingMaterial &material, size_t seed) noexcept
{
    return qHashMulti(seed, material.m_label, material.m_context, material.m_value,
                      material.m_requestedSize);
}

// friend
bool comparesEqual(const QSslKeyingMaterial &lhs, const QSslKeyingMaterial &rhs) noexcept
{
    return lhs.m_requestedSize == rhs.m_requestedSize
        && lhs.m_label == rhs.m_label
        && lhs.m_context.isNull() == rhs.m_context.isNull()
        && lhs.m_context == rhs.m_context
        && lhs.m_value == rhs.m_value;
}

#ifndef QT_NO_DEBUG_STREAM
/*!
    \relates QSslKeyingMaterial

    Writes a textual representation of the keying material \a keying
    to the debug object \a debug.

    \sa {Debugging Techniques}
*/
QDebug operator<<(QDebug debug, const QSslKeyingMaterial &keying)
{
    QDebugStateSaver saver(debug);
    debug.resetFormat().nospace();
    debug << "QSslKeyingMaterial("
          << keying.label() << ',' << keying.context()
          << ", requested size: " << keying.requestedSize()
          << ", actual size: " << keying.value().size() << ')';
    return debug;
}
#endif

QT_END_NAMESPACE
