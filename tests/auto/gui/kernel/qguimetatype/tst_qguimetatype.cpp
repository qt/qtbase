/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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


#include <QtCore>
#include <QtGui>
#include <QtTest/QtTest>

#include "../../../qtest-config.h"

Q_DECLARE_METATYPE(QMetaType::Type)

class tst_QGuiMetaType: public QObject
{
    Q_OBJECT
private slots:
    void create_data();
    void create();
    void createCopy_data();
    void createCopy();
    void sizeOf_data();
    void sizeOf();
    void flags_data();
    void flags();
    void construct_data();
    void construct();
    void constructCopy_data();
    void constructCopy();
};

#define FOR_EACH_GUI_METATYPE_BASE(F) \
    F(QFont, QFont) \
    F(QPixmap, QPixmap) \
    F(QBrush, QBrush) \
    F(QColor, QColor) \
    F(QPalette, QPalette) \
    F(QImage, QImage) \
    F(QPolygon, QPolygon) \
    F(QRegion, QRegion) \
    F(QBitmap, QBitmap) \
    F(QKeySequence, QKeySequence) \
    F(QPen, QPen) \
    F(QTextLength, QTextLength) \
    F(QTextFormat, QTextFormat) \
    F(QMatrix, QMatrix) \
    F(QTransform, QTransform) \
    F(QMatrix4x4, QMatrix4x4) \
    F(QVector2D, QVector2D) \
    F(QVector3D, QVector3D) \
    F(QVector4D, QVector4D) \
    F(QQuaternion, QQuaternion)

#ifndef QTEST_NO_CURSOR
#   define FOR_EACH_GUI_METATYPE(F) \
        FOR_EACH_GUI_METATYPE_BASE(F) \
        F(QCursor, QCursor)
#else // !QTEST_NO_CURSOR
#   define FOR_EACH_GUI_METATYPE(F) \
        FOR_EACH_GUI_METATYPE_BASE(F)
#endif // !QTEST_NO_CURSOR


namespace {
    template <typename T>
    struct static_assert_trigger {
        Q_STATIC_ASSERT(( QMetaTypeId2<T>::IsBuiltIn ));
        enum { value = true };
    };
}

#define CHECK_BUILTIN(TYPE, ID) static_assert_trigger< TYPE >::value &&
Q_STATIC_ASSERT(( FOR_EACH_GUI_METATYPE(CHECK_BUILTIN) true ));
#undef CHECK_BUILTIN
Q_STATIC_ASSERT((!QMetaTypeId2<QList<QPen> >::IsBuiltIn));
Q_STATIC_ASSERT((!QMetaTypeId2<QMap<QString,QPen> >::IsBuiltIn));

template <int ID>
struct MetaEnumToType {};

#define DEFINE_META_ENUM_TO_TYPE(TYPE, ID) \
template<> \
struct MetaEnumToType<QMetaType::ID> { \
    typedef TYPE Type; \
};
FOR_EACH_GUI_METATYPE(DEFINE_META_ENUM_TO_TYPE)
#undef DEFINE_META_ENUM_TO_TYPE

// Not all types have operator==
template <int ID>
struct TypeComparator
{
    typedef typename MetaEnumToType<ID>::Type Type;
    static bool equal(const Type &v1, const Type &v2)
    { return v1 == v2; }
};

template<> struct TypeComparator<QMetaType::QPixmap>
{
    static bool equal(const QPixmap &v1, const QPixmap &v2)
    { return v1.size() == v2.size(); }
};

template<> struct TypeComparator<QMetaType::QBitmap>
{
    static bool equal(const QBitmap &v1, const QBitmap &v2)
    { return v1.size() == v2.size(); }
};

#ifndef QTEST_NO_CURSOR
template<> struct TypeComparator<QMetaType::QCursor>
{
    static bool equal(const QCursor &v1, const QCursor &v2)
    { return v1.shape() == v2.shape(); }
};
#endif

template <int ID>
struct DefaultValueFactory
{
    typedef typename MetaEnumToType<ID>::Type Type;
    static Type *create() { return new Type; }
};

template <int ID>
struct TestValueFactory {};

