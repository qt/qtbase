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

#include <QDebug>
#include <QRegExp>
#include <QString>
#include <QFile>

#include <qtest.h>
#ifdef HAVE_BOOST
#include <boost/regex.hpp>
#endif

#ifdef HAVE_JSC
#include <QtScript>
#include "pcre/pcre.h"
#endif
#define ZLIB_VERSION "1.2.3.4"

class tst_qregexp : public QObject
{
    Q_OBJECT
public:
    tst_qregexp();
private slots:
    void escape_old();
    void escape_old_data() { escape_data(); }
    void escape_new1();
    void escape_new1_data() { escape_data(); }
    void escape_new2();
    void escape_new2_data() { escape_data(); }
    void escape_new3();
    void escape_new3_data() { escape_data(); }
    void escape_new4();
    void escape_new4_data() { escape_data(); }
/*
   JSC outperforms everything.
   Boost is less impressive then expected.
 */
    void simpleFind1();
    void rangeReplace1();
    void matchReplace1();

    void simpleFind2();
    void rangeReplace2();
    void matchReplace2();

    void simpleFindJSC();
    void rangeReplaceJSC();
    void matchReplaceJSC();

    void simpleFindBoost();
    void rangeReplaceBoost();
    void matchReplaceBoost();

/* those apply an (incorrect) regexp on entire source
   (this main.cpp). JSC appears to handle this
   (ab)use case best. QRegExp performs extremly bad.
 */
    void horribleWrongReplace1();
    void horribleReplace1();
    void horribleReplace2();
    void horribleWrongReplace2();
    void horribleWrongReplaceJSC();
    void horribleReplaceJSC();
    void horribleWrongReplaceBoost();
    void horribleReplaceBoost();
private:
    QString str1;
    QString str2;
    void escape_data();
};

tst_qregexp::tst_qregexp()
    :QObject()
    ,str1("We are all happy monkeys")
{
        QFile f(":/main.cpp");
        f.open(QFile::ReadOnly);
        str2=f.readAll();
}

static void verify(const QString &quoted, const QString &expected)
{
    if (quoted != expected)
        qDebug() << "ERROR:" << quoted << expected;
}

void tst_qregexp::escape_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("expected");

    QTest::newRow("escape 0") << "Hello world" << "Hello world";
    QTest::newRow("escape 1") << "(Hello world)" << "\\(Hello world\\)";
    {
        QString s;
        for (int i = 0; i < 10; ++i)
            s += "(escape)";
        QTest::newRow("escape 10") << s << QRegExp::escape(s);
    }
    {
        QString s;
        for (int i = 0; i < 100; ++i)
            s += "(escape)";
        QTest::newRow("escape 100") << s << QRegExp::escape(s);
    }
}

void tst_qregexp::escape_old()
{
    QFETCH(QString, pattern);
    QFETCH(QString, expected);

    QBENCHMARK {
        static const char meta[] = "$()*+.?[\\]^{|}";
        QString quoted = pattern;
        int i = 0;

        while (i < quoted.length()) {
            if (strchr(meta, quoted.at(i).toLatin1()) != 0)
                quoted.insert(i++, QLatin1Char('\\'));
            ++i;
        }

        verify(quoted, expected);
    }
}

void tst_qregexp::escape_new1()
{
    QFETCH(QString, pattern);
    QFETCH(QString, expected);

    QBENCHMARK {
        QString quoted;
        const int count = pattern.count();
        quoted.reserve(count * 2);
        const QLatin1Char backslash('\\');
        for (int i = 0; i < count; i++) {
            switch (pattern.at(i).toLatin1()) {
            case '$':
            case '(':
            case ')':
            case '*':
            case '+':
            case '.':
            case '?':
            case '[':
            case '\\':
            case ']':
            case '^':
            case '{':
            case '|':
            case '}':
                quoted.append(backslash);
            }
            quoted.append(pattern.at(i));
        }
        verify(quoted, expected);
    }
}

