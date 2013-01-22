/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtGui/qmatrix4x4.h>

class tst_QMatrix4x4 : public QObject
{
    Q_OBJECT
public:
    tst_QMatrix4x4() {}
    ~tst_QMatrix4x4() {}

private slots:
    void multiply_data();
    void multiply();

    void multiplyInPlace_data();
    void multiplyInPlace();

    void multiplyDirect_data();
    void multiplyDirect();

    void mapVector3D_data();
    void mapVector3D();

    void mapVector2D_data();
    void mapVector2D();

    void mapVectorDirect_data();
    void mapVectorDirect();

    void compareTranslate_data();
    void compareTranslate();

    void compareTranslateAfterScale_data();
    void compareTranslateAfterScale();

    void compareTranslateAfterRotate_data();
    void compareTranslateAfterRotate();

    void compareScale_data();
    void compareScale();

    void compareScaleAfterTranslate_data();
    void compareScaleAfterTranslate();

    void compareScaleAfterRotate_data();
    void compareScaleAfterRotate();

    void compareRotate_data();
    void compareRotate();

    void compareRotateAfterTranslate_data();
    void compareRotateAfterTranslate();

    void compareRotateAfterScale_data();
    void compareRotateAfterScale();
};

static float const generalValues[16] =
    {1.0f, 2.0f, 3.0f, 4.0f,
     5.0f, 6.0f, 7.0f, 8.0f,
     9.0f, 10.0f, 11.0f, 12.0f,
     13.0f, 14.0f, 15.0f, 16.0f};

void tst_QMatrix4x4::multiply_data()
{
    QTest::addColumn<QMatrix4x4>("m1");
    QTest::addColumn<QMatrix4x4>("m2");

    QTest::newRow("identity * identity")
        << QMatrix4x4() << QMatrix4x4();
    QTest::newRow("identity * general")
        << QMatrix4x4() << QMatrix4x4(generalValues);
    QTest::newRow("general * identity")
        << QMatrix4x4(generalValues) << QMatrix4x4();
    QTest::newRow("general * general")
        << QMatrix4x4(generalValues) << QMatrix4x4(generalValues);
}

QMatrix4x4 mresult;

void tst_QMatrix4x4::multiply()
{
    QFETCH(QMatrix4x4, m1);
    QFETCH(QMatrix4x4, m2);

    QMatrix4x4 m3;

    QBENCHMARK {
        m3 = m1 * m2;
    }

    // Force the result to be stored so the compiler doesn't
    // optimize away the contents of the benchmark loop.
    mresult = m3;
}

void tst_QMatrix4x4::multiplyInPlace_data()
{
    multiply_data();
}

void tst_QMatrix4x4::multiplyInPlace()
{
    QFETCH(QMatrix4x4, m1);
    QFETCH(QMatrix4x4, m2);

    QMatrix4x4 m3;

    QBENCHMARK {
        m3 = m1;
        m3 *= m2;
    }

    // Force the result to be stored so the compiler doesn't
    // optimize away the contents of the benchmark loop.
    mresult = m3;
}

// Use a direct naive multiplication algorithm.  This is used
// to compare against the optimized routines to see if they are
// actually faster than the naive implementation.
void tst_QMatrix4x4::multiplyDirect_data()
{
    multiply_data();
}
void tst_QMatrix4x4::multiplyDirect()
{
    QFETCH(QMatrix4x4, m1);
    QFETCH(QMatrix4x4, m2);

    QMatrix4x4 m3;

    const float *m1data = m1.constData();
    const float *m2data = m2.constData();
    float *m3data = m3.data();

    QBENCHMARK {
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                m3data[col * 4 + row] = 0.0f;
                for (int j = 0; j < 4; ++j) {
                    m3data[col * 4 + row] +=
                        m1data[j * 4 + row] * m2data[col * 4 + j];
                }
            }
        }
    }
}

QVector3D vresult;

void tst_QMatrix4x4::mapVector3D_data()
{
    QTest::addColumn<QMatrix4x4>("m1");

    QTest::newRow("identity") << QMatrix4x4();
    QTest::newRow("general") << QMatrix4x4(generalValues);

    QMatrix4x4 t1;
    t1.translate(-100.5f, 64.0f, 75.25f);
    QTest::newRow("translate3D") << t1;

    QMatrix4x4 t2;
    t2.translate(-100.5f, 64.0f);
    QTest::newRow("translate2D") << t2;

    QMatrix4x4 s1;
    s1.scale(-100.5f, 64.0f, 75.25f);
    QTest::newRow("scale3D") << s1;

    QMatrix4x4 s2;
    s2.scale(-100.5f, 64.0f);
    QTest::newRow("scale2D") << s2;
}
void tst_QMatrix4x4::mapVector3D()
{
    QFETCH(QMatrix4x4, m1);

    QVector3D v(10.5f, -2.0f, 3.0f);
    QVector3D result;

    m1.optimize();

    QBENCHMARK {
        result = m1 * v;
    }

    // Force the result to be stored so the compiler doesn't
    // optimize away the contents of the benchmark loop.
    vresult = result;
}

