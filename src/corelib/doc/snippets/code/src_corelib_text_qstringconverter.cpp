// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QByteArray encodedString = "...";
auto toUtf16 = QStringDecoder(QStringDecoder::Utf8);
QString string = toUtf16(encodedString);
//! [0]


//! [1]
QString string = "...";
auto fromUtf16 = QStringEncoder(QStringEncoder::Utf8);
QByteArray encodedString = fromUtf16(string);
//! [1]


//! [2]
auto toUtf16 = QStringDecoder(QStringDecoder::Utf8);

QString string;
while (new_data_available()) {
    QByteArray chunk = get_new_data();
    string += toUtf16(chunk);
}
//! [2]

//! [3]
auto fromUtf16 = QStringEncoder(QStringEncoder::Utf8);

QByteArray encoded;
while (new_data_available()) {
    QString chunk = get_new_data();
    encoded += fromUtf16(chunk);
}
//! [3]
