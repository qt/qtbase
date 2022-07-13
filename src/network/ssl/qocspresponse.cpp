// Copyright (C) 2011 Richard J. Moore <rich@kde.org>
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qocspresponse_p.h"
#include "qocspresponse.h"

#include "qhashfunctions.h"

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN(QOcspResponse)

/*!
    \class QOcspResponse
    \brief This class represents Online Certificate Status Protocol response.
    \since 5.13

    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork

    The QOcspResponse class represents the revocation status of a server's certificate,
    received by the client-side socket during the TLS handshake. QSslSocket must be
    configured with OCSP stapling enabled.

    \sa QSslSocket, QSslSocket::ocspResponses(), certificateStatus(),
    revocationReason(), responder(), subject(), QOcspCertificateStatus, QOcspRevocationReason,
    QSslConfiguration::setOcspStaplingEnabled(), QSslConfiguration::ocspStaplingEnabled(),
    QSslConfiguration::peerCertificate()
*/

/*!
    \enum QOcspCertificateStatus
    \brief Describes the Online Certificate Status
    \relates QOcspResponse
    \since 5.13

    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork

    \value Good The certificate is not revoked, but this does not necessarily
           mean that the certificate was ever issued or that the time at which
           the response was produced is within the certificate's validity interval.
    \value Revoked This state indicates that the certificate has been revoked
           (either permanently or temporarily - on hold).
    \value Unknown This state indicates that the responder doesn't know about
           the certificate being requested.

    \sa QOcspRevocationReason
*/

/*!
    \enum QOcspRevocationReason
    \brief Describes the reason for revocation
    \relates QOcspResponse
    \since 5.13

    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork


    This enumeration describes revocation reasons, defined in \l{RFC 5280, section 5.3.1}

    \value None
    \value Unspecified
    \value KeyCompromise
    \value CACompromise
    \value AffiliationChanged
    \value Superseded
    \value CessationOfOperation
    \value CertificateHold
    \value RemoveFromCRL
*/

/*!
    \since 5.13

    Creates a new response with status QOcspCertificateStatus::Unknown
    and revocation reason QOcspRevocationReason::None.

    \sa QOcspCertificateStatus
*/
QOcspResponse::QOcspResponse()
    : d(new QOcspResponsePrivate)
{
}

/*!
    \since 5.13

    Copy-constructs a QOcspResponse instance.
*/
QOcspResponse::QOcspResponse(const QOcspResponse &) = default;

/*!
    \since 5.13

    Move-constructs a QOcspResponse instance.
*/
QOcspResponse::QOcspResponse(QOcspResponse &&) noexcept = default;

/*!
    \since 5.13

    Destroys the response.
*/
QOcspResponse::~QOcspResponse() = default;

/*!
    \since 5.13

    Copy-assigns \a other and returns a reference to this response.
*/
QOcspResponse &QOcspResponse::operator=(const QOcspResponse &) = default;

/*!
    \since 5.13

    Move-assigns \a other to this QOcspResponse instance.
*/
QOcspResponse &QOcspResponse::operator=(QOcspResponse &&) noexcept = default;

/*!
    \fn void QOcspResponse::swap(QOcspResponse &other)
    \since 5.13

    Swaps this response with \a other.
*/

/*!
    \since 5.13

    Returns the certificate status.

    \sa QOcspCertificateStatus
*/
QOcspCertificateStatus QOcspResponse::certificateStatus() const
{
    return d->certificateStatus;
}

/*!
    \since 5.13

    Returns the reason for revocation.
*/
QOcspRevocationReason QOcspResponse::revocationReason() const
{
    return d->revocationReason;
}

/*!
    \since 5.13

    This function returns a certificate used to sign OCSP response.
*/
QSslCertificate QOcspResponse::responder() const
{
    return d->signerCert;
}

/*!
    \since 5.13

    This function returns a certificate, for which this response was issued.
*/
QSslCertificate QOcspResponse::subject() const
{
    return d->subjectCert;
}

/*!
    \fn bool QOcspResponse::operator==(const QOcspResponse &lhs, const QOcspResponse &rhs)

    Returns \c true if \a lhs and \a rhs are the responses for the same
    certificate, signed by the same responder, have the same
    revocation reason and the same certificate status.

    \since 5.13
*/

/*!
    \fn bool QOcspResponse::operator!=(const QOcspResponse &lhs, const QOcspResponse &rhs)

    Returns \c true if \a lhs and \a rhs are responses for different certificates,
    or signed by different responders, or have different revocation reasons, or different
    certificate statuses.

    \since 5.13
*/

/*!
    \internal
*/
bool QOcspResponse::isEqual(const QOcspResponse &other) const
{
    return d == other.d || *d == *other.d;
}

/*!
    Returns the hash value for the \a response, using \a seed to seed the calculation.

    \since 5.13
    \relates QHash
*/
size_t qHash(const QOcspResponse &response, size_t seed) noexcept
{
    const QOcspResponsePrivate *d = response.d.data();
    Q_ASSERT(d);

    QtPrivate::QHashCombine hasher;
    size_t hash = hasher(seed, int(d->certificateStatus));
    hash = hasher(hash, int(d->revocationReason));
    if (!d->signerCert.isNull())
        hash = hasher(hash, d->signerCert);
    if (!d->subjectCert.isNull())
        hash = hasher(hash, d->subjectCert);

    return hash;
}

QT_END_NAMESPACE
