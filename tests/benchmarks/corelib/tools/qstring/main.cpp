/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
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
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
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
    void toUpper_data();
    void toUpper();
    void toLower_data();
    void toLower();
    void toCaseFolded_data();
    void toCaseFolded();
};

tst_QString::tst_QString()
{
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
