/****************************************************************************
**
** Copyright (C) 2018 Intel Corporation.
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
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
    QRandomGenerator::fillRange(vector.data(), vector.size());
//! [9]

//! [10]
    quint32 array[2];
    QRandomGenerator::fillRange(array);
//! [10]

//! [11]
    QRandomGenerator64 rd;
    return std::generate_canonical<qreal, std::numeric_limits<qreal>::digits>(rd);
//! [11]

//! [12]
    return generateDouble() * highest;
//! [12]

//! [13]
    quint32 v = QRandomGenerator::bounded(256);
//! [13]

//! [14]
    quint32 v = QRandomGenerator::bounded(1000, 2000);
//! [14]

//! [15]
    return QColor::fromRgb(QRandomGenerator::global()->generate());
//! [15]

//! [16]
    qint64 value = QRandomGenerator64::generate() & std::numeric_limits<qint64>::max();
//! [16]
