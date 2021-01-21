/****************************************************************************
** Copyright (C) 2011 Richard J. Moore <rich@kde.org>
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QOCSPRESPONSE_H
#define QOCSPRESPONSE_H

#include <QtNetwork/qtnetworkglobal.h>

#include <QtCore/qshareddata.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qobject.h>

#ifndef Q_CLANG_QDOC
QT_REQUIRE_CONFIG(ssl);
#endif

QT_BEGIN_NAMESPACE

enum class QOcspCertificateStatus
{
    Good,
    Revoked,
    Unknown
};

enum class QOcspRevocationReason
{
    None = -1,
    Unspecified,
    KeyCompromise,
    CACompromise,
    AffiliationChanged,
    Superseded,
    CessationOfOperation,
    CertificateHold,
    RemoveFromCRL
};

class QOcspResponse;
Q_NETWORK_EXPORT uint qHash(const QOcspResponse &response, uint seed = 0) noexcept;

class QOcspResponsePrivate;
class Q_NETWORK_EXPORT QOcspResponse
{
public:

    QOcspResponse();
    QOcspResponse(const QOcspResponse &other);
    QOcspResponse(QOcspResponse && other)  noexcept;
    ~QOcspResponse();

    QOcspResponse &operator = (const QOcspResponse &other);
    QOcspResponse &operator = (QOcspResponse &&other) noexcept;

    QOcspCertificateStatus certificateStatus() const;
    QOcspRevocationReason revocationReason() const;

    class QSslCertificate responder() const;
    QSslCertificate subject() const;

    void swap(QOcspResponse &other) noexcept { d.swap(other.d); }

private:

    friend class QSslSocketBackendPrivate;
    friend Q_NETWORK_EXPORT bool operator==(const QOcspResponse &lhs, const QOcspResponse &rhs);
    friend Q_NETWORK_EXPORT uint qHash(const QOcspResponse &response, uint seed) noexcept;

    QSharedDataPointer<QOcspResponsePrivate> d;
};

inline bool operator!=(const QOcspResponse &lhs, const QOcspResponse &rhs) { return !(lhs == rhs); }

Q_DECLARE_SHARED(QOcspResponse)

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QOcspResponse)

#endif // QOCSPRESPONSE_H
