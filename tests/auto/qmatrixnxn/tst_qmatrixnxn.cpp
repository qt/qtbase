/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtCore/qmath.h>
#include <QtGui/qmatrix4x4.h>

class tst_QMatrixNxN : public QObject
{
    Q_OBJECT
public:
    tst_QMatrixNxN() {}
    ~tst_QMatrixNxN() {}

private slots:
    void create2x2();
    void create3x3();
    void create4x4();
    void create4x3();

    void isIdentity2x2();
    void isIdentity3x3();
    void isIdentity4x4();
    void isIdentity4x3();

    void compare2x2();
    void compare3x3();
    void compare4x4();
    void compare4x3();

    void transposed2x2();
    void transposed3x3();
    void transposed4x4();
    void transposed4x3();

    void add2x2_data();
    void add2x2();
    void add3x3_data();
    void add3x3();
    void add4x4_data();
    void add4x4();
    void add4x3_data();
    void add4x3();

    void subtract2x2_data();
    void subtract2x2();
    void subtract3x3_data();
    void subtract3x3();
    void subtract4x4_data();
    void subtract4x4();
    void subtract4x3_data();
    void subtract4x3();

    void multiply2x2_data();
    void multiply2x2();
    void multiply3x3_data();
    void multiply3x3();
    void multiply4x4_data();
    void multiply4x4();
    void multiply4x3_data();
    void multiply4x3();

    void multiplyFactor2x2_data();
    void multiplyFactor2x2();
    void multiplyFactor3x3_data();
    void multiplyFactor3x3();
    void multiplyFactor4x4_data();
    void multiplyFactor4x4();
    void multiplyFactor4x3_data();
    void multiplyFactor4x3();

    void divideFactor2x2_data();
    void divideFactor2x2();
    void divideFactor3x3_data();
    void divideFactor3x3();
    void divideFactor4x4_data();
    void divideFactor4x4();
    void divideFactor4x3_data();
    void divideFactor4x3();

    void negate2x2_data();
    void negate2x2();
    void negate3x3_data();
    void negate3x3();
    void negate4x4_data();
    void negate4x4();
    void negate4x3_data();
    void negate4x3();

    void inverted4x4_data();
    void inverted4x4();

    void orthonormalInverse4x4();

    void scale4x4_data();
    void scale4x4();

    void translate4x4_data();
    void translate4x4();

    void rotate4x4_data();
    void rotate4x4();

    void normalMatrix_data();
    void normalMatrix();

    void optimizedTransforms();

    void ortho();
    void frustum();
    void perspective();
    void flipCoordinates();

    void convertGeneric();

    void optimize_data();
    void optimize();

    void columnsAndRows();

    void convertQMatrix();
    void convertQTransform();

    void fill();

    void mapRect_data();
    void mapRect();

    void mapVector_data();
    void mapVector();

    void properties();
    void metaTypes();

private:
    static void setMatrix(QMatrix2x2& m, const qreal *values);
    static void setMatrixDirect(QMatrix2x2& m, const qreal *values);
    static bool isSame(const QMatrix2x2& m, const qreal *values);
    static bool isIdentity(const QMatrix2x2& m);

    static void setMatrix(QMatrix3x3& m, const qreal *values);
    static void setMatrixDirect(QMatrix3x3& m, const qreal *values);
    static bool isSame(const QMatrix3x3& m, const qreal *values);
    static bool isIdentity(const QMatrix3x3& m);

    static void setMatrix(QMatrix4x4& m, const qreal *values);
    static void setMatrixDirect(QMatrix4x4& m, const qreal *values);
    static bool isSame(const QMatrix4x4& m, const qreal *values);
    static bool isIdentity(const QMatrix4x4& m);

    static void setMatrix(QMatrix4x3& m, const qreal *values);
    static void setMatrixDirect(QMatrix4x3& m, const qreal *values);
    static bool isSame(const QMatrix4x3& m, const qreal *values);
    static bool isIdentity(const QMatrix4x3& m);
};

static const qreal nullValues2[] =
    {0.0f, 0.0f,
     0.0f, 0.0f};

static qreal const identityValues2[16] =
    {1.0f, 0.0f,
     0.0f, 1.0f};

static const qreal doubleIdentity2[] =
    {2.0f, 0.0f,
     0.0f, 2.0f};

static qreal const uniqueValues2[16] =
    {1.0f, 2.0f,
     5.0f, 6.0f};

static qreal const transposedValues2[16] =
    {1.0f, 5.0f,
     2.0f, 6.0f};

static const qreal nullValues3[] =
    {0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f};

static qreal const identityValues3[16] =
    {1.0f, 0.0f, 0.0f,
     0.0f, 1.0f, 0.0f,
     0.0f, 0.0f, 1.0f};

static const qreal doubleIdentity3[] =
    {2.0f, 0.0f, 0.0f,
     0.0f, 2.0f, 0.0f,
     0.0f, 0.0f, 2.0f};

static qreal const uniqueValues3[16] =
    {1.0f, 2.0f, 3.0f,
     5.0f, 6.0f, 7.0f,
     9.0f, 10.0f, 11.0f};

static qreal const transposedValues3[16] =
    {1.0f, 5.0f, 9.0f,
     2.0f, 6.0f, 10.0f,
     3.0f, 7.0f, 11.0f};

static const qreal nullValues4[] =
    {0.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f};

static qreal const identityValues4[16] =
    {1.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 1.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 1.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 1.0f};

static const qreal doubleIdentity4[] =
    {2.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 2.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 2.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 2.0f};

static qreal const uniqueValues4[16] =
    {1.0f, 2.0f, 3.0f, 4.0f,
     5.0f, 6.0f, 7.0f, 8.0f,
     9.0f, 10.0f, 11.0f, 12.0f,
     13.0f, 14.0f, 15.0f, 16.0f};

static qreal const transposedValues4[16] =
    {1.0f, 5.0f, 9.0f, 13.0f,
     2.0f, 6.0f, 10.0f, 14.0f,
     3.0f, 7.0f, 11.0f, 15.0f,
     4.0f, 8.0f, 12.0f, 16.0f};

static const qreal nullValues4x3[] =
    {0.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f};

static qreal const identityValues4x3[12] =
    {1.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 1.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 1.0f, 0.0f};

static qreal const doubleIdentity4x3[12] =
    {2.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 2.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 2.0f, 0.0f};

static qreal const uniqueValues4x3[12] =
    {1.0f, 2.0f, 3.0f, 4.0f,
     5.0f, 6.0f, 7.0f, 8.0f,
     9.0f, 10.0f, 11.0f, 12.0f};

static qreal const transposedValues3x4[12] =
    {1.0f, 5.0f, 9.0f,
     2.0f, 6.0f, 10.0f,
     3.0f, 7.0f, 11.0f,
     4.0f, 8.0f, 12.0f};

// Set a matrix to a specified array of values, which are assumed
// to be in row-major order.  This sets the values using floating-point.
void tst_QMatrixNxN::setMatrix(QMatrix2x2& m, const qreal *values)
{
    for (int row = 0; row < 2; ++row)
        for (int col = 0; col < 2; ++col)
            m(row, col) = values[row * 2 + col];
}
void tst_QMatrixNxN::setMatrix(QMatrix3x3& m, const qreal *values)
{
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col)
            m(row, col) = values[row * 3 + col];
}
void tst_QMatrixNxN::setMatrix(QMatrix4x4& m, const qreal *values)
{
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            m(row, col) = values[row * 4 + col];
}
void tst_QMatrixNxN::setMatrix(QMatrix4x3& m, const qreal *values)
{
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 4; ++col)
            m(row, col) = values[row * 4 + col];
}

// Set a matrix to a specified array of values, which are assumed
// to be in row-major order.  This sets the values directly into
// the internal data() array.
void tst_QMatrixNxN::setMatrixDirect(QMatrix2x2& m, const qreal *values)
{
    qreal *data = m.data();
    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 2; ++col) {
            data[row + col * 2] = values[row * 2 + col];
        }
    }
}
void tst_QMatrixNxN::setMatrixDirect(QMatrix3x3& m, const qreal *values)
{
    qreal *data = m.data();
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            data[row + col * 3] = values[row * 3 + col];
        }
    }
}
void tst_QMatrixNxN::setMatrixDirect(QMatrix4x4& m, const qreal *values)
{
    qreal *data = m.data();
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            data[row + col * 4] = values[row * 4 + col];
        }
    }
}
void tst_QMatrixNxN::setMatrixDirect(QMatrix4x3& m, const qreal *values)
{
    qreal *data = m.data();
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 4; ++col) {
            data[row + col * 3] = values[row * 4 + col];
        }
    }
}

// QVector2/3/4D use float internally, which can sometimes lead
// to precision issues when converting to and from qreal during
// operations involving QMatrix4x4.  This fuzzy compare is slightly
// "fuzzier" than the default qFuzzyCompare for qreal to compensate.
static bool fuzzyCompare(qreal x, qreal y)
{
    return qFuzzyIsNull((float)(x - y));
}

static bool fuzzyCompare(const QVector3D &v1, const QVector3D &v2)
{
    if (!fuzzyCompare(v1.x(), v2.x()))
        return false;
    if (!fuzzyCompare(v1.y(), v2.y()))
        return false;
    if (!fuzzyCompare(v1.z(), v2.z()))
        return false;
    return true;
}

// Determine if a matrix is the same as a specified array of values.
// The values are assumed to be specified in row-major order.
bool tst_QMatrixNxN::isSame(const QMatrix2x2& m, const qreal *values)
{
    const qreal *mv = m.constData();
    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 2; ++col) {
            // Check the values using the operator() function.
            if (!fuzzyCompare(m(row, col), values[row * 2 + col])) {
                qDebug() << "floating-point failure at" << row << col << "actual =" << m(row, col) << "expected =" << values[row * 2 + col];
                return false;
            }

            // Check the values using direct access, which verifies that the values
            // are stored internally in column-major order.
            if (!fuzzyCompare(mv[col * 2 + row], values[row * 2 + col])) {
                qDebug() << "column floating-point failure at" << row << col << "actual =" << mv[col * 2 + row] << "expected =" << values[row * 2 + col];
                return false;
            }
        }
    }
    return true;
}
bool tst_QMatrixNxN::isSame(const QMatrix3x3& m, const qreal *values)
{
    const qreal *mv = m.constData();
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            // Check the values using the operator() access function.
            if (!fuzzyCompare(m(row, col), values[row * 3 + col])) {
                qDebug() << "floating-point failure at" << row << col << "actual =" << m(row, col) << "expected =" << values[row * 3 + col];
                return false;
            }

            // Check the values using direct access, which verifies that the values
            // are stored internally in column-major order.
            if (!fuzzyCompare(mv[col * 3 + row], values[row * 3 + col])) {
                qDebug() << "column floating-point failure at" << row << col << "actual =" << mv[col * 3 + row] << "expected =" << values[row * 3 + col];
                return false;
            }
        }
    }
    return true;
}
bool tst_QMatrixNxN::isSame(const QMatrix4x4& m, const qreal *values)
{
    const qreal *mv = m.constData();
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            // Check the values using the operator() access function.
            if (!fuzzyCompare(m(row, col), values[row * 4 + col])) {
                qDebug() << "floating-point failure at" << row << col << "actual =" << m(row, col) << "expected =" << values[row * 4 + col];
                return false;
            }

            // Check the values using direct access, which verifies that the values
            // are stored internally in column-major order.
            if (!fuzzyCompare(mv[col * 4 + row], values[row * 4 + col])) {
                qDebug() << "column floating-point failure at" << row << col << "actual =" << mv[col * 4 + row] << "expected =" << values[row * 4 + col];
                return false;
            }
        }
    }
    return true;
}
bool tst_QMatrixNxN::isSame(const QMatrix4x3& m, const qreal *values)
{
    const qreal *mv = m.constData();
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 4; ++col) {
            // Check the values using the operator() access function.
            if (!fuzzyCompare(m(row, col), values[row * 4 + col])) {
                qDebug() << "floating-point failure at" << row << col << "actual =" << m(row, col) << "expected =" << values[row * 4 + col];
                return false;
            }

            // Check the values using direct access, which verifies that the values
            // are stored internally in column-major order.
            if (!fuzzyCompare(mv[col * 3 + row], values[row * 4 + col])) {
                qDebug() << "column floating-point failure at" << row << col << "actual =" << mv[col * 3 + row] << "expected =" << values[row * 4 + col];
                return false;
            }
        }
    }
    return true;
}

// Determine if a matrix is the identity.
bool tst_QMatrixNxN::isIdentity(const QMatrix2x2& m)
{
    return isSame(m, identityValues2);
}
bool tst_QMatrixNxN::isIdentity(const QMatrix3x3& m)
{
    return isSame(m, identityValues3);
}
bool tst_QMatrixNxN::isIdentity(const QMatrix4x4& m)
{
    return isSame(m, identityValues4);
}
bool tst_QMatrixNxN::isIdentity(const QMatrix4x3& m)
{
    return isSame(m, identityValues4x3);
}

// Test the creation of QMatrix2x2 objects in various ways:
// construct, copy, and modify.
void tst_QMatrixNxN::create2x2()
{
    QMatrix2x2 m1;
    QVERIFY(isIdentity(m1));
    QVERIFY(m1.isIdentity());

    QMatrix2x2 m2;
    setMatrix(m2, uniqueValues2);
    QVERIFY(isSame(m2, uniqueValues2));
    QVERIFY(!m2.isIdentity());

    QMatrix2x2 m3;
    setMatrixDirect(m3, uniqueValues2);
    QVERIFY(isSame(m3, uniqueValues2));

    QMatrix2x2 m4(m3);
    QVERIFY(isSame(m4, uniqueValues2));

    QMatrix2x2 m5;
    m5 = m3;
    QVERIFY(isSame(m5, uniqueValues2));

    m5.setToIdentity();
    QVERIFY(isIdentity(m5));

    QMatrix2x2 m6(uniqueValues2);
    QVERIFY(isSame(m6, uniqueValues2));
    qreal vals[4];
    m6.copyDataTo(vals);
    for (int index = 0; index < 4; ++index)
        QCOMPARE(vals[index], uniqueValues2[index]);
}

// Test the creation of QMatrix3x3 objects in various ways:
// construct, copy, and modify.
void tst_QMatrixNxN::create3x3()
{
    QMatrix3x3 m1;
    QVERIFY(isIdentity(m1));
    QVERIFY(m1.isIdentity());

    QMatrix3x3 m2;
    setMatrix(m2, uniqueValues3);
    QVERIFY(isSame(m2, uniqueValues3));
    QVERIFY(!m2.isIdentity());

    QMatrix3x3 m3;
    setMatrixDirect(m3, uniqueValues3);
    QVERIFY(isSame(m3, uniqueValues3));

    QMatrix3x3 m4(m3);
    QVERIFY(isSame(m4, uniqueValues3));

    QMatrix3x3 m5;
    m5 = m3;
    QVERIFY(isSame(m5, uniqueValues3));

    m5.setToIdentity();
    QVERIFY(isIdentity(m5));

    QMatrix3x3 m6(uniqueValues3);
    QVERIFY(isSame(m6, uniqueValues3));
    qreal vals[9];
    m6.copyDataTo(vals);
    for (int index = 0; index < 9; ++index)
        QCOMPARE(vals[index], uniqueValues3[index]);
}

