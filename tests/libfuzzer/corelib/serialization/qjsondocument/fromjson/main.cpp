// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QJsonDocument>

extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size) {
    QJsonDocument::fromJson(QByteArray::fromRawData(Data, Size));
    return 0;
}
