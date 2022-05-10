// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore>
#ifdef QT_GUI_LIB
#  include <QtGui/QPixmap>
#endif
#include <qtest.h>

#define ITERATION_COUNT 1e5

class tst_QVariant : public QObject
{
    Q_OBJECT

public:
    enum ABenchmarkEnum {
        FirstEnumValue,
        SecondEnumValue,
        ThirdEnumValue
    };
    Q_ENUM(ABenchmarkEnum)

private slots:
    void testBound();

    void doubleVariantCreation();
    void floatVariantCreation();
    void rectVariantCreation();
    void stringVariantCreation();
#ifdef QT_GUI_LIB
    void pixmapVariantCreation();
#endif
    void stringListVariantCreation();
    void bigClassVariantCreation();
    void smallClassVariantCreation();
    void enumVariantCreation();

    void doubleVariantSetValue();
    void floatVariantSetValue();
    void rectVariantSetValue();
    void stringVariantSetValue();
    void stringListVariantSetValue();
    void bigClassVariantSetValue();
    void smallClassVariantSetValue();
    void enumVariantSetValue();

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
static_assert(sizeof(BigClass) > sizeof(QVariant::Private::MaxInternalSize));
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(BigClass, Q_RELOCATABLE_TYPE);
QT_END_NAMESPACE
Q_DECLARE_METATYPE(BigClass);

struct SmallClass
{
    char s;
};
static_assert(sizeof(SmallClass) <= sizeof(QVariant::Private::MaxInternalSize));
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(SmallClass, Q_RELOCATABLE_TYPE);
QT_END_NAMESPACE
Q_DECLARE_METATYPE(SmallClass);

void tst_QVariant::testBound()
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

template <>
void variantCreation<tst_QVariant::ABenchmarkEnum>(tst_QVariant::ABenchmarkEnum val)
{
    QBENCHMARK {
        for (int i = 0; i < ITERATION_COUNT; ++i) {
            QVariant::fromValue(val);
        }
    }
}


void tst_QVariant::doubleVariantCreation()
{
    variantCreation<double>(0.0);
}

void tst_QVariant::floatVariantCreation()
{
    variantCreation<float>(0.0f);
}

void tst_QVariant::rectVariantCreation()
{
    variantCreation<QRect>(QRect(1, 2, 3, 4));
}

void tst_QVariant::stringVariantCreation()
{
    variantCreation<QString>(QString());
}

#ifdef QT_GUI_LIB
void tst_QVariant::pixmapVariantCreation()
{
    variantCreation<QPixmap>(QPixmap());
}
#endif

void tst_QVariant::stringListVariantCreation()
{
    variantCreation<QStringList>(QStringList());
}

void tst_QVariant::bigClassVariantCreation()
{
    variantCreation<BigClass>(BigClass());
}

void tst_QVariant::smallClassVariantCreation()
{
    variantCreation<SmallClass>(SmallClass());
}

void tst_QVariant::enumVariantCreation()
{
    variantCreation<ABenchmarkEnum>(FirstEnumValue);
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

void tst_QVariant::doubleVariantSetValue()
{
    variantSetValue<double>(0.0);
}

void tst_QVariant::floatVariantSetValue()
{
    variantSetValue<float>(0.0f);
}

void tst_QVariant::rectVariantSetValue()
{
    variantSetValue<QRect>(QRect());
}

void tst_QVariant::stringVariantSetValue()
{
    variantSetValue<QString>(QString());
}

void tst_QVariant::stringListVariantSetValue()
{
    variantSetValue<QStringList>(QStringList());
}

void tst_QVariant::bigClassVariantSetValue()
{
    variantSetValue<BigClass>(BigClass());
}

void tst_QVariant::smallClassVariantSetValue()
{
    variantSetValue<SmallClass>(SmallClass());
}

void tst_QVariant::enumVariantSetValue()
{
    variantSetValue<ABenchmarkEnum>(FirstEnumValue);
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

void tst_QVariant::doubleVariantAssignment()
{
    variantAssignment<double>(0.0);
}

void tst_QVariant::floatVariantAssignment()
{
    variantAssignment<float>(0.0f);
}

void tst_QVariant::rectVariantAssignment()
{
    variantAssignment<QRect>(QRect());
}

void tst_QVariant::stringVariantAssignment()
{
    variantAssignment<QString>(QString());
}

void tst_QVariant::stringListVariantAssignment()
{
    variantAssignment<QStringList>(QStringList());
}

void tst_QVariant::doubleVariantValue()
{
    QVariant v(0.0);
    QBENCHMARK {
        for(int i = 0; i < ITERATION_COUNT; ++i) {
            v.toDouble();
        }
    }
}

void tst_QVariant::floatVariantValue()
{
    QVariant v(0.0f);
    QBENCHMARK {
        for(int i = 0; i < ITERATION_COUNT; ++i) {
            v.toFloat();
        }
    }
}

void tst_QVariant::rectVariantValue()
{
    QVariant v(QRect(1,2,3,4));
    QBENCHMARK {
        for(int i = 0; i < ITERATION_COUNT; ++i) {
            v.toRect();
        }
    }
}

void tst_QVariant::stringVariantValue()
{
    QVariant v = QString();
    QBENCHMARK {
        for(int i = 0; i < ITERATION_COUNT; ++i) {
            v.toString();
        }
    }
}

void tst_QVariant::createCoreType_data()
{
    QTest::addColumn<int>("typeId");
    for (int i = QMetaType::FirstCoreType; i <= QMetaType::LastCoreType; ++i) {
        if (QMetaType metaType(i); metaType.isValid()) // QMetaType(27) does not exist
            QTest::newRow(metaType.name()) << i;
    }
}

// Tests how fast a Qt core type can be default-constructed by a
// QVariant. The purpose of this benchmark is to measure the overhead
// of creating (and destroying) a QVariant compared to creating the
// type directly.
void tst_QVariant::createCoreType()
{
    QFETCH(int, typeId);
    QBENCHMARK {
        for (int i = 0; i < ITERATION_COUNT; ++i)
            QVariant(QMetaType(typeId));
    }
}

void tst_QVariant::createCoreTypeCopy_data()
{
    createCoreType_data();
}

// Tests how fast a Qt core type can be copy-constructed by a
// QVariant. The purpose of this benchmark is to measure the overhead
// of creating (and destroying) a QVariant compared to creating the
// type directly.
void tst_QVariant::createCoreTypeCopy()
{
    QFETCH(int, typeId);
    QMetaType metaType(typeId);
    QVariant other(metaType);
    const void *copy = other.constData();
    QBENCHMARK {
        for (int i = 0; i < ITERATION_COUNT; ++i)
            QVariant(metaType, copy);
    }
}

QTEST_MAIN(tst_QVariant)

#include "tst_bench_qvariant.moc"