// Test the creation of QMatrix4x4 objects in various ways:
// construct, copy, and modify.
void tst_QMatrixNxN::create4x4()
{
    QMatrix4x4 m1;
    QVERIFY(isIdentity(m1));
    QVERIFY(m1.isIdentity());

    QMatrix4x4 m2;
    setMatrix(m2, uniqueValues4);
    QVERIFY(isSame(m2, uniqueValues4));
    QVERIFY(!m2.isIdentity());

    QMatrix4x4 m3;
    setMatrixDirect(m3, uniqueValues4);
    QVERIFY(isSame(m3, uniqueValues4));

    QMatrix4x4 m4(m3);
    QVERIFY(isSame(m4, uniqueValues4));

    QMatrix4x4 m5;
    m5 = m3;
    QVERIFY(isSame(m5, uniqueValues4));

    m5.setToIdentity();
    QVERIFY(isIdentity(m5));

    QMatrix4x4 m6(uniqueValues4);
    QVERIFY(isSame(m6, uniqueValues4));
    qreal vals[16];
    m6.copyDataTo(vals);
    for (int index = 0; index < 16; ++index)
        QCOMPARE(vals[index], uniqueValues4[index]);

    QMatrix4x4 m8
        (uniqueValues4[0], uniqueValues4[1], uniqueValues4[2], uniqueValues4[3],
         uniqueValues4[4], uniqueValues4[5], uniqueValues4[6], uniqueValues4[7],
         uniqueValues4[8], uniqueValues4[9], uniqueValues4[10], uniqueValues4[11],
         uniqueValues4[12], uniqueValues4[13], uniqueValues4[14], uniqueValues4[15]);
    QVERIFY(isSame(m8, uniqueValues4));
}

// Test the creation of QMatrix4x3 objects in various ways:
// construct, copy, and modify.
void tst_QMatrixNxN::create4x3()
{
    QMatrix4x3 m1;
    QVERIFY(isIdentity(m1));
    QVERIFY(m1.isIdentity());

    QMatrix4x3 m2;
    setMatrix(m2, uniqueValues4x3);
    QVERIFY(isSame(m2, uniqueValues4x3));
    QVERIFY(!m2.isIdentity());

    QMatrix4x3 m3;
    setMatrixDirect(m3, uniqueValues4x3);
    QVERIFY(isSame(m3, uniqueValues4x3));

    QMatrix4x3 m4(m3);
    QVERIFY(isSame(m4, uniqueValues4x3));

    QMatrix4x3 m5;
    m5 = m3;
    QVERIFY(isSame(m5, uniqueValues4x3));

    m5.setToIdentity();
    QVERIFY(isIdentity(m5));

    QMatrix4x3 m6(uniqueValues4x3);
    QVERIFY(isSame(m6, uniqueValues4x3));
    qreal vals[12];
    m6.copyDataTo(vals);
    for (int index = 0; index < 12; ++index)
        QCOMPARE(vals[index], uniqueValues4x3[index]);
}

// Test isIdentity() for 2x2 matrices.
void tst_QMatrixNxN::isIdentity2x2()
{
    for (int i = 0; i < 2 * 2; ++i) {
        QMatrix2x2 m;
        QVERIFY(m.isIdentity());
        m.data()[i] = 42.0f;
        QVERIFY(!m.isIdentity());
    }
}

// Test isIdentity() for 3x3 matrices.
void tst_QMatrixNxN::isIdentity3x3()
{
    for (int i = 0; i < 3 * 3; ++i) {
        QMatrix3x3 m;
        QVERIFY(m.isIdentity());
        m.data()[i] = 42.0f;
        QVERIFY(!m.isIdentity());
    }
}

// Test isIdentity() for 4x4 matrices.
void tst_QMatrixNxN::isIdentity4x4()
{
    for (int i = 0; i < 4 * 4; ++i) {
        QMatrix4x4 m;
        QVERIFY(m.isIdentity());
        m.data()[i] = 42.0f;
        QVERIFY(!m.isIdentity());
    }

    // Force the "Identity" flag bit to be lost and check again.
    QMatrix4x4 m2;
    m2.data()[0] = 1.0f;
    QVERIFY(m2.isIdentity());
}

// Test isIdentity() for 4x3 matrices.
void tst_QMatrixNxN::isIdentity4x3()
{
    for (int i = 0; i < 4 * 3; ++i) {
        QMatrix4x3 m;
        QVERIFY(m.isIdentity());
        m.data()[i] = 42.0f;
        QVERIFY(!m.isIdentity());
    }
}

// Test 2x2 matrix comparisons.
void tst_QMatrixNxN::compare2x2()
{
    QMatrix2x2 m1(uniqueValues2);
    QMatrix2x2 m2(uniqueValues2);
    QMatrix2x2 m3(transposedValues2);

    QVERIFY(m1 == m2);
    QVERIFY(!(m1 != m2));
    QVERIFY(m1 != m3);
    QVERIFY(!(m1 == m3));
}

// Test 3x3 matrix comparisons.
void tst_QMatrixNxN::compare3x3()
{
    QMatrix3x3 m1(uniqueValues3);
    QMatrix3x3 m2(uniqueValues3);
    QMatrix3x3 m3(transposedValues3);

    QVERIFY(m1 == m2);
    QVERIFY(!(m1 != m2));
    QVERIFY(m1 != m3);
    QVERIFY(!(m1 == m3));
}

// Test 4x4 matrix comparisons.
void tst_QMatrixNxN::compare4x4()
{
    QMatrix4x4 m1(uniqueValues4);
    QMatrix4x4 m2(uniqueValues4);
    QMatrix4x4 m3(transposedValues4);

    QVERIFY(m1 == m2);
    QVERIFY(!(m1 != m2));
    QVERIFY(m1 != m3);
    QVERIFY(!(m1 == m3));
}

// Test 4x3 matrix comparisons.
void tst_QMatrixNxN::compare4x3()
{
    QMatrix4x3 m1(uniqueValues4x3);
    QMatrix4x3 m2(uniqueValues4x3);
    QMatrix4x3 m3(transposedValues3x4);

    QVERIFY(m1 == m2);
    QVERIFY(!(m1 != m2));
    QVERIFY(m1 != m3);
    QVERIFY(!(m1 == m3));
}

// Test matrix 2x2 transpose operations.
void tst_QMatrixNxN::transposed2x2()
{
    // Transposing the identity should result in the identity.
    QMatrix2x2 m1;
    QMatrix2x2 m2 = m1.transposed();
    QVERIFY(isIdentity(m2));

    // Transpose a more interesting matrix that allows us to track
    // exactly where each source element ends up.
    QMatrix2x2 m3(uniqueValues2);
    QMatrix2x2 m4 = m3.transposed();
    QVERIFY(isSame(m4, transposedValues2));

    // Transpose in-place, just to check that the compiler is sane.
    m3 = m3.transposed();
    QVERIFY(isSame(m3, transposedValues2));
}

// Test matrix 3x3 transpose operations.
void tst_QMatrixNxN::transposed3x3()
{
    // Transposing the identity should result in the identity.
    QMatrix3x3 m1;
    QMatrix3x3 m2 = m1.transposed();
    QVERIFY(isIdentity(m2));

    // Transpose a more interesting matrix that allows us to track
    // exactly where each source element ends up.
    QMatrix3x3 m3(uniqueValues3);
    QMatrix3x3 m4 = m3.transposed();
    QVERIFY(isSame(m4, transposedValues3));

    // Transpose in-place, just to check that the compiler is sane.
    m3 = m3.transposed();
    QVERIFY(isSame(m3, transposedValues3));
}

// Test matrix 4x4 transpose operations.
void tst_QMatrixNxN::transposed4x4()
{
    // Transposing the identity should result in the identity.
    QMatrix4x4 m1;
    QMatrix4x4 m2 = m1.transposed();
    QVERIFY(isIdentity(m2));

    // Transpose a more interesting matrix that allows us to track
    // exactly where each source element ends up.
    QMatrix4x4 m3(uniqueValues4);
    QMatrix4x4 m4 = m3.transposed();
    QVERIFY(isSame(m4, transposedValues4));

    // Transpose in-place, just to check that the compiler is sane.
    m3 = m3.transposed();
    QVERIFY(isSame(m3, transposedValues4));
}

// Test matrix 4x3 transpose operations.
void tst_QMatrixNxN::transposed4x3()
{
    QMatrix4x3 m3(uniqueValues4x3);
    QMatrix3x4 m4 = m3.transposed();
    qreal values[12];
    m4.copyDataTo(values);
    for (int index = 0; index < 12; ++index)
        QCOMPARE(values[index], transposedValues3x4[index]);
}

// Test matrix addition for 2x2 matrices.
void tst_QMatrixNxN::add2x2_data()
{
    QTest::addColumn<void *>("m1Values");
    QTest::addColumn<void *>("m2Values");
    QTest::addColumn<void *>("m3Values");

    QTest::newRow("null")
        << (void *)nullValues2 << (void *)nullValues2 << (void *)nullValues2;

    QTest::newRow("identity/null")
        << (void *)identityValues2 << (void *)nullValues2 << (void *)identityValues2;

    QTest::newRow("identity/identity")
        << (void *)identityValues2 << (void *)identityValues2 << (void *)doubleIdentity2;

    static qreal const sumValues[16] =
        {2.0f, 7.0f,
         7.0f, 12.0f};
    QTest::newRow("unique")
        << (void *)uniqueValues2 << (void *)transposedValues2 << (void *)sumValues;
}
void tst_QMatrixNxN::add2x2()
{
    QFETCH(void *, m1Values);
    QFETCH(void *, m2Values);
    QFETCH(void *, m3Values);

    QMatrix2x2 m1((const qreal *)m1Values);
    QMatrix2x2 m2((const qreal *)m2Values);

    QMatrix2x2 m4(m1);
    m4 += m2;
    QVERIFY(isSame(m4, (const qreal *)m3Values));

    QMatrix2x2 m5;
    m5 = m1 + m2;
    QVERIFY(isSame(m5, (const qreal *)m3Values));
}

// Test matrix addition for 3x3 matrices.
void tst_QMatrixNxN::add3x3_data()
{
    QTest::addColumn<void *>("m1Values");
    QTest::addColumn<void *>("m2Values");
    QTest::addColumn<void *>("m3Values");

    QTest::newRow("null")
        << (void *)nullValues3 << (void *)nullValues3 << (void *)nullValues3;

    QTest::newRow("identity/null")
        << (void *)identityValues3 << (void *)nullValues3 << (void *)identityValues3;

    QTest::newRow("identity/identity")
        << (void *)identityValues3 << (void *)identityValues3 << (void *)doubleIdentity3;

    static qreal const sumValues[16] =
        {2.0f, 7.0f, 12.0f,
         7.0f, 12.0f, 17.0f,
         12.0f, 17.0f, 22.0f};
    QTest::newRow("unique")
        << (void *)uniqueValues3 << (void *)transposedValues3 << (void *)sumValues;
}
void tst_QMatrixNxN::add3x3()
{
    QFETCH(void *, m1Values);
    QFETCH(void *, m2Values);
    QFETCH(void *, m3Values);

    QMatrix3x3 m1((const qreal *)m1Values);
    QMatrix3x3 m2((const qreal *)m2Values);

    QMatrix3x3 m4(m1);
    m4 += m2;
    QVERIFY(isSame(m4, (const qreal *)m3Values));

    QMatrix3x3 m5;
    m5 = m1 + m2;
    QVERIFY(isSame(m5, (const qreal *)m3Values));
}

// Test matrix addition for 4x4 matrices.
void tst_QMatrixNxN::add4x4_data()
{
    QTest::addColumn<void *>("m1Values");
    QTest::addColumn<void *>("m2Values");
    QTest::addColumn<void *>("m3Values");

    QTest::newRow("null")
        << (void *)nullValues4 << (void *)nullValues4 << (void *)nullValues4;

    QTest::newRow("identity/null")
        << (void *)identityValues4 << (void *)nullValues4 << (void *)identityValues4;

    QTest::newRow("identity/identity")
        << (void *)identityValues4 << (void *)identityValues4 << (void *)doubleIdentity4;

    static qreal const sumValues[16] =
        {2.0f, 7.0f, 12.0f, 17.0f,
         7.0f, 12.0f, 17.0f, 22.0f,
         12.0f, 17.0f, 22.0f, 27.0f,
         17.0f, 22.0f, 27.0f, 32.0f};
    QTest::newRow("unique")
        << (void *)uniqueValues4 << (void *)transposedValues4 << (void *)sumValues;
}
void tst_QMatrixNxN::add4x4()
{
    QFETCH(void *, m1Values);
    QFETCH(void *, m2Values);
    QFETCH(void *, m3Values);

    QMatrix4x4 m1((const qreal *)m1Values);
    QMatrix4x4 m2((const qreal *)m2Values);

    QMatrix4x4 m4(m1);
    m4 += m2;
    QVERIFY(isSame(m4, (const qreal *)m3Values));

    QMatrix4x4 m5;
    m5 = m1 + m2;
    QVERIFY(isSame(m5, (const qreal *)m3Values));
}

// Test matrix addition for 4x3 matrices.
void tst_QMatrixNxN::add4x3_data()
{
    QTest::addColumn<void *>("m1Values");
    QTest::addColumn<void *>("m2Values");
    QTest::addColumn<void *>("m3Values");

    QTest::newRow("null")
        << (void *)nullValues4x3 << (void *)nullValues4x3 << (void *)nullValues4x3;

    QTest::newRow("identity/null")
        << (void *)identityValues4x3 << (void *)nullValues4x3 << (void *)identityValues4x3;

    QTest::newRow("identity/identity")
        << (void *)identityValues4x3 << (void *)identityValues4x3 << (void *)doubleIdentity4x3;

    static qreal const sumValues[16] =
        {2.0f, 7.0f, 12.0f, 6.0f,
         11.0f, 16.0f, 10.0f, 15.0f,
         20.0f, 14.0f, 19.0f, 24.0f};
    QTest::newRow("unique")
        << (void *)uniqueValues4x3 << (void *)transposedValues3x4 << (void *)sumValues;
}
void tst_QMatrixNxN::add4x3()
{
    QFETCH(void *, m1Values);
    QFETCH(void *, m2Values);
    QFETCH(void *, m3Values);

    QMatrix4x3 m1((const qreal *)m1Values);
    QMatrix4x3 m2((const qreal *)m2Values);

    QMatrix4x3 m4(m1);
    m4 += m2;
    QVERIFY(isSame(m4, (const qreal *)m3Values));

    QMatrix4x3 m5;
    m5 = m1 + m2;
    QVERIFY(isSame(m5, (const qreal *)m3Values));
}

