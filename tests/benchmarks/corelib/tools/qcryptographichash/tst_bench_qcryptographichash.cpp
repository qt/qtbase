// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2017 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QMetaEnum>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QString>
#include <QTest>

#include <qxpfunctional.h>
#include <numeric>

#include <time.h>

class tst_QCryptographicHash : public QObject
{
    Q_OBJECT
    QByteArray blockOfData;

    using Algorithm = QCryptographicHash::Algorithm;

public:
    tst_QCryptographicHash();

private Q_SLOTS:
    void hash_data();
    void hash();
    void addData_data() { hash_data(); }
    void addData();
    void addDataChunked_data() { hash_data(); }
    void addDataChunked();

    // QMessageAuthenticationCode:
    void hmac_hash_data() { hash_data(); }
    void hmac_hash();
    void hmac_addData_data() { hash_data(); }
    void hmac_addData();
    void hmac_setKey_data();
    void hmac_setKey();
};

const int MaxBlockSize = 65536;

static void for_each_algorithm(qxp::function_ref<void(QCryptographicHash::Algorithm, const char*) const> f)
{
    using A = QCryptographicHash::Algorithm;
    static const auto metaEnum = QMetaEnum::fromType<A>();
    for (int i = 0, value = metaEnum.value(i); value != -1; value = metaEnum.value(++i))
        f(A(value), metaEnum.key(i));
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
    QTest::addColumn<Algorithm>("algo");
    QTest::addColumn<QByteArray>("data");

    static const int datasizes[] = { 0, 1, 64, 65, 512, 4095, 4096, 4097, 65536 };
    for (uint i = 0; i < sizeof(datasizes)/sizeof(datasizes[0]); ++i) {
        Q_ASSERT(datasizes[i] < MaxBlockSize);
        QByteArray data = QByteArray::fromRawData(blockOfData.constData(), datasizes[i]);

        for_each_algorithm([&] (Algorithm algo, const char *name) {
            if (algo == Algorithm::NumAlgorithms)
                return;
            QTest::addRow("%s-%d", name, datasizes[i]) << algo << data;
        });
    }
}

#define SKIP_IF_NOT_SUPPORTED(algo) do { \
        if (!QCryptographicHash::supportsAlgorithm(algo)) \
            QSKIP("This algorithm is not supported in this configuration"); \
    } while (false) \
    /* end */

void tst_QCryptographicHash::hash()
{
    QFETCH(const Algorithm, algo);
    QFETCH(QByteArray, data);

    SKIP_IF_NOT_SUPPORTED(algo);

    QBENCHMARK {
        [[maybe_unused]]
        auto r = QCryptographicHash::hash(data, algo);
    }
}

void tst_QCryptographicHash::addData()
{
    QFETCH(const Algorithm, algo);
    QFETCH(QByteArray, data);

    SKIP_IF_NOT_SUPPORTED(algo);

    QCryptographicHash hash(algo);
    QBENCHMARK {
        hash.reset();
        hash.addData(data);
        [[maybe_unused]]
        auto r = hash.resultView();
    }
}

void tst_QCryptographicHash::addDataChunked()
{
    QFETCH(const Algorithm, algo);
    QFETCH(QByteArray, data);

    SKIP_IF_NOT_SUPPORTED(algo);

    QCryptographicHash hash(algo);
    QBENCHMARK {
        hash.reset();

        // add the data in chunks of 64 bytes
        for (int i = 0; i < data.size() / 64; ++i)
            hash.addData({data.constData() + 64 * i, 64});
        hash.addData({data.constData() + data.size() / 64 * 64, data.size() % 64});

        [[maybe_unused]]
        auto r = hash.resultView();
    }
}

static QByteArray hmacKey() {
    static QByteArray key = [] {
            QByteArray result(277, Qt::Uninitialized);
            std::iota(result.begin(), result.end(), uchar(0)); // uchar so wraps after UCHAR_MAX
            return result;
        }();
    return key;
}

void tst_QCryptographicHash::hmac_hash()
{
    QFETCH(const Algorithm, algo);
    QFETCH(const QByteArray, data);

    SKIP_IF_NOT_SUPPORTED(algo);

    const auto key = hmacKey();
    QBENCHMARK {
        [[maybe_unused]]
        auto r = QMessageAuthenticationCode::hash(data, key, algo);
    }
}

void tst_QCryptographicHash::hmac_addData()
{
    QFETCH(const Algorithm, algo);
    QFETCH(const QByteArray, data);

    SKIP_IF_NOT_SUPPORTED(algo);

    const auto key = hmacKey();
    QMessageAuthenticationCode mac(algo, key);
    QBENCHMARK {
        mac.reset();
        mac.addData(data);
        [[maybe_unused]]
        auto r = mac.resultView();
    }
}

void tst_QCryptographicHash::hmac_setKey_data()
{
    QTest::addColumn<Algorithm>("algo");
    for_each_algorithm([] (Algorithm algo, const char *name) {
        if (algo == Algorithm::NumAlgorithms)
            return;
        QTest::addRow("%s", name) << algo;
    });
}

void tst_QCryptographicHash::hmac_setKey()
{
    QFETCH(const Algorithm, algo);

    SKIP_IF_NOT_SUPPORTED(algo);

    const QByteArrayList keys = [] {
            QByteArrayList result;
            const auto fullKey = hmacKey();
            result.reserve(fullKey.size());
            for (auto i = fullKey.size(); i > 0; --i)
                result.push_back(fullKey.sliced(i));
            return result;
        }();

    QMessageAuthenticationCode mac(algo);
    QBENCHMARK {
        for (const auto &key : keys) {
            mac.setKey(key);
            mac.addData("abc", 3); // avoid lazy setKey()
        }
    }
}

#undef SKIP_IF_NOT_SUPPORTED

QTEST_APPLESS_MAIN(tst_QCryptographicHash)

#include "tst_bench_qcryptographichash.moc"
