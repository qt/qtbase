/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore>
#include <QtGui/QPixmap>
#include <qtest.h>

#define ITERATION_COUNT 1e5

class tst_qvariant : public QObject
{
    Q_OBJECT
private slots:
    void testBound();

    void doubleVariantCreation();
    void floatVariantCreation();
    void rectVariantCreation();
    void stringVariantCreation();
    void pixmapVariantCreation();
    void stringListVariantCreation();
    void bigClassVariantCreation();
    void smallClassVariantCreation();

    void doubleVariantSetValue();
    void floatVariantSetValue();
    void rectVariantSetValue();
    void stringVariantSetValue();
    void stringListVariantSetValue();
    void bigClassVariantSetValue();
    void smallClassVariantSetValue();

    void doubleVariantAssignment();
    void floatVariantAssignment();
    void rectVariantAssignment();
    void stringVariantAssignment();
    void stringListVariantAssignment();

    void doubleVariantValue();
    void floatVariantValue();
    void rectVariantValue();
    void stringVariantValue();

    void createCoreType_data();
    void createCoreType();
    void createCoreTypeCopy_data();
    void createCoreTypeCopy();
};

struct BigClass
{
    double n,i,e,r,o,b;
};
Q_STATIC_ASSERT(sizeof(BigClass) > sizeof(QVariant::Private::Data));
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(BigClass, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
Q_DECLARE_METATYPE(BigClass);

struct SmallClass
{
    char s;
};
Q_STATIC_ASSERT(sizeof(SmallClass) <= sizeof(QVariant::Private::Data));
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(SmallClass, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
Q_DECLARE_METATYPE(SmallClass);

void tst_qvariant::testBound()
{
    qreal d = qreal(.5);
    QBENCHMARK {
        for(int i = 0; i < ITERATION_COUNT; ++i) {
            d = qBound<qreal>(0, d, 1);
        }
    }
}

template <typename T>
static void variantCreation(T val)
{
    QBENCHMARK {
        for(int i = 0; i < ITERATION_COUNT; ++i) {
            QVariant v(val);
        }
    }
}

template <>
void variantCreation<BigClass>(BigClass val)
{
    QBENCHMARK {
        for (int i = 0; i < ITERATION_COUNT; ++i) {
            QVariant::fromValue(val);
        }
    }
}

template <>
void variantCreation<SmallClass>(SmallClass val)
{
    QBENCHMARK {
        for (int i = 0; i < ITERATION_COUNT; ++i) {
            QVariant::fromValue(val);
        }
    }
}


void tst_qvariant::doubleVariantCreation()
{
    variantCreation<double>(0.0);
}

void tst_qvariant::floatVariantCreation()
{
    variantCreation<float>(0.0f);
}

void tst_qvariant::rectVariantCreation()
{
    variantCreation<QRect>(QRect(1, 2, 3, 4));
}

void tst_qvariant::stringVariantCreation()
{
    variantCreation<QString>(QString());
}

void tst_qvariant::pixmapVariantCreation()
{
    variantCreation<QPixmap>(QPixmap());
}

void tst_qvariant::stringListVariantCreation()
{
    variantCreation<QStringList>(QStringList());
}

void tst_qvariant::bigClassVariantCreation()
{
    variantCreation<BigClass>(BigClass());
}

void tst_qvariant::smallClassVariantCreation()
{
    variantCreation<SmallClass>(SmallClass());
}

template <typename T>
static void variantSetValue(T d)
{
    QVariant v;
    QBENCHMARK {
        for(int i = 0; i < ITERATION_COUNT; ++i) {
            v.setValue(d);
        }
    }
}

void tst_qvariant::doubleVariantSetValue()
{
    variantSetValue<double>(0.0);
}

void tst_qvariant::floatVariantSetValue()
{
    variantSetValue<float>(0.0f);
}

void tst_qvariant::rectVariantSetValue()
{
    variantSetValue<QRect>(QRect());
}

void tst_qvariant::stringVariantSetValue()
{
    variantSetValue<QString>(QString());
}

void tst_qvariant::stringListVariantSetValue()
{
    variantSetValue<QStringList>(QStringList());
}

void tst_qvariant::bigClassVariantSetValue()
{
    variantSetValue<BigClass>(BigClass());
}

void tst_qvariant::smallClassVariantSetValue()
{
    variantSetValue<SmallClass>(SmallClass());
}

template <typename T>
static void variantAssignment(T d)
{
    QVariant v;
    QBENCHMARK {
        for(int i = 0; i < ITERATION_COUNT; ++i) {
            v = d;
        }
    }
}

void tst_qvariant::doubleVariantAssignment()
{
    variantAssignment<double>(0.0);
}

void tst_qvariant::floatVariantAssignment()
{
    variantAssignment<float>(0.0f);
}

void tst_qvariant::rectVariantAssignment()
{
    variantAssignment<QRect>(QRect());
}

void tst_qvariant::stringVariantAssignment()
{
    variantAssignment<QString>(QString());
}

void tst_qvariant::stringListVariantAssignment()
{
    variantAssignment<QStringList>(QStringList());
}

void tst_qvariant::doubleVariantValue()
{
    QVariant v(0.0);
    QBENCHMARK {
        for(int i = 0; i < ITERATION_COUNT; ++i) {
            v.toDouble();
        }
    }
}

void tst_qvariant::floatVariantValue()
{
    QVariant v(0.0f);
    QBENCHMARK {
        for(int i = 0; i < ITERATION_COUNT; ++i) {
            v.toFloat();
        }
    }
}

void tst_qvariant::rectVariantValue()
{
    QVariant v(QRect(1,2,3,4));
    QBENCHMARK {
        for(int i = 0; i < ITERATION_COUNT; ++i) {
            v.toRect();
        }
    }
}

void tst_qvariant::stringVariantValue()
{
    QVariant v = QString();
    QBENCHMARK {
        for(int i = 0; i < ITERATION_COUNT; ++i) {
            v.toString();
        }
    }
}

void tst_qvariant::createCoreType_data()
{
    QTest::addColumn<int>("typeId");
    for (int i = QMetaType::FirstCoreType; i <= QMetaType::LastCoreType; ++i)
        QTest::newRow(QMetaType::typeName(i)) << i;
}

// Tests how fast a Qt core type can be default-constructed by a
// QVariant. The purpose of this benchmark is to measure the overhead
// of creating (and destroying) a QVariant compared to creating the
// type directly.
void tst_qvariant::createCoreType()
{
    QFETCH(int, typeId);
    QBENCHMARK {
        for (int i = 0; i < ITERATION_COUNT; ++i)
            QVariant(typeId, (void *)0);
    }
}

void tst_qvariant::createCoreTypeCopy_data()
{
    createCoreType_data();
}

// Tests how fast a Qt core type can be copy-constructed by a
// QVariant. The purpose of this benchmark is to measure the overhead
// of creating (and destroying) a QVariant compared to creating the
// type directly.
void tst_qvariant::createCoreTypeCopy()
{
    QFETCH(int, typeId);
    QVariant other(typeId, (void *)0);
    const void *copy = other.constData();
    QBENCHMARK {
        for (int i = 0; i < ITERATION_COUNT; ++i)
            QVariant(typeId, copy);
    }
}

QTEST_MAIN(tst_qvariant)

#include "tst_qvariant.moc"
