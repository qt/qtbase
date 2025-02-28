// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QPainterPath>
#include <QFont>

class tst_QPainterPath : public QObject
{
    Q_OBJECT

public:
    tst_QPainterPath();

private slots:
    void initTestCase_data();

    void general_data();
    void length_data() { general_data(); }
    void length();
    void percentAtLength_data() { general_data(); }
    void percentAtLength();
    void pointAtPercent_data() { general_data(); }
    void pointAtPercent();
    void trimmed_data() { general_data(); }
    void trimmed();
};

tst_QPainterPath::tst_QPainterPath()
{
}

void tst_QPainterPath::initTestCase_data()
{
    QTest::addColumn<QPainterPath>("path");

    QPainterPath p;
    QTest::newRow("null") << p;

    p.moveTo(100, 100);
    QTest::newRow("only_move") << p;

    p.lineTo(0, 0);
    QTest::newRow("single_line") << p;

    p.clear();
    p.cubicTo(QPointF(100, 25), QPointF(0, 75), QPointF(100, 100));
    QTest::newRow("single_curve") << p;

    p.clear();
    for (int i = 0; i < 10; i++)
        p.addRect(i * 10, i * 10, 100, 100);
    QTest::newRow("40_lines") << p;

    p.clear();
    for (int i = 0; i < 10; i++)
        p.addEllipse(i * 10, i * 10, 100, 100);
    QTest::newRow("40_curves") << p;

    p.clear();
    for (int i = 0; i < 10; i++)
        p.addRoundedRect(QRectF(i * 10, i * 10, 100, 100), 10, 20);
    QTest::newRow("80_mixed") << p;

    p.clear();
    p.addText(QPoint(), QFont(), "Dommarane skal velja det som er best for domfelte.");
    QTest::newRow("2k_text") << p;
}

void tst_QPainterPath::general_data()
{
    QTest::addColumn<bool>("caching");

    QTest::newRow("Uncached") << false;
    QTest::newRow("Cached") << true;
}

void tst_QPainterPath::length()
{
    QFETCH_GLOBAL(QPainterPath, path);
    QFETCH(bool, caching);
    path.setCachingEnabled(caching);

    QBENCHMARK {
        path.length();
    }
}

void tst_QPainterPath::percentAtLength()
{
    QFETCH_GLOBAL(QPainterPath, path);
    QFETCH(bool, caching);
    path.setCachingEnabled(caching);

    const qreal len = path.length() * 0.72;

    QBENCHMARK {
        path.percentAtLength(len);
    }
}

void tst_QPainterPath::pointAtPercent()
{
    QFETCH_GLOBAL(QPainterPath, path);
    QFETCH(bool, caching);
    path.setCachingEnabled(caching);

    const qreal t = 0.72;

    QBENCHMARK {
        path.pointAtPercent(t);
    }
}

void tst_QPainterPath::trimmed()
{
    QFETCH_GLOBAL(QPainterPath, path);
    QFETCH(bool, caching);
    path.setCachingEnabled(caching);

    const qreal fromF = 0.47;
    const qreal toF = 0.77;

    QBENCHMARK {
        (void)path.trimmed(fromF, toF);
        (void)path.trimmed(fromF, toF, 0.4);
    }
}

QTEST_MAIN(tst_QPainterPath)
#include "tst_bench_qpainterpath.moc"
