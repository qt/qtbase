/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <qtest.h>
#include <QTransform>

class tst_QTransform : public QObject
{
    Q_OBJECT

public:
    tst_QTransform();
    virtual ~tst_QTransform();

public slots:
    void init();
    void cleanup();

private slots:
    void construct();
    void translate_data();
    void translate();
    void scale_data();
    void scale();
    void shear_data();
    void shear();
    void rotate_data();
    void rotate();
    void rotateXYZ_data();
    void rotateXYZ();
    void operatorAssign_data();
    void operatorAssign();
    void operatorEqual_data();
    void operatorEqual();
    void operatorNotEqual_data();
    void operatorNotEqual();
    void operatorMultiply_data();
    void operatorMultiply();
    void operatorPlusEqualScalar_data();
    void operatorPlusEqualScalar();
    void operatorMinusEqualScalar_data();
    void operatorMinusEqualScalar();
    void operatorMultiplyEqual_data();
    void operatorMultiplyEqual();
    void operatorMultiplyEqualScalar_data();
    void operatorMultiplyEqualScalar();
    void operatorDivideEqualScalar_data();
    void operatorDivideEqualScalar();
    void mapQPoint_data();
    void mapQPoint();
    void mapQPointF_data();
    void mapQPointF();
    void mapRect_data();
    void mapRect();
    void mapRectF_data();
    void mapRectF();
    void mapQPolygon_data();
    void mapQPolygon();
    void mapQPolygonF_data();
    void mapQPolygonF();
    void mapQRegion_data();
    void mapQRegion();
    void mapToPolygon_data();
    void mapToPolygon();
    void mapQPainterPath_data();
    void mapQPainterPath();
    void isIdentity_data();
    void isIdentity();
    void isAffine_data();
    void isAffine();
    void isInvertible_data();
    void isInvertible();
    void isRotating_data();
    void isRotating();
    void isScaling_data();
    void isScaling();
    void isTranslating_data();
    void isTranslating();
    void type_data();
    void type();
    void determinant_data();
    void determinant();
    void adjoint_data();
    void adjoint();
    void transposed_data();
    void transposed();
    void inverted_data();
    void inverted();

private:
    QMap<const char *, QTransform> generateTransforms() const;
};

tst_QTransform::tst_QTransform()
{
}

tst_QTransform::~tst_QTransform()
{
}

void tst_QTransform::init()
{
}

void tst_QTransform::cleanup()
{
}

QMap<const char *, QTransform> tst_QTransform::generateTransforms() const
{
    QMap<const char *, QTransform> x;
    x["0: identity"] = QTransform();
    x["1: translate"] = QTransform().translate(10, 10);
    x["2: translate"] = QTransform().translate(-10, -10);
    x["3: rotate45"] = QTransform().rotate(45);
    x["4: rotate90"] = QTransform().rotate(90);
    x["5: rotate180"] = QTransform().rotate(180);
    x["6: shear2,2"] = QTransform().shear(2, 2);
    x["7: shear-2,-2"] = QTransform().shear(-2, -2);
    x["8: scaleUp2,2"] = QTransform().scale(2, 2);
    x["9: scaleUp2,3"] = QTransform().scale(2, 3);
    x["10: scaleDown0.5,0.5"] = QTransform().scale(0.5, 0.5);
    x["11: scaleDown0.5,0.25"] = QTransform().scale(0.5, 0.25);
    x["12: rotateX"] = QTransform().rotate(45, Qt::XAxis);
    x["13: rotateY"] = QTransform().rotate(45, Qt::YAxis);
    x["14: rotateXY"] = QTransform().rotate(45, Qt::XAxis).rotate(45, Qt::YAxis);
    x["15: rotateYZ"] = QTransform().rotate(45, Qt::YAxis).rotate(45, Qt::ZAxis);
    x["16: full"] = QTransform().translate(10, 10).rotate(45).shear(2, 2).scale(2, 2).rotate(45, Qt::YAxis).rotate(45, Qt::XAxis).rotate(45, Qt::ZAxis);
    return x;
}

void tst_QTransform::construct()
{
    QBENCHMARK {
        QTransform x;
    }
}

#define SINGLE_DATA_IMPLEMENTATION(func)        \
void tst_QTransform::func##_data() \
{ \
    QTest::addColumn<QTransform>("transform"); \
    QMap<const char *, QTransform> x = generateTransforms(); \
    QMapIterator<const char *, QTransform> it(x); \
    while (it.hasNext()) { \
        it.next(); \
        QTest::newRow(it.key()) << it.value(); \
    } \
}