// Test matrix subtraction for 2x2 matrices.
void tst_QMatrixNxN::subtract2x2_data()
{
    // Use the same test cases as the add test.
    add2x2_data();
}
void tst_QMatrixNxN::subtract2x2()
{
    QFETCH(void *, m1Values);
    QFETCH(void *, m2Values);
    QFETCH(void *, m3Values);

    QMatrix2x2 m1((const qreal *)m1Values);
    QMatrix2x2 m2((const qreal *)m2Values);
    QMatrix2x2 m3((const qreal *)m3Values);

    QMatrix2x2 m4(m3);
    m4 -= m1;
    QVERIFY(isSame(m4, (const qreal *)m2Values));

    QMatrix2x2 m5;
    m5 = m3 - m1;
    QVERIFY(isSame(m5, (const qreal *)m2Values));

    QMatrix2x2 m6(m3);
    m6 -= m2;
    QVERIFY(isSame(m6, (const qreal *)m1Values));

    QMatrix2x2 m7;
    m7 = m3 - m2;
    QVERIFY(isSame(m7, (const qreal *)m1Values));
}

// Test matrix subtraction for 3x3 matrices.
void tst_QMatrixNxN::subtract3x3_data()
{
    // Use the same test cases as the add test.
    add3x3_data();
}
void tst_QMatrixNxN::subtract3x3()
{
    QFETCH(void *, m1Values);
    QFETCH(void *, m2Values);
    QFETCH(void *, m3Values);

    QMatrix3x3 m1((const qreal *)m1Values);
    QMatrix3x3 m2((const qreal *)m2Values);
    QMatrix3x3 m3((const qreal *)m3Values);

    QMatrix3x3 m4(m3);
    m4 -= m1;
    QVERIFY(isSame(m4, (const qreal *)m2Values));

    QMatrix3x3 m5;
    m5 = m3 - m1;
    QVERIFY(isSame(m5, (const qreal *)m2Values));

    QMatrix3x3 m6(m3);
    m6 -= m2;
    QVERIFY(isSame(m6, (const qreal *)m1Values));

    QMatrix3x3 m7;
    m7 = m3 - m2;
    QVERIFY(isSame(m7, (const qreal *)m1Values));
}

// Test matrix subtraction for 4x4 matrices.
void tst_QMatrixNxN::subtract4x4_data()
{
    // Use the same test cases as the add test.
    add4x4_data();
}
void tst_QMatrixNxN::subtract4x4()
{
    QFETCH(void *, m1Values);
    QFETCH(void *, m2Values);
    QFETCH(void *, m3Values);

    QMatrix4x4 m1((const qreal *)m1Values);
    QMatrix4x4 m2((const qreal *)m2Values);
    QMatrix4x4 m3((const qreal *)m3Values);

    QMatrix4x4 m4(m3);
    m4 -= m1;
    QVERIFY(isSame(m4, (const qreal *)m2Values));

    QMatrix4x4 m5;
    m5 = m3 - m1;
    QVERIFY(isSame(m5, (const qreal *)m2Values));

    QMatrix4x4 m6(m3);
    m6 -= m2;
    QVERIFY(isSame(m6, (const qreal *)m1Values));

    QMatrix4x4 m7;
    m7 = m3 - m2;
    QVERIFY(isSame(m7, (const qreal *)m1Values));
}

// Test matrix subtraction for 4x3 matrices.
void tst_QMatrixNxN::subtract4x3_data()
{
    // Use the same test cases as the add test.
    add4x3_data();
}
void tst_QMatrixNxN::subtract4x3()
{
    QFETCH(void *, m1Values);
    QFETCH(void *, m2Values);
    QFETCH(void *, m3Values);

    QMatrix4x3 m1((const qreal *)m1Values);
    QMatrix4x3 m2((const qreal *)m2Values);
    QMatrix4x3 m3((const qreal *)m3Values);

    QMatrix4x3 m4(m3);
    m4 -= m1;
    QVERIFY(isSame(m4, (const qreal *)m2Values));

    QMatrix4x3 m5;
    m5 = m3 - m1;
    QVERIFY(isSame(m5, (const qreal *)m2Values));

    QMatrix4x3 m6(m3);
    m6 -= m2;
    QVERIFY(isSame(m6, (const qreal *)m1Values));

    QMatrix4x3 m7;
    m7 = m3 - m2;
    QVERIFY(isSame(m7, (const qreal *)m1Values));
}

// Test matrix multiplication for 2x2 matrices.
void tst_QMatrixNxN::multiply2x2_data()
{
    QTest::addColumn<void *>("m1Values");
    QTest::addColumn<void *>("m2Values");
    QTest::addColumn<void *>("m3Values");

    QTest::newRow("null")
        << (void *)nullValues2 << (void *)nullValues2 << (void *)nullValues2;

    QTest::newRow("null/unique")
        << (void *)nullValues2 << (void *)uniqueValues2 << (void *)nullValues2;

    QTest::newRow("unique/null")
        << (void *)uniqueValues2 << (void *)nullValues2 << (void *)nullValues2;

    QTest::newRow("unique/identity")
        << (void *)uniqueValues2 << (void *)identityValues2 << (void *)uniqueValues2;

    QTest::newRow("identity/unique")
        << (void *)identityValues2 << (void *)uniqueValues2 << (void *)uniqueValues2;

    static qreal uniqueResult[4];
    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 2; ++col) {
            qreal sum = 0.0f;
            for (int j = 0; j < 2; ++j)
                sum += uniqueValues2[row * 2 + j] * transposedValues2[j * 2 + col];
            uniqueResult[row * 2 + col] = sum;
        }
    }

    QTest::newRow("unique/transposed")
        << (void *)uniqueValues2 << (void *)transposedValues2 << (void *)uniqueResult;
}
void tst_QMatrixNxN::multiply2x2()
{
    QFETCH(void *, m1Values);
    QFETCH(void *, m2Values);
    QFETCH(void *, m3Values);

    QMatrix2x2 m1((const qreal *)m1Values);
    QMatrix2x2 m2((const qreal *)m2Values);

    QMatrix2x2 m5;
    m5 = m1 * m2;
    QVERIFY(isSame(m5, (const qreal *)m3Values));
}

// Test matrix multiplication for 3x3 matrices.
void tst_QMatrixNxN::multiply3x3_data()
{
    QTest::addColumn<void *>("m1Values");
    QTest::addColumn<void *>("m2Values");
    QTest::addColumn<void *>("m3Values");

    QTest::newRow("null")
        << (void *)nullValues3 << (void *)nullValues3 << (void *)nullValues3;

    QTest::newRow("null/unique")
        << (void *)nullValues3 << (void *)uniqueValues3 << (void *)nullValues3;

    QTest::newRow("unique/null")
        << (void *)uniqueValues3 << (void *)nullValues3 << (void *)nullValues3;

    QTest::newRow("unique/identity")
        << (void *)uniqueValues3 << (void *)identityValues3 << (void *)uniqueValues3;

    QTest::newRow("identity/unique")
        << (void *)identityValues3 << (void *)uniqueValues3 << (void *)uniqueValues3;

    static qreal uniqueResult[9];
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            qreal sum = 0.0f;
            for (int j = 0; j < 3; ++j)
                sum += uniqueValues3[row * 3 + j] * transposedValues3[j * 3 + col];
            uniqueResult[row * 3 + col] = sum;
        }
    }

    QTest::newRow("unique/transposed")
        << (void *)uniqueValues3 << (void *)transposedValues3 << (void *)uniqueResult;
}
void tst_QMatrixNxN::multiply3x3()
{
    QFETCH(void *, m1Values);
    QFETCH(void *, m2Values);
    QFETCH(void *, m3Values);

    QMatrix3x3 m1((const qreal *)m1Values);
    QMatrix3x3 m2((const qreal *)m2Values);

    QMatrix3x3 m5;
    m5 = m1 * m2;
    QVERIFY(isSame(m5, (const qreal *)m3Values));
}

// Test matrix multiplication for 4x4 matrices.
void tst_QMatrixNxN::multiply4x4_data()
{
    QTest::addColumn<void *>("m1Values");
    QTest::addColumn<void *>("m2Values");
    QTest::addColumn<void *>("m3Values");

    QTest::newRow("null")
        << (void *)nullValues4 << (void *)nullValues4 << (void *)nullValues4;

    QTest::newRow("null/unique")
        << (void *)nullValues4 << (void *)uniqueValues4 << (void *)nullValues4;

    QTest::newRow("unique/null")
        << (void *)uniqueValues4 << (void *)nullValues4 << (void *)nullValues4;

    QTest::newRow("unique/identity")
        << (void *)uniqueValues4 << (void *)identityValues4 << (void *)uniqueValues4;

    QTest::newRow("identity/unique")
        << (void *)identityValues4 << (void *)uniqueValues4 << (void *)uniqueValues4;

    static qreal uniqueResult[16];
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            qreal sum = 0.0f;
            for (int j = 0; j < 4; ++j)
                sum += uniqueValues4[row * 4 + j] * transposedValues4[j * 4 + col];
            uniqueResult[row * 4 + col] = sum;
        }
    }

    QTest::newRow("unique/transposed")
        << (void *)uniqueValues4 << (void *)transposedValues4 << (void *)uniqueResult;
}
void tst_QMatrixNxN::multiply4x4()
{
    QFETCH(void *, m1Values);
    QFETCH(void *, m2Values);
    QFETCH(void *, m3Values);

    QMatrix4x4 m1((const qreal *)m1Values);
    QMatrix4x4 m2((const qreal *)m2Values);

    QMatrix4x4 m4;
    m4 = m1;
    m4 *= m2;
    QVERIFY(isSame(m4, (const qreal *)m3Values));

    QMatrix4x4 m5;
    m5 = m1 * m2;
    QVERIFY(isSame(m5, (const qreal *)m3Values));
}

// Test matrix multiplication for 4x3 matrices.
void tst_QMatrixNxN::multiply4x3_data()
{
    QTest::addColumn<void *>("m1Values");
    QTest::addColumn<void *>("m2Values");
    QTest::addColumn<void *>("m3Values");

    QTest::newRow("null")
        << (void *)nullValues4x3 << (void *)nullValues4x3 << (void *)nullValues3;

    QTest::newRow("null/unique")
        << (void *)nullValues4x3 << (void *)uniqueValues4x3 << (void *)nullValues3;

    QTest::newRow("unique/null")
        << (void *)uniqueValues4x3 << (void *)nullValues4x3 << (void *)nullValues3;

    static qreal uniqueResult[9];
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            qreal sum = 0.0f;
            for (int j = 0; j < 4; ++j)
                sum += uniqueValues4x3[row * 4 + j] * transposedValues3x4[j * 3 + col];
            uniqueResult[row * 3 + col] = sum;
        }
    }

    QTest::newRow("unique/transposed")
        << (void *)uniqueValues4x3 << (void *)transposedValues3x4 << (void *)uniqueResult;
}
void tst_QMatrixNxN::multiply4x3()
{
    QFETCH(void *, m1Values);
    QFETCH(void *, m2Values);
    QFETCH(void *, m3Values);

    QMatrix4x3 m1((const qreal *)m1Values);
    QMatrix3x4 m2((const qreal *)m2Values);

    QGenericMatrix<3, 3, qreal> m4;
    m4 = m1 * m2;
    qreal values[9];
    m4.copyDataTo(values);
    for (int index = 0; index < 9; ++index)
        QCOMPARE(values[index], ((const qreal *)m3Values)[index]);
}

// Test matrix multiplication by a factor for 2x2 matrices.
void tst_QMatrixNxN::multiplyFactor2x2_data()
{
    QTest::addColumn<void *>("m1Values");
    QTest::addColumn<qreal>("factor");
    QTest::addColumn<void *>("m2Values");

    QTest::newRow("null")
        << (void *)nullValues2 << (qreal)1.0f << (void *)nullValues2;

    QTest::newRow("double identity")
        << (void *)identityValues2 << (qreal)2.0f << (void *)doubleIdentity2;

    static qreal const values[16] =
        {1.0f, 2.0f,
         5.0f, 6.0f};
    static qreal const doubleValues[16] =
        {2.0f, 4.0f,
         10.0f, 12.0f};
    static qreal const negDoubleValues[16] =
        {-2.0f, -4.0f,
         -10.0f, -12.0f};

    QTest::newRow("unique")
        << (void *)values << (qreal)2.0f << (void *)doubleValues;

    QTest::newRow("neg")
        << (void *)values << (qreal)-2.0f << (void *)negDoubleValues;

    QTest::newRow("zero")
        << (void *)values << (qreal)0.0f << (void *)nullValues4;
}
void tst_QMatrixNxN::multiplyFactor2x2()
{
    QFETCH(void *, m1Values);
    QFETCH(qreal, factor);
    QFETCH(void *, m2Values);

    QMatrix2x2 m1((const qreal *)m1Values);

    QMatrix2x2 m3;
    m3 = m1;
    m3 *= factor;
    QVERIFY(isSame(m3, (const qreal *)m2Values));

    QMatrix2x2 m4;
    m4 = m1 * factor;
    QVERIFY(isSame(m4, (const qreal *)m2Values));

    QMatrix2x2 m5;
    m5 = factor * m1;
    QVERIFY(isSame(m5, (const qreal *)m2Values));
}

// Test matrix multiplication by a factor for 3x3 matrices.
void tst_QMatrixNxN::multiplyFactor3x3_data()
{
    QTest::addColumn<void *>("m1Values");
    QTest::addColumn<qreal>("factor");
    QTest::addColumn<void *>("m2Values");

    QTest::newRow("null")
        << (void *)nullValues3 << (qreal)1.0f << (void *)nullValues3;

    QTest::newRow("double identity")
        << (void *)identityValues3 << (qreal)2.0f << (void *)doubleIdentity3;

    static qreal const values[16] =
        {1.0f, 2.0f, 3.0f,
         5.0f, 6.0f, 7.0f,
         9.0f, 10.0f, 11.0f};
    static qreal const doubleValues[16] =
        {2.0f, 4.0f, 6.0f,
         10.0f, 12.0f, 14.0f,
         18.0f, 20.0f, 22.0f};
    static qreal const negDoubleValues[16] =
        {-2.0f, -4.0f, -6.0f,
         -10.0f, -12.0f, -14.0f,
         -18.0f, -20.0f, -22.0f};

    QTest::newRow("unique")
        << (void *)values << (qreal)2.0f << (void *)doubleValues;

    QTest::newRow("neg")
        << (void *)values << (qreal)-2.0f << (void *)negDoubleValues;

    QTest::newRow("zero")
        << (void *)values << (qreal)0.0f << (void *)nullValues4;
}
void tst_QMatrixNxN::multiplyFactor3x3()
{
    QFETCH(void *, m1Values);
    QFETCH(qreal, factor);
    QFETCH(void *, m2Values);

    QMatrix3x3 m1((const qreal *)m1Values);

    QMatrix3x3 m3;
    m3 = m1;
    m3 *= factor;
    QVERIFY(isSame(m3, (const qreal *)m2Values));

    QMatrix3x3 m4;
    m4 = m1 * factor;
    QVERIFY(isSame(m4, (const qreal *)m2Values));

    QMatrix3x3 m5;
    m5 = factor * m1;
    QVERIFY(isSame(m5, (const qreal *)m2Values));
}

