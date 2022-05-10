// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QCborStreamReader>

extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size) {
    QCborStreamReader reader(QByteArray::fromRawData(Data, Size));
    while (reader.isValid())
        reader.next(1024);
    return 0;
}
