// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QCoreApplication>
#include <QElapsedTimer>
#include <QTest>
#include <QScopeGuard>
#include <QCryptographicHash>

#include <thread>

Q_DECLARE_METATYPE(QCryptographicHash::Algorithm)

class tst_QCryptographicHashBigData : public QObject
{
    Q_OBJECT
private slots:
    void moreThan4GiBOfData_data();
    void moreThan4GiBOfData();
    void keccakBufferOverflow();
private:
    void ensureLargeData();
    std::vector<char> large;
};

void tst_QCryptographicHashBigData::ensureLargeData()
{
#if QT_POINTER_SIZE > 4
    QElapsedTimer timer;
    timer.start();
    const size_t GiB = 1024 * 1024 * 1024;
    if (large.size() == 4 * GiB + 1)
        return;
    try {
        large.resize(4 * GiB + 1, '\0');
    } catch (const std::bad_alloc &) {
        QSKIP("Could not allocate 4GiB plus one byte of RAM.");
    }
    QCOMPARE(large.size(), 4 * GiB + 1);
    large.back() = '\1';
    qDebug("created dataset in %lld ms", timer.elapsed());
#endif
}

void tst_QCryptographicHashBigData::moreThan4GiBOfData_data()
{
#if QT_POINTER_SIZE > 4
    if (ensureLargeData(); large.empty())
        return;
    QTest::addColumn<QCryptographicHash::Algorithm>("algorithm");
    auto me = QMetaEnum::fromType<QCryptographicHash::Algorithm>();
    auto row = [me] (QCryptographicHash::Algorithm algo) {
        QTest::addRow("%s", me.valueToKey(int(algo))) << algo;
    };
    // these are reasonably fast (O(secs))
    row(QCryptographicHash::Md4);
    row(QCryptographicHash::Md5);
    const bool runsOnCI = qgetenv("QTEST_ENVIRONMENT").split(' ').contains("ci");
    const bool runsUnderASan =
        #ifdef QT_ASAN_ENABLED
            true
        #else
            false
        #endif
            ;
    if (!runsOnCI || !runsUnderASan)
        row(QCryptographicHash::Sha1);
    if (!runsOnCI) {
        // This is important but so slow (O(minute)) that, on CI, it tends to time out.
        // Retain it for manual runs, all the same, as most dev machines will be fast enough.
        row(QCryptographicHash::Sha512);
    }
    // the rest is just too slow
#else
    QSKIP("This test is 64-bit only.");
#endif
}

void tst_QCryptographicHashBigData::moreThan4GiBOfData()
{
    QFETCH(const QCryptographicHash::Algorithm, algorithm);

    using MaybeThread = std::thread;

    QElapsedTimer timer;
    timer.start();
    const auto sg = qScopeGuard([&] {
        qDebug() << algorithm << "test finished in" << timer.restart() << "ms";
    });

    const auto view = QByteArrayView{large};
    const auto first = view.first(view.size() / 2);
    const auto last = view.sliced(view.size() / 2);

    QByteArray single;
    QByteArray chunked;

    auto t = MaybeThread{[&] {
        QCryptographicHash h(algorithm);
        h.addData(view);
        single = h.result();
    }};
    {
        QCryptographicHash h(algorithm);
        h.addData(first);
        h.addData(last);
        chunked = h.result();
    }
    t.join();

    QCOMPARE(single, chunked);
}

void tst_QCryptographicHashBigData::keccakBufferOverflow()
{
#if QT_POINTER_SIZE == 4
    QSKIP("This is a 64-bit-only test");
#else

    if (ensureLargeData(); large.empty())
        return;

    QElapsedTimer timer;
    timer.start();
    const auto sg = qScopeGuard([&] {
        qDebug() << "test finished in" << timer.restart() << "ms";
    });

    constexpr qsizetype magic = INT_MAX/4;
    QCOMPARE_GE(large.size(), size_t(magic + 1));

    QCryptographicHash hash(QCryptographicHash::Algorithm::Keccak_224);
    const auto first = QByteArrayView{large}.first(1);
    const auto second = QByteArrayView{large}.sliced(1, magic);
    hash.addData(first);
    hash.addData(second);
    (void)hash.resultView();
    QVERIFY(true); // didn't crash
#endif
}

QTEST_MAIN(tst_QCryptographicHashBigData)
#include "tst_qcryptographichash_bigdata.moc"