// Test matrix multiplication by a factor for 4x4 matrices.
void tst_QMatrixNxN::multiplyFactor4x4_data()
{
    QTest::addColumn<void *>("m1Values");
    QTest::addColumn<qreal>("factor");
    QTest::addColumn<void *>("m2Values");

    QTest::newRow("null")
        << (void *)nullValues4 << (qreal)1.0f << (void *)nullValues4;

    QTest::newRow("double identity")
        << (void *)identityValues4 << (qreal)2.0f << (void *)doubleIdentity4;

    static qreal const values[16] =
        {1.0f, 2.0f, 3.0f, 4.0f,
         5.0f, 6.0f, 7.0f, 8.0f,
         9.0f, 10.0f, 11.0f, 12.0f,
         13.0f, 14.0f, 15.0f, 16.0f};
    static qreal const doubleValues[16] =
        {2.0f, 4.0f, 6.0f, 8.0f,
         10.0f, 12.0f, 14.0f, 16.0f,
         18.0f, 20.0f, 22.0f, 24.0f,
         26.0f, 28.0f, 30.0f, 32.0f};
    static qreal const negDoubleValues[16] =
        {-2.0f, -4.0f, -6.0f, -8.0f,
         -10.0f, -12.0f, -14.0f, -16.0f,
         -18.0f, -20.0f, -22.0f, -24.0f,
         -26.0f, -28.0f, -30.0f, -32.0f};

    QTest::newRow("unique")
        << (void *)values << (qreal)2.0f << (void *)doubleValues;

    QTest::newRow("neg")
        << (void *)values << (qreal)-2.0f << (void *)negDoubleValues;

    QTest::newRow("zero")
        << (void *)values << (qreal)0.0f << (void *)nullValues4;
}
void tst_QMatrixNxN::multiplyFactor4x4()
{
    QFETCH(void *, m1Values);
    QFETCH(qreal, factor);
    QFETCH(void *, m2Values);

    QMatrix4x4 m1((const qreal *)m1Values);

    QMatrix4x4 m3;
    m3 = m1;
    m3 *= factor;
    QVERIFY(isSame(m3, (const qreal *)m2Values));

    QMatrix4x4 m4;
    m4 = m1 * factor;
    QVERIFY(isSame(m4, (const qreal *)m2Values));

    QMatrix4x4 m5;
    m5 = factor * m1;
    QVERIFY(isSame(m5, (const qreal *)m2Values));
}

// Test matrix multiplication by a factor for 4x3 matrices.
void tst_QMatrixNxN::multiplyFactor4x3_data()
{
    QTest::addColumn<void *>("m1Values");
    QTest::addColumn<qreal>("factor");
    QTest::addColumn<void *>("m2Values");

    QTest::newRow("null")
        << (void *)nullValues4x3 << (qreal)1.0f << (void *)nullValues4x3;

    QTest::newRow("double identity")
        << (void *)identityValues4x3 << (qreal)2.0f << (void *)doubleIdentity4x3;

    static qreal const values[12] =
        {1.0f, 2.0f, 3.0f, 4.0f,
         5.0f, 6.0f, 7.0f, 8.0f,
         9.0f, 10.0f, 11.0f, 12.0f};
    static qreal const doubleValues[12] =
        {2.0f, 4.0f, 6.0f, 8.0f,
         10.0f, 12.0f, 14.0f, 16.0f,
         18.0f, 20.0f, 22.0f, 24.0f};
    static qreal const negDoubleValues[12] =
        {-2.0f, -4.0f, -6.0f, -8.0f,
         -10.0f, -12.0f, -14.0f, -16.0f,
         -18.0f, -20.0f, -22.0f, -24.0f};

    QTest::newRow("unique")
        << (void *)values << (qreal)2.0f << (void *)doubleValues;

    QTest::newRow("neg")
        << (void *)values << (qreal)-2.0f << (void *)negDoubleValues;

    QTest::newRow("zero")
        << (void *)values << (qreal)0.0f << (void *)nullValues4x3;
}
void tst_QMatrixNxN::multiplyFactor4x3()
{
    QFETCH(void *, m1Values);
    QFETCH(qreal, factor);
    QFETCH(void *, m2Values);

    QMatrix4x3 m1((const qreal *)m1Values);

    QMatrix4x3 m3;
    m3 = m1;
    m3 *= factor;
    QVERIFY(isSame(m3, (const qreal *)m2Values));

    QMatrix4x3 m4;
    m4 = m1 * factor;
    QVERIFY(isSame(m4, (const qreal *)m2Values));

    QMatrix4x3 m5;
    m5 = factor * m1;
    QVERIFY(isSame(m5, (const qreal *)m2Values));
}

// Test matrix division by a factor for 2x2 matrices.
void tst_QMatrixNxN::divideFactor2x2_data()
{
    // Use the same test cases as the multiplyFactor test.
    multiplyFactor2x2_data();
}
void tst_QMatrixNxN::divideFactor2x2()
{
    QFETCH(void *, m1Values);
    QFETCH(qreal, factor);
    QFETCH(void *, m2Values);

    if (factor == 0.0f)
        return;

    QMatrix2x2 m2((const qreal *)m2Values);

    QMatrix2x2 m3;
    m3 = m2;
    m3 /= factor;
    QVERIFY(isSame(m3, (const qreal *)m1Values));

    QMatrix2x2 m4;
    m4 = m2 / factor;
    QVERIFY(isSame(m4, (const qreal *)m1Values));
}

// Test matrix division by a factor for 3x3 matrices.
void tst_QMatrixNxN::divideFactor3x3_data()
{
    // Use the same test cases as the multiplyFactor test.
    multiplyFactor3x3_data();
}
void tst_QMatrixNxN::divideFactor3x3()
{
    QFETCH(void *, m1Values);
    QFETCH(qreal, factor);
    QFETCH(void *, m2Values);

    if (factor == 0.0f)
        return;

    QMatrix3x3 m2((const qreal *)m2Values);

    QMatrix3x3 m3;
    m3 = m2;
    m3 /= factor;
    QVERIFY(isSame(m3, (const qreal *)m1Values));

    QMatrix3x3 m4;
    m4 = m2 / factor;
    QVERIFY(isSame(m4, (const qreal *)m1Values));
}

// Test matrix division by a factor for 4x4 matrices.
void tst_QMatrixNxN::divideFactor4x4_data()
{
    // Use the same test cases as the multiplyFactor test.
    multiplyFactor4x4_data();
}
void tst_QMatrixNxN::divideFactor4x4()
{
    QFETCH(void *, m1Values);
    QFETCH(qreal, factor);
    QFETCH(void *, m2Values);

    if (factor == 0.0f)
        return;

    QMatrix4x4 m2((const qreal *)m2Values);

    QMatrix4x4 m3;
    m3 = m2;
    m3 /= factor;
    QVERIFY(isSame(m3, (const qreal *)m1Values));

    QMatrix4x4 m4;
    m4 = m2 / factor;
    QVERIFY(isSame(m4, (const qreal *)m1Values));
}

// Test matrix division by a factor for 4x3 matrices.
void tst_QMatrixNxN::divideFactor4x3_data()
{
    // Use the same test cases as the multiplyFactor test.
    multiplyFactor4x3_data();
}
void tst_QMatrixNxN::divideFactor4x3()
{
    QFETCH(void *, m1Values);
    QFETCH(qreal, factor);
    QFETCH(void *, m2Values);

    if (factor == 0.0f)
        return;

    QMatrix4x3 m2((const qreal *)m2Values);

    QMatrix4x3 m3;
    m3 = m2;
    m3 /= factor;
    QVERIFY(isSame(m3, (const qreal *)m1Values));

    QMatrix4x3 m4;
    m4 = m2 / factor;
    QVERIFY(isSame(m4, (const qreal *)m1Values));
}

// Test matrix negation for 2x2 matrices.
void tst_QMatrixNxN::negate2x2_data()
{
    // Use the same test cases as the multiplyFactor test.
    multiplyFactor2x2_data();
}
void tst_QMatrixNxN::negate2x2()
{
    QFETCH(void *, m1Values);

    const qreal *values = (const qreal *)m1Values;

    QMatrix2x2 m1(values);

    qreal negated[4];
    for (int index = 0; index < 4; ++index)
        negated[index] = -values[index];

    QMatrix2x2 m2;
    m2 = -m1;
    QVERIFY(isSame(m2, negated));
}

// Test matrix negation for 3x3 matrices.
void tst_QMatrixNxN::negate3x3_data()
{
    // Use the same test cases as the multiplyFactor test.
    multiplyFactor3x3_data();
}
void tst_QMatrixNxN::negate3x3()
{
    QFETCH(void *, m1Values);

    const qreal *values = (const qreal *)m1Values;

    QMatrix3x3 m1(values);

    qreal negated[9];
    for (int index = 0; index < 9; ++index)
        negated[index] = -values[index];

    QMatrix3x3 m2;
    m2 = -m1;
    QVERIFY(isSame(m2, negated));
}

// Test matrix negation for 4x4 matrices.
void tst_QMatrixNxN::negate4x4_data()
{
    // Use the same test cases as the multiplyFactor test.
    multiplyFactor4x4_data();
}
void tst_QMatrixNxN::negate4x4()
{
    QFETCH(void *, m1Values);

    const qreal *values = (const qreal *)m1Values;

    QMatrix4x4 m1(values);

    qreal negated[16];
    for (int index = 0; index < 16; ++index)
        negated[index] = -values[index];

    QMatrix4x4 m2;
    m2 = -m1;
    QVERIFY(isSame(m2, negated));
}

// Test matrix negation for 4x3 matrices.
void tst_QMatrixNxN::negate4x3_data()
{
    // Use the same test cases as the multiplyFactor test.
    multiplyFactor4x3_data();
}
void tst_QMatrixNxN::negate4x3()
{
    QFETCH(void *, m1Values);

    const qreal *values = (const qreal *)m1Values;

    QMatrix4x3 m1(values);

    qreal negated[12];
    for (int index = 0; index < 12; ++index)
        negated[index] = -values[index];

    QMatrix4x3 m2;
    m2 = -m1;
    QVERIFY(isSame(m2, negated));
}

// Matrix inverted.  This is a more straight-forward implementation
// of the algorithm at http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q24
// than the optimized version in the QMatrix4x4 code.  Hopefully it is
// easier to verify that this version is the same as the reference.

struct Matrix3
{
    qreal v[9];
};
struct Matrix4
{
    qreal v[16];
};

static qreal m3Determinant(const Matrix3& m)
{
    return m.v[0] * (m.v[4] * m.v[8] - m.v[7] * m.v[5]) -
           m.v[1] * (m.v[3] * m.v[8] - m.v[6] * m.v[5]) +
           m.v[2] * (m.v[3] * m.v[7] - m.v[6] * m.v[4]);
}

static bool m3Inverse(const Matrix3& min, Matrix3& mout)
{
    qreal det = m3Determinant(min);
    if (det == 0.0f)
        return false;
    mout.v[0] =  (min.v[4] * min.v[8] - min.v[5] * min.v[7]) / det;
    mout.v[1] = -(min.v[1] * min.v[8] - min.v[2] * min.v[7]) / det;
    mout.v[2] =  (min.v[1] * min.v[5] - min.v[4] * min.v[2]) / det;
    mout.v[3] = -(min.v[3] * min.v[8] - min.v[5] * min.v[6]) / det;
    mout.v[4] =  (min.v[0] * min.v[8] - min.v[6] * min.v[2]) / det;
    mout.v[5] = -(min.v[0] * min.v[5] - min.v[3] * min.v[2]) / det;
    mout.v[6] =  (min.v[3] * min.v[7] - min.v[6] * min.v[4]) / det;
    mout.v[7] = -(min.v[0] * min.v[7] - min.v[6] * min.v[1]) / det;
    mout.v[8] =  (min.v[0] * min.v[4] - min.v[1] * min.v[3]) / det;
    return true;
}

static void m3Transpose(Matrix3& m)
{
    qSwap(m.v[1], m.v[3]);
    qSwap(m.v[2], m.v[6]);
    qSwap(m.v[5], m.v[7]);
}

static void m4Submatrix(const Matrix4& min, Matrix3& mout, int i, int j)
{
    for (int di = 0; di < 3; ++di) {
        for (int dj = 0; dj < 3; ++dj) {
            int si = di + ((di >= i) ? 1 : 0);
            int sj = dj + ((dj >= j) ? 1 : 0);
            mout.v[di * 3 + dj] = min.v[si * 4 + sj];
        }
    }
}

static qreal m4Determinant(const Matrix4& m)
{
    qreal det;
    qreal result = 0.0f;
    qreal i = 1.0f;
    Matrix3 msub;
    for (int n = 0; n < 4; ++n, i *= -1.0f) {
        m4Submatrix(m, msub, 0, n);
        det = m3Determinant(msub);
        result += m.v[n] * det * i;
    }
    return result;
}

static void m4Inverse(const Matrix4& min, Matrix4& mout)
{
    qreal det = m4Determinant(min);
    Matrix3 msub;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            qreal sign = 1.0f - ((i + j) % 2) * 2.0f;
            m4Submatrix(min, msub, i, j);
            mout.v[i + j * 4] = (m3Determinant(msub) * sign) / det;
        }
    }
}

// Test matrix inverted for 4x4 matrices.
void tst_QMatrixNxN::inverted4x4_data()
{
    QTest::addColumn<void *>("m1Values");
    QTest::addColumn<void *>("m2Values");
    QTest::addColumn<bool>("invertible");

    QTest::newRow("null")
        << (void *)nullValues4 << (void *)identityValues4 << false;

    QTest::newRow("identity")
        << (void *)identityValues4 << (void *)identityValues4 << true;

    QTest::newRow("unique")
        << (void *)uniqueValues4 << (void *)identityValues4 << false;

    static Matrix4 const invertible = {
        {5.0f, 0.0f, 0.0f, 2.0f,
         0.0f, 6.0f, 0.0f, 3.0f,
         0.0f, 0.0f, 7.0f, 4.0f,
         0.0f, 0.0f, 0.0f, 1.0f}
    };
    static Matrix4 inverted;
    m4Inverse(invertible, inverted);

    QTest::newRow("invertible")
        << (void *)invertible.v << (void *)inverted.v << true;

    static Matrix4 const translate = {
        {1.0f, 0.0f, 0.0f, 2.0f,
         0.0f, 1.0f, 0.0f, 3.0f,
         0.0f, 0.0f, 1.0f, 4.0f,
         0.0f, 0.0f, 0.0f, 1.0f}
    };
    static Matrix4 const inverseTranslate = {
        {1.0f, 0.0f, 0.0f, -2.0f,
         0.0f, 1.0f, 0.0f, -3.0f,
         0.0f, 0.0f, 1.0f, -4.0f,
         0.0f, 0.0f, 0.0f, 1.0f}
    };

    QTest::newRow("translate")
        << (void *)translate.v << (void *)inverseTranslate.v << true;
}
void tst_QMatrixNxN::inverted4x4()
{
    QFETCH(void *, m1Values);
    QFETCH(void *, m2Values);
    QFETCH(bool, invertible);

    QMatrix4x4 m1((const qreal *)m1Values);

    if (invertible)
        QVERIFY(m1.determinant() != 0.0f);
    else
        QVERIFY(m1.determinant() == 0.0f);

    Matrix4 m1alt;
    memcpy(m1alt.v, (const qreal *)m1Values, sizeof(m1alt.v));

    QCOMPARE(m1.determinant(), m4Determinant(m1alt));

    QMatrix4x4 m2;
    bool inv;
    m2 = m1.inverted(&inv);
    QVERIFY(isSame(m2, (const qreal *)m2Values));

    if (invertible) {
        QVERIFY(inv);

        Matrix4 m2alt;
        m4Inverse(m1alt, m2alt);
        QVERIFY(isSame(m2, m2alt.v));

        QMatrix4x4 m3;
        m3 = m1 * m2;
        QVERIFY(isIdentity(m3));

        QMatrix4x4 m4;
        m4 = m2 * m1;
        QVERIFY(isIdentity(m4));
    } else {
        QVERIFY(!inv);
    }

    // Test again, after inferring the special matrix type.
    m1.optimize();
    m2 = m1.inverted(&inv);
    QVERIFY(isSame(m2, (const qreal *)m2Values));
    QCOMPARE(inv, invertible);
}

