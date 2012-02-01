/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>

class tst_Warnings: public QObject
{
    Q_OBJECT
private slots:
    void testWarnings();
    void testMissingWarnings();
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

    QTest::ignoreMessage(QtDebugMsg, "Bubu");
    qDebug("Baba");
    qDebug("Bubu");
    qDebug("Baba");
}

void tst_Warnings::testMissingWarnings()
{
    QTest::ignoreMessage(QtWarningMsg, "Warning0");
    QTest::ignoreMessage(QtWarningMsg, "Warning1");
    QTest::ignoreMessage(QtWarningMsg, "Warning2");

    qWarning("Warning2");
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
