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
#include <QStringList>
#include <QFile>
#include <QtTest/QtTest>

class tst_QString: public QObject
{
    Q_OBJECT
public:
    tst_QString();
private slots:
    void section_regexp_data() { section_data_impl(); }
    void section_regexp() { section_impl<QRegExp>(); }
    void section_regularexpression_data() { section_data_impl(); }
    void section_regularexpression() { section_impl<QRegularExpression>(); }
    void section_string_data() { section_data_impl(false); }
    void section_string() { section_impl<QString>(); }

    void toUpper_data();
    void toUpper();
    void toLower_data();
    void toLower();
    void toCaseFolded_data();
    void toCaseFolded();

private:
    void section_data_impl(bool includeRegExOnly = true);
    template <typename RX> void section_impl();
};

tst_QString::tst_QString()
{
}

void tst_QString::section_data_impl(bool includeRegExOnly)
{
    QTest::addColumn<QString>("s");
    QTest::addColumn<QString>("sep");
    QTest::addColumn<bool>("isRegExp");

    QTest::newRow("IPv4") << QStringLiteral("192.168.0.1") << QStringLiteral(".") << false;
    QTest::newRow("IPv6") << QStringLiteral("2001:0db8:85a3:0000:0000:8a2e:0370:7334") << QStringLiteral(":") << false;
    if (includeRegExOnly) {
        QTest::newRow("IPv6-reversed-roles") << QStringLiteral("2001:0db8:85a3:0000:0000:8a2e:0370:7334") << QStringLiteral("\\d+") << true;
        QTest::newRow("IPv6-complex") << QStringLiteral("2001:0db8:85a3:0000:0000:8a2e:0370:7334") << QStringLiteral("(\\d+):\\1") << true;
    }
}

template <typename RX>
inline QString escape(const QString &s)
{ return RX::escape(s); }

template <>
inline QString escape<QString>(const QString &s)
{ return s; }

template <typename RX>
inline void optimize(RX &) {}

template <>
inline void optimize(QRegularExpression &rx)
{ rx.optimize(); }

template <typename RX>
void tst_QString::section_impl()
{
    QFETCH(QString, s);
    QFETCH(QString, sep);
    QFETCH(bool, isRegExp);

    RX rx(isRegExp ? sep : escape<RX>(sep));
    optimize(rx);
    for (int i = 0; i < 20; ++i)
        (void) s.count(rx); // make (s, rx) hot

    QBENCHMARK {
        const QString result = s.section(rx, 0, 16);
        Q_UNUSED(result);
    }
}

void tst_QString::toUpper_data()
{
    QTest::addColumn<QString>("s");

    QString lowerLatin1(300, QChar('a'));
    QString upperLatin1(300, QChar('A'));

    QString lowerDeseret;
    {
        QString pattern;
        pattern += QChar(QChar::highSurrogate(0x10428));
        pattern += QChar(QChar::lowSurrogate(0x10428));
        for (int i = 0; i < 300 / pattern.size(); ++i)
            lowerDeseret += pattern;
    }
    QString upperDeseret;
    {
        QString pattern;
        pattern += QChar(QChar::highSurrogate(0x10400));
        pattern += QChar(QChar::lowSurrogate(0x10400));
        for (int i = 0; i < 300 / pattern.size(); ++i)
            upperDeseret += pattern;
    }

    QString lowerLigature(600, QChar(0xFB03));

    QTest::newRow("600<a>") << (lowerLatin1 + lowerLatin1);
    QTest::newRow("600<A>") << (upperLatin1 + upperLatin1);

    QTest::newRow("300<a>+300<A>") << (lowerLatin1 + upperLatin1);
    QTest::newRow("300<A>+300<a>") << (upperLatin1 + lowerLatin1);

    QTest::newRow("300<10428>") << (lowerDeseret + lowerDeseret);
    QTest::newRow("300<10400>") << (upperDeseret + upperDeseret);

    QTest::newRow("150<10428>+150<10400>") << (lowerDeseret + upperDeseret);
    QTest::newRow("150<10400>+150<10428>") << (upperDeseret + lowerDeseret);

    QTest::newRow("300a+150<10400>") << (lowerLatin1 + upperDeseret);
    QTest::newRow("300a+150<10428>") << (lowerLatin1 + lowerDeseret);
    QTest::newRow("300A+150<10400>") << (upperLatin1 + upperDeseret);
    QTest::newRow("300A+150<10428>") << (upperLatin1 + lowerDeseret);

    QTest::newRow("600<FB03> (ligature)") << lowerLigature;
}

void tst_QString::toUpper()
{
    QFETCH(QString, s);

    QBENCHMARK {
        s.toUpper();
    }
}

void tst_QString::toLower_data()
{
    toUpper_data();
}

void tst_QString::toLower()
{
    QFETCH(QString, s);

    QBENCHMARK {
        s.toLower();
    }
}

void tst_QString::toCaseFolded_data()
{
    toUpper_data();
}

void tst_QString::toCaseFolded()
{
    QFETCH(QString, s);

    QBENCHMARK {
        s.toCaseFolded();
    }
}

QTEST_APPLESS_MAIN(tst_QString)

#include "main.moc"