void tst_QMatrixNxN::orthonormalInverse4x4()
{
    QMatrix4x4 m1;
    QVERIFY(qFuzzyCompare(m1.inverted(), m1));

    QMatrix4x4 m2;
    m2.rotate(45.0, 1.0, 0.0, 0.0);
    m2.translate(10.0, 0.0, 0.0);

    // Use optimize() to drop the internal flags that
    // mark the matrix as orthonormal.  This will force inverted()
    // to compute m3.inverted() the long way.  We can then compare
    // the result to what the faster algorithm produces on m2.
    QMatrix4x4 m3 = m2;
    m3.optimize();
    bool invertible;
    QVERIFY(qFuzzyCompare(m2.inverted(&invertible), m3.inverted()));
    QVERIFY(invertible);

    QMatrix4x4 m4;
    m4.rotate(45.0, 0.0, 1.0, 0.0);
    QMatrix4x4 m5 = m4;
    m5.optimize();
    QVERIFY(qFuzzyCompare(m4.inverted(), m5.inverted()));

    QMatrix4x4 m6;
    m1.rotate(88, 0.0, 0.0, 1.0);
    m1.translate(-20.0, 20.0, 15.0);
    m1.rotate(25, 1.0, 0.0, 0.0);
    QMatrix4x4 m7 = m6;
    m7.optimize();
    QVERIFY(qFuzzyCompare(m6.inverted(), m7.inverted()));
}

