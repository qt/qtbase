/****************************************************************************
** Copyright (C) 2011 Richard J. Moore <rich@kde.org>
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qocspresponse_p.h"
#include "qocspresponse.h"

#include "qhashfunctions.h"

QT_BEGIN_NAMESPACE

/*!
    \class QOcspResponse
    \brief This class represents Online Certificate Status Protocol response.
    \since 5.13

    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork

    The QOcspResponse class represents the revocation status of a server's certficate,
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


    This enumeration describes revocation reasons, defined in \l{https://tools.ietf.org/html/rfc5280#section-5.3.1}{RFC 5280, section 5.3.1}

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

    Copy-assigns and returns a reference to this response.
*/
QOcspResponse &QOcspResponse::operator=(const QOcspResponse &) = default;

/*!
    \since 5.13

    Move-assigns to this QOcspResponse instance.
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
    \fn bool operator==(const QOcspResponse &lhs, const QOcspResponse &rhs)

    Returns \c true if \a lhs and \a rhs are the responses for the same
    certificate, signed by the same responder, have the same
    revocation reason and the same certificate status.

    \since 5.13
    \relates QOcspResponse
 */
Q_NETWORK_EXPORT bool operator==(const QOcspResponse &lhs, const QOcspResponse &rhs)
{
    return lhs.d == rhs.d || *lhs.d == *rhs.d;
}

/*!
  \fn bool operator != (const QOcspResponse &lhs, const QOcspResponse &rhs)

  Returns \c true if \a lhs and \a rhs are responses for different certificates,
  or signed by different responders, or have different revocation reasons, or different
  certificate statuses.

  \since 5.13
  \relates QOcspResponse
*/

/*!
    \fn uint qHash(const QOcspResponse &response, uint seed)

    Returns the hash value for the \a response, using \a seed to seed the calculation.

    \since 5.13
    \relates QHash
*/
uint qHash(const QOcspResponse &response, uint seed) noexcept
{
    const QOcspResponsePrivate *d = response.d.data();
    Q_ASSERT(d);

    QtPrivate::QHashCombine hasher;
    uint hash = hasher(seed, int(d->certificateStatus));
    hash = hasher(hash, int(d->revocationReason));
    if (!d->signerCert.isNull())
        hash = hasher(hash, d->signerCert);
    if (!d->subjectCert.isNull())
        hash = hasher(hash, d->subjectCert);

    return hash;
}

QT_END_NAMESPACE
