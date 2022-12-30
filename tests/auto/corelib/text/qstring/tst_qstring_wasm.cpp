// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QString>
#include <QTest>

#include <emscripten/val.h>

void tst_QString_wasmTypes()
{
    // QString <-> emscripten::val
    {
        QString qtString("test string");
        const emscripten::val jsString = qtString.toJsString();
        QString qtStringCopy(qtString);
        qtString = qtString.toUpper(); // modify
        QCOMPARE(QString::fromJsString(jsString), qtStringCopy);
    }
    {
        QString longString;
        for (uint64_t i = 0; i < 1000; ++i)
            longString += "Lorem ipsum FTW";
        const emscripten::val jsString = longString.toJsString();
        QString qtStringCopy(longString);
        longString = longString.toUpper(); // modify
        QCOMPARE(QString::fromJsString(jsString), qtStringCopy);
    }
}
