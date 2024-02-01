// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
Content-Type: text/plain
Content-Disposition: form-data; name="text"

here goes the body
//! [0]

//! [1]
QHttpPart textPart;
textPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain"));
textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"text\""));
textPart.setBody("here goes the body");
//! [1]

//! [2]
QHttpPart imagePart;
imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"image\""));
imagePart.setRawHeader("Content-ID", "my@content.id"); // add any headers you like via setRawHeader()
QFile *file = new QFile("image.jpg");
file->open(QIODevice::ReadOnly);
imagePart.setBodyDevice(file);
//! [2]

