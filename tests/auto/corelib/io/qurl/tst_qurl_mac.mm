// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtCore/private/qcore_mac_p.h>

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

using namespace Qt::StringLiterals;

void tst_QUrl_mactypes_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("cfOutput");
    QTest::addColumn<QString>("nsOutput");

    auto addSimple = [](const char *label, QLatin1StringView urll1) {
        QString url = urll1;
        QTest::addRow("%s", label) << url << url << url;
    };
    addSimple("empty", {});

    addSimple("https-empty-path", "https://example.com"_L1);
    addSimple("https-nonempty-path", "https://example.com/"_L1);
    addSimple("https-query", "https://example.com/?a=b"_L1);
    addSimple("https-fragment", "https://example.com/#id"_L1);
    addSimple("https-query-fragment", "https://example.com/?a=b#id"_L1);
    addSimple("file-root", "file:///"_L1);
    addSimple("file-path", "file:///etc/passwd"_L1);

    addSimple("file-relative", "file:README.txt"_L1);
    addSimple("file-relative-dotdot", "file:../README.txt"_L1);
    addSimple("uri-relative", "README.txt"_L1);
    addSimple("uri-relative-dotdot", "../README.txt"_L1);

    // QUrl retains [] unencoded, unlike CFURL & NSURL
    QTest::newRow("gen-delims") << "x://:@host/:@/[]?:/?@[]?#:/?@[]"
                                << "x://:@host/:@/%5B%5D?:/?@%5B%5D?#:/?@%5B%5D"
                                << "x://:@host/:@/%5B%5D?:/?@%5B%5D?#:/?@%5B%5D";
}

void tst_QUrl_mactypes()
{
    QFETCH(QString, input);
    QFETCH(QString, cfOutput);
    QFETCH(QString, nsOutput);
    QUrl qtUrl(input);
    QUrl otherUrl = qtUrl.isEmpty() ? QUrl("https://example.com") : QUrl();

    // confirm the conversions result in what we expect it to result
    CFURLRef cfUrl = qtUrl.toCFURL();
    QCFString cfStr = CFURLGetString(cfUrl);
    QCOMPARE(QString(cfStr), cfOutput);

    const NSURL *nsUrl = qtUrl.toNSURL();
    QVERIFY(nsUrl);
    const NSString *nsString = [nsUrl absoluteString];
    QVERIFY(nsString);
    QCOMPARE(QString::fromNSString(nsString), nsOutput);

    // confirm that roundtripping works and the equality operator does too
    QUrl qtCfUrl = QUrl::fromCFURL(cfUrl);
    if (input == cfOutput) {
        QCOMPARE(qtCfUrl, qtUrl);
        QCOMPARE_NE(qtCfUrl, otherUrl);
    }
    QCOMPARE(qtCfUrl.isEmpty(), qtUrl.isEmpty());

    QUrl qtNsUrl = QUrl::fromNSURL(nsUrl);
    if (input == nsOutput) {
        QCOMPARE(qtNsUrl, qtUrl);
        QCOMPARE_NE(qtNsUrl, otherUrl);
    }
    QCOMPARE(qtNsUrl.isEmpty(), qtUrl.isEmpty());
}
