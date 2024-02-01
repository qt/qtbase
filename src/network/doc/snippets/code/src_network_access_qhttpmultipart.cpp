// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

QHttpPart textPart;
textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"text\""));
textPart.setBody("my text");

QHttpPart imagePart;
imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"image\""));
QFile *file = new QFile("image.jpg");
file->open(QIODevice::ReadOnly);
imagePart.setBodyDevice(file);
file->setParent(multiPart); // we cannot delete the file now, so delete it with the multiPart

multiPart->append(textPart);
multiPart->append(imagePart);

QUrl url("http://my.server.tld");
QNetworkRequest request(url);

QNetworkAccessManager manager;
QNetworkReply *reply = manager.post(request, multiPart);
multiPart->setParent(reply); // delete the multiPart with the reply
// here connect signals etc.
//! [0]
