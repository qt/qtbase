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

#ifndef QOCSPRESPONSE_P_H
#define QOCSPRESPONSE_P_H

#include <private/qtnetworkglobal_p.h>

#include <qsslcertificate.h>
#include <qocspresponse.h>

#include <qshareddata.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QOcspResponsePrivate : public QSharedData
{
public:

    QOcspCertificateStatus certificateStatus = QOcspCertificateStatus::Unknown;
    QOcspRevocationReason revocationReason = QOcspRevocationReason::None;

    QSslCertificate signerCert;
    QSslCertificate subjectCert;
};

inline bool operator==(const QOcspResponsePrivate &lhs, const QOcspResponsePrivate &rhs)
{
    return lhs.certificateStatus == rhs.certificateStatus
            && lhs.revocationReason == rhs.revocationReason
            && lhs.signerCert == rhs.signerCert
            && lhs.subjectCert == rhs.subjectCert;
}

QT_END_NAMESPACE

#endif // QOCSPRESPONSE_P_H
