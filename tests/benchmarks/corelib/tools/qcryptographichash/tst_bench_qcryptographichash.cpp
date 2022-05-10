// Copyright (C) 2017 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QRandomGenerator>
#include <QString>
#include <QTest>

#include <time.h>

class tst_QCryptographicHash : public QObject
{
    Q_OBJECT
    QByteArray blockOfData;

public:
    tst_QCryptographicHash();

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
    case QCryptographicHash::Keccak_224:
        return "keccak_224-";
    case QCryptographicHash::Keccak_256:
        return "keccak_256-";
    case QCryptographicHash::Keccak_384:
        return "keccak_384-";
    case QCryptographicHash::Keccak_512:
        return "keccak_512-";
    case QCryptographicHash::Blake2b_160:
        return "blake2b_160-";
    case QCryptographicHash::Blake2b_256:
        return "blake2b_256-";
    case QCryptographicHash::Blake2b_384:
        return "blake2b_384-";
    case QCryptographicHash::Blake2b_512:
        return "blake2b_512-";
    case QCryptographicHash::Blake2s_128:
        return "blake2s_128-";
    case QCryptographicHash::Blake2s_160:
        return "blake2s_160-";
    case QCryptographicHash::Blake2s_224:
        return "blake2s_224-";
    case QCryptographicHash::Blake2s_256:
        return "blake2s_256-";
    }
    Q_UNREACHABLE();
    return nullptr;
}

tst_QCryptographicHash::tst_QCryptographicHash()
    : blockOfData(MaxBlockSize, Qt::Uninitialized)
{
#ifdef Q_OS_UNIX
    QFile urandom("/dev/urandom");
    if (urandom.open(QIODevice::ReadOnly | QIODevice::Unbuffered)) {
        QCOMPARE(urandom.read(blockOfData.data(), blockOfData.size()), qint64(MaxBlockSize));
    } else
#endif
    {
        for (int i = 0; i < MaxBlockSize; ++i)
            blockOfData[i] = QRandomGenerator::global()->generate();
    }
}

void tst_QCryptographicHash::hash_data()
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

void tst_QCryptographicHash::hash()
{
    QFETCH(int, algorithm);
    QFETCH(QByteArray, data);

    QCryptographicHash::Algorithm algo = QCryptographicHash::Algorithm(algorithm);
    QBENCHMARK {
        QCryptographicHash::hash(data, algo);
    }
}

void tst_QCryptographicHash::addData()
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

void tst_QCryptographicHash::addDataChunked()
{
    QFETCH(int, algorithm);
    QFETCH(QByteArray, data);

    QCryptographicHash::Algorithm algo = QCryptographicHash::Algorithm(algorithm);
    QCryptographicHash hash(algo);
    QBENCHMARK {
        hash.reset();

        // add the data in chunks of 64 bytes
        for (int i = 0; i < data.size() / 64; ++i)
            hash.addData({data.constData() + 64 * i, 64});
        hash.addData({data.constData() + data.size() / 64 * 64, data.size() % 64});

        hash.result();
    }
}

QTEST_APPLESS_MAIN(tst_QCryptographicHash)

#include "tst_bench_qcryptographichash.moc"
