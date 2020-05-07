/****************************************************************************
**
** Copyright (C) 2018 Intel Corporation.
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    QVector<quint32> vector;
    vector.resize(16);
    QRandomGenerator::global()->fillRange(vector.data(), vector.size());
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
    qint64 value = QRandomGenerator64::generate() & std::numeric_limits<qint64>::max();
//! [16]
