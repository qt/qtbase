// Copyright (C) 2023 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QDataStream>
#include <QHash>
#include <QList>
#include <QMap>
#include <QSet>
#include <QTest>

// These tests are way too slow to be part of automatic unit tests
class tst_QDataStream : public QObject
{
    Q_OBJECT

    template <class T>
    void fill(T &input);

    void fill(QSet<qsizetype> &input);
    void fill(QMap<qsizetype, qsizetype> &input);
    void fill(QHash<qsizetype, qsizetype> &input);

    template <class T>
    void stream_big();

public slots:
    void initTestCase();

private slots:
    void stream_bigQString();
    void stream_bigQList();
    void stream_bigQSet();
    void stream_bigQMap();
    void stream_bigQHash();
};

void tst_QDataStream::initTestCase()
{
    qputenv("QTEST_FUNCTION_TIMEOUT", "9000000");
}

template <class T>
void tst_QDataStream::fill(T &input)
{
    constexpr qsizetype GiB = 1024 * 1024 * 1024;
    constexpr qsizetype BaseSize = 4 * GiB + 1;
    qDebug("Filling container with %lld entries", qint64(BaseSize));
    QElapsedTimer timer;
    timer.start();
    try {
        input.reserve(BaseSize);
        input.resize(BaseSize, 'a');
    } catch (const std::bad_alloc &) {
        QSKIP("Could not allocate 4 GiB of RAM.");
    }
    qDebug("Created dataset in %lld ms", timer.elapsed());
}

void tst_QDataStream::fill(QSet<qsizetype> &input)
{
    constexpr qsizetype GiB = 1024 * 1024 * 1024;
    constexpr qsizetype BaseSize = 4 * GiB + 1;
    qDebug("Filling container with %lld entries", qint64(BaseSize));
    QElapsedTimer timer;
    timer.start();
    try {
        input.reserve(BaseSize);
        for (qsizetype i = 0; i < BaseSize; ++i)
            input.insert(i);
    } catch (const std::bad_alloc &) {
        QSKIP("Could not allocate 4 Gi entries.");
    }
    qDebug("Created dataset in %lld ms", timer.elapsed());
}

void tst_QDataStream::fill(QMap<qsizetype, qsizetype> &input)
{
    constexpr qsizetype GiB = 1024 * 1024 * 1024;
    constexpr qsizetype BaseSize = 4 * GiB + 1;
    qDebug("Filling container with %lld entries", qint64(BaseSize));
    QElapsedTimer timer;
    timer.start();
    try {
        for (qsizetype i = 0; i < BaseSize; ++i)
            input.insert(i, i);
    } catch (const std::bad_alloc &) {
        QSKIP("Could not allocate 4 Gi entries.");
    }
    qDebug("Created dataset in %lld ms", timer.elapsed());
}

void tst_QDataStream::fill(QHash<qsizetype, qsizetype> &input)
{
    constexpr qsizetype GiB = 1024 * 1024 * 1024;
    constexpr qsizetype BaseSize = 4 * GiB + 1;
    qDebug("Filling container with %lld entries", qint64(BaseSize));
    QElapsedTimer timer;
    timer.start();
    try {
        input.reserve(BaseSize);
        for (qsizetype i = 0; i < BaseSize; ++i)
            input.emplace(i, i);
    } catch (const std::bad_alloc &) {
        QSKIP("Could not allocate 4 Gi entries.");
    }
    qDebug("Created dataset in %lld ms", timer.elapsed());
}

template <class T>
void tst_QDataStream::stream_big()
{
#if QT_POINTER_SIZE > 4
    QElapsedTimer timer;
    T input;
    fill(input);
    QByteArray ba;
    QDataStream inputstream(&ba, QIODevice::WriteOnly);
    timer.start();
    try {
        inputstream << input;
    } catch (const std::bad_alloc &) {
        QSKIP("Not enough memory to copy into QDataStream.");
    }
    qDebug("Streamed into QDataStream in %lld ms", timer.elapsed());
    T output;
    QDataStream outputstream(ba);
    timer.start();
    try {
        outputstream >> output;
    } catch (const std::bad_alloc &) {
        QSKIP("Not enough memory to copy out of QDataStream.");
    }
    qDebug("Streamed out of QDataStream in %lld ms", timer.elapsed());
    QCOMPARE(input.size(), output.size());
    QCOMPARE(input, output);
#else
    QSKIP("This test is 64-bit only.");
#endif
}

void tst_QDataStream::stream_bigQString()
{
    stream_big<QString>();
}
void tst_QDataStream::stream_bigQList()
{
    stream_big<QList<char>>();
}

void tst_QDataStream::stream_bigQSet()
{
    stream_big<QSet<qsizetype>>();
}

void tst_QDataStream::stream_bigQMap()
{
    stream_big<QMap<qsizetype, qsizetype>>();
}
void tst_QDataStream::stream_bigQHash()
{
    stream_big<QHash<qsizetype, qsizetype>>();
}

QTEST_MAIN(tst_QDataStream)

#include "tst_manualqdatastream.moc"
