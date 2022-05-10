// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QRegularExpression>

extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size) {
    QRegularExpression qre(QByteArray::fromRawData(Data, Size));
    qre.optimize();
    return 0;
}
