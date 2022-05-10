// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTextStream>

extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size) {
    QTextStream qts(QByteArray::fromRawData(Data, Size));
    float f;
    qts >> f;
    return 0;
}
