// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QDebug>
#include <QIODevice>
#include <QFile>
#include <QString>

#include <qtest.h>

class tst_QIODevice : public QObject
{
    Q_OBJECT
private slots:
    void read_old();
    void read_old_data() { read_data(); }
    void peekAndRead();
    void peekAndRead_data() { read_data(); }
    //void read_new();
    //void read_new_data() { read_data(); }
private:
    void read_data();
};


void tst_QIODevice::read_data()
{
    QTest::addColumn<qint64>("size");
    QTest::newRow("10k")      << qint64(10 * 1024);
    QTest::newRow("100k")     << qint64(100 * 1024);
    QTest::newRow("1000k")    << qint64(1000 * 1024);
    QTest::newRow("10000k")   << qint64(10000 * 1024);
    QTest::newRow("100000k")  << qint64(100000 * 1024);
    QTest::newRow("1000000k") << qint64(1000000 * 1024);
}

void tst_QIODevice::read_old()
{
    QFETCH(qint64, size);

    QString name = "tmp" + QString::number(size);

    {
        QFile file(name);
        Q_UNUSED(file.open(QIODevice::WriteOnly));
        file.seek(size);
        file.write("x", 1);
        file.close();
    }

    QBENCHMARK {
        QFile file(name);
        Q_UNUSED(file.open(QIODevice::ReadOnly));
        QByteArray ba;
        qint64 s = size - 1024;
        file.seek(512);
        ba = file.read(s);  // crash happens during this read / assignment operation
    }

    {
        QFile file(name);
        file.remove();
    }
}

void tst_QIODevice::peekAndRead()
{
    QFETCH(qint64, size);

    QString name = "tmp" + QString::number(size);

    {
        QFile file(name);
        Q_UNUSED(file.open(QIODevice::WriteOnly));
        file.seek(size);
        file.write("x", 1);
        file.close();
    }

    QBENCHMARK {
        QFile file(name);
        Q_UNUSED(file.open(QIODevice::ReadOnly));

        QByteArray ba(size / 1024, Qt::Uninitialized);
        while (!file.atEnd()) {
            file.peek(ba.data(), ba.size());
            file.read(ba.data(), ba.size());
        }
    }

    {
        QFile file(name);
        file.remove();
    }
}

QTEST_MAIN(tst_QIODevice)

#include "tst_bench_qiodevice.moc"