#define DOUBLE_DATA_IMPLEMENTATION(func) \
void tst_QTransform::func##_data() \
{ \
    QTest::addColumn<QTransform>("x1"); \
    QTest::addColumn<QTransform>("x2"); \
    QMap<const char *, QTransform> x = generateTransforms(); \
    QMapIterator<const char *, QTransform> it(x); \
    while (it.hasNext()) { \
        it.next(); \
        const char *key1 = it.key(); \
        QTransform x1 = it.value(); \
        QMapIterator<const char *, QTransform> it2(x); \
        while (it2.hasNext()) { \
            it2.next(); \
            QTest::newRow(QString("%1 + %2").arg(key1).arg(it2.key()).toLatin1().constData()) \
                << x1 << it2.value(); \
        } \
    } \
}

SINGLE_DATA_IMPLEMENTATION(translate)

void tst_QTransform::translate()
{
    QFETCH(QTransform, transform);
    QTransform x = transform;
    QBENCHMARK {
        x.translate(10, 10);
    }
}

SINGLE_DATA_IMPLEMENTATION(scale)

void tst_QTransform::scale()
{
    QFETCH(QTransform, transform);
    QTransform x = transform;
    QBENCHMARK {
        x.scale(2, 2);
    }
}

SINGLE_DATA_IMPLEMENTATION(shear)

void tst_QTransform::shear()
{
    QFETCH(QTransform, transform);
    QTransform x = transform;
    QBENCHMARK {
        x.shear(2, 2);
    }
}

SINGLE_DATA_IMPLEMENTATION(rotate)

void tst_QTransform::rotate()
{
    QFETCH(QTransform, transform);
    QTransform x = transform;
    QBENCHMARK {
        x.rotate(45);
    }
}

SINGLE_DATA_IMPLEMENTATION(rotateXYZ)

void tst_QTransform::rotateXYZ()
{
    QFETCH(QTransform, transform);
    QTransform x = transform;
    QBENCHMARK {
        x.rotate(45, Qt::XAxis);
        x.rotate(45, Qt::YAxis);
        x.rotate(45, Qt::ZAxis);
    }
}

DOUBLE_DATA_IMPLEMENTATION(operatorAssign)

void tst_QTransform::operatorAssign()
{
    QFETCH(QTransform, x1);
    QFETCH(QTransform, x2);
    QTransform x = x1;
    QBENCHMARK {
        x = x2;
    }
}

DOUBLE_DATA_IMPLEMENTATION(operatorEqual)

void tst_QTransform::operatorEqual()
{
    QFETCH(QTransform, x1);
    QFETCH(QTransform, x2);
    QTransform x = x1;
    QBENCHMARK {
        x == x2;
    }
}

DOUBLE_DATA_IMPLEMENTATION(operatorNotEqual)

void tst_QTransform::operatorNotEqual()
{
    QFETCH(QTransform, x1);
    QFETCH(QTransform, x2);
    QTransform x = x1;
    QBENCHMARK {
        x != x2;
    }
}

DOUBLE_DATA_IMPLEMENTATION(operatorMultiply)

void tst_QTransform::operatorMultiply()
{
    QFETCH(QTransform, x1);
    QFETCH(QTransform, x2);
    QTransform x = x1;
    QBENCHMARK {
        x * x2;
    }
}

SINGLE_DATA_IMPLEMENTATION(operatorPlusEqualScalar)

void tst_QTransform::operatorPlusEqualScalar()
{
    QFETCH(QTransform, transform);
    QTransform x = transform;
    QBENCHMARK {
        x += 3.14;
    }
}

SINGLE_DATA_IMPLEMENTATION(operatorMinusEqualScalar)

void tst_QTransform::operatorMinusEqualScalar()
{
    QFETCH(QTransform, transform);
    QTransform x = transform;
    QBENCHMARK {
        x -= 3.14;
    }
}

DOUBLE_DATA_IMPLEMENTATION(operatorMultiplyEqual)

void tst_QTransform::operatorMultiplyEqual()
{
    QFETCH(QTransform, x1);
    QFETCH(QTransform, x2);
    QTransform x = x1;
    QBENCHMARK {
        x *= x2;
    }
}

SINGLE_DATA_IMPLEMENTATION(operatorMultiplyEqualScalar)

void tst_QTransform::operatorMultiplyEqualScalar()
{
    QFETCH(QTransform, transform);
    QTransform x = transform;
    QBENCHMARK {
        x * 3;
    }
}

SINGLE_DATA_IMPLEMENTATION(operatorDivideEqualScalar)

void tst_QTransform::operatorDivideEqualScalar()
{
    QFETCH(QTransform, transform);
    QTransform x = transform;
    QBENCHMARK {
        x /= 3;
    }
}

SINGLE_DATA_IMPLEMENTATION(mapQPoint)

void tst_QTransform::mapQPoint()
{
    QFETCH(QTransform, transform);
    QTransform x = transform;
    QBENCHMARK {
        x.map(QPoint(3, 3));
    }
}

SINGLE_DATA_IMPLEMENTATION(mapQPointF)

