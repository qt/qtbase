// Copyright (C) 2026 Governikus GmbH & Co. KG.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qsslkeyingmaterial.h"
#include "qtlsbackend_p.h"
#include "qsslsocket_p.h"

#ifndef QT_NO_DEBUG_STREAM
#include <QDebug>
#endif

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN(QSslKeyingMaterial)

/*!
    \class QSslKeyingMaterial
    \since 6.12

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
    qDebug() << "Derived keying material:" << derived;
    \endcode
*/

/*!
    \fn explicit QSslKeyingMaterial::QSslKeyingMaterial(const QByteArray &label, int size, const QByteArray &context) noexcept

    Constructs a QSslKeyingMaterial object with the given exporter
    \a label, output \a size, and optional \a context.

    The \a label identifies the purpose of the exported keying material
    and must be non-empty. The \a size specifies the number of bytes
    to be derived from the TLS exporter.

    The optional \a context is application-defined data that is mixed
    into the key derivation process to provide domain separation.

    The keying material itself is not generated until a TLS handshake
    has completed successfully.

    \sa isValid(), label(), context(), value()
*/

/*!
    \fn bool QSslKeyingMaterial::isValid() const noexcept

    Returns true if this QSslKeyingMaterial object describes a valid
    exporter request.

    A QSslKeyingMaterial object is considered valid if it has a
    non-empty exporter label and a positive output size.

    \sa label(), value()
*/

/*!
    \fn QByteArray QSslKeyingMaterial::label() const noexcept

    Returns the exporter label used for deriving the keying material.

    The label identifies the purpose of the exported keying material
    and is included verbatim in the TLS exporter derivation.

    \sa context(), value()
*/

/*!
    \fn QByteArray QSslKeyingMaterial::context() const noexcept

    Returns the optional context value used for deriving the keying material.

    The context value binds the exported keying material to
    application-specific data and helps prevent accidental reuse of
    identical keys across different purposes.

    If no context was specified, an empty QByteArray is returned.

    \sa label(), value()
*/

/*!
    \fn QByteArray QSslKeyingMaterial::value() const noexcept

    Returns the exported keying material.

    The returned QByteArray contains the keying material derived from
    the TLS session using the configured exporter label and context.

    If the TLS handshake has not completed successfully or if the TLS
    backend does not support key exporters, this function returns an
    empty value.

    \note The contents of the returned keying material are#
          security-sensitive and must be handled with care.

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
    \fn size_t qHash(const QSslKeyingMaterial &key, size_t seed) noexcept
    \qhashold{QHash}
*/

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