QPointF vresult2;

void tst_QMatrix4x4::mapVector2D_data()
{
    mapVector3D_data();
}
void tst_QMatrix4x4::mapVector2D()
{
    QFETCH(QMatrix4x4, m1);

    QPointF v(10.5f, -2.0f);
    QPointF result;

    m1.optimize();

    QBENCHMARK {
        result = m1 * v;
    }

    // Force the result to be stored so the compiler doesn't
    // optimize away the contents of the benchmark loop.
    vresult2 = result;
}

// Use a direct naive multiplication algorithm.  This is used
// to compare against the optimized routines to see if they are
// actually faster than the naive implementation.
void tst_QMatrix4x4::mapVectorDirect_data()
{
    mapVector3D_data();
}
void tst_QMatrix4x4::mapVectorDirect()
{
    QFETCH(QMatrix4x4, m1);

    const float *m1data = m1.constData();
    float v[4] = {10.5f, -2.0f, 3.0f, 1.0f};
    float result[4];

    QBENCHMARK {
        for (int row = 0; row < 4; ++row) {
            result[row] = 0.0f;
            for (int col = 0; col < 4; ++col) {
                result[row] += m1data[col * 4 + row] * v[col];
            }
        }
        result[0] /= result[3];
        result[1] /= result[3];
        result[2] /= result[3];
    }
}

// Compare the performance of QTransform::translate() to
// QMatrix4x4::translate().
void tst_QMatrix4x4::compareTranslate_data()
{
    QTest::addColumn<bool>("useQTransform");
    QTest::addColumn<QVector3D>("translation");

    QTest::newRow("QTransform::translate(0, 0, 0)")
        << true << QVector3D(0, 0, 0);
    QTest::newRow("QMatrix4x4::translate(0, 0, 0)")
        << false << QVector3D(0, 0, 0);

    QTest::newRow("QTransform::translate(1, 2, 0)")
        << true << QVector3D(1, 2, 0);
    QTest::newRow("QMatrix4x4::translate(1, 2, 0)")
        << false << QVector3D(1, 2, 0);

    QTest::newRow("QTransform::translate(1, 2, 4)")
        << true << QVector3D(1, 2, 4);
    QTest::newRow("QMatrix4x4::translate(1, 2, 4)")
        << false << QVector3D(1, 2, 4);
}
void tst_QMatrix4x4::compareTranslate()
{
    QFETCH(bool, useQTransform);
    QFETCH(QVector3D, translation);

    float x = translation.x();
    float y = translation.y();
    float z = translation.z();

    if (useQTransform) {
        QTransform t;
        QBENCHMARK {
            t.translate(x, y);
        }
    } else if (z == 0.0f) {
        QMatrix4x4 m;
        QBENCHMARK {
            m.translate(x, y);
        }
    } else {
        QMatrix4x4 m;
        QBENCHMARK {
            m.translate(x, y, z);
        }
    }
}

// Compare the performance of QTransform::translate() to
// QMatrix4x4::translate() after priming the matrix with a scale().
void tst_QMatrix4x4::compareTranslateAfterScale_data()
{
    compareTranslate_data();
}
void tst_QMatrix4x4::compareTranslateAfterScale()
{
    QFETCH(bool, useQTransform);
    QFETCH(QVector3D, translation);

    float x = translation.x();
    float y = translation.y();
    float z = translation.z();

    if (useQTransform) {
        QTransform t;
        t.scale(3, 4);
        QBENCHMARK {
            t.translate(x, y);
        }
    } else if (z == 0.0f) {
        QMatrix4x4 m;
        m.scale(3, 4);
        QBENCHMARK {
            m.translate(x, y);
        }
    } else {
        QMatrix4x4 m;
        m.scale(3, 4, 5);
        QBENCHMARK {
            m.translate(x, y, z);
        }
    }
}

// Compare the performance of QTransform::translate() to
// QMatrix4x4::translate() after priming the matrix with a rotate().
void tst_QMatrix4x4::compareTranslateAfterRotate_data()
{
    compareTranslate_data();
}
void tst_QMatrix4x4::compareTranslateAfterRotate()
{
    QFETCH(bool, useQTransform);
    QFETCH(QVector3D, translation);

    float x = translation.x();
    float y = translation.y();
    float z = translation.z();

    if (useQTransform) {
        QTransform t;
        t.rotate(45.0f);
        QBENCHMARK {
            t.translate(x, y);
        }
    } else if (z == 0.0f) {
        QMatrix4x4 m;
        m.rotate(45.0f, 0, 0, 1);
        QBENCHMARK {
            m.translate(x, y);
        }
    } else {
        QMatrix4x4 m;
        m.rotate(45.0f, 0, 0, 1);
        QBENCHMARK {
            m.translate(x, y, z);
        }
    }
}

