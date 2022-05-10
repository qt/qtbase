// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QCoreApplication>
#include <QtCore/QUuid>
#include <QTest>

class tst_QUuid : public QObject
{
    Q_OBJECT

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

void tst_QUuid::createUuid()
{
    QBENCHMARK {
        [[maybe_unused]] auto r = QUuid::createUuid();
    }
}

void tst_QUuid::fromChar()
{
    QBENCHMARK {
        QUuid uuid("{67C8770B-44F1-410A-AB9A-F9B5446F13EE}");
    }
}

void tst_QUuid::toString()
{
    QUuid uuid = QUuid::createUuid();
    QBENCHMARK {
        [[maybe_unused]] auto r = uuid.toString();
    }
}

void tst_QUuid::fromString()
{
    QString string = "{67C8770B-44F1-410A-AB9A-F9B5446F13EE}";
    QBENCHMARK {
        QUuid uuid(string);
    }
}

void tst_QUuid::toByteArray()
{
    QUuid uuid = QUuid::createUuid();
    QBENCHMARK {
        [[maybe_unused]] auto r = uuid.toByteArray();
    }
}

void tst_QUuid::fromByteArray()
{
    QByteArray string = "{67C8770B-44F1-410A-AB9A-F9B5446F13EE}";
    QBENCHMARK {
        QUuid uuid(string);
    }
}

void tst_QUuid::toRfc4122()
{
    QUuid uuid = QUuid::createUuid();
    QBENCHMARK {
        [[maybe_unused]] auto r = uuid.toRfc4122();
    }
}

void tst_QUuid::fromRfc4122()
{
    QByteArray string = QByteArray::fromHex("67C8770B44F1410AAB9AF9B5446F13EE");
    QBENCHMARK {
        QUuid uuid = QUuid::fromRfc4122(string);
        Q_UNUSED(uuid)
    }
}

void tst_QUuid::createUuidV3()
{
    QUuid ns = QUuid::createUuid();
    QByteArray name = QByteArray("Test");
    QBENCHMARK {
        QUuid uuid = QUuid::createUuidV3(ns, name);
        Q_UNUSED(uuid)
    }
}

void tst_QUuid::createUuidV5()
{
    QUuid ns = QUuid::createUuid();
    QByteArray name = QByteArray("Test");
    QBENCHMARK {
        QUuid uuid = QUuid::createUuidV5(ns, name);
        Q_UNUSED(uuid)
    }
}

void tst_QUuid::toDataStream()
{
    QUuid uuid = QUuid::createUuid();
    QByteArray ar;
    {
        QDataStream out(&ar,QIODevice::WriteOnly);
        QBENCHMARK {
            out << uuid;
        }
    }
}

void tst_QUuid::fromDataStream()
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

void tst_QUuid::isNull()
{
    QUuid uuid = QUuid();
    QBENCHMARK {
        [[maybe_unused]] auto r = uuid.isNull();
    }
}

void tst_QUuid::operatorLess()
{
    QUuid uuid1, uuid2;
    uuid1 = QUuid::createUuid();
    uuid2 = QUuid::createUuid();
    QBENCHMARK {
        [[maybe_unused]] auto r = uuid1 < uuid2;
    }
}

void tst_QUuid::operatorMore()
{
    QUuid uuid1, uuid2;
    uuid1 = QUuid::createUuid();
    uuid2 = QUuid::createUuid();
    QBENCHMARK {
        [[maybe_unused]] auto r = uuid1 > uuid2;
    }
}

QTEST_MAIN(tst_QUuid)

#include "tst_bench_quuid.moc"