template<> struct TestValueFactory<QMetaType::QFont> {
    static QFont *create() { return new QFont("Arial"); }
};
template<> struct TestValueFactory<QMetaType::QPixmap> {
    static QPixmap *create() { return new QPixmap(16, 32); }
};
template<> struct TestValueFactory<QMetaType::QBrush> {
    static QBrush *create() { return new QBrush(Qt::SolidPattern); }
};
template<> struct TestValueFactory<QMetaType::QColor> {
    static QColor *create() { return new QColor(Qt::blue); }
};
template<> struct TestValueFactory<QMetaType::QPalette> {
    static QPalette *create() { return new QPalette(Qt::yellow, Qt::green); }
};
template<> struct TestValueFactory<QMetaType::QImage> {
    static QImage *create() { return new QImage(16, 32, QImage::Format_ARGB32_Premultiplied); }
};
template<> struct TestValueFactory<QMetaType::QPolygon> {
    static QPolygon *create() { return new QPolygon(QRect(10, 20, 30, 40), true); }
};
template<> struct TestValueFactory<QMetaType::QRegion> {
    static QRegion *create() { return new QRegion(QRect(10, 20, 30, 40), QRegion::Ellipse); }
};
template<> struct TestValueFactory<QMetaType::QBitmap> {
    static QBitmap *create() { return new QBitmap(16, 32); }
};
#ifndef QTEST_NO_CURSOR
template<> struct TestValueFactory<QMetaType::QCursor> {
    static QCursor *create() { return new QCursor(Qt::WaitCursor); }
};
#endif
template<> struct TestValueFactory<QMetaType::QKeySequence> {
    static QKeySequence *create() { return new QKeySequence(QKeySequence::Close); }
};
template<> struct TestValueFactory<QMetaType::QPen> {
    static QPen *create() { return new QPen(Qt::DashDotDotLine); }
};
template<> struct TestValueFactory<QMetaType::QTextLength> {
    static QTextLength *create() { return new QTextLength(QTextLength::PercentageLength, 50); }
};
template<> struct TestValueFactory<QMetaType::QTextFormat> {
    static QTextFormat *create() { return new QTextFormat(QTextFormat::TableFormat); }
};
template<> struct TestValueFactory<QMetaType::QMatrix> {
    static QMatrix *create() { return new QMatrix(10, 20, 30, 40, 50, 60); }
};
template<> struct TestValueFactory<QMetaType::QTransform> {
    static QTransform *create() { return new QTransform(10, 20, 30, 40, 50, 60); }
};
template<> struct TestValueFactory<QMetaType::QMatrix4x4> {
    static QMatrix4x4 *create() { return new QMatrix4x4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16); }
};
template<> struct TestValueFactory<QMetaType::QVector2D> {
    static QVector2D *create() { return new QVector2D(10, 20); }
};
template<> struct TestValueFactory<QMetaType::QVector3D> {
    static QVector3D *create() { return new QVector3D(10, 20, 30); }
};
template<> struct TestValueFactory<QMetaType::QVector4D> {
    static QVector4D *create() { return new QVector4D(10, 20, 30, 40); }
};
template<> struct TestValueFactory<QMetaType::QQuaternion> {
    static QQuaternion *create() { return new QQuaternion(10, 20, 30, 40); }
};

void tst_QGuiMetaType::create_data()
{
    QTest::addColumn<QMetaType::Type>("type");
#define ADD_METATYPE_TEST_ROW(TYPE, ID) \
    QTest::newRow(QMetaType::typeName(QMetaType::ID)) << QMetaType::ID;
FOR_EACH_GUI_METATYPE(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
}

template <int ID>
static void testCreateHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    void *actual = QMetaType::create(ID);
    Type *expected = DefaultValueFactory<ID>::create();
    QVERIFY(TypeComparator<ID>::equal(*static_cast<Type *>(actual), *expected));
    delete expected;
    QMetaType::destroy(ID, actual);
}

typedef void (*TypeTestFunction)();

void tst_QGuiMetaType::create()
{
    struct TypeTestFunctionGetter
    {
        static TypeTestFunction get(int type)
        {
            switch (type) {
#define RETURN_CREATE_FUNCTION(TYPE, ID) \
            case QMetaType::ID: \
            return testCreateHelper<QMetaType::ID>;
FOR_EACH_GUI_METATYPE(RETURN_CREATE_FUNCTION)
#undef RETURN_CREATE_FUNCTION
            }
            return 0;
        }
    };

    QFETCH(QMetaType::Type, type);
    TypeTestFunctionGetter::get(type)();
}

void tst_QGuiMetaType::createCopy_data()
{
    create_data();
}

template <int ID>
static void testCreateCopyHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    Type *expected = TestValueFactory<ID>::create();
    void *actual = QMetaType::create(ID, expected);
    QVERIFY(TypeComparator<ID>::equal(*static_cast<Type*>(actual), *expected));
    QMetaType::destroy(ID, actual);
    delete expected;
}

void tst_QGuiMetaType::createCopy()
{
    struct TypeTestFunctionGetter
    {
        static TypeTestFunction get(int type)
        {
            switch (type) {
#define RETURN_CREATE_COPY_FUNCTION(TYPE, ID) \
            case QMetaType::ID: \
            return testCreateCopyHelper<QMetaType::ID>;
FOR_EACH_GUI_METATYPE(RETURN_CREATE_COPY_FUNCTION)
#undef RETURN_CREATE_COPY_FUNCTION
            }
            return 0;
        }
    };

    QFETCH(QMetaType::Type, type);
    TypeTestFunctionGetter::get(type)();
}

