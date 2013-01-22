/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

// Select one of the scenarios below
#define SCENARIO 1

#if SCENARIO == 1
// this is the "no harm done" version. Only operator% is active,
// with NO_CAST * defined
#define P %
#undef QT_USE_FAST_OPERATOR_PLUS
#undef QT_USE_FAST_CONCATENATION
#define QT_NO_CAST_FROM_ASCII
#define QT_NO_CAST_TO_ASCII
#endif


#if SCENARIO == 2
// this is the "full" version. Operator+ is replaced by a QStringBuilder
// based version
// with NO_CAST * defined
#define P +
#define QT_USE_FAST_OPERATOR_PLUS
#define QT_USE_FAST_CONCATENATION
#define QT_NO_CAST_FROM_ASCII
#define QT_NO_CAST_TO_ASCII
#endif

#if SCENARIO == 3
// this is the "no harm done" version. Only operator% is active,
// with NO_CAST * _not_ defined
#define P %
#undef QT_USE_FAST_OPERATOR_PLUS
#undef QT_USE_FAST_CONCATENATION
#undef QT_NO_CAST_FROM_ASCII
#undef QT_NO_CAST_TO_ASCII
#endif

#if SCENARIO == 4
// this is the "full" version. Operator+ is replaced by a QStringBuilder
// based version
// with NO_CAST * _not_ defined
#define P +
#define QT_USE_FAST_OPERATOR_PLUS
#define QT_USE_FAST_CONCATENATION
#undef QT_NO_CAST_FROM_ASCII
#undef QT_NO_CAST_TO_ASCII
#endif


#include <qbytearray.h>
#include <qdebug.h>
#include <qstring.h>
#include <qstringbuilder.h>

#include <qtest.h>

#include <string>

#define COMPARE(a, b) QCOMPARE(a, b)
//#define COMPARE(a, b)

#define SEP(s) qDebug() << "\n\n-------- " s "  ---------";

#define LITERAL "some string literal"

class tst_qstringbuilder : public QObject
{
    Q_OBJECT

public:
    tst_qstringbuilder()
      : l1literal(LITERAL),
        l1string(LITERAL),
        ba(LITERAL),
        string(l1string),
        stdstring(LITERAL),
        stringref(&string, 2, 10),
        achar('c'),
        r2(QLatin1String(LITERAL LITERAL)),
        r3(QLatin1String(LITERAL LITERAL LITERAL)),
        r4(QLatin1String(LITERAL LITERAL LITERAL LITERAL)),
        r5(QLatin1String(LITERAL LITERAL LITERAL LITERAL LITERAL))
    {}


public:
    enum { N = 10000 };

    int run_traditional()
    {
        int s = 0;
        for (int i = 0; i < N; ++i) {
#if 0
            s += QString(l1string + l1string).size();
            s += QString(l1string + l1string + l1string).size();
            s += QString(l1string + l1string + l1string + l1string).size();
            s += QString(l1string + l1string + l1string + l1string + l1string).size();
#endif
            s += QString(achar + l1string + achar).size();
        }
        return s;
    }

    int run_builder()
    {
        int s = 0;
        for (int i = 0; i < N; ++i) {
#if 0
            s += QString(l1literal P l1literal).size();
            s += QString(l1literal P l1literal P l1literal).size();
            s += QString(l1literal P l1literal P l1literal P l1literal).size();
            s += QString(l1literal P l1literal P l1literal P l1literal P l1literal).size();
#endif
            s += QString(achar % l1literal % achar).size();
        }
        return s;
    }

private slots:

    void separator_0() {
        qDebug() << "\nIn each block the QStringBuilder based result appear first "
            "(with a 'b_' prefix), QStringBased second ('q_' prefix), std::string "
            "last ('s_' prefix)\n";
    }

    void separator_1() { SEP("literal + literal  (builder first)"); }

    void b_2_l1literal() {
        QBENCHMARK { r = l1literal P l1literal; }
        COMPARE(r, r2);
    }
    #ifndef QT_NO_CAST_FROM_ASCII
    void b_l1literal_LITERAL() {
        QBENCHMARK { r = l1literal P LITERAL; }
        COMPARE(r, r2);
    }
    #endif
    void q_2_l1string() {
        QBENCHMARK { r = l1string + l1string; }
        COMPARE(r, r2);
    }


    void separator_2() { SEP("2 strings"); }

    void b_2_string() {
        QBENCHMARK { r = string P string; }
        COMPARE(r, r2);
    }
    void q_2_string() {
        QBENCHMARK { r = string + string; }
        COMPARE(r, r2);
    }
    void s_2_string() {
        QBENCHMARK { stdr = stdstring + stdstring; }
        COMPARE(stdr, stdstring + stdstring);
    }


    void separator_2c() { SEP("2 string refs"); }

    void b_2_stringref() {
        QBENCHMARK { r = stringref % stringref; }
        COMPARE(r, QString(stringref.toString() + stringref.toString()));
    }
    void q_2_stringref() {
        QBENCHMARK { r = stringref.toString() + stringref.toString(); }
        COMPARE(r, QString(stringref % stringref));
    }


    void separator_2b() { SEP("3 strings"); }

    void b_3_string() {
        QBENCHMARK { r = string P string P string; }
        COMPARE(r, r3);
    }
    void q_3_string() {
        QBENCHMARK { r = string + string + string; }
        COMPARE(r, r3);
    }
    void s_3_string() {
        QBENCHMARK { stdr = stdstring + stdstring + stdstring; }
        COMPARE(stdr, stdstring + stdstring + stdstring);
    }

    void separator_2e() { SEP("4 strings"); }

