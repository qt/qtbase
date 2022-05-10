// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QNetworkAccessManager *manager = new QNetworkAccessManager(this);
connect(manager, &QNetworkAccessManager::finished,
        this, &MyClass::replyFinished);

manager->get(QNetworkRequest(QUrl("http://qt-project.org")));
//! [0]


//! [1]
QNetworkRequest request;
request.setUrl(QUrl("http://qt-project.org"));
request.setRawHeader("User-Agent", "MyOwnBrowser 1.0");

QNetworkReply *reply = manager->get(request);
connect(reply, &QIODevice::readyRead, this, &MyClass::slotReadyRead);
connect(reply, &QNetworkReply::errorOccurred,
        this, &MyClass::slotError);
connect(reply, &QNetworkReply::sslErrors,
        this, &MyClass::slotSslErrors);
//! [1]