// Test the generation and use of 4x4 scale matrices.
void tst_QMatrixNxN::scale4x4_data()
{
    QTest::addColumn<qreal>("x");
    QTest::addColumn<qreal>("y");
    QTest::addColumn<qreal>("z");
    QTest::addColumn<void *>("resultValues");

    static const qreal nullScale[] =
        {0.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    QTest::newRow("null")
        << (qreal)0.0f << (qreal)0.0f << (qreal)0.0f << (void *)nullScale;

    QTest::newRow("identity")
        << (qreal)1.0f << (qreal)1.0f << (qreal)1.0f << (void *)identityValues4;

    static const qreal doubleScale[] =
        {2.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 2.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 2.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    QTest::newRow("double")
        << (qreal)2.0f << (qreal)2.0f << (qreal)2.0f << (void *)doubleScale;

    static const qreal complexScale[] =
        {2.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 11.0f, 0.0f, 0.0f,
         0.0f, 0.0f, -6.5f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    QTest::newRow("complex")
        << (qreal)2.0f << (qreal)11.0f << (qreal)-6.5f << (void *)complexScale;

    static const qreal complexScale2D[] =
        {2.0f, 0.0f, 0.0f, 0.0f,
         0.0f, -11.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    QTest::newRow("complex2D")
        << (qreal)2.0f << (qreal)-11.0f << (qreal)1.0f << (void *)complexScale2D;
}
void tst_QMatrixNxN::scale4x4()
{
    QFETCH(qreal, x);
    QFETCH(qreal, y);
    QFETCH(qreal, z);
    QFETCH(void *, resultValues);

    QMatrix4x4 result((const qreal *)resultValues);

    QMatrix4x4 m1;
    m1.scale(QVector3D(x, y, z));
    QVERIFY(isSame(m1, (const qreal *)resultValues));

    QMatrix4x4 m2;
    m2.scale(x, y, z);
    QVERIFY(isSame(m2, (const qreal *)resultValues));

    if (z == 1.0f) {
        QMatrix4x4 m2b;
        m2b.scale(x, y);
        QVERIFY(m2b == m2);
    }

    QVector3D v1(2.0f, 3.0f, -4.0f);
    QVector3D v2 = m1 * v1;
    QCOMPARE(v2.x(), (qreal)(2.0f * x));
    QCOMPARE(v2.y(), (qreal)(3.0f * y));
    QCOMPARE(v2.z(), (qreal)(-4.0f * z));

    v2 = v1 * m1;
    QCOMPARE(v2.x(), (qreal)(2.0f * x));
    QCOMPARE(v2.y(), (qreal)(3.0f * y));
    QCOMPARE(v2.z(), (qreal)(-4.0f * z));

    QVector4D v3(2.0f, 3.0f, -4.0f, 34.0f);
    QVector4D v4 = m1 * v3;
    QCOMPARE(v4.x(), (qreal)(2.0f * x));
    QCOMPARE(v4.y(), (qreal)(3.0f * y));
    QCOMPARE(v4.z(), (qreal)(-4.0f * z));
    QCOMPARE(v4.w(), (qreal)34.0f);

    v4 = v3 * m1;
    QCOMPARE(v4.x(), (qreal)(2.0f * x));
    QCOMPARE(v4.y(), (qreal)(3.0f * y));
    QCOMPARE(v4.z(), (qreal)(-4.0f * z));
    QCOMPARE(v4.w(), (qreal)34.0f);

    QPoint p1(2, 3);
    QPoint p2 = m1 * p1;
    QCOMPARE(p2.x(), (int)(2.0f * x));
    QCOMPARE(p2.y(), (int)(3.0f * y));

    p2 = p1 * m1;
    QCOMPARE(p2.x(), (int)(2.0f * x));
    QCOMPARE(p2.y(), (int)(3.0f * y));

    QPointF p3(2.0f, 3.0f);
    QPointF p4 = m1 * p3;
    QCOMPARE(p4.x(), (qreal)(2.0f * x));
    QCOMPARE(p4.y(), (qreal)(3.0f * y));

    p4 = p3 * m1;
    QCOMPARE(p4.x(), (qreal)(2.0f * x));
    QCOMPARE(p4.y(), (qreal)(3.0f * y));

    QMatrix4x4 m3(uniqueValues4);
    QMatrix4x4 m4(m3);
    m4.scale(x, y, z);
    QVERIFY(m4 == m3 * m1);

    if (x == y && y == z) {
        QMatrix4x4 m5;
        m5.scale(x);
        QVERIFY(isSame(m5, (const qreal *)resultValues));
    }

    if (z == 1.0f) {
        QMatrix4x4 m4b(m3);
        m4b.scale(x, y);
        QVERIFY(m4b == m4);
    }

    // Test coverage when the special matrix type is unknown.

    QMatrix4x4 m6;
    m6(0, 0) = 1.0f;
    m6.scale(QVector3D(x, y, z));
    QVERIFY(isSame(m6, (const qreal *)resultValues));

    QMatrix4x4 m7;
    m7(0, 0) = 1.0f;
    m7.scale(x, y, z);
    QVERIFY(isSame(m7, (const qreal *)resultValues));

    if (x == y && y == z) {
        QMatrix4x4 m8;
        m8(0, 0) = 1.0f;
        m8.scale(x);
        QVERIFY(isSame(m8, (const qreal *)resultValues));

        m8.optimize();
        m8.scale(1.0f);
        QVERIFY(isSame(m8, (const qreal *)resultValues));

        QMatrix4x4 m9;
        m9.translate(0.0f, 0.0f, 0.0f);
        m9.scale(x);
        QVERIFY(isSame(m9, (const qreal *)resultValues));
    }
}

// Test the generation and use of 4x4 translation matrices.
void tst_QMatrixNxN::translate4x4_data()
{
    QTest::addColumn<qreal>("x");
    QTest::addColumn<qreal>("y");
    QTest::addColumn<qreal>("z");
    QTest::addColumn<void *>("resultValues");

    QTest::newRow("null")
        << (qreal)0.0f << (qreal)0.0f << (qreal)0.0f << (void *)identityValues4;

    static const qreal identityTranslate[] =
        {1.0f, 0.0f, 0.0f, 1.0f,
         0.0f, 1.0f, 0.0f, 1.0f,
         0.0f, 0.0f, 1.0f, 1.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    QTest::newRow("identity")
        << (qreal)1.0f << (qreal)1.0f << (qreal)1.0f << (void *)identityTranslate;

    static const qreal complexTranslate[] =
        {1.0f, 0.0f, 0.0f, 2.0f,
         0.0f, 1.0f, 0.0f, 11.0f,
         0.0f, 0.0f, 1.0f, -6.5f,
         0.0f, 0.0f, 0.0f, 1.0f};
    QTest::newRow("complex")
        << (qreal)2.0f << (qreal)11.0f << (qreal)-6.5f << (void *)complexTranslate;

    static const qreal complexTranslate2D[] =
        {1.0f, 0.0f, 0.0f, 2.0f,
         0.0f, 1.0f, 0.0f, -11.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    QTest::newRow("complex2D")
        << (qreal)2.0f << (qreal)-11.0f << (qreal)0.0f << (void *)complexTranslate2D;
}
void tst_QMatrixNxN::translate4x4()
{
    QFETCH(qreal, x);
    QFETCH(qreal, y);
    QFETCH(qreal, z);
    QFETCH(void *, resultValues);

    QMatrix4x4 result((const qreal *)resultValues);

    QMatrix4x4 m1;
    m1.translate(QVector3D(x, y, z));
    QVERIFY(isSame(m1, (const qreal *)resultValues));

    QMatrix4x4 m2;
    m2.translate(x, y, z);
    QVERIFY(isSame(m2, (const qreal *)resultValues));

    if (z == 0.0f) {
        QMatrix4x4 m2b;
        m2b.translate(x, y);
        QVERIFY(m2b == m2);
    }

    QVector3D v1(2.0f, 3.0f, -4.0f);
    QVector3D v2 = m1 * v1;
    QCOMPARE(v2.x(), (qreal)(2.0f + x));
    QCOMPARE(v2.y(), (qreal)(3.0f + y));
    QCOMPARE(v2.z(), (qreal)(-4.0f + z));

    QVector4D v3(2.0f, 3.0f, -4.0f, 1.0f);
    QVector4D v4 = m1 * v3;
    QCOMPARE(v4.x(), (qreal)(2.0f + x));
    QCOMPARE(v4.y(), (qreal)(3.0f + y));
    QCOMPARE(v4.z(), (qreal)(-4.0f + z));
    QCOMPARE(v4.w(), (qreal)1.0f);

    QVector4D v5(2.0f, 3.0f, -4.0f, 34.0f);
    QVector4D v6 = m1 * v5;
    QCOMPARE(v6.x(), (qreal)(2.0f + x * 34.0f));
    QCOMPARE(v6.y(), (qreal)(3.0f + y * 34.0f));
    QCOMPARE(v6.z(), (qreal)(-4.0f + z * 34.0f));
    QCOMPARE(v6.w(), (qreal)34.0f);

    QPoint p1(2, 3);
    QPoint p2 = m1 * p1;
    QCOMPARE(p2.x(), (int)(2.0f + x));
    QCOMPARE(p2.y(), (int)(3.0f + y));

    QPointF p3(2.0f, 3.0f);
    QPointF p4 = m1 * p3;
    QCOMPARE(p4.x(), (qreal)(2.0f + x));
    QCOMPARE(p4.y(), (qreal)(3.0f + y));

    QMatrix4x4 m3(uniqueValues4);
    QMatrix4x4 m4(m3);
    m4.translate(x, y, z);
    QVERIFY(m4 == m3 * m1);

    if (z == 0.0f) {
        QMatrix4x4 m4b(m3);
        m4b.translate(x, y);
        QVERIFY(m4b == m4);
    }
}

// Test the generation and use of 4x4 rotation matrices.
void tst_QMatrixNxN::rotate4x4_data()
{
    QTest::addColumn<qreal>("angle");
    QTest::addColumn<qreal>("x");
    QTest::addColumn<qreal>("y");
    QTest::addColumn<qreal>("z");
    QTest::addColumn<void *>("resultValues");

    static const qreal nullRotate[] =
        {0.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    QTest::newRow("null")
        << (qreal)90.0f
        << (qreal)0.0f << (qreal)0.0f << (qreal)0.0f
        << (void *)nullRotate;

    static const qreal noRotate[] =
        {1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    QTest::newRow("zerodegrees")
        << (qreal)0.0f
        << (qreal)2.0f << (qreal)3.0f << (qreal)-4.0f
        << (void *)noRotate;

    static const qreal xRotate[] =
        {1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 0.0f, -1.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    QTest::newRow("xrotate")
        << (qreal)90.0f
        << (qreal)1.0f << (qreal)0.0f << (qreal)0.0f
        << (void *)xRotate;

    static const qreal xRotateNeg[] =
        {1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         0.0f, -1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    QTest::newRow("-xrotate")
        << (qreal)90.0f
        << (qreal)-1.0f << (qreal)0.0f << (qreal)0.0f
        << (void *)xRotateNeg;

    static const qreal yRotate[] =
        {0.0f, 0.0f, 1.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
         -1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    QTest::newRow("yrotate")
        << (qreal)90.0f
        << (qreal)0.0f << (qreal)1.0f << (qreal)0.0f
        << (void *)yRotate;

    static const qreal yRotateNeg[] =
        {0.0f, 0.0f, -1.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
         1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    QTest::newRow("-yrotate")
        << (qreal)90.0f
        << (qreal)0.0f << (qreal)-1.0f << (qreal)0.0f
        << (void *)yRotateNeg;

    static const qreal zRotate[] =
        {0.0f, -1.0f, 0.0f, 0.0f,
         1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    QTest::newRow("zrotate")
        << (qreal)90.0f
        << (qreal)0.0f << (qreal)0.0f << (qreal)1.0f
        << (void *)zRotate;

    static const qreal zRotateNeg[] =
        {0.0f, 1.0f, 0.0f, 0.0f,
         -1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    QTest::newRow("-zrotate")
        << (qreal)90.0f
        << (qreal)0.0f << (qreal)0.0f << (qreal)-1.0f
        << (void *)zRotateNeg;

    // Algorithm from http://en.wikipedia.org/wiki/Rotation_matrix.
    // Deliberately different from the one in the code for cross-checking.
    static qreal complexRotate[16];
    qreal x = 1.0f;
    qreal y = 2.0f;
    qreal z = -6.0f;
    qreal angle = -45.0f;
    qreal c = qCos(angle * M_PI / 180.0f);
    qreal s = qSin(angle * M_PI / 180.0f);
    qreal len = qSqrt(x * x + y * y + z * z);
    qreal xu = x / len;
    qreal yu = y / len;
    qreal zu = z / len;
    complexRotate[0] = (qreal)((1 - xu * xu) * c + xu * xu);
    complexRotate[1] = (qreal)(-zu * s - xu * yu * c + xu * yu);
    complexRotate[2] = (qreal)(yu * s - xu * zu * c + xu * zu);
    complexRotate[3] = 0;
    complexRotate[4] = (qreal)(zu * s - xu * yu * c + xu * yu);
    complexRotate[5] = (qreal)((1 - yu * yu) * c + yu * yu);
    complexRotate[6] = (qreal)(-xu * s - yu * zu * c + yu * zu);
    complexRotate[7] = 0;
    complexRotate[8] = (qreal)(-yu * s - xu * zu * c + xu * zu);
    complexRotate[9] = (qreal)(xu * s - yu * zu * c + yu * zu);
    complexRotate[10] = (qreal)((1 - zu * zu) * c + zu * zu);
    complexRotate[11] = 0;
    complexRotate[12] = 0;
    complexRotate[13] = 0;
    complexRotate[14] = 0;
    complexRotate[15] = 1;

    QTest::newRow("complex")
        << (qreal)angle
        << (qreal)x << (qreal)y << (qreal)z
        << (void *)complexRotate;
}
void tst_QMatrixNxN::rotate4x4()
{
    QFETCH(qreal, angle);
    QFETCH(qreal, x);
    QFETCH(qreal, y);
    QFETCH(qreal, z);
    QFETCH(void *, resultValues);

    QMatrix4x4 m1;
    m1.rotate(angle, QVector3D(x, y, z));
    QVERIFY(isSame(m1, (const qreal *)resultValues));

    QMatrix4x4 m2;
    m2.rotate(angle, x, y, z);
    QVERIFY(isSame(m2, (const qreal *)resultValues));

    QMatrix4x4 m3(uniqueValues4);
    QMatrix4x4 m4(m3);
    m4.rotate(angle, x, y, z);
    QVERIFY(qFuzzyCompare(m4, m3 * m1));

    // Null vectors don't make sense for quaternion rotations.
    if (x != 0 || y != 0 || z != 0) {
        QMatrix4x4 m5;
        m5.rotate(QQuaternion::fromAxisAndAngle(QVector3D(x, y, z), angle));
        QVERIFY(isSame(m5, (const qreal *)resultValues));
    }

#define ROTATE4(xin,yin,zin,win,xout,yout,zout,wout) \
    do { \
        xout = ((const qreal *)resultValues)[0] * xin + \
               ((const qreal *)resultValues)[1] * yin + \
               ((const qreal *)resultValues)[2] * zin + \
               ((const qreal *)resultValues)[3] * win; \
        yout = ((const qreal *)resultValues)[4] * xin + \
               ((const qreal *)resultValues)[5] * yin + \
               ((const qreal *)resultValues)[6] * zin + \
               ((const qreal *)resultValues)[7] * win; \
        zout = ((const qreal *)resultValues)[8] * xin + \
               ((const qreal *)resultValues)[9] * yin + \
               ((const qreal *)resultValues)[10] * zin + \
               ((const qreal *)resultValues)[11] * win; \
        wout = ((const qreal *)resultValues)[12] * xin + \
               ((const qreal *)resultValues)[13] * yin + \
               ((const qreal *)resultValues)[14] * zin + \
               ((const qreal *)resultValues)[15] * win; \
    } while (0)

    // Rotate various test vectors using the straight-forward approach.
    qreal v1x, v1y, v1z, v1w;
    ROTATE4(2.0f, 3.0f, -4.0f, 1.0f, v1x, v1y, v1z, v1w);
    v1x /= v1w;
    v1y /= v1w;
    v1z /= v1w;
    qreal v3x, v3y, v3z, v3w;
    ROTATE4(2.0f, 3.0f, -4.0f, 1.0f, v3x, v3y, v3z, v3w);
    qreal v5x, v5y, v5z, v5w;
    ROTATE4(2.0f, 3.0f, -4.0f, 34.0f, v5x, v5y, v5z, v5w);
    qreal p1x, p1y, p1z, p1w;
    ROTATE4(2.0f, 3.0f, 0.0f, 1.0f, p1x, p1y, p1z, p1w);
    p1x /= p1w;
    p1y /= p1w;
    p1z /= p1w;

    QVector3D v1(2.0f, 3.0f, -4.0f);
    QVector3D v2 = m1 * v1;
    QVERIFY(fuzzyCompare(v2.x(), v1x));
    QVERIFY(fuzzyCompare(v2.y(), v1y));
    QVERIFY(fuzzyCompare(v2.z(), v1z));

    QVector4D v3(2.0f, 3.0f, -4.0f, 1.0f);
    QVector4D v4 = m1 * v3;
    QVERIFY(fuzzyCompare(v4.x(), v3x));
    QVERIFY(fuzzyCompare(v4.y(), v3y));
    QVERIFY(fuzzyCompare(v4.z(), v3z));
    QVERIFY(fuzzyCompare(v4.w(), v3w));

    QVector4D v5(2.0f, 3.0f, -4.0f, 34.0f);
    QVector4D v6 = m1 * v5;
    QVERIFY(fuzzyCompare(v6.x(), v5x));
    QVERIFY(fuzzyCompare(v6.y(), v5y));
    QVERIFY(fuzzyCompare(v6.z(), v5z));
    QVERIFY(fuzzyCompare(v6.w(), v5w));

    QPoint p1(2, 3);
    QPoint p2 = m1 * p1;
    QCOMPARE(p2.x(), qRound(p1x));
    QCOMPARE(p2.y(), qRound(p1y));

    QPointF p3(2.0f, 3.0f);
    QPointF p4 = m1 * p3;
    QVERIFY(fuzzyCompare(p4.x(), p1x));
    QVERIFY(fuzzyCompare(p4.y(), p1y));

    if (x != 0 || y != 0 || z != 0) {
        QQuaternion q = QQuaternion::fromAxisAndAngle(QVector3D(x, y, z), angle);
        QVector3D vq = q.rotatedVector(v1);
        QVERIFY(fuzzyCompare(vq.x(), v1x));
        QVERIFY(fuzzyCompare(vq.y(), v1y));
        QVERIFY(fuzzyCompare(vq.z(), v1z));
    }
}

static bool isSame(const QMatrix3x3& m1, const Matrix3& m2)
{
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (!fuzzyCompare(m1(row, col), m2.v[row * 3 + col]))
                return false;
        }
    }
    return true;
}

// Test the computation of normal matrices from 4x4 transformation matrices.
void tst_QMatrixNxN::normalMatrix_data()
{
    QTest::addColumn<void *>("mValues");

    QTest::newRow("identity")
        << (void *)identityValues4;
    QTest::newRow("unique")
        << (void *)uniqueValues4;   // Not invertible because determinant == 0.

    static qreal const translateValues[16] =
        {1.0f, 0.0f, 0.0f, 4.0f,
         0.0f, 1.0f, 0.0f, 5.0f,
         0.0f, 0.0f, 1.0f, -3.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    static qreal const scaleValues[16] =
        {2.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 7.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 9.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    static qreal const bothValues[16] =
        {2.0f, 0.0f, 0.0f, 4.0f,
         0.0f, 7.0f, 0.0f, 5.0f,
         0.0f, 0.0f, 9.0f, -3.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    static qreal const nullScaleValues1[16] =
        {0.0f, 0.0f, 0.0f, 4.0f,
         0.0f, 7.0f, 0.0f, 5.0f,
         0.0f, 0.0f, 9.0f, -3.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    static qreal const nullScaleValues2[16] =
        {2.0f, 0.0f, 0.0f, 4.0f,
         0.0f, 0.0f, 0.0f, 5.0f,
         0.0f, 0.0f, 9.0f, -3.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    static qreal const nullScaleValues3[16] =
        {2.0f, 0.0f, 0.0f, 4.0f,
         0.0f, 7.0f, 0.0f, 5.0f,
         0.0f, 0.0f, 0.0f, -3.0f,
         0.0f, 0.0f, 0.0f, 1.0f};

    QTest::newRow("translate") << (void *)translateValues;
    QTest::newRow("scale") << (void *)scaleValues;
    QTest::newRow("both") << (void *)bothValues;
    QTest::newRow("null scale 1") << (void *)nullScaleValues1;
    QTest::newRow("null scale 2") << (void *)nullScaleValues2;
    QTest::newRow("null scale 3") << (void *)nullScaleValues3;
}
void tst_QMatrixNxN::normalMatrix()
{
    QFETCH(void *, mValues);
    const qreal *values = (const qreal *)mValues;

    // Compute the expected answer the long way.
    Matrix3 min;
    Matrix3 answer;
    min.v[0] = values[0];
    min.v[1] = values[1];
    min.v[2] = values[2];
    min.v[3] = values[4];
    min.v[4] = values[5];
    min.v[5] = values[6];
    min.v[6] = values[8];
    min.v[7] = values[9];
    min.v[8] = values[10];
    bool invertible = m3Inverse(min, answer);
    m3Transpose(answer);

    // Perform the test.
    QMatrix4x4 m1(values);
    QMatrix3x3 n1 = m1.normalMatrix();

    if (invertible)
        QVERIFY(::isSame(n1, answer));
    else
        QVERIFY(isIdentity(n1));

    // Perform the test again, after inferring special matrix types.
    // This tests the optimized paths in the normalMatrix() function.
    m1.optimize();
    n1 = m1.normalMatrix();

    if (invertible)
        QVERIFY(::isSame(n1, answer));
    else
        QVERIFY(isIdentity(n1));
}

// Test optimized transformations on 4x4 matrices.
void tst_QMatrixNxN::optimizedTransforms()
{
    static qreal const translateValues[16] =
        {1.0f, 0.0f, 0.0f, 4.0f,
         0.0f, 1.0f, 0.0f, 5.0f,
         0.0f, 0.0f, 1.0f, -3.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    static qreal const translateDoubleValues[16] =
        {1.0f, 0.0f, 0.0f, 8.0f,
         0.0f, 1.0f, 0.0f, 10.0f,
         0.0f, 0.0f, 1.0f, -6.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    static qreal const scaleValues[16] =
        {2.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 7.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 9.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    static qreal const scaleDoubleValues[16] =
        {4.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 49.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 81.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    static qreal const bothValues[16] =
        {2.0f, 0.0f, 0.0f, 4.0f,
         0.0f, 7.0f, 0.0f, 5.0f,
         0.0f, 0.0f, 9.0f, -3.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    static qreal const bothReverseValues[16] =
        {2.0f, 0.0f, 0.0f, 4.0f * 2.0f,
         0.0f, 7.0f, 0.0f, 5.0f * 7.0f,
         0.0f, 0.0f, 9.0f, -3.0f * 9.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    static qreal const bothThenTranslateValues[16] =
        {2.0f, 0.0f, 0.0f, 4.0f + 2.0f * 4.0f,
         0.0f, 7.0f, 0.0f, 5.0f + 7.0f * 5.0f,
         0.0f, 0.0f, 9.0f, -3.0f + 9.0f * -3.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    static qreal const bothThenScaleValues[16] =
        {4.0f, 0.0f, 0.0f, 4.0f,
         0.0f, 49.0f, 0.0f, 5.0f,
         0.0f, 0.0f, 81.0f, -3.0f,
         0.0f, 0.0f, 0.0f, 1.0f};

    QMatrix4x4 translate(translateValues);
    QMatrix4x4 scale(scaleValues);
    QMatrix4x4 both(bothValues);

    QMatrix4x4 m1;
    m1.translate(4.0f, 5.0f, -3.0f);
    QVERIFY(isSame(m1, translateValues));
    m1.translate(4.0f, 5.0f, -3.0f);
    QVERIFY(isSame(m1, translateDoubleValues));

    QMatrix4x4 m2;
    m2.translate(QVector3D(4.0f, 5.0f, -3.0f));
    QVERIFY(isSame(m2, translateValues));
    m2.translate(QVector3D(4.0f, 5.0f, -3.0f));
    QVERIFY(isSame(m2, translateDoubleValues));

    QMatrix4x4 m3;
    m3.scale(2.0f, 7.0f, 9.0f);
    QVERIFY(isSame(m3, scaleValues));
    m3.scale(2.0f, 7.0f, 9.0f);
    QVERIFY(isSame(m3, scaleDoubleValues));

    QMatrix4x4 m4;
    m4.scale(QVector3D(2.0f, 7.0f, 9.0f));
    QVERIFY(isSame(m4, scaleValues));
    m4.scale(QVector3D(2.0f, 7.0f, 9.0f));
    QVERIFY(isSame(m4, scaleDoubleValues));

    QMatrix4x4 m5;
    m5.translate(4.0f, 5.0f, -3.0f);
    m5.scale(2.0f, 7.0f, 9.0f);
    QVERIFY(isSame(m5, bothValues));
    m5.translate(4.0f, 5.0f, -3.0f);
    QVERIFY(isSame(m5, bothThenTranslateValues));

    QMatrix4x4 m6;
    m6.translate(QVector3D(4.0f, 5.0f, -3.0f));
    m6.scale(QVector3D(2.0f, 7.0f, 9.0f));
    QVERIFY(isSame(m6, bothValues));
    m6.translate(QVector3D(4.0f, 5.0f, -3.0f));
    QVERIFY(isSame(m6, bothThenTranslateValues));

    QMatrix4x4 m7;
    m7.scale(2.0f, 7.0f, 9.0f);
    m7.translate(4.0f, 5.0f, -3.0f);
    QVERIFY(isSame(m7, bothReverseValues));

    QMatrix4x4 m8;
    m8.scale(QVector3D(2.0f, 7.0f, 9.0f));
    m8.translate(QVector3D(4.0f, 5.0f, -3.0f));
    QVERIFY(isSame(m8, bothReverseValues));

    QMatrix4x4 m9;
    m9.translate(4.0f, 5.0f, -3.0f);
    m9.scale(2.0f, 7.0f, 9.0f);
    QVERIFY(isSame(m9, bothValues));
    m9.scale(2.0f, 7.0f, 9.0f);
    QVERIFY(isSame(m9, bothThenScaleValues));

    QMatrix4x4 m10;
    m10.translate(QVector3D(4.0f, 5.0f, -3.0f));
    m10.scale(QVector3D(2.0f, 7.0f, 9.0f));
    QVERIFY(isSame(m10, bothValues));
    m10.scale(QVector3D(2.0f, 7.0f, 9.0f));
    QVERIFY(isSame(m10, bothThenScaleValues));
}

// Test orthographic projections.
void tst_QMatrixNxN::ortho()
{
    QMatrix4x4 m1;
    m1.ortho(QRect(0, 0, 300, 150));
    QPointF p1 = m1 * QPointF(0, 0);
    QPointF p2 = m1 * QPointF(300, 0);
    QPointF p3 = m1 * QPointF(0, 150);
    QPointF p4 = m1 * QPointF(300, 150);
    QVector3D p5 = m1 * QVector3D(300, 150, 1);
    QVERIFY(fuzzyCompare(p1.x(), -1.0));
    QVERIFY(fuzzyCompare(p1.y(), 1.0));
    QVERIFY(fuzzyCompare(p2.x(), 1.0));
    QVERIFY(fuzzyCompare(p2.y(), 1.0));
    QVERIFY(fuzzyCompare(p3.x(), -1.0));
    QVERIFY(fuzzyCompare(p3.y(), -1.0));
    QVERIFY(fuzzyCompare(p4.x(), 1.0));
    QVERIFY(fuzzyCompare(p4.y(), -1.0));
    QVERIFY(fuzzyCompare(p5.x(), (qreal)1.0));
    QVERIFY(fuzzyCompare(p5.y(), (qreal)-1.0));
    QVERIFY(fuzzyCompare(p5.z(), (qreal)-1.0));

    QMatrix4x4 m2;
    m2.ortho(QRectF(0, 0, 300, 150));
    p1 = m2 * QPointF(0, 0);
    p2 = m2 * QPointF(300, 0);
    p3 = m2 * QPointF(0, 150);
    p4 = m2 * QPointF(300, 150);
    p5 = m2 * QVector3D(300, 150, 1);
    QVERIFY(fuzzyCompare(p1.x(), -1.0));
    QVERIFY(fuzzyCompare(p1.y(), 1.0));
    QVERIFY(fuzzyCompare(p2.x(), 1.0));
    QVERIFY(fuzzyCompare(p2.y(), 1.0));
    QVERIFY(fuzzyCompare(p3.x(), -1.0));
    QVERIFY(fuzzyCompare(p3.y(), -1.0));
    QVERIFY(fuzzyCompare(p4.x(), 1.0));
    QVERIFY(fuzzyCompare(p4.y(), -1.0));
    QVERIFY(fuzzyCompare(p5.x(), (qreal)1.0));
    QVERIFY(fuzzyCompare(p5.y(), (qreal)-1.0));
    QVERIFY(fuzzyCompare(p5.z(), (qreal)-1.0));

    QMatrix4x4 m3;
    m3.ortho(0, 300, 150, 0, -1, 1);
    p1 = m3 * QPointF(0, 0);
    p2 = m3 * QPointF(300, 0);
    p3 = m3 * QPointF(0, 150);
    p4 = m3 * QPointF(300, 150);
    p5 = m3 * QVector3D(300, 150, 1);
    QVERIFY(fuzzyCompare(p1.x(), -1.0));
    QVERIFY(fuzzyCompare(p1.y(), 1.0));
    QVERIFY(fuzzyCompare(p2.x(), 1.0));
    QVERIFY(fuzzyCompare(p2.y(), 1.0));
    QVERIFY(fuzzyCompare(p3.x(), -1.0));
    QVERIFY(fuzzyCompare(p3.y(), -1.0));
    QVERIFY(fuzzyCompare(p4.x(), 1.0));
    QVERIFY(fuzzyCompare(p4.y(), -1.0));
    QVERIFY(fuzzyCompare(p5.x(), (qreal)1.0));
    QVERIFY(fuzzyCompare(p5.y(), (qreal)-1.0));
    QVERIFY(fuzzyCompare(p5.z(), (qreal)-1.0));

    QMatrix4x4 m4;
    m4.ortho(0, 300, 150, 0, -2, 3);
    p1 = m4 * QPointF(0, 0);
    p2 = m4 * QPointF(300, 0);
    p3 = m4 * QPointF(0, 150);
    p4 = m4 * QPointF(300, 150);
    p5 = m4 * QVector3D(300, 150, 1);
    QVERIFY(fuzzyCompare(p1.x(), -1.0));
    QVERIFY(fuzzyCompare(p1.y(), 1.0));
    QVERIFY(fuzzyCompare(p2.x(), 1.0));
    QVERIFY(fuzzyCompare(p2.y(), 1.0));
    QVERIFY(fuzzyCompare(p3.x(), -1.0));
    QVERIFY(fuzzyCompare(p3.y(), -1.0));
    QVERIFY(fuzzyCompare(p4.x(), 1.0));
    QVERIFY(fuzzyCompare(p4.y(), -1.0));
    QVERIFY(fuzzyCompare(p5.x(), (qreal)1.0));
    QVERIFY(fuzzyCompare(p5.y(), (qreal)-1.0));
    QVERIFY(fuzzyCompare(p5.z(), (qreal)-0.6));

    // An empty view volume should leave the matrix alone.
    QMatrix4x4 m5;
    m5.ortho(0, 0, 150, 0, -2, 3);
    QVERIFY(m5.isIdentity());
    m5.ortho(0, 300, 150, 150, -2, 3);
    QVERIFY(m5.isIdentity());
    m5.ortho(0, 300, 150, 0, 2, 2);
    QVERIFY(m5.isIdentity());
}

// Test perspective frustum projections.
void tst_QMatrixNxN::frustum()
{
    QMatrix4x4 m1;
    m1.frustum(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    QVector3D p1 = m1 * QVector3D(-1.0f, -1.0f, 1.0f);
    QVector3D p2 = m1 * QVector3D(1.0f, -1.0f, 1.0f);
    QVector3D p3 = m1 * QVector3D(-1.0f, 1.0f, 1.0f);
    QVector3D p4 = m1 * QVector3D(1.0f, 1.0f, 1.0f);
    QVector3D p5 = m1 * QVector3D(0.0f, 0.0f, 2.0f);
    QVERIFY(fuzzyCompare(p1.x(), -1.0f));
    QVERIFY(fuzzyCompare(p1.y(), -1.0f));
    QVERIFY(fuzzyCompare(p1.z(), -1.0f));
    QVERIFY(fuzzyCompare(p2.x(), 1.0f));
    QVERIFY(fuzzyCompare(p2.y(), -1.0f));
    QVERIFY(fuzzyCompare(p2.z(), -1.0f));
    QVERIFY(fuzzyCompare(p3.x(), -1.0f));
    QVERIFY(fuzzyCompare(p3.y(), 1.0f));
    QVERIFY(fuzzyCompare(p3.z(), -1.0f));
    QVERIFY(fuzzyCompare(p4.x(), 1.0f));
    QVERIFY(fuzzyCompare(p4.y(), 1.0f));
    QVERIFY(fuzzyCompare(p4.z(), -1.0f));
    QVERIFY(fuzzyCompare(p5.x(), 0.0f));
    QVERIFY(fuzzyCompare(p5.y(), 0.0f));
    QVERIFY(fuzzyCompare(p5.z(), -0.5f));

    // An empty view volume should leave the matrix alone.
    QMatrix4x4 m5;
    m5.frustum(0, 0, 150, 0, -2, 3);
    QVERIFY(m5.isIdentity());
    m5.frustum(0, 300, 150, 150, -2, 3);
    QVERIFY(m5.isIdentity());
    m5.frustum(0, 300, 150, 0, 2, 2);
    QVERIFY(m5.isIdentity());
}

// Test perspective field-of-view projections.
void tst_QMatrixNxN::perspective()
{
    QMatrix4x4 m1;
    m1.perspective(45.0f, 1.0f, -1.0f, 1.0f);
    QVector3D p1 = m1 * QVector3D(-1.0f, -1.0f, 1.0f);
    QVector3D p2 = m1 * QVector3D(1.0f, -1.0f, 1.0f);
    QVector3D p3 = m1 * QVector3D(-1.0f, 1.0f, 1.0f);
    QVector3D p4 = m1 * QVector3D(1.0f, 1.0f, 1.0f);
    QVector3D p5 = m1 * QVector3D(0.0f, 0.0f, 2.0f);
    QVERIFY(fuzzyCompare(p1.x(), 2.41421));
    QVERIFY(fuzzyCompare(p1.y(), 2.41421));
    QVERIFY(fuzzyCompare(p1.z(), -1));
    QVERIFY(fuzzyCompare(p2.x(), -2.41421));
    QVERIFY(fuzzyCompare(p2.y(), 2.41421));
    QVERIFY(fuzzyCompare(p2.z(), -1.0f));
    QVERIFY(fuzzyCompare(p3.x(), 2.41421));
    QVERIFY(fuzzyCompare(p3.y(), -2.41421));
    QVERIFY(fuzzyCompare(p3.z(), -1.0f));
    QVERIFY(fuzzyCompare(p4.x(), -2.41421));
    QVERIFY(fuzzyCompare(p4.y(), -2.41421));
    QVERIFY(fuzzyCompare(p4.z(), -1.0f));
    QVERIFY(fuzzyCompare(p5.x(), 0.0f));
    QVERIFY(fuzzyCompare(p5.y(), 0.0f));
    QVERIFY(fuzzyCompare(p5.z(), -0.5f));

    // An empty view volume should leave the matrix alone.
    QMatrix4x4 m5;
    m5.perspective(45.0f, 1.0f, 0.0f, 0.0f);
    QVERIFY(m5.isIdentity());
    m5.perspective(45.0f, 0.0f, -1.0f, 1.0f);
    QVERIFY(m5.isIdentity());
    m5.perspective(0.0f, 1.0f, -1.0f, 1.0f);
    QVERIFY(m5.isIdentity());
}

// Test left-handed vs right-handed coordinate flipping.
void tst_QMatrixNxN::flipCoordinates()
{
    QMatrix4x4 m1;
    m1.flipCoordinates();
    QVector3D p1 = m1 * QVector3D(2, 3, 4);
    QVERIFY(p1 == QVector3D(2, -3, -4));

    QMatrix4x4 m2;
    m2.scale(2.0f, 3.0f, 1.0f);
    m2.flipCoordinates();
    QVector3D p2 = m2 * QVector3D(2, 3, 4);
    QVERIFY(p2 == QVector3D(4, -9, -4));

    QMatrix4x4 m3;
    m3.translate(2.0f, 3.0f, 1.0f);
    m3.flipCoordinates();
    QVector3D p3 = m3 * QVector3D(2, 3, 4);
    QVERIFY(p3 == QVector3D(4, 0, -3));

    QMatrix4x4 m4;
    m4.rotate(90.0f, 0.0f, 0.0f, 1.0f);
    m4.flipCoordinates();
    QVector3D p4 = m4 * QVector3D(2, 3, 4);
    QVERIFY(p4 == QVector3D(3, 2, -4));
}

// Test conversion of generic matrices to and from the non-generic types.
void tst_QMatrixNxN::convertGeneric()
{
    QMatrix4x3 m1(uniqueValues4x3);

    static qreal const unique4x4[16] = {
        1.0f, 2.0f, 3.0f, 4.0f,
        5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    QMatrix4x4 m4(m1);
    QVERIFY(isSame(m4, unique4x4));

    QMatrix4x4 m5 = qGenericMatrixToMatrix4x4(m1);
    QVERIFY(isSame(m5, unique4x4));

    static qreal const conv4x4[12] = {
        1.0f, 2.0f, 3.0f, 4.0f,
        5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f
    };
    QMatrix4x4 m9(uniqueValues4);

    QMatrix4x3 m10 = m9.toGenericMatrix<4, 3>();
    QVERIFY(isSame(m10, conv4x4));

    QMatrix4x3 m11 = qGenericMatrixFromMatrix4x4<4, 3>(m9);
    QVERIFY(isSame(m11, conv4x4));
}

// Copy of "flagBits" in qmatrix4x4.h.
enum {
    Identity        = 0x0001,   // Identity matrix
    General         = 0x0002,   // General matrix, unknown contents
    Translation     = 0x0004,   // Contains a simple translation
    Scale           = 0x0008,   // Contains a simple scale
    Rotation        = 0x0010    // Contains a simple rotation
};

// Structure that allows direct access to "flagBits" for testing.
struct Matrix4x4
{
    qreal m[4][4];
    int flagBits;
};

// Test the inferring of special matrix types.
void tst_QMatrixNxN::optimize_data()
{
    QTest::addColumn<void *>("mValues");
    QTest::addColumn<int>("flagBits");

    QTest::newRow("null")
        << (void *)nullValues4 << (int)General;
    QTest::newRow("identity")
        << (void *)identityValues4 << (int)Identity;
    QTest::newRow("unique")
        << (void *)uniqueValues4 << (int)General;

    static qreal scaleValues[16] = {
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 3.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 4.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    QTest::newRow("scale")
        << (void *)scaleValues << (int)Scale;

    static qreal translateValues[16] = {
        1.0f, 0.0f, 0.0f, 2.0f,
        0.0f, 1.0f, 0.0f, 3.0f,
        0.0f, 0.0f, 1.0f, 4.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    QTest::newRow("scale")
        << (void *)translateValues << (int)Translation;

    static qreal bothValues[16] = {
        1.0f, 0.0f, 0.0f, 2.0f,
        0.0f, 2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 4.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    QTest::newRow("both")
        << (void *)bothValues << (int)(Scale | Translation);

    static qreal belowValues[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        4.0f, 0.0f, 0.0f, 1.0f
    };
    QTest::newRow("below")
        << (void *)belowValues << (int)General;
}
void tst_QMatrixNxN::optimize()
{
    QFETCH(void *, mValues);
    QFETCH(int, flagBits);

    QMatrix4x4 m((const qreal *)mValues);
    m.optimize();

    QCOMPARE(reinterpret_cast<Matrix4x4 *>(&m)->flagBits, flagBits);
}

void tst_QMatrixNxN::columnsAndRows()
{
    QMatrix4x4 m1(uniqueValues4);

    QVERIFY(m1.column(0) == QVector4D(1, 5, 9, 13));
    QVERIFY(m1.column(1) == QVector4D(2, 6, 10, 14));
    QVERIFY(m1.column(2) == QVector4D(3, 7, 11, 15));
    QVERIFY(m1.column(3) == QVector4D(4, 8, 12, 16));

    QVERIFY(m1.row(0) == QVector4D(1, 2, 3, 4));
    QVERIFY(m1.row(1) == QVector4D(5, 6, 7, 8));
    QVERIFY(m1.row(2) == QVector4D(9, 10, 11, 12));
    QVERIFY(m1.row(3) == QVector4D(13, 14, 15, 16));

    m1.setColumn(0, QVector4D(-1, -5, -9, -13));
    m1.setColumn(1, QVector4D(-2, -6, -10, -14));
    m1.setColumn(2, QVector4D(-3, -7, -11, -15));
    m1.setColumn(3, QVector4D(-4, -8, -12, -16));

    QVERIFY(m1.column(0) == QVector4D(-1, -5, -9, -13));
    QVERIFY(m1.column(1) == QVector4D(-2, -6, -10, -14));
    QVERIFY(m1.column(2) == QVector4D(-3, -7, -11, -15));
    QVERIFY(m1.column(3) == QVector4D(-4, -8, -12, -16));

    QVERIFY(m1.row(0) == QVector4D(-1, -2, -3, -4));
    QVERIFY(m1.row(1) == QVector4D(-5, -6, -7, -8));
    QVERIFY(m1.row(2) == QVector4D(-9, -10, -11, -12));
    QVERIFY(m1.row(3) == QVector4D(-13, -14, -15, -16));

    m1.setRow(0, QVector4D(1, 5, 9, 13));
    m1.setRow(1, QVector4D(2, 6, 10, 14));
    m1.setRow(2, QVector4D(3, 7, 11, 15));
    m1.setRow(3, QVector4D(4, 8, 12, 16));

    QVERIFY(m1.column(0) == QVector4D(1, 2, 3, 4));
    QVERIFY(m1.column(1) == QVector4D(5, 6, 7, 8));
    QVERIFY(m1.column(2) == QVector4D(9, 10, 11, 12));
    QVERIFY(m1.column(3) == QVector4D(13, 14, 15, 16));

    QVERIFY(m1.row(0) == QVector4D(1, 5, 9, 13));
    QVERIFY(m1.row(1) == QVector4D(2, 6, 10, 14));
    QVERIFY(m1.row(2) == QVector4D(3, 7, 11, 15));
    QVERIFY(m1.row(3) == QVector4D(4, 8, 12, 16));
}

// Test converting QMatrix objects into QMatrix4x4 and then
// checking that transformations in the original perform the
// equivalent transformations in the new matrix.
void tst_QMatrixNxN::convertQMatrix()
{
    QMatrix m1;
    m1.translate(-3.5, 2.0);
    QPointF p1 = m1.map(QPointF(100.0, 150.0));
    QCOMPARE(p1.x(), 100.0 - 3.5);
    QCOMPARE(p1.y(), 150.0 + 2.0);

    QMatrix4x4 m2(m1);
    QPointF p2 = m2 * QPointF(100.0, 150.0);
    QCOMPARE((double)p2.x(), 100.0 - 3.5);
    QCOMPARE((double)p2.y(), 150.0 + 2.0);
    QVERIFY(m1 == m2.toAffine());

    QMatrix m3;
    m3.scale(1.5, -2.0);
    QPointF p3 = m3.map(QPointF(100.0, 150.0));
    QCOMPARE(p3.x(), 1.5 * 100.0);
    QCOMPARE(p3.y(), -2.0 * 150.0);

    QMatrix4x4 m4(m3);
    QPointF p4 = m4 * QPointF(100.0, 150.0);
    QCOMPARE((double)p4.x(), 1.5 * 100.0);
    QCOMPARE((double)p4.y(), -2.0 * 150.0);
    QVERIFY(m3 == m4.toAffine());

    QMatrix m5;
    m5.rotate(45.0);
    QPointF p5 = m5.map(QPointF(100.0, 150.0));

    QMatrix4x4 m6(m5);
    QPointF p6 = m6 * QPointF(100.0, 150.0);
    QVERIFY(fuzzyCompare(p5.x(), p6.x()));
    QVERIFY(fuzzyCompare(p5.y(), p6.y()));

    QMatrix m7 = m6.toAffine();
    QVERIFY(fuzzyCompare(m5.m11(), m7.m11()));
    QVERIFY(fuzzyCompare(m5.m12(), m7.m12()));
    QVERIFY(fuzzyCompare(m5.m21(), m7.m21()));
    QVERIFY(fuzzyCompare(m5.m22(), m7.m22()));
    QVERIFY(fuzzyCompare(m5.dx(), m7.dx()));
    QVERIFY(fuzzyCompare(m5.dy(), m7.dy()));
}

// Test converting QTransform objects into QMatrix4x4 and then
// checking that transformations in the original perform the
// equivalent transformations in the new matrix.
void tst_QMatrixNxN::convertQTransform()
{
    QTransform m1;
    m1.translate(-3.5, 2.0);
    QPointF p1 = m1.map(QPointF(100.0, 150.0));
    QCOMPARE(p1.x(), 100.0 - 3.5);
    QCOMPARE(p1.y(), 150.0 + 2.0);

    QMatrix4x4 m2(m1);
    QPointF p2 = m2 * QPointF(100.0, 150.0);
    QCOMPARE((double)p2.x(), 100.0 - 3.5);
    QCOMPARE((double)p2.y(), 150.0 + 2.0);
    QVERIFY(m1 == m2.toTransform());

    QTransform m3;
    m3.scale(1.5, -2.0);
    QPointF p3 = m3.map(QPointF(100.0, 150.0));
    QCOMPARE(p3.x(), 1.5 * 100.0);
    QCOMPARE(p3.y(), -2.0 * 150.0);

    QMatrix4x4 m4(m3);
    QPointF p4 = m4 * QPointF(100.0, 150.0);
    QCOMPARE((double)p4.x(), 1.5 * 100.0);
    QCOMPARE((double)p4.y(), -2.0 * 150.0);
    QVERIFY(m3 == m4.toTransform());

    QTransform m5;
    m5.rotate(45.0);
    QPointF p5 = m5.map(QPointF(100.0, 150.0));

    QMatrix4x4 m6(m5);
    QPointF p6 = m6 * QPointF(100.0, 150.0);
    QVERIFY(fuzzyCompare(p5.x(), p6.x()));
    QVERIFY(fuzzyCompare(p5.y(), p6.y()));

    QTransform m7 = m6.toTransform();
    QVERIFY(fuzzyCompare(m5.m11(), m7.m11()));
    QVERIFY(fuzzyCompare(m5.m12(), m7.m12()));
    QVERIFY(fuzzyCompare(m5.m21(), m7.m21()));
    QVERIFY(fuzzyCompare(m5.m22(), m7.m22()));
    QVERIFY(fuzzyCompare(m5.dx(), m7.dx()));
    QVERIFY(fuzzyCompare(m5.dy(), m7.dy()));
    QVERIFY(fuzzyCompare(m5.m13(), m7.m13()));
    QVERIFY(fuzzyCompare(m5.m23(), m7.m23()));
    QVERIFY(fuzzyCompare(m5.m33(), m7.m33()));
}

// Test filling matrices with specific values.
void tst_QMatrixNxN::fill()
{
    QMatrix4x4 m1;
    m1.fill(0.0f);
    QVERIFY(isSame(m1, nullValues4));

    static const qreal fillValues4[] =
        {2.5f, 2.5f, 2.5f, 2.5f,
         2.5f, 2.5f, 2.5f, 2.5f,
         2.5f, 2.5f, 2.5f, 2.5f,
         2.5f, 2.5f, 2.5f, 2.5f};
    m1.fill(2.5f);
    QVERIFY(isSame(m1, fillValues4));

    QMatrix4x3 m2;
    m2.fill(0.0f);
    QVERIFY(isSame(m2, nullValues4x3));

    static const qreal fillValues4x3[] =
        {2.5f, 2.5f, 2.5f, 2.5f,
         2.5f, 2.5f, 2.5f, 2.5f,
         2.5f, 2.5f, 2.5f, 2.5f};
    m2.fill(2.5f);
    QVERIFY(isSame(m2, fillValues4x3));
}

// Test the mapRect() function for QRect and QRectF.
void tst_QMatrixNxN::mapRect_data()
{
    QTest::addColumn<qreal>("x");
    QTest::addColumn<qreal>("y");
    QTest::addColumn<qreal>("width");
    QTest::addColumn<qreal>("height");

    QTest::newRow("null")
        << (qreal)0.0f << (qreal)0.0f << (qreal)0.0f << (qreal)0.0f;
    QTest::newRow("rect")
        << (qreal)1.0f << (qreal)-20.5f << (qreal)100.0f << (qreal)63.75f;
}
void tst_QMatrixNxN::mapRect()
{
    QFETCH(qreal, x);
    QFETCH(qreal, y);
    QFETCH(qreal, width);
    QFETCH(qreal, height);

    QRectF rect(x, y, width, height);
    QRect recti(qRound(x), qRound(y), qRound(width), qRound(height));

    QMatrix4x4 m1;
    QVERIFY(m1.mapRect(rect) == rect);
    QVERIFY(m1.mapRect(recti) == recti);

    QMatrix4x4 m2;
    m2.translate(-100.5f, 64.0f);
    QRectF translated = rect.translated(-100.5f, 64.0f);
    QRect translatedi = QRect(qRound(recti.x() - 100.5f), recti.y() + 64,
                              recti.width(), recti.height());
    QVERIFY(m2.mapRect(rect) == translated);
    QVERIFY(m2.mapRect(recti) == translatedi);

    QMatrix4x4 m3;
    m3.scale(-100.5f, 64.0f);
    qreal scalex = x * -100.5f;
    qreal scaley = y * 64.0f;
    qreal scalewid = width * -100.5f;
    qreal scaleht = height * 64.0f;
    if (scalewid < 0.0f) {
        scalewid = -scalewid;
        scalex -= scalewid;
    }
    if (scaleht < 0.0f) {
        scaleht = -scaleht;
        scaley -= scaleht;
    }
    QRectF scaled(scalex, scaley, scalewid, scaleht);
    QVERIFY(m3.mapRect(rect) == scaled);
    scalex = recti.x() * -100.5f;
    scaley = recti.y() * 64.0f;
    scalewid = recti.width() * -100.5f;
    scaleht = recti.height() * 64.0f;
    if (scalewid < 0.0f) {
        scalewid = -scalewid;
        scalex -= scalewid;
    }
    if (scaleht < 0.0f) {
        scaleht = -scaleht;
        scaley -= scaleht;
    }
    QRect scaledi(qRound(scalex), qRound(scaley),
                  qRound(scalewid), qRound(scaleht));
    QVERIFY(m3.mapRect(recti) == scaledi);

    QMatrix4x4 m4;
    m4.translate(-100.5f, 64.0f);
    m4.scale(-2.5f, 4.0f);
    qreal transx1 = x * -2.5f - 100.5f;
    qreal transy1 = y * 4.0f + 64.0f;
    qreal transx2 = (x + width) * -2.5f - 100.5f;
    qreal transy2 = (y + height) * 4.0f + 64.0f;
    if (transx1 > transx2)
        qSwap(transx1, transx2);
    if (transy1 > transy2)
        qSwap(transy1, transy2);
    QRectF trans(transx1, transy1, transx2 - transx1, transy2 - transy1);
    QVERIFY(m4.mapRect(rect) == trans);
    transx1 = recti.x() * -2.5f - 100.5f;
    transy1 = recti.y() * 4.0f + 64.0f;
    transx2 = (recti.x() + recti.width()) * -2.5f - 100.5f;
    transy2 = (recti.y() + recti.height()) * 4.0f + 64.0f;
    if (transx1 > transx2)
        qSwap(transx1, transx2);
    if (transy1 > transy2)
        qSwap(transy1, transy2);
    QRect transi(qRound(transx1), qRound(transy1),
                 qRound(transx2) - qRound(transx1),
                 qRound(transy2) - qRound(transy1));
    QVERIFY(m4.mapRect(recti) == transi);

    m4.rotate(45.0f, 0.0f, 0.0f, 1.0f);

    QTransform t4;
    t4.translate(-100.5f, 64.0f);
    t4.scale(-2.5f, 4.0f);
    t4.rotate(45.0f);
    QRectF mr = m4.mapRect(rect);
    QRectF tr = t4.mapRect(rect);
    QVERIFY(fuzzyCompare(mr.x(), tr.x()));
    QVERIFY(fuzzyCompare(mr.y(), tr.y()));
    QVERIFY(fuzzyCompare(mr.width(), tr.width()));
    QVERIFY(fuzzyCompare(mr.height(), tr.height()));

    QRect mri = m4.mapRect(recti);
    QRect tri = t4.mapRect(recti);
    QVERIFY(mri == tri);
}

void tst_QMatrixNxN::mapVector_data()
{
    QTest::addColumn<void *>("mValues");

    QTest::newRow("null")
        << (void *)nullValues4;

    QTest::newRow("identity")
        << (void *)identityValues4;

    QTest::newRow("unique")
        << (void *)uniqueValues4;

    static const qreal scale[] =
        {2.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 11.0f, 0.0f, 0.0f,
         0.0f, 0.0f, -6.5f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    QTest::newRow("scale")
        << (void *)scale;

    static const qreal scaleTranslate[] =
        {2.0f, 0.0f, 0.0f, 1.0f,
         0.0f, 11.0f, 0.0f, 2.0f,
         0.0f, 0.0f, -6.5f, 3.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    QTest::newRow("scaleTranslate")
        << (void *)scaleTranslate;

    static const qreal translate[] =
        {1.0f, 0.0f, 0.0f, 1.0f,
         0.0f, 1.0f, 0.0f, 2.0f,
         0.0f, 0.0f, 1.0f, 3.0f,
         0.0f, 0.0f, 0.0f, 1.0f};
    QTest::newRow("translate")
        << (void *)translate;
}
void tst_QMatrixNxN::mapVector()
{
    QFETCH(void *, mValues);

    QMatrix4x4 m1((const qreal *)mValues);
    m1.optimize();

    QVector3D v(3.5f, -1.0f, 2.5f);

    QVector3D expected
        (v.x() * m1(0, 0) + v.y() * m1(0, 1) + v.z() * m1(0, 2),
         v.x() * m1(1, 0) + v.y() * m1(1, 1) + v.z() * m1(1, 2),
         v.x() * m1(2, 0) + v.y() * m1(2, 1) + v.z() * m1(2, 2));

    QVector3D actual = m1.mapVector(v);

    QVERIFY(fuzzyCompare(actual.x(), expected.x()));
    QVERIFY(fuzzyCompare(actual.y(), expected.y()));
    QVERIFY(fuzzyCompare(actual.z(), expected.z()));
}

class tst_QMatrixNxN4x4Properties : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QMatrix4x4 matrix READ matrix WRITE setMatrix)
public:
    tst_QMatrixNxN4x4Properties(QObject *parent = 0) : QObject(parent) {}

    QMatrix4x4 matrix() const { return m; }
    void setMatrix(const QMatrix4x4& value) { m = value; }

private:
    QMatrix4x4 m;
};

// Test getting and setting matrix properties via the metaobject system.
void tst_QMatrixNxN::properties()
{
    tst_QMatrixNxN4x4Properties obj;

    QMatrix4x4 m1(uniqueValues4);
    obj.setMatrix(m1);

    QMatrix4x4 m2 = qVariantValue<QMatrix4x4>(obj.property("matrix"));
    QVERIFY(isSame(m2, uniqueValues4));

    QMatrix4x4 m3(transposedValues4);
    obj.setProperty("matrix", qVariantFromValue(m3));

    m2 = qVariantValue<QMatrix4x4>(obj.property("matrix"));
    QVERIFY(isSame(m2, transposedValues4));
}

void tst_QMatrixNxN::metaTypes()
{
    QVERIFY(QMetaType::type("QMatrix4x4") == QMetaType::QMatrix4x4);

    QCOMPARE(QByteArray(QMetaType::typeName(QMetaType::QMatrix4x4)),
             QByteArray("QMatrix4x4"));

    QVERIFY(QMetaType::isRegistered(QMetaType::QMatrix4x4));

    QVERIFY(qMetaTypeId<QMatrix4x4>() == QMetaType::QMatrix4x4);
}

QTEST_APPLESS_MAIN(tst_QMatrixNxN)

#include "tst_qmatrixnxn.moc"
