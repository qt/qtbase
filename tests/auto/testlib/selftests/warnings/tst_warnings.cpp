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


#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>
#include <QtTest/QtTest>

class tst_Warnings: public QObject
{
    Q_OBJECT
private slots:
    void testWarnings();
    void testMissingWarnings();
    void testMissingWarningsRegularExpression();
    void testMissingWarningsWithData_data();
    void testMissingWarningsWithData();
};

void tst_Warnings::testWarnings()
{
    qWarning("Warning");

    QTest::ignoreMessage(QtWarningMsg, "Warning");
    qWarning("Warning");

    qWarning("Warning");

    qDebug("Debug");

    QTest::ignoreMessage(QtDebugMsg, "Debug");
    qDebug("Debug");

    qDebug("Debug");

    qInfo("Info");

    QTest::ignoreMessage(QtInfoMsg, "Info");
    qInfo("Info");

    qInfo("Info");

    QTest::ignoreMessage(QtDebugMsg, "Bubu");
    qDebug("Baba");
    qDebug("Bubu");
    qDebug("Baba");

    QTest::ignoreMessage(QtDebugMsg, QRegularExpression("^Bubu.*"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Baba.*"));
    qDebug("Bubublabla");
    qWarning("Babablabla");
    qDebug("Bubublabla");
    qWarning("Babablabla");

    // accept redundant space at end to keep compatibility with Qt < 5.2
    QTest::ignoreMessage(QtDebugMsg, "Bubu ");
    qDebug() << "Bubu";
}

void tst_Warnings::testMissingWarnings()
{
    QTest::ignoreMessage(QtWarningMsg, "Warning0");
    QTest::ignoreMessage(QtWarningMsg, "Warning1");
    QTest::ignoreMessage(QtWarningMsg, "Warning2");

    qWarning("Warning2");
}

void tst_Warnings::testMissingWarningsRegularExpression()
{
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Warning\\d\\d"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Warning\\s\\d"));

    qWarning("Warning11");
}

void tst_Warnings::testMissingWarningsWithData_data()
{
    QTest::addColumn<int>("dummy");

    QTest::newRow("first row") << 0;
    QTest::newRow("second row") << 1;
}

void tst_Warnings::testMissingWarningsWithData()
{
    QTest::ignoreMessage(QtWarningMsg, "Warning0");
    QTest::ignoreMessage(QtWarningMsg, "Warning1");
    QTest::ignoreMessage(QtWarningMsg, "Warning2");

    qWarning("Warning2");
}

QTEST_MAIN(tst_Warnings)

#include "tst_warnings.moc"
