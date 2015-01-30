/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <ioutils.h>
#include <proitems.h>

#include <QObject>

using namespace QMakeInternal;

class tst_qmakelib : public QObject
{
    Q_OBJECT

public:
    tst_qmakelib() {}
    virtual ~tst_qmakelib() {}

private slots:
    void quoteArgUnix_data();
    void quoteArgUnix();
    void quoteArgWin_data();
    void quoteArgWin();
    void pathUtils();

    void proStringList();
};

void tst_qmakelib::proStringList()
{
    ProStringList sl1;
    sl1 << ProString("qt") << ProString(QLatin1String("is"))
        << ProString(QStringLiteral("uncool")).mid(2);

    QCOMPARE(sl1.toQStringList(), QStringList() << "qt" << "is" << "cool");
    QCOMPARE(sl1.join(QStringLiteral("~~")), QStringLiteral("qt~~is~~cool"));

    ProStringList sl2;
    sl2 << ProString("mostly") << ProString("...") << ProString("is") << ProString("...");
    sl1.insertUnique(sl2);
    QCOMPARE(sl1.toQStringList(), QStringList() << "qt" << "is" << "cool" << "mostly" << "...");

    QVERIFY(sl1.contains("cool"));
    QVERIFY(!sl1.contains("COOL"));
    QVERIFY(sl1.contains("COOL", Qt::CaseInsensitive));
}

void tst_qmakelib::quoteArgUnix_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");

    static const struct {
        const char * const in;
        const char * const out;
    } vals[] = {
        { "", "''" },
        { "hallo", "hallo" },
        { "hallo du", "'hallo du'" },
        { "ha'llo", "'ha'\\''llo'" },
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++)
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << QString::fromLatin1(vals[i].out);
}

void tst_qmakelib::quoteArgUnix()
{
    QFETCH(QString, in);
    QFETCH(QString, out);

    QCOMPARE(IoUtils::shellQuoteUnix(in), out);
}

void tst_qmakelib::quoteArgWin_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");

    static const struct {
        const char * const in;
        const char * const out;
    } vals[] = {
        { "", "\"\"" },
        { "hallo", "hallo" },
        { "hallo du", "\"hallo du\"" },
        { "hallo\\", "hallo\\" },
        { "hallo du\\", "\"hallo du\\\\\"" },
        { "ha\"llo", "\"ha\\\"llo^\"" },
        { "ha\\\"llo", "\"ha\\\\\\\"llo^\"" },
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++)
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << QString::fromLatin1(vals[i].out);
}

void tst_qmakelib::quoteArgWin()
{
    QFETCH(QString, in);
    QFETCH(QString, out);

    QCOMPARE(IoUtils::shellQuoteWin(in), out);
}

void tst_qmakelib::pathUtils()
{
    QString afp = QCoreApplication::applicationFilePath();
    QVERIFY(IoUtils::exists(afp));
    QVERIFY(!IoUtils::exists(afp + "-tehfail"));
    QCOMPARE(IoUtils::fileType(afp), IoUtils::FileIsRegular);
    QString adp = QCoreApplication::applicationDirPath();
    QCOMPARE(IoUtils::fileType(adp), IoUtils::FileIsDir);

    QString fn0 = "file/path";
    QVERIFY(IoUtils::isRelativePath(fn0));

    QString fn1 = "/a/unix/file/path";
    QVERIFY(IoUtils::isAbsolutePath(fn1));
    QCOMPARE(IoUtils::pathName(fn1).toString(), QStringLiteral("/a/unix/file/"));
    QCOMPARE(IoUtils::fileName(fn1).toString(), QStringLiteral("path"));

#ifdef Q_OS_WIN
    QString fn0a = "c:file/path";
    QVERIFY(IoUtils::isRelativePath(fn0a));

    QString fn1a = "c:\\file\\path";
    QVERIFY(IoUtils::isAbsolutePath(fn1a));
#endif

    QString fnbase = "/another/dir";
    QCOMPARE(IoUtils::resolvePath(fnbase, fn0), QStringLiteral("/another/dir/file/path"));
    QCOMPARE(IoUtils::resolvePath(fnbase, fn1), QStringLiteral("/a/unix/file/path"));
}

QTEST_MAIN(tst_qmakelib)
#include "tst_qmakelib.moc"