void tst_QGuiMetaType::sizeOf_data()
{
    QTest::addColumn<QMetaType::Type>("type");
    QTest::addColumn<int>("size");
#define ADD_METATYPE_TEST_ROW(TYPE, ID) \
    QTest::newRow(QMetaType::typeName(QMetaType::ID)) << QMetaType::ID << int(sizeof(TYPE));
FOR_EACH_GUI_METATYPE(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
}

void tst_QGuiMetaType::sizeOf()
{
    QFETCH(QMetaType::Type, type);
    QFETCH(int, size);
    QCOMPARE(QMetaType::sizeOf(type), size);
}

#ifndef Q_ALIGNOF
template<uint N>
struct RoundToNextHighestPowerOfTwo
{
private:
    enum { V1 = N-1 };
    enum { V2 = V1 | (V1 >> 1) };
    enum { V3 = V2 | (V2 >> 2) };
    enum { V4 = V3 | (V3 >> 4) };
    enum { V5 = V4 | (V4 >> 8) };
    enum { V6 = V5 | (V5 >> 16) };
public:
    enum { Value = V6 + 1 };
};
#endif

template<class T>
struct TypeAlignment
{
#ifdef Q_ALIGNOF
    enum { Value = Q_ALIGNOF(T) };
#else
    enum { Value = RoundToNextHighestPowerOfTwo<sizeof(T)>::Value };
#endif
};

void tst_QGuiMetaType::flags_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<bool>("isRelocatable");
    QTest::addColumn<bool>("isComplex");

#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(#RealType) << MetaTypeId << bool(QTypeInfoQuery<RealType>::isRelocatable) << bool(QTypeInfoQuery<RealType>::isComplex);
QT_FOR_EACH_STATIC_GUI_CLASS(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
}

void tst_QGuiMetaType::flags()
{
    QFETCH(int, type);
    QFETCH(bool, isRelocatable);
    QFETCH(bool, isComplex);

    QCOMPARE(bool(QMetaType::typeFlags(type) & QMetaType::NeedsConstruction), isComplex);
    QCOMPARE(bool(QMetaType::typeFlags(type) & QMetaType::NeedsDestruction), isComplex);
    QCOMPARE(bool(QMetaType::typeFlags(type) & QMetaType::MovableType), isRelocatable);
}


void tst_QGuiMetaType::construct_data()
{
    create_data();
}

template <int ID>
static void testConstructHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    int size = QMetaType::sizeOf(ID);
    void *storage = qMallocAligned(size, TypeAlignment<Type>::Value);
    void *actual = QMetaType::construct(ID, storage, /*copy=*/0);
    QCOMPARE(actual, storage);
    Type *expected = DefaultValueFactory<ID>::create();
    QVERIFY2(TypeComparator<ID>::equal(*static_cast<Type *>(actual), *expected), QMetaType::typeName(ID));
    delete expected;
    QMetaType::destruct(ID, actual);
    qFreeAligned(storage);
}

void tst_QGuiMetaType::construct()
{
    struct TypeTestFunctionGetter
    {
        static TypeTestFunction get(int type)
        {
            switch (type) {
#define RETURN_CONSTRUCT_FUNCTION(TYPE, ID) \
            case QMetaType::ID: \
            return testConstructHelper<QMetaType::ID>;
FOR_EACH_GUI_METATYPE(RETURN_CONSTRUCT_FUNCTION)
#undef RETURN_CONSTRUCT_FUNCTION
            }
            return 0;
        }
    };

    QFETCH(QMetaType::Type, type);
    TypeTestFunctionGetter::get(type)();
}

void tst_QGuiMetaType::constructCopy_data()
{
    create_data();
}

template <int ID>
static void testConstructCopyHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    Type *expected = TestValueFactory<ID>::create();
    int size = QMetaType::sizeOf(ID);
    void *storage = qMallocAligned(size, TypeAlignment<Type>::Value);
    void *actual = QMetaType::construct(ID, storage, expected);
    QCOMPARE(actual, storage);
    QVERIFY2(TypeComparator<ID>::equal(*static_cast<Type*>(actual), *expected), QMetaType::typeName(ID));
    QMetaType::destruct(ID, actual);
    qFreeAligned(storage);
    delete expected;
}

void tst_QGuiMetaType::constructCopy()
{
    struct TypeTestFunctionGetter
    {
        static TypeTestFunction get(int type)
        {
            switch (type) {
#define RETURN_CONSTRUCT_COPY_FUNCTION(TYPE, ID) \
            case QMetaType::ID: \
            return testConstructCopyHelper<QMetaType::ID>;
FOR_EACH_GUI_METATYPE(RETURN_CONSTRUCT_COPY_FUNCTION)
#undef RETURN_CONSTRUCT_COPY_FUNCTION
            }
            return 0;
        }
    };

    QFETCH(QMetaType::Type, type);
    TypeTestFunctionGetter::get(type)();
}

QTEST_MAIN(tst_QGuiMetaType)
#include "tst_qguimetatype.moc"
