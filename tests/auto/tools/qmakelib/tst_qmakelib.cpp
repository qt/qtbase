/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "tst_qmakelib.h"

#include <ioutils.h>

using namespace QMakeInternal;

void tst_qmakelib::initTestCase()
{
    m_indir = QFINDTESTDATA("testdata");
    m_outdir = m_indir + QLatin1String("_build");
    m_env.insert(QStringLiteral("E1"), QStringLiteral("env var"));
#ifdef Q_OS_WIN
    m_env.insert(QStringLiteral("COMSPEC"), qgetenv("COMSPEC"));
#endif
    m_prop.insert(ProKey("P1"), ProString("prop val"));
    m_prop.insert(ProKey("QT_HOST_DATA/get"), ProString(m_indir));

    QVERIFY(!m_indir.isEmpty());
    QVERIFY(QDir(m_outdir).removeRecursively());
    QVERIFY(QDir().mkpath(m_outdir));
}

void tst_qmakelib::cleanupTestCase()
{
    QVERIFY(QDir(m_outdir).removeRecursively());
}

void tst_qmakelib::proString()
{
    QString qs1(QStringLiteral("this is a string"));

    ProString s1(qs1);
    QCOMPARE(s1.toQString(), QStringLiteral("this is a string"));

    ProString s2(qs1, 5, 8);
    QCOMPARE(s2.toQString(), QStringLiteral("is a str"));

    QCOMPARE(s2.hash(), 0x80000000);
    qHash(s2);
    QCOMPARE(s2.hash(), 90404018U);

    QCOMPARE(s2.mid(0, 10).toQString(), QStringLiteral("is a str"));
    QCOMPARE(s2.mid(1, 5).toQString(), QStringLiteral("s a s"));
    QCOMPARE(s2.mid(10, 3).toQString(), QStringLiteral(""));

    QString qs2(QStringLiteral("   spacy  string   "));
    QCOMPARE(ProString(qs2, 3, 13).trimmed().toQString(), QStringLiteral("spacy  string"));
    QCOMPARE(ProString(qs2, 1, 17).trimmed().toQString(), QStringLiteral("spacy  string"));

    QVERIFY(s2.toQStringRef().string()->isSharedWith(qs1));
    s2.prepend(ProString("there "));
    QCOMPARE(s2.toQString(), QStringLiteral("there is a str"));
    QVERIFY(!s2.toQStringRef().string()->isSharedWith(qs1));

    ProString s3("this is a longish string with bells and whistles");
    s3 = s3.mid(18, 17);
    // Prepend to detached string with lots of spare space in it.
    s3.prepend(ProString("another "));
    QCOMPARE(s3.toQString(), QStringLiteral("another string with bells"));

    // Note: The string still has plenty of spare space.
    s3.append(QLatin1Char('.'));
    QCOMPARE(s3.toQString(), QStringLiteral("another string with bells."));
    s3.append(QLatin1String(" eh?"));
    QCOMPARE(s3.toQString(), QStringLiteral("another string with bells. eh?"));

    s3.append(ProString(" yeah!"));
    QCOMPARE(s3.toQString(), QStringLiteral("another string with bells. eh? yeah!"));

    bool pending = false; // Not in string, but joining => add space
    s3.append(ProString("..."), &pending);
    QCOMPARE(s3.toQString(), QStringLiteral("another string with bells. eh? yeah! ..."));
    QVERIFY(pending);

    ProStringList sl1;
    sl1 << ProString("") << ProString("foo") << ProString("barbaz");
    ProString s4a("hallo");
    s4a.append(sl1);
    QCOMPARE(s4a.toQString(), QStringLiteral("hallo foo barbaz"));
    ProString s4b("hallo");
    pending = false;
    s4b.append(sl1, &pending);
    QCOMPARE(s4b.toQString(), QStringLiteral("hallo  foo barbaz"));
    ProString s4c;
    pending = false;
    s4c.append(sl1, &pending);
    QCOMPARE(s4c.toQString(), QStringLiteral(" foo barbaz"));
    // bizarreness
    ProString s4d("hallo");
    pending = false;
    s4d.append(sl1, &pending, true);
    QCOMPARE(s4d.toQString(), QStringLiteral("hallo foo barbaz"));
    ProString s4e;
    pending = false;
    s4e.append(sl1, &pending, true);
    QCOMPARE(s4e.toQString(), QStringLiteral("foo barbaz"));

    ProStringList sl2;
    sl2 << ProString("foo");
    ProString s5;
    s5.append(sl2);
    QCOMPARE(s5.toQString(), QStringLiteral("foo"));
    QVERIFY(s5.toQStringRef().string()->isSharedWith(*sl2.first().toQStringRef().string()));

    QCOMPARE(ProString("one") + ProString(" more"), QStringLiteral("one more"));
}

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

void QMakeTestHandler::print(const QString &fileName, int lineNo, int type, const QString &msg)
{
    QString pfx = ((type & QMakeParserHandler::CategoryMask) == QMakeParserHandler::WarningMessage)
                  ? QString::fromLatin1("WARNING: ") : QString();
    if (lineNo)
        doPrint(QStringLiteral("%1%2:%3: %4").arg(pfx, fileName, QString::number(lineNo), msg));
    else
        doPrint(QStringLiteral("%1%2").arg(pfx, msg));
}

void QMakeTestHandler::doPrint(const QString &msg)
{
    if (!expected.isEmpty() && expected.first() == msg) {
        expected.removeAt(0);
    } else {
        qWarning("%s", qPrintable(msg));
        printed = true;
    }
}

QTEST_MAIN(tst_qmakelib)
