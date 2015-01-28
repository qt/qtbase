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

#include <QtTest/QtTest>

#include <qsignalmapper.h>

class tst_QSignalMapper : public QObject
{
    Q_OBJECT
private slots:
    void mapped();
};

class QtTestObject : public QObject
{
    Q_OBJECT
public slots:
    void myslot(int id);
    void myslot(const QString &str);

signals:
    void mysignal(int);

public:
    void emit_mysignal(int);

    int id;
    QString str;
};

void QtTestObject::myslot(int id)
{
    this->id = id;
}

void QtTestObject::myslot(const QString &str)
{
    this->str = str;
}

void QtTestObject::emit_mysignal(int value)
{
    emit mysignal(value);
}

void tst_QSignalMapper::mapped()
{
    QSignalMapper mapper(0);

    QtTestObject target;
    QtTestObject src1;
    QtTestObject src2;
    QtTestObject src3;

    connect(&src1, SIGNAL(mysignal(int)), &mapper, SLOT(map()));
    connect(&src2, SIGNAL(mysignal(int)), &mapper, SLOT(map()));
    connect(&src3, SIGNAL(mysignal(int)), &mapper, SLOT(map()));

    mapper.setMapping(&src1, 7);
    mapper.setMapping(&src1, 1);
    mapper.setMapping(&src2, 2);
    mapper.setMapping(&src2, "two");
    mapper.setMapping(&src3, "three");

    connect(&mapper, SIGNAL(mapped(int)), &target, SLOT(myslot(int)));
    connect(&mapper, SIGNAL(mapped(QString)), &target, SLOT(myslot(QString)));

    src1.emit_mysignal(20);
    QVERIFY(target.id == 1);
    QVERIFY(target.str.isEmpty());

    src2.emit_mysignal(20);
    QVERIFY(target.id == 2);
    QVERIFY(target.str == "two");

    src3.emit_mysignal(20);
    QVERIFY(target.id == 2);
    QVERIFY(target.str == "three");
}

QTEST_MAIN(tst_QSignalMapper)
#include "tst_qsignalmapper.moc"