    void b_4_string() {
        QBENCHMARK { r = string P string P string P string; }
        COMPARE(r, r4);
    }
    void q_4_string() {
        QBENCHMARK { r = string + string + string + string; }
        COMPARE(r, r4);
    }
    void s_4_string() {
        QBENCHMARK { stdr = stdstring + stdstring + stdstring + stdstring; }
        COMPARE(stdr, stdstring + stdstring + stdstring + stdstring);
    }



    void separator_2a() { SEP("string + literal  (builder first)"); }

    void b_string_l1literal() {
        QBENCHMARK { r = string % l1literal; }
        COMPARE(r, r2);
    }
    #ifndef QT_NO_CAST_FROM_ASCII
    void b_string_LITERAL() {
        QBENCHMARK { r = string P LITERAL; }
        COMPARE(r, r2);
    }
    void b_LITERAL_string() {
        QBENCHMARK { r = LITERAL P string; }
        COMPARE(r, r2);
    }
    #endif
    void b_string_l1string() {
        QBENCHMARK { r = string P l1string; }
        COMPARE(r, r2);
    }
    void q_string_l1literal() {
        QBENCHMARK { r = string + l1string; }
        COMPARE(r, r2);
    }
    void q_string_l1string() {
        QBENCHMARK { r = string + l1string; }
        COMPARE(r, r2);
    }
    void s_LITERAL_string() {
        QBENCHMARK { stdr = LITERAL + stdstring; }
        COMPARE(stdr, stdstring + stdstring);
    }


    void separator_3() { SEP("3 literals"); }

    void b_3_l1literal() {
        QBENCHMARK { r = l1literal P l1literal P l1literal; }
        COMPARE(r, r3);
    }
    void q_3_l1string() {
        QBENCHMARK { r = l1string + l1string + l1string; }
        COMPARE(r, r3);
    }
    void s_3_l1string() {
        QBENCHMARK { stdr = stdstring + LITERAL + LITERAL; }
        COMPARE(stdr, stdstring + stdstring + stdstring);
    }


    void separator_4() { SEP("4 literals"); }

    void b_4_l1literal() {
        QBENCHMARK { r = l1literal P l1literal P l1literal P l1literal; }
        COMPARE(r, r4);
    }
    void q_4_l1string() {
        QBENCHMARK { r = l1string + l1string + l1string + l1string; }
        COMPARE(r, r4);
    }


    void separator_5() { SEP("5 literals"); }

    void b_5_l1literal() {
        QBENCHMARK { r = l1literal P l1literal P l1literal P l1literal P l1literal; }
        COMPARE(r, r5);
    }

    void q_5_l1string() {
        QBENCHMARK { r = l1string + l1string + l1string + l1string + l1string; }
        COMPARE(r, r5);
    }


    void separator_6() { SEP("4 chars"); }

    void b_string_4_char() {
        QBENCHMARK { r = string + achar + achar + achar + achar; }
        COMPARE(r, QString(string P achar P achar P achar P achar));
    }

    void q_string_4_char() {
        QBENCHMARK { r = string + achar + achar + achar + achar; }
        COMPARE(r, QString(string P achar P achar P achar P achar));
    }

    void s_string_4_char() {
        QBENCHMARK { stdr = stdstring + 'c' + 'c' + 'c' + 'c'; }
        COMPARE(stdr, stdstring + 'c' + 'c' + 'c' + 'c');
    }


    void separator_7() { SEP("char + string + char"); }

    void b_char_string_char() {
        QBENCHMARK { r = achar + string + achar; }
        COMPARE(r, QString(achar P string P achar));
    }

    void q_char_string_char() {
        QBENCHMARK { r = achar + string + achar; }
        COMPARE(r, QString(achar P string P achar));
    }

    void s_char_string_char() {
        QBENCHMARK { stdr = 'c' + stdstring + 'c'; }
        COMPARE(stdr, 'c' + stdstring + 'c');
    }


    void separator_8() { SEP("string.arg"); }

    void b_string_arg() {
        const QString pattern = l1string + QString::fromLatin1("%1") + l1string;
        QBENCHMARK { r = l1literal P string P l1literal; }
        COMPARE(r, r3);
    }

    void q_string_arg() {
        const QString pattern = l1string + QLatin1String("%1") + l1string;
        QBENCHMARK { r = pattern.arg(string); }
        COMPARE(r, r3);
    }

    void q_bytearray_arg() {
        QByteArray result;
        QBENCHMARK { result = ba + ba + ba; }
    }


    void separator_9() { SEP("QString::reserve()"); }

    void b_reserve() {
        QBENCHMARK {
            r.clear();
            r = string P string P string P string;
        }
        COMPARE(r, r4);
    }
    void b_reserve_lit() {
        QBENCHMARK {
            r.clear();
            r = string P l1literal P string P string;
        }
        COMPARE(r, r4);
    }
    void s_reserve() {
        QBENCHMARK {
            r.clear();
            r.reserve(string.size() + string.size() + string.size() + string.size());
            r += string;
            r += string;
            r += string;
            r += string;
        }
        COMPARE(r, r4);
    }
    void s_reserve_lit() {
        QBENCHMARK {
            r.clear();
            //r.reserve(string.size() + qstrlen(l1string.latin1())
            //    + string.size() + string.size());
            r.reserve(1024);
            r += string;
            r += l1string;
            r += string;
            r += string;
        }
        COMPARE(r, r4);
    }

private:
    const QLatin1Literal l1literal;
    const QLatin1String l1string;
    const QByteArray ba;
    const QString string;
    const std::string stdstring;
    const QStringRef stringref;
    const QLatin1Char achar;
    const QString r2, r3, r4, r5;

    // short cuts for results
    QString r;
    std::string stdr;
};

QTEST_MAIN(tst_qstringbuilder)

#include "main.moc"