// Compare the performance of QTransform::scale() to
// QMatrix4x4::scale().
void tst_QMatrix4x4::compareScale_data()
{
    QTest::addColumn<bool>("useQTransform");
    QTest::addColumn<QVector3D>("scale");

    QTest::newRow("QTransform::scale(1, 1, 1)")
        << true << QVector3D(1, 1, 1);
    QTest::newRow("QMatrix4x4::scale(1, 1, 1)")
        << false << QVector3D(1, 1, 1);

    QTest::newRow("QTransform::scale(3, 6, 1)")
        << true << QVector3D(3, 6, 1);
    QTest::newRow("QMatrix4x4::scale(3, 6, 1)")
        << false << QVector3D(3, 6, 1);

    QTest::newRow("QTransform::scale(3, 6, 4)")
        << true << QVector3D(3, 6, 4);
    QTest::newRow("QMatrix4x4::scale(3, 6, 4)")
        << false << QVector3D(3, 6, 4);
}
void tst_QMatrix4x4::compareScale()
{
    QFETCH(bool, useQTransform);
    QFETCH(QVector3D, scale);

    float x = scale.x();
    float y = scale.y();
    float z = scale.z();

    if (useQTransform) {
        QTransform t;
        QBENCHMARK {
            t.scale(x, y);
        }
    } else if (z == 1.0f) {
        QMatrix4x4 m;
        QBENCHMARK {
            m.scale(x, y);
        }
    } else {
        QMatrix4x4 m;
        QBENCHMARK {
            m.scale(x, y, z);
        }
    }
}

// Compare the performance of QTransform::scale() to
// QMatrix4x4::scale() after priming the matrix with a translate().
void tst_QMatrix4x4::compareScaleAfterTranslate_data()
{
    compareScale_data();
}
void tst_QMatrix4x4::compareScaleAfterTranslate()
{
    QFETCH(bool, useQTransform);
    QFETCH(QVector3D, scale);

    float x = scale.x();
    float y = scale.y();
    float z = scale.z();

    if (useQTransform) {
        QTransform t;
        t.translate(20, 34);
        QBENCHMARK {
            t.scale(x, y);
        }
    } else if (z == 1.0f) {
        QMatrix4x4 m;
        m.translate(20, 34);
        QBENCHMARK {
            m.scale(x, y);
        }
    } else {
        QMatrix4x4 m;
        m.translate(20, 34, 42);
        QBENCHMARK {
            m.scale(x, y, z);
        }
    }
}

// Compare the performance of QTransform::scale() to
// QMatrix4x4::scale() after priming the matrix with a rotate().
void tst_QMatrix4x4::compareScaleAfterRotate_data()
{
    compareScale_data();
}
void tst_QMatrix4x4::compareScaleAfterRotate()
{
    QFETCH(bool, useQTransform);
    QFETCH(QVector3D, scale);

    float x = scale.x();
    float y = scale.y();
    float z = scale.z();

    if (useQTransform) {
        QTransform t;
        t.rotate(45.0f);
        QBENCHMARK {
            t.scale(x, y);
        }
    } else if (z == 1.0f) {
        QMatrix4x4 m;
        m.rotate(45.0f, 0, 0, 1);
        QBENCHMARK {
            m.scale(x, y);
        }
    } else {
        QMatrix4x4 m;
        m.rotate(45.0f, 0, 0, 1);
        QBENCHMARK {
            m.scale(x, y, z);
        }
    }
}

