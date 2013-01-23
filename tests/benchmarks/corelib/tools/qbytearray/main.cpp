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
#include <QDebug>
#include <QIODevice>
#include <QFile>
#include <QString>

#include <qtest.h>


class tst_qbytearray : public QObject
{
    Q_OBJECT
private slots:
    void append();
    void append_data();
};


void tst_qbytearray::append_data()
{
    QTest::addColumn<int>("size");
    QTest::newRow("1")         << int(1);
    QTest::newRow("10")        << int(10);
    QTest::newRow("100")       << int(100);
    QTest::newRow("1000")      << int(1000);
    QTest::newRow("10000")     << int(10000);
    QTest::newRow("100000")    << int(100000);
    QTest::newRow("1000000")   << int(1000000);
    QTest::newRow("10000000")  << int(10000000);
    QTest::newRow("100000000") << int(100000000);
}

void tst_qbytearray::append()
{
    QFETCH(int, size);

    QByteArray ba;
    QBENCHMARK {
        QByteArray ba2(size, 'x');
        ba.append(ba2);
        ba.clear();
    }
}


QTEST_MAIN(tst_qbytearray)

#include "main.moc"
