// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QGuiApplication>
#include <QTextLayout>

extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size) {
    static int argc = 3;
    static char arg1[] = "fuzzer";
    static char arg2[] = "-platform";
    static char arg3[] = "minimal";
    static char *argv[] = {arg1, arg2, arg3, nullptr};
    static QGuiApplication qga(argc, argv);
    QTextLayout tl(QByteArray::fromRawData(Data, Size));
    tl.beginLayout();
    tl.endLayout();
    return 0;
}
