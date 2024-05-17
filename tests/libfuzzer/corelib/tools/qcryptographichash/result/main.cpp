// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QCryptographicHash>

extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size) {
    for (QCryptographicHash::Algorithm algo = QCryptographicHash::Md4;
         algo <= QCryptographicHash::RealSha3_512;
         algo = QCryptographicHash::Algorithm(algo + 1)) {
        QCryptographicHash qh(algo);
        qh.addData(QByteArray::fromRawData(Data, Size));
        qh.result();
    }
    return 0;
}
