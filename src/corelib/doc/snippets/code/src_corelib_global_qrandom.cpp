// Copyright (C) 2018 Intel Corporation.
// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
    quint32 value = QRandomGenerator::global()->generate();
//! [0]

//! [1]
    QRandomGenerator prng1(1234), prng2(1234);
    Q_ASSERT(prng1.generate() == prng2.generate());
    Q_ASSERT(prng1.generate64() == prng2.generate64());
//! [1]

//! [2]
    int x = QRandomGenerator::global()->generate();
    int y = QRandomGenerator::global()->generate();
    int w = QRandomGenerator::global()->bounded(16384);
    int h = QRandomGenerator::global()->bounded(16384);
//! [2]

//! [3]
    std::uniform_real_distribution dist(1, 2.5);
    return dist(*QRandomGenerator::global());
//! [3]

//! [4]
    std::seed_seq sseq(seedBuffer, seedBuffer + len);
    QRandomGenerator generator(sseq);
//! [4]

//! [5]
    std::seed_seq sseq(begin, end);
    QRandomGenerator generator(sseq);
//! [5]

//! [6]
    while (z--)
        generator.generate();
//! [6]

//! [7]
    std::generate(begin, end, [this]() { return generate(); });
//! [7]

//! [8]
    std::generate(begin, end, []() { return QRandomGenerator::global()->generate64(); });
//! [8]

//! [9]
    QList<quint32> list;
    list.resize(16);
    QRandomGenerator::global()->fillRange(list.data(), list.size());
//! [9]

//! [10]
    quint32 array[2];
    QRandomGenerator::global()->fillRange(array);
//! [10]

//! [11]
    QRandomGenerator64 rd;
    return std::generate_canonical<qreal, std::numeric_limits<qreal>::digits>(rd);
//! [11]

//! [12]
    return generateDouble() * highest;
//! [12]

//! [13]
    quint32 v = QRandomGenerator::global()->bounded(256);
//! [13]

//! [14]
    quint32 v = QRandomGenerator::global()->bounded(1000, 2000);
//! [14]

//! [15]
    return QColor::fromRgb(QRandomGenerator::global()->generate());
//! [15]

//! [16]
    qint64 value = QRandomGenerator64::global()->generate() & std::numeric_limits<qint64>::max();
//! [16]
