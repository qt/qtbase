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


#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>

class tst_Skip: public QObject
{
    Q_OBJECT

private slots:
    void test_data();
    void test();

    void emptytest_data();
    void emptytest();

    void singleSkip_data();
    void singleSkip();
};


void tst_Skip::test_data()
{
    QTest::addColumn<bool>("booll");
    QTest::newRow("local 1") << false;
    QTest::newRow("local 2") << true;

    QSKIP("skipping all");
}

void tst_Skip::test()
{
    qDebug("this line should never be reached, since we skip in the _data function");
}

void tst_Skip::emptytest_data()
{
    QSKIP("skipping all");
}

void tst_Skip::emptytest()
{
    qDebug("this line should never be reached, since we skip in the _data function");
}

void tst_Skip::singleSkip_data()
{
    QTest::addColumn<bool>("booll");
    QTest::newRow("local 1") << false;
    QTest::newRow("local 2") << true;
}

void tst_Skip::singleSkip()
{
    QFETCH(bool, booll);
    if (!booll)
        QSKIP("skipping one");
    qDebug("this line should only be reached once (%s)", booll ? "true" : "false");
}

QTEST_MAIN(tst_Skip)

#include "tst_skip.moc"
