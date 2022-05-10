// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

using namespace Qt::StringLiterals;

//! [0]
QList<QSslCertificate> cert = QSslCertificate::fromPath("server-certificate.pem"_L1);
QSslError error(QSslError::SelfSignedCertificate, cert.at(0));
QList<QSslError> expectedSslErrors;
expectedSslErrors.append(error);

QNetworkReply *reply = manager.get(QNetworkRequest(QUrl("https://server.tld/index.html")));
reply->ignoreSslErrors(expectedSslErrors);
// here connect signals etc.
//! [0]
