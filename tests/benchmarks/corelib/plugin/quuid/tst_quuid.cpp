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
#include <QtCore/QUuid>
#include <QtTest/QtTest>

class tst_bench_QUuid : public QObject
{
    Q_OBJECT

public:
    tst_bench_QUuid()
    { }

private slots:
    void createUuid();
    void fromChar();
    void toString();
    void fromString();
    void toByteArray();
    void fromByteArray();
    void toRfc4122();
    void fromRfc4122();
    void createUuidV3();
    void createUuidV5();
    void toDataStream();
    void fromDataStream();
    void isNull();
    void operatorLess();
    void operatorMore();
};

void tst_bench_QUuid::createUuid()
{
    QBENCHMARK {
        QUuid::createUuid();
    }
}

void tst_bench_QUuid::fromChar()
{
    QBENCHMARK {
        QUuid uuid("{67C8770B-44F1-410A-AB9A-F9B5446F13EE}");
    }
}

void tst_bench_QUuid::toString()
{
    QUuid uuid = QUuid::createUuid();
    QBENCHMARK {
        uuid.toString();
    }
}

void tst_bench_QUuid::fromString()
{
    QString string = "{67C8770B-44F1-410A-AB9A-F9B5446F13EE}";
    QBENCHMARK {
        QUuid uuid(string);
    }
}

void tst_bench_QUuid::toByteArray()
{
    QUuid uuid = QUuid::createUuid();
    QBENCHMARK {
        uuid.toByteArray();
    }
}

void tst_bench_QUuid::fromByteArray()
{
    QByteArray string = "{67C8770B-44F1-410A-AB9A-F9B5446F13EE}";
    QBENCHMARK {
        QUuid uuid(string);
    }
}

void tst_bench_QUuid::toRfc4122()
{
    QUuid uuid = QUuid::createUuid();
    QBENCHMARK {
        uuid.toRfc4122();
    }
}

void tst_bench_QUuid::fromRfc4122()
{
    QByteArray string = QByteArray::fromHex("67C8770B44F1410AAB9AF9B5446F13EE");
    QBENCHMARK {
        QUuid uuid = QUuid::fromRfc4122(string);
    }
}

void tst_bench_QUuid::createUuidV3()
{
    QUuid ns = QUuid::createUuid();
    QByteArray name = QByteArray("Test");
    QBENCHMARK {
        QUuid uuid = QUuid::createUuidV3(ns, name);
    }
}

void tst_bench_QUuid::createUuidV5()
{
    QUuid ns = QUuid::createUuid();
    QByteArray name = QByteArray("Test");
    QBENCHMARK {
        QUuid uuid = QUuid::createUuidV5(ns, name);
    }
}

void tst_bench_QUuid::toDataStream()
{
    QUuid uuid1, uuid2;
    uuid1 = QUuid::createUuid();
    QByteArray ar;
    {
        QDataStream out(&ar,QIODevice::WriteOnly);
        QBENCHMARK {
            out << uuid1;
        }
    }
}

void tst_bench_QUuid::fromDataStream()
{
    QUuid uuid1, uuid2;
    uuid1 = QUuid::createUuid();
    QByteArray ar;
    {
        QDataStream out(&ar,QIODevice::WriteOnly);
        out << uuid1;
    }
    {
        QDataStream in(&ar,QIODevice::ReadOnly);
        QBENCHMARK {
            in >> uuid2;
        }
    }
}

void tst_bench_QUuid::isNull()
{
    QUuid uuid = QUuid();
    QBENCHMARK {
        uuid.isNull();
    }
}

void tst_bench_QUuid::operatorLess()
{
    QUuid uuid1, uuid2;
    uuid1 = QUuid::createUuid();
    uuid2 = QUuid::createUuid();
    QBENCHMARK {
        uuid1 < uuid2;
    }
}

void tst_bench_QUuid::operatorMore()
{
    QUuid uuid1, uuid2;
    uuid1 = QUuid::createUuid();
    uuid2 = QUuid::createUuid();
    QBENCHMARK {
        uuid1 > uuid2;
    }
}

QTEST_MAIN(tst_bench_QUuid);
#include "tst_quuid.moc"
