// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QCoreApplication>
#include <QByteArray>
#include <QSslCertificate>

extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size) {
    // to reduce noise and increase speed
    static char quiet[] = "QT_LOGGING_RULES=qt.*=false";
    static int pe = putenv(quiet);
    Q_UNUSED(pe);
    static int argc = 1;
    static char arg1[] = "fuzzer";
    static char *argv[] = { arg1, nullptr };
    QCoreApplication app(argc, argv);
    QSslCertificate ssl(QByteArray::fromRawData(Data, Size), QSsl::Pem);
    return 0;
}
