// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QCborValue>

extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size) {
    QCborValue::fromCbor(QByteArray::fromRawData(Data, Size));
    return 0;
}
