// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QDebug>
#include <QIODevice>
#include <QFile>
#include <QString>
#include <QTemporaryFile>
#include <qtest.h>

class tst_QTemporaryFile : public QObject
{
    Q_OBJECT
private slots:
    void openclose_data();
    void openclose();
    void readwrite_data() { openclose_data(); }
    void readwrite();

private:
};

void tst_QTemporaryFile::openclose_data()
{
    QTest::addColumn<qint64>("amount");
    QTest::newRow("100")   << qint64(100);
    QTest::newRow("1000")  << qint64(1000);
    QTest::newRow("10000") << qint64(10000);
}

void tst_QTemporaryFile::openclose()
{
    QFETCH(qint64, amount);

    QBENCHMARK {
        for (qint64 i = 0; i < amount; ++i) {
            QTemporaryFile file;
            file.open();
            file.close();
        }
    }
}

void tst_QTemporaryFile::readwrite()
{
    QFETCH(qint64, amount);

    const int dataSize = 4096;
    QByteArray data;
    data.fill('a', dataSize);
    QBENCHMARK {
        for (qint64 i = 0; i < amount; ++i) {
            QTemporaryFile file;
            file.open();
            file.write(data);
            file.seek(0);
            file.read(dataSize);
            file.close();
        }
    }
}

QTEST_MAIN(tst_QTemporaryFile)

#include "tst_bench_qtemporaryfile.moc"
