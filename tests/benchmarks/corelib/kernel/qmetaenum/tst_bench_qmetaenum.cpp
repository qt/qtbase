/****************************************************************************
**
** Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QMetaEnum>
#include <QTest>

class tst_QMetaEnum: public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

private Q_SLOTS:
    void valueToKeys_data();
    void valueToKeys();
    void keysToValue_data() { valueToKeys_data(); }
    void keysToValue();
};

void tst_QMetaEnum::valueToKeys_data()
{
    QTest::addColumn<int>("buttons");
    QTest::addColumn<QByteArray>("string");
    // Qt::MouseButtons has at least 24 enumerators, so it's a good performance test
    const auto me = QMetaEnum::fromType<Qt::MouseButtons>();
    int accu = 0;
    for (int i = 0; i < std::min(31, me.keyCount()); ++i) {
        accu <<= 1;
        accu |= 1;
        QTest::addRow("%d bits set", i) << accu << me.valueToKeys(accu);
    }
}

void tst_QMetaEnum::valueToKeys()
{
    QFETCH(const int, buttons);
    const auto me = QMetaEnum::fromType<Qt::MouseButtons>();
    QBENCHMARK {
        [[maybe_unused]] auto r = me.valueToKeys(buttons);
    }
}

void tst_QMetaEnum::keysToValue()
{
    QFETCH(const QByteArray, string);
    const auto me = QMetaEnum::fromType<Qt::MouseButtons>();
    bool ok;
    QBENCHMARK {
        [[maybe_unused]] auto r = me.keysToValue(string.data(), &ok);
    }
}

QTEST_MAIN(tst_QMetaEnum)

#include "tst_bench_qmetaenum.moc"
