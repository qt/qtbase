// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QXmlStreamReader>

extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size) {
    QXmlStreamReader reader(QByteArray::fromRawData(Data, Size));
    while (!reader.atEnd())
        reader.readNext();
    return 0;
}
