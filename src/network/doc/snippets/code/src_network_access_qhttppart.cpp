/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
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

