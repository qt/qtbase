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
    QMessageAuthenticationCode code(QCryptographicHash::Sha1);
    code.setKey(key);
    code.addData(message);
    code.result().toHex();      // returns "de7c9b85b8b78aa6bc8a7a36f70a90701c9db4d9"
//! [1]

//! [2]
    QMessageAuthenticationCode::hash(message, key, QCryptographicHash::Sha1).toHex();
//! [2]
}