void tst_qregexp::escape_new2()
{
    QFETCH(QString, pattern);
    QFETCH(QString, expected);

    QBENCHMARK {
        int count = pattern.count();
        const QLatin1Char backslash('\\');
        QString quoted(count * 2, backslash);
        const QChar *patternData = pattern.data();
        QChar *quotedData = quoted.data();
        int escaped = 0;
        for ( ; --count >= 0; ++patternData) {
            const QChar c = *patternData;
            switch (c.unicode()) {
            case '$':
            case '(':
            case ')':
            case '*':
            case '+':
            case '.':
            case '?':
            case '[':
            case '\\':
            case ']':
            case '^':
            case '{':
            case '|':
            case '}':
                ++escaped;
                ++quotedData;
            }
            *quotedData = c;
            ++quotedData;
        }
        quoted.resize(pattern.size() + escaped);

        verify(quoted, expected);
    }
}

void tst_qregexp::escape_new3()
{
    QFETCH(QString, pattern);
    QFETCH(QString, expected);

    QBENCHMARK {
        QString quoted;
        const int count = pattern.count();
        quoted.reserve(count * 2);
        const QLatin1Char backslash('\\');
        for (int i = 0; i < count; i++) {
            switch (pattern.at(i).toLatin1()) {
            case '$':
            case '(':
            case ')':
            case '*':
            case '+':
            case '.':
            case '?':
            case '[':
            case '\\':
            case ']':
            case '^':
            case '{':
            case '|':
            case '}':
                quoted += backslash;
            }
            quoted += pattern.at(i);
        }

        verify(quoted, expected);
    }
}


static inline bool needsEscaping(int c)
{
    switch (c) {
    case '$':
    case '(':
    case ')':
    case '*':
    case '+':
    case '.':
    case '?':
    case '[':
    case '\\':
    case ']':
    case '^':
    case '{':
    case '|':
    case '}':
        return true;
    }
    return false;
}

void tst_qregexp::escape_new4()
{
    QFETCH(QString, pattern);
    QFETCH(QString, expected);

    QBENCHMARK {
        const int n = pattern.size();
        const QChar *patternData = pattern.data();
        // try to prevent copy if no escape is needed
        int i = 0;
        for (int i = 0; i != n; ++i) {
            const QChar c = patternData[i];
            if (needsEscaping(c.unicode()))
                break;
        }
        if (i == n) {
            verify(pattern, expected);
            // no escaping needed, "return pattern" should be done here.
            return;
        }
        const QLatin1Char backslash('\\');
        QString quoted(n * 2, backslash);
        QChar *quotedData = quoted.data();
        for (int j = 0; j != i; ++j)
            *quotedData++ = *patternData++;
        int escaped = 0;
        for (; i != n; ++i) {
            const QChar c = *patternData;
            if (needsEscaping(c.unicode())) {
                ++escaped;
                ++quotedData;
            }
            *quotedData = c;
            ++quotedData;
            ++patternData;
        }
        quoted.resize(n + escaped);
        verify(quoted, expected);
        // "return quoted"
    }
}


void tst_qregexp::simpleFind1()
{
    int roff;
    QRegExp rx("happy");
    rx.setPatternSyntax(QRegExp::RegExp);
    QBENCHMARK{
        roff = rx.indexIn(str1);
    }
    QCOMPARE(roff, 11);
}

void tst_qregexp::rangeReplace1()
{
    QString r;
    QRegExp rx("[a-f]");
    rx.setPatternSyntax(QRegExp::RegExp);
    QBENCHMARK{
        r = QString(str1).replace(rx, "-");
    }
    QCOMPARE(r, QString("W- -r- -ll h-ppy monk-ys"));
}

void tst_qregexp::matchReplace1()
{
    QString r;
    QRegExp rx("[^a-f]*([a-f]+)[^a-f]*");
    rx.setPatternSyntax(QRegExp::RegExp);
    QBENCHMARK{
        r = QString(str1).replace(rx, "\\1");
    }
    QCOMPARE(r, QString("eaeaae"));
}

void tst_qregexp::horribleWrongReplace1()
{
    QString r;
    QRegExp rx(".*#""define ZLIB_VERSION \"([0-9]+)\\.([0-9]+)\\.([0-9]+)\".*");
    rx.setPatternSyntax(QRegExp::RegExp);
    QBENCHMARK{
        r = QString(str2).replace(rx, "\\1.\\2.\\3");
    }
    QCOMPARE(r, str2);
}