void tst_QTransform::mapQPointF()
{
    QFETCH(QTransform, transform);
    QTransform x = transform;
    QBENCHMARK {
        x.map(QPointF(3, 3));
    }
}

SINGLE_DATA_IMPLEMENTATION(mapRect)

void tst_QTransform::mapRect()
{
    QFETCH(QTransform, transform);
    QTransform x = transform;
    QBENCHMARK {
        x.mapRect(QRect(0, 0, 100, 100));
    }
}

SINGLE_DATA_IMPLEMENTATION(mapRectF)

void tst_QTransform::mapRectF()
{
    QFETCH(QTransform, transform);
    QTransform x = transform;
    QBENCHMARK {
        x.mapRect(QRectF(0, 0, 100, 100));
    }
}

SINGLE_DATA_IMPLEMENTATION(mapQPolygon)

void tst_QTransform::mapQPolygon()
{
    QFETCH(QTransform, transform);
    QTransform x = transform;
    QPolygon poly = QPolygon(QRect(0, 0, 100, 100));
    QBENCHMARK {
        x.map(poly);
    }
}

SINGLE_DATA_IMPLEMENTATION(mapQPolygonF)

void tst_QTransform::mapQPolygonF()
{
    QFETCH(QTransform, transform);
    QTransform x = transform;
    QPolygonF poly = QPolygonF(QRectF(0, 0, 100, 100));
    QBENCHMARK {
        x.map(poly);
    }
}

SINGLE_DATA_IMPLEMENTATION(mapQRegion)

void tst_QTransform::mapQRegion()
{
    QFETCH(QTransform, transform);
    QTransform x = transform;
    QRegion region;
    for (int i = 0; i < 10; ++i)
        region += QRect(i * 10, i * 10, 100, 100);
    QBENCHMARK {
        x.map(region);
    }
}

SINGLE_DATA_IMPLEMENTATION(mapToPolygon)

void tst_QTransform::mapToPolygon()
{
    QFETCH(QTransform, transform);
    QTransform x = transform;
    QBENCHMARK {
        x.mapToPolygon(QRect(0, 0, 100, 100));
    }
}


SINGLE_DATA_IMPLEMENTATION(mapQPainterPath)

void tst_QTransform::mapQPainterPath()
{
    QFETCH(QTransform, transform);
    QTransform x = transform;
    QPainterPath path;
    for (int i = 0; i < 10; ++i)
        path.addEllipse(i * 10, i * 10, 100, 100);
    QBENCHMARK {
        x.map(path);
    }
}

SINGLE_DATA_IMPLEMENTATION(isIdentity)

void tst_QTransform::isIdentity()
{
    QFETCH(QTransform, transform);
    QBENCHMARK {
        transform.isIdentity();
    }
}

SINGLE_DATA_IMPLEMENTATION(isAffine)

void tst_QTransform::isAffine()
{
    QFETCH(QTransform, transform);
    QBENCHMARK {
        transform.isAffine();
    }
}

SINGLE_DATA_IMPLEMENTATION(isInvertible)

void tst_QTransform::isInvertible()
{
    QFETCH(QTransform, transform);
    QBENCHMARK {
        transform.isInvertible();
    }
}

SINGLE_DATA_IMPLEMENTATION(isRotating)

void tst_QTransform::isRotating()
{
    QFETCH(QTransform, transform);
    QBENCHMARK {
        transform.isRotating();
    }
}

SINGLE_DATA_IMPLEMENTATION(isScaling)

void tst_QTransform::isScaling()
{
    QFETCH(QTransform, transform);
    QBENCHMARK {
        transform.isScaling();
    }
}

SINGLE_DATA_IMPLEMENTATION(isTranslating)

void tst_QTransform::isTranslating()
{
    QFETCH(QTransform, transform);
    QBENCHMARK {
        transform.isTranslating();
    }
}

SINGLE_DATA_IMPLEMENTATION(type)

void tst_QTransform::type()
{
    QFETCH(QTransform, transform);
    QBENCHMARK {
        transform.type();
    }
}

SINGLE_DATA_IMPLEMENTATION(determinant)

void tst_QTransform::determinant()
{
    QFETCH(QTransform, transform);
    QBENCHMARK {
        transform.determinant();
    }
}

SINGLE_DATA_IMPLEMENTATION(adjoint)

void tst_QTransform::adjoint()
{
    QFETCH(QTransform, transform);
    QBENCHMARK {
        transform.adjoint();
    }
}

SINGLE_DATA_IMPLEMENTATION(transposed)

void tst_QTransform::transposed()
{
    QFETCH(QTransform, transform);
    QBENCHMARK {
        transform.transposed();
    }
}

SINGLE_DATA_IMPLEMENTATION(inverted)

void tst_QTransform::inverted()
{
    QFETCH(QTransform, transform);
    QBENCHMARK {
        transform.inverted();
    }
}

QTEST_MAIN(tst_QTransform)
#include "tst_qtransform.moc"