// Compare the performance of QTransform::rotate() to
// QMatrix4x4::rotate().
void tst_QMatrix4x4::compareRotate_data()
{
    QTest::addColumn<bool>("useQTransform");
    QTest::addColumn<float>("angle");
    QTest::addColumn<QVector3D>("rotation");
    QTest::addColumn<int>("axis");

    QTest::newRow("QTransform::rotate(0, ZAxis)")
        << true << 0.0f << QVector3D(0, 0, 1) << int(Qt::ZAxis);
    QTest::newRow("QMatrix4x4::rotate(0, ZAxis)")
        << false << 0.0f << QVector3D(0, 0, 1) << int(Qt::ZAxis);

    QTest::newRow("QTransform::rotate(45, ZAxis)")
        << true << 45.0f << QVector3D(0, 0, 1) << int(Qt::ZAxis);
    QTest::newRow("QMatrix4x4::rotate(45, ZAxis)")
        << false << 45.0f << QVector3D(0, 0, 1) << int(Qt::ZAxis);

    QTest::newRow("QTransform::rotate(90, ZAxis)")
        << true << 90.0f << QVector3D(0, 0, 1) << int(Qt::ZAxis);
    QTest::newRow("QMatrix4x4::rotate(90, ZAxis)")
        << false << 90.0f << QVector3D(0, 0, 1) << int(Qt::ZAxis);

    QTest::newRow("QTransform::rotate(0, YAxis)")
        << true << 0.0f << QVector3D(0, 1, 0) << int(Qt::YAxis);
    QTest::newRow("QMatrix4x4::rotate(0, YAxis)")
        << false << 0.0f << QVector3D(0, 1, 0) << int(Qt::YAxis);

    QTest::newRow("QTransform::rotate(45, YAxis)")
        << true << 45.0f << QVector3D(0, 1, 0) << int(Qt::YAxis);
    QTest::newRow("QMatrix4x4::rotate(45, YAxis)")
        << false << 45.0f << QVector3D(0, 1, 0) << int(Qt::YAxis);

    QTest::newRow("QTransform::rotate(90, YAxis)")
        << true << 90.0f << QVector3D(0, 1, 0) << int(Qt::YAxis);
    QTest::newRow("QMatrix4x4::rotate(90, YAxis)")
        << false << 90.0f << QVector3D(0, 1, 0) << int(Qt::YAxis);

    QTest::newRow("QTransform::rotate(0, XAxis)")
        << true << 0.0f << QVector3D(0, 1, 0) << int(Qt::XAxis);
    QTest::newRow("QMatrix4x4::rotate(0, XAxis)")
        << false << 0.0f << QVector3D(0, 1, 0) << int(Qt::XAxis);

    QTest::newRow("QTransform::rotate(45, XAxis)")
        << true << 45.0f << QVector3D(1, 0, 0) << int(Qt::XAxis);
    QTest::newRow("QMatrix4x4::rotate(45, XAxis)")
        << false << 45.0f << QVector3D(1, 0, 0) << int(Qt::XAxis);

    QTest::newRow("QTransform::rotate(90, XAxis)")
        << true << 90.0f << QVector3D(1, 0, 0) << int(Qt::XAxis);
    QTest::newRow("QMatrix4x4::rotate(90, XAxis)")
        << false << 90.0f << QVector3D(1, 0, 0) << int(Qt::XAxis);
}
void tst_QMatrix4x4::compareRotate()
{
    QFETCH(bool, useQTransform);
    QFETCH(float, angle);
    QFETCH(QVector3D, rotation);
    QFETCH(int, axis);

    float x = rotation.x();
    float y = rotation.y();
    float z = rotation.z();

    if (useQTransform) {
        QTransform t;
        QBENCHMARK {
            t.rotate(angle, Qt::Axis(axis));
        }
    } else {
        QMatrix4x4 m;
        QBENCHMARK {
            m.rotate(angle, x, y, z);
        }
    }
}

// Compare the performance of QTransform::rotate() to
// QMatrix4x4::rotate() after priming the matrix with a translate().
void tst_QMatrix4x4::compareRotateAfterTranslate_data()
{
    compareRotate_data();
}
void tst_QMatrix4x4::compareRotateAfterTranslate()
{
    QFETCH(bool, useQTransform);
    QFETCH(float, angle);
    QFETCH(QVector3D, rotation);
    QFETCH(int, axis);

    float x = rotation.x();
    float y = rotation.y();
    float z = rotation.z();

    if (useQTransform) {
        QTransform t;
        t.translate(3, 4);
        QBENCHMARK {
            t.rotate(angle, Qt::Axis(axis));
        }
    } else {
        QMatrix4x4 m;
        m.translate(3, 4, 5);
        QBENCHMARK {
            m.rotate(angle, x, y, z);
        }
    }
}

// Compare the performance of QTransform::rotate() to
// QMatrix4x4::rotate() after priming the matrix with a scale().
void tst_QMatrix4x4::compareRotateAfterScale_data()
{
    compareRotate_data();
}
void tst_QMatrix4x4::compareRotateAfterScale()
{
    QFETCH(bool, useQTransform);
    QFETCH(float, angle);
    QFETCH(QVector3D, rotation);
    QFETCH(int, axis);

    float x = rotation.x();
    float y = rotation.y();
    float z = rotation.z();

    if (useQTransform) {
        QTransform t;
        t.scale(3, 4);
        QBENCHMARK {
            t.rotate(angle, Qt::Axis(axis));
        }
    } else {
        QMatrix4x4 m;
        m.scale(3, 4, 5);
        QBENCHMARK {
            m.rotate(angle, x, y, z);
        }
    }
}

QTEST_MAIN(tst_QMatrix4x4)

#include "tst_qmatrix4x4.moc"
