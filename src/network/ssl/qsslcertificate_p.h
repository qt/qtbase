/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/


#ifndef QSSLCERTIFICATE_P_H
#define QSSLCERTIFICATE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include "qsslcertificateextension.h"
#include "qsslcertificate.h"
#include "qtlsbackend_p.h"

#include <qlist.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QSslCertificatePrivate
{
public:
    QSslCertificatePrivate();
    ~QSslCertificatePrivate();

    QList<QSslCertificateExtension> extensions() const;
    Q_NETWORK_PRIVATE_EXPORT static bool isBlacklisted(const QSslCertificate &certificate);
    Q_NETWORK_PRIVATE_EXPORT static QByteArray subjectInfoToString(QSslCertificate::SubjectInfo info);

    QAtomicInt ref;
    std::unique_ptr<QTlsPrivate::X509Certificate> backend;
};

QT_END_NAMESPACE

#endif // QSSLCERTIFICATE_P_H
