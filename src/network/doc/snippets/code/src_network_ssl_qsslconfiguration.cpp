// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QSslConfiguration config = sslSocket.sslConfiguration();
config.setProtocol(QSsl::TlsV1_2);
sslSocket.setSslConfiguration(config);
//! [0]


//! [1]
QSslConfiguration tlsConfig = QSslConfiguration::defaultConfiguration();
tlsConfig.setCiphers(QStringLiteral("DHE-RSA-AES256-SHA:DHE-DSS-AES256-SHA:AES256-SHA"));
//! [1]

