// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2016 Ruslan Nigmatullin <euroelessar@yandex.ru>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

//! [0]
    QByteArray key = "key";
    QByteArray message = "The quick brown fox jumps over the lazy dog";
//! [0]

//! [1]
    QMessageAuthenticationCode code(QCryptographicHash::Sha256, key);
    code.addData(message);
    code.result().toHex(); // returns "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8"
//! [1]

//! [2]
    QMessageAuthenticationCode::hash(message, key, QCryptographicHash::Sha256).toHex();
//! [2]
}