void tst_qregexp::horribleReplace1()
{
    QString r;
    QRegExp rx(".*#""define ZLIB_VERSION \"([0-9]+)\\.([0-9]+)\\.([0-9]+).*");
    rx.setPatternSyntax(QRegExp::RegExp);
    QBENCHMARK{
        r = QString(str2).replace(rx, "\\1.\\2.\\3");
    }
    QCOMPARE(r, QString("1.2.3"));
}


void tst_qregexp::simpleFind2()
{
    int roff;
    QRegExp rx("happy");
    rx.setPatternSyntax(QRegExp::RegExp2);
    QBENCHMARK{
        roff = rx.indexIn(str1);
    }
    QCOMPARE(roff, 11);
}

void tst_qregexp::rangeReplace2()
{
    QString r;
    QRegExp rx("[a-f]");
    rx.setPatternSyntax(QRegExp::RegExp2);
    QBENCHMARK{
        r = QString(str1).replace(rx, "-");
    }
    QCOMPARE(r, QString("W- -r- -ll h-ppy monk-ys"));
}

void tst_qregexp::matchReplace2()
{
    QString r;
    QRegExp rx("[^a-f]*([a-f]+)[^a-f]*");
    rx.setPatternSyntax(QRegExp::RegExp2);
    QBENCHMARK{
        r = QString(str1).replace(rx, "\\1");
    }
    QCOMPARE(r, QString("eaeaae"));
}

void tst_qregexp::horribleWrongReplace2()
{
    QString r;
    QRegExp rx(".*#""define ZLIB_VERSION \"([0-9]+)\\.([0-9]+)\\.([0-9]+)\".*");
    rx.setPatternSyntax(QRegExp::RegExp2);
    QBENCHMARK{
        r = QString(str2).replace(rx, "\\1.\\2.\\3");
    }
    QCOMPARE(r, str2);
}

void tst_qregexp::horribleReplace2()
{
    QString r;
    QRegExp rx(".*#""define ZLIB_VERSION \"([0-9]+)\\.([0-9]+)\\.([0-9]+).*");
    rx.setPatternSyntax(QRegExp::RegExp2);
    QBENCHMARK{
        r = QString(str2).replace(rx, "\\1.\\2.\\3");
    }
    QCOMPARE(r, QString("1.2.3"));
}
void tst_qregexp::simpleFindJSC()
{
#ifdef HAVE_JSC
    int numr;
    const char * errmsg="  ";
    QString rxs("happy");
    JSRegExp *rx = jsRegExpCompile(rxs.utf16(), rxs.length(), JSRegExpDoNotIgnoreCase, JSRegExpSingleLine, 0, &errmsg);
    QVERIFY(rx != 0);
    QString s(str1);
    int offsetVector[3];
    QBENCHMARK{
        numr = jsRegExpExecute(rx, s.utf16(), s.length(), 0,  offsetVector, 3);
    }
    jsRegExpFree(rx);
    QCOMPARE(numr, 1);
    QCOMPARE(offsetVector[0], 11);
#else
    QSKIP("JSC is not enabled for this platform");
#endif
}

void tst_qregexp::rangeReplaceJSC()
{
#ifdef HAVE_JSC
    QScriptValue r;
    QScriptEngine engine;
    engine.globalObject().setProperty("s", str1);
    QScriptValue replaceFunc = engine.evaluate("(function() { return s.replace(/[a-f]/g, '-')  } )");
    QVERIFY(replaceFunc.isFunction());
    QBENCHMARK{
        r = replaceFunc.call(QScriptValue());
    }
    QCOMPARE(r.toString(), QString("W- -r- -ll h-ppy monk-ys"));
#else
    QSKIP("JSC is not enabled for this platform");
#endif
}

void tst_qregexp::matchReplaceJSC()
{
#ifdef HAVE_JSC
    QScriptValue r;
    QScriptEngine engine;
    engine.globalObject().setProperty("s", str1);
    QScriptValue replaceFunc = engine.evaluate("(function() { return s.replace(/[^a-f]*([a-f]+)[^a-f]*/g, '$1')  } )");
    QVERIFY(replaceFunc.isFunction());
    QBENCHMARK{
        r = replaceFunc.call(QScriptValue());
    }
    QCOMPARE(r.toString(), QString("eaeaae"));
#else
    QSKIP("JSC is not enabled for this platform");
#endif
}

