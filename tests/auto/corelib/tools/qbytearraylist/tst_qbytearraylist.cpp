/****************************************************************************
**
** Copyright (C) 2013 by Southwest Research Institute (R)
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <QtTest/QtTest>
#include <qbytearraylist.h>

#include <qmetatype.h>

Q_DECLARE_METATYPE(QByteArrayList)

class tst_QByteArrayList : public QObject
{
    Q_OBJECT
private slots:
    void join() const;
    void join_data() const;
    void joinByteArray() const;
    void joinByteArray_data() const;
    void joinChar() const;
    void joinChar_data() const;
    void joinEmptiness() const;

    void operator_plus() const;
    void operator_plus_data() const;

    void initializeList() const;
};

void tst_QByteArrayList::join() const
{
    QFETCH(QByteArrayList, input);
    QFETCH(QByteArray, expectedResult);

    QCOMPARE(input.join(), expectedResult);
}

void tst_QByteArrayList::join_data() const
{
    QTest::addColumn<QByteArrayList>("input");
    QTest::addColumn<QByteArray>("expectedResult");

    QTest::newRow("data1") << QByteArrayList()
                           << QByteArray();

    QTest::newRow("data2") << QByteArrayList("one")
                           << QByteArray("one");

    QTest::newRow("data3") << (QByteArrayList() << "a" << "b")
                           << QByteArray("ab");

    QTest::newRow("data4") << (QByteArrayList() << "a" << "b" << "c")
                           << QByteArray("abc");
}

void tst_QByteArrayList::joinByteArray() const
{
    QFETCH(QByteArrayList, input);
    QFETCH(QByteArray, separator);
    QFETCH(QByteArray, expectedResult);

    QCOMPARE(input.join(separator), expectedResult);
}

void tst_QByteArrayList::joinByteArray_data() const
{
    QTest::addColumn<QByteArrayList>("input");
    QTest::addColumn<QByteArray>("separator");
    QTest::addColumn<QByteArray>("expectedResult");

    QTest::newRow("data1") << QByteArrayList()
                           << QByteArray()
                           << QByteArray();

    QTest::newRow("data2") << QByteArrayList()
                           << QByteArray("separator")
                           << QByteArray();

    QTest::newRow("data3") << QByteArrayList("one")
                           << QByteArray("separator")
                           << QByteArray("one");

    QTest::newRow("data4") << (QByteArrayList() << "a" << "b")
                           << QByteArray(" ")
                           << QByteArray("a b");

    QTest::newRow("data5") << (QByteArrayList() << "a" << "b" << "c")
                           << QByteArray(" ")
                           << QByteArray("a b c");

    QTest::newRow("data6") << (QByteArrayList()  << "a" << "b" << "c")
                           << QByteArray()
                           << QByteArray("abc");

    QTest::newRow("data7") << (QByteArrayList()  << "a" << "b" << "c")
                           << QByteArray("") //empty
                           << QByteArray("abc");
}

void tst_QByteArrayList::joinChar() const
{
    QFETCH(QByteArrayList, input);
    QFETCH(char, separator);
    QFETCH(QByteArray, expectedResult);

    QCOMPARE(input.join(separator), expectedResult);
}

void tst_QByteArrayList::joinChar_data() const
{
    QTest::addColumn<QByteArrayList>("input");
    QTest::addColumn<char>("separator");
    QTest::addColumn<QByteArray>("expectedResult");

    QTest::newRow("data1") << QByteArrayList()
                           << ' '
                           << QByteArray();

    QTest::newRow("data2") << (QByteArrayList() << "a" << "b")
                           << ' '
                           << QByteArray("a b");

    QTest::newRow("data3") << (QByteArrayList() << "a" << "b" << "c")
                           << ' '
                           << QByteArray("a b c");
}

void tst_QByteArrayList::joinEmptiness() const
{
    QByteArrayList list;
    QByteArray string = list.join(QByteArray());

    QVERIFY(string.isEmpty());
    QVERIFY(string.isNull());
}

void tst_QByteArrayList::operator_plus() const
{
    QFETCH(QByteArrayList, a1);
    QFETCH(QByteArrayList, a2);
    QFETCH(QByteArrayList, expectedResult);

    QCOMPARE(a1+a2, expectedResult);
    a1 += a2;
    QCOMPARE(a1, expectedResult);
}

void tst_QByteArrayList::operator_plus_data() const
{
    QTest::addColumn<QByteArrayList>("a1");
    QTest::addColumn<QByteArrayList>("a2");
    QTest::addColumn<QByteArrayList>("expectedResult");

    QTest::newRow("simpl") << ( QByteArrayList() << "a" )
                           << ( QByteArrayList() << "b" << "c" )
                           << ( QByteArrayList() << "a" << "b" << "c" );

    QTest::newRow("blank1") << QByteArrayList()
                            << QByteArrayList()
                            << QByteArrayList();

    QTest::newRow("blank2") << ( QByteArrayList() )
                            << ( QByteArrayList() << "b" << "c" )
                            << ( QByteArrayList() << "b" << "c" );

    QTest::newRow("empty1") << ( QByteArrayList() << "" )
                            << ( QByteArrayList() << "b" << "c" )
                            << ( QByteArrayList() << "" << "b" << "c" );

    QTest::newRow("empty2") << ( QByteArrayList() << "a" )
                            << ( QByteArrayList() << "" << "c" )
                            << ( QByteArrayList() << "a" << "" << "c" );
}

void tst_QByteArrayList::initializeList() const
{
#ifdef Q_COMPILER_INITIALIZER_LISTS
    // C++11 support is required
    QByteArrayList v1{QByteArray("hello"),"world",QByteArray("plop")};
    QCOMPARE(v1, (QByteArrayList() << "hello" << "world" << "plop"));
    QCOMPARE(v1, (QByteArrayList{"hello","world","plop"}));
#endif
}

QTEST_APPLESS_MAIN(tst_QByteArrayList)
#include "tst_qbytearraylist.moc"
