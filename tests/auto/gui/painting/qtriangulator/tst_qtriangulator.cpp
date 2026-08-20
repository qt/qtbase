// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QtGui/qpainterpath.h>
#include <QtGui/qtransform.h>
#include <QtGui/private/qtriangulator_p.h>

static const qreal maxCoordinate = qreal((1 << 21) - 1) / 32;

class tst_QTriangulator : public QObject
{
    Q_OBJECT
private slots:
    void square();
    void clampRect_data();
    void clampRect();
    void clampPolygon();
    void clampWithTransform();
    void clampCurve();
    void clampPolyline();
};

static quint32 indexAt(const QVertexIndexVector &indices, int i)
{
    if (indices.type() == QVertexIndexVector::UnsignedInt)
        return static_cast<const quint32 *>(indices.data())[i];
    return static_cast<const quint16 *>(indices.data())[i];
}

static bool verticesInRange(const QList<qreal> &vertices)
{
    for (qreal v : vertices) {
        if (!(qAbs(v) <= maxCoordinate)) // written this way to also catch NaN
            return false;
    }
    return true;
}

static bool indicesInRange(const QVertexIndexVector &indices, int vertexCount)
{
    for (int i = 0; i < indices.size(); ++i) {
        quint32 index = indexAt(indices, i);
        if (index >= quint32(vertexCount))
            return false;
    }
    return true;
}

static qreal triangleSetArea(const QTriangleSet &set)
{
    qreal area = 0.0;
    for (int i = 0; i + 2 < set.indices.size(); i += 3) {
        const quint32 a = indexAt(set.indices, i);
        const quint32 b = indexAt(set.indices, i + 1);
        const quint32 c = indexAt(set.indices, i + 2);
        const qreal ax = set.vertices.at(2 * a), ay = set.vertices.at(2 * a + 1);
        const qreal bx = set.vertices.at(2 * b), by = set.vertices.at(2 * b + 1);
        const qreal cx = set.vertices.at(2 * c), cy = set.vertices.at(2 * c + 1);
        area += qAbs((bx - ax) * (cy - ay) - (cx - ax) * (by - ay)) / 2;
    }
    return area;
}

void tst_QTriangulator::square()
{
    QPainterPath path;
    path.addRect(QRectF(0, 0, 64, 64));

    const QTriangleSet set = qTriangulate(path);
    QVERIFY(!set.vertices.isEmpty());
    QCOMPARE(set.indices.size() % 3, 0);
    QVERIFY(verticesInRange(set.vertices));
    QVERIFY(indicesInRange(set.indices, set.vertices.size() / 2));
    QCOMPARE(triangleSetArea(set), qreal(64 * 64));
}

void tst_QTriangulator::clampRect_data()
{
    QTest::addColumn<QRectF>("rect");

    QTest::newRow("just out of range") << QRectF(-70000, -70000, 140000, 140000);
    QTest::newRow("large") << QRectF(1e6, 1e6, 1e6, 1e6);
    QTest::newRow("huge") << QRectF(-1e12, -1e12, 2e12, 2e12);
    QTest::newRow("extreme") << QRectF(1e30, 1e30, 1e30, 1e30);
    QTest::newRow("negative extreme") << QRectF(-2e30, -2e30, 1e30, 1e30);
    QTest::newRow("partially in range") << QRectF(-100, -100, 1e8, 1e8);
}

void tst_QTriangulator::clampRect()
{
    QFETCH(QRectF, rect);

    QPainterPath path;
    path.addRect(rect);

    // Out-of-range coordinates must be clamped instead of triggering
    // asserts or integer overflow in the fixed point representation.
    const QTriangleSet set = qTriangulate(path);
    QVERIFY(verticesInRange(set.vertices));
    QVERIFY(indicesInRange(set.indices, set.vertices.size() / 2));
}

void tst_QTriangulator::clampPolygon()
{
    // The raw polygon overload performs no validation of its input, so pass
    // extreme values directly.
    const qreal polygon[] = {
        -1e100, 0.0,
        1e100, -1e100,
        1e100, 1e100,
        0.0, 65536.0,
    };

    const QTriangleSet set = qTriangulate(polygon, 4);
    QVERIFY(verticesInRange(set.vertices));
    QVERIFY(indicesInRange(set.indices, set.vertices.size() / 2));
}

void tst_QTriangulator::clampWithTransform()
{
    // In-range path coordinates mapped out of range by the transform.
    QPainterPath path;
    path.addRect(QRectF(0, 0, 100, 100));

    const QTriangleSet set = qTriangulate(path, QTransform::fromScale(1e7, 1e7));
    QVERIFY(verticesInRange(set.vertices));
    QVERIFY(indicesInRange(set.indices, set.vertices.size() / 2));
}

void tst_QTriangulator::clampCurve()
{
    // Exercises the bezier flattening code path.
    QPainterPath path;
    path.moveTo(0, 0);
    path.cubicTo(1e8, 0, 1e8, 1e8, 0, 1e8);
    path.closeSubpath();

    const QTriangleSet set = qTriangulate(path);
    QVERIFY(verticesInRange(set.vertices));
    QVERIFY(indicesInRange(set.indices, set.vertices.size() / 2));
}

void tst_QTriangulator::clampPolyline()
{
    QPainterPath path;
    path.moveTo(-1e8, -1e8);
    path.lineTo(1e8, -1e8);
    path.lineTo(1e8, 1e8);
    path.lineTo(-1e8, 1e8);
    path.closeSubpath();

    const QPolylineSet set = qPolyline(path);
    QVERIFY(verticesInRange(set.vertices));
}

QTEST_MAIN(tst_QTriangulator)

#include "tst_qtriangulator.moc"
