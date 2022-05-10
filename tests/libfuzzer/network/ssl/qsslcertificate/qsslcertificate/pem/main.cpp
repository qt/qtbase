// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QSslCertificate>

extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size) {
    // to reduce noise and increase speed
    static char quiet[] = "QT_LOGGING_RULES=qt.*=false";
    static int pe = putenv(quiet);
    Q_UNUSED(pe);
    QSslCertificate ssl(QByteArray::fromRawData(Data, Size), QSsl::Pem);
    return 0;
}
