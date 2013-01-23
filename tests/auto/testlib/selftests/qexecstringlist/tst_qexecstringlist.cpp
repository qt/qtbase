/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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


#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>

class tst_QExecStringList: public QObject
{
    Q_OBJECT

private slots:
    void testA() const;
    void testB() const;
    void testB_data() const;
    void testC() const;
};

void tst_QExecStringList::testA() const
{
}

void tst_QExecStringList::testB() const
{
    QFETCH(bool, dummy);
    Q_UNUSED(dummy);
}

void tst_QExecStringList::testB_data() const
{
    QTest::addColumn<bool>("dummy");

    QTest::newRow("Data1") << false;
    QTest::newRow("Data2") << false;
    QTest::newRow("Data3") << false;
}

void tst_QExecStringList::testC() const
{
}

int main(int argc,char *argv[])
{
    QCoreApplication app(argc, argv);

    tst_QExecStringList test;

    QTest::qExec(&test, app.arguments());
    QTest::qExec(&test, QStringList("appName"));
    QTest::qExec(&test, QStringList("appName") << "testA");
    QTest::qExec(&test, QStringList("appName") << "testB");
    QTest::qExec(&test, QStringList("appName") << "testB:Data2");
    QTest::qExec(&test, QStringList("appName") << "testC");

    return 0;
}

#include "tst_qexecstringlist.moc"