void tst_qregexp::horribleWrongReplaceJSC()
{
#ifdef HAVE_JSC
    QScriptValue r;
    QScriptEngine engine;
    engine.globalObject().setProperty("s", str2);
    QScriptValue replaceFunc = engine.evaluate("(function() { return s.replace(/.*#""define ZLIB_VERSION \"([0-9]+)\\.([0-9]+)\\.([0-9]+)\".*/gm, '$1.$2.$3')  } )");
    QVERIFY(replaceFunc.isFunction());
    QBENCHMARK{
        r = replaceFunc.call(QScriptValue());
    }
    QCOMPARE(r.toString(), str2);
#else
    QSKIP("JSC is not enabled for this platform");
#endif
}

void tst_qregexp::horribleReplaceJSC()
{
#ifdef HAVE_JSC
    QScriptValue r;
    QScriptEngine engine;
    // the m flag doesn't actually work here; dunno
    engine.globalObject().setProperty("s", str2.replace('\n', ' '));
    QScriptValue replaceFunc = engine.evaluate("(function() { return s.replace(/.*#""define ZLIB_VERSION \"([0-9]+)\\.([0-9]+)\\.([0-9]+).*/gm, '$1.$2.$3')  } )");
    QVERIFY(replaceFunc.isFunction());
    QBENCHMARK{
        r = replaceFunc.call(QScriptValue());
    }
    QCOMPARE(r.toString(), QString("1.2.3"));
#else
    QSKIP("JSC is not enabled for this platform");
#endif
}

void tst_qregexp::simpleFindBoost()
{
#ifdef HAVE_BOOST
    int roff;
    boost::regex rx ("happy", boost::regex_constants::perl);
    std::string s = str1.toStdString();
    std::string::const_iterator start, end;
    start = s.begin();
    end = s.end();
    boost::match_flag_type flags = boost::match_default;
    QBENCHMARK{
        boost::match_results<std::string::const_iterator> what;
        regex_search(start, end, what, rx, flags);
        roff = (what[0].first)-start;
    }
    QCOMPARE(roff, 11);
#else
    QSKIP("Boost is not enabled for this platform");
#endif

}

void tst_qregexp::rangeReplaceBoost()
{
#ifdef HAVE_BOOST
    boost::regex pattern ("[a-f]", boost::regex_constants::perl);
    std::string s = str1.toStdString();
    std::string r;
    QBENCHMARK{
        r = boost::regex_replace (s, pattern, "-");
    }
    QCOMPARE(r, std::string("W- -r- -ll h-ppy monk-ys"));
#else
    QSKIP("Boost is not enabled for this platform");
#endif
}

void tst_qregexp::matchReplaceBoost()
{
#ifdef HAVE_BOOST
    boost::regex pattern ("[^a-f]*([a-f]+)[^a-f]*",boost::regex_constants::perl);
    std::string s = str1.toStdString();
    std::string r;
    QBENCHMARK{
        r = boost::regex_replace (s, pattern, "$1");
    }
    QCOMPARE(r, std::string("eaeaae"));
#else
    QSKIP("Boost is not enabled for this platform");
#endif
}

void tst_qregexp::horribleWrongReplaceBoost()
{
#ifdef HAVE_BOOST
    boost::regex pattern (".*#""define ZLIB_VERSION \"([0-9]+)\\.([0-9]+)\\.([0-9]+)\".*", boost::regex_constants::perl);
    std::string s = str2.toStdString();
    std::string r;
    QBENCHMARK{
        r = boost::regex_replace (s, pattern, "$1.$2.$3");
    }
    QCOMPARE(r, s);
#else
    QSKIP("Boost is not enabled for this platform");
#endif
}

void tst_qregexp::horribleReplaceBoost()
{
#ifdef HAVE_BOOST
    boost::regex pattern (".*#""define ZLIB_VERSION \"([0-9]+)\\.([0-9]+)\\.([0-9]+).*", boost::regex_constants::perl);
    std::string s = str2.toStdString();
    std::string r;
    QBENCHMARK{
        r = boost::regex_replace (s, pattern, "$1.$2.$3");
    }
    QCOMPARE(r, std::string("1.2.3"));
#else
    QSKIP("Boost is not enabled for this platform");
#endif
}

QTEST_MAIN(tst_qregexp)

#include "main.moc"
