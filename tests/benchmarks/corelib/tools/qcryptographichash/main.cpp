/****************************************************************************
**
** Copyright (C) 2014 Intel Corporation
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QString>
#include <QtTest>

#include <time.h>

#ifdef Q_OS_WINCE
// no C89 time() on Windows CE:
// http://blogs.msdn.com/b/cenet/archive/2006/04/29/time-h-on-windows-ce.aspx
uint time(void *)
{
    return uint(-1);
}
#endif

class tst_bench_QCryptographicHash : public QObject
{
    Q_OBJECT
    QByteArray blockOfData;

public:
    tst_bench_QCryptographicHash();

private Q_SLOTS:
    void hash_data();
    void hash();
    void addData_data() { hash_data(); }
    void addData();
    void addDataChunked_data() { hash_data(); }
    void addDataChunked();
};

const int MaxCryptoAlgorithm = QCryptographicHash::Sha3_512;
const int MaxBlockSize = 65536;

const char *algoname(int i)
{
    switch (QCryptographicHash::Algorithm(i)) {
    case QCryptographicHash::Md4:
        return "md4-";
    case QCryptographicHash::Md5:
        return "md5-";
    case QCryptographicHash::Sha1:
        return "sha1-";
    case QCryptographicHash::Sha224:
        return "sha2_224-";
    case QCryptographicHash::Sha256:
        return "sha2_256-";
    case QCryptographicHash::Sha384:
        return "sha2_384-";
    case QCryptographicHash::Sha512:
        return "sha2_512-";
    case QCryptographicHash::Sha3_224:
        return "sha3_224-";
    case QCryptographicHash::Sha3_256:
        return "sha3_256-";
    case QCryptographicHash::Sha3_384:
        return "sha3_384-";
    case QCryptographicHash::Sha3_512:
        return "sha3_512-";
    }
    Q_UNREACHABLE();
    return 0;
}

tst_bench_QCryptographicHash::tst_bench_QCryptographicHash()
    : blockOfData(MaxBlockSize, Qt::Uninitialized)
{
#ifdef Q_OS_UNIX
    QFile urandom("/dev/urandom");
    if (urandom.open(QIODevice::ReadOnly | QIODevice::Unbuffered)) {
        QCOMPARE(urandom.read(blockOfData.data(), blockOfData.size()), qint64(MaxBlockSize));
    } else
#endif
    {
        qsrand(time(NULL));
        for (int i = 0; i < MaxBlockSize; ++i)
            blockOfData[i] = qrand();
    }
}

void tst_bench_QCryptographicHash::hash_data()
{
    QTest::addColumn<int>("algorithm");
    QTest::addColumn<QByteArray>("data");

    static const int datasizes[] = { 0, 1, 64, 65, 512, 4095, 4096, 4097, 65536 };
    for (uint i = 0; i < sizeof(datasizes)/sizeof(datasizes[0]); ++i) {
        Q_ASSERT(datasizes[i] < MaxBlockSize);
        QByteArray data = QByteArray::fromRawData(blockOfData.constData(), datasizes[i]);

        for (int algo = QCryptographicHash::Md4; algo <= MaxCryptoAlgorithm; ++algo)
            QTest::newRow(algoname(algo) + QByteArray::number(datasizes[i])) << algo << data;
    }
}

void tst_bench_QCryptographicHash::hash()
{
    QFETCH(int, algorithm);
    QFETCH(QByteArray, data);

    QCryptographicHash::Algorithm algo = QCryptographicHash::Algorithm(algorithm);
    QBENCHMARK {
        QCryptographicHash::hash(data, algo);
    }
}

void tst_bench_QCryptographicHash::addData()
{
    QFETCH(int, algorithm);
    QFETCH(QByteArray, data);

    QCryptographicHash::Algorithm algo = QCryptographicHash::Algorithm(algorithm);
    QCryptographicHash hash(algo);
    QBENCHMARK {
        hash.reset();
        hash.addData(data);
        hash.result();
    }
}

void tst_bench_QCryptographicHash::addDataChunked()
{
    QFETCH(int, algorithm);
    QFETCH(QByteArray, data);

    QCryptographicHash::Algorithm algo = QCryptographicHash::Algorithm(algorithm);
    QCryptographicHash hash(algo);
    QBENCHMARK {
        hash.reset();

        // add the data in chunks of 64 bytes
        for (int i = 0; i < data.size() / 64; ++i)
            hash.addData(data.constData() + 64 * i, 64);
        hash.addData(data.constData() + data.size() / 64 * 64, data.size() % 64);

        hash.result();
    }
}

QTEST_APPLESS_MAIN(tst_bench_QCryptographicHash)

#include "main.moc"
