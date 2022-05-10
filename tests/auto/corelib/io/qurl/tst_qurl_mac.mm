// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

void tst_QUrl_macTypes()
{
    {
        QUrl qtUrl("example.com");
        const CFURLRef cfUrl = qtUrl.toCFURL();
        QCOMPARE(QUrl::fromCFURL(cfUrl), qtUrl);
        qtUrl.setUrl("www.example.com");
        QVERIFY(QUrl::fromCFURL(cfUrl) != qtUrl);
    }
    {
        QUrl qtUrl("example.com");
        const NSURL *nsUrl = qtUrl.toNSURL();
        QCOMPARE(QUrl::fromNSURL(nsUrl), qtUrl);
        qtUrl.setUrl("www.example.com");
        QVERIFY(QUrl::fromNSURL(nsUrl) != qtUrl);
    }
}
