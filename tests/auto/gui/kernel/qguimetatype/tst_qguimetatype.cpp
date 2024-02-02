// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtCore>
#include <QtGui>
#include <QTest>

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
    void flags2_data();
    void flags2();
    void construct_data();
    void construct();
    void constructCopy_data();
    void constructCopy();
    void saveAndLoadBuiltin_data();
    void saveAndLoadBuiltin();
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
    F(QTransform, QTransform) \
    F(QMatrix4x4, QMatrix4x4) \
    F(QVector2D, QVector2D) \
    F(QVector3D, QVector3D) \
    F(QVector4D, QVector4D) \
    F(QQuaternion, QQuaternion)

#ifndef QT_NO_CURSOR
#   define FOR_EACH_GUI_METATYPE(F) \
        FOR_EACH_GUI_METATYPE_BASE(F) \
        F(QCursor, QCursor)
#else // !QT_NO_CURSOR
#   define FOR_EACH_GUI_METATYPE(F) \
        FOR_EACH_GUI_METATYPE_BASE(F)
#endif // !QT_NO_CURSOR


namespace {
    template <typename T>
    struct static_assert_trigger {
        static_assert(( QMetaTypeId2<T>::IsBuiltIn ));
        enum { value = true };
    };
}

#define CHECK_BUILTIN(TYPE, ID) static_assert_trigger< TYPE >::value &&
static_assert(( FOR_EACH_GUI_METATYPE(CHECK_BUILTIN) true ));
#undef CHECK_BUILTIN
static_assert((!QMetaTypeId2<QList<QPen> >::IsBuiltIn));
static_assert((!QMetaTypeId2<QMap<QString,QPen> >::IsBuiltIn));

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

#ifndef QT_NO_CURSOR
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
#ifndef QT_NO_CURSOR
template<> struct TestValueFactory<QMetaType::QCursor> {
    static QCursor *create() { return new QCursor(Qt::WaitCursor); }
};
#endif

#if QT_CONFIG(shortcut)
template<> struct TestValueFactory<QMetaType::QKeySequence> {
    static QKeySequence *create() { return new QKeySequence(QKeySequence::Close); }
};
#endif

template<> struct TestValueFactory<QMetaType::QPen> {
    static QPen *create() { return new QPen(Qt::DashDotDotLine); }
};
template<> struct TestValueFactory<QMetaType::QTextLength> {
    static QTextLength *create() { return new QTextLength(QTextLength::PercentageLength, 50); }
};
template<> struct TestValueFactory<QMetaType::QTextFormat> {
    static QTextFormat *create() { return new QTextFormat(QTextFormat::FrameFormat); }
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
    QTest::newRow(QMetaType(QMetaType::ID).name()) << QMetaType::ID;
FOR_EACH_GUI_METATYPE(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
}

template <int ID>
static void testCreateHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    void *actual = QMetaType(ID).create();
    Type *expected = DefaultValueFactory<ID>::create();
    QVERIFY(TypeComparator<ID>::equal(*static_cast<Type *>(actual), *expected));
    delete expected;
    QMetaType(ID).destroy(actual);
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
    void *actual = QMetaType(ID).create(expected);
    QVERIFY(TypeComparator<ID>::equal(*static_cast<Type*>(actual), *expected));
    QMetaType(ID).destroy(actual);
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
    QTest::newRow(QMetaType(QMetaType::ID).name()) << QMetaType::ID << int(sizeof(TYPE));
FOR_EACH_GUI_METATYPE(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
}

void tst_QGuiMetaType::sizeOf()
{
    QFETCH(QMetaType::Type, type);
    QFETCH(int, size);
    QCOMPARE(QMetaType(type).sizeOf(), size);
}

template<class T>
struct TypeAlignment
{
    enum { Value = alignof(T) };
};

template <typename T> void addFlagsRow(const char *name, int id = qMetaTypeId<T>())
{
    QTest::newRow(name)
            << id
            << bool(QTypeInfo<T>::isRelocatable)
            << bool(!std::is_trivially_default_constructible_v<T>)
            << bool(!std::is_trivially_copy_constructible_v<T>)
            << bool(!std::is_trivially_destructible_v<T>);
}

// tst_QGuiMetaType::flags is nearly identical to tst_QMetaType::flags
void tst_QGuiMetaType::flags_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<bool>("isRelocatable");
    QTest::addColumn<bool>("needsConstruction");
    QTest::addColumn<bool>("needsCopyConstruction");
    QTest::addColumn<bool>("needsDestruction");

#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    addFlagsRow<RealType>(#RealType, MetaTypeId);
QT_FOR_EACH_STATIC_GUI_CLASS(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
}

void tst_QGuiMetaType::flags()
{
    QFETCH(int, type);
    QFETCH(bool, isRelocatable);
    QFETCH(bool, needsConstruction);
    QFETCH(bool, needsCopyConstruction);
    QFETCH(bool, needsDestruction);

    QCOMPARE(bool(QMetaType(type).flags() & QMetaType::NeedsConstruction), needsConstruction);
    QCOMPARE(bool(QMetaType(type).flags() & QMetaType::NeedsCopyConstruction), needsCopyConstruction);
    QCOMPARE(bool(QMetaType(type).flags() & QMetaType::NeedsDestruction), needsDestruction);
    QCOMPARE(bool(QMetaType(type).flags() & QMetaType::RelocatableType), isRelocatable);
}

template <typename T> static void addFlags2Row(QMetaType metaType = QMetaType::fromType<T>())
{
    QTest::newRow(metaType.name() ? metaType.name() : "UnknownType")
            << metaType
            << std::is_default_constructible_v<T>
            << std::is_copy_constructible_v<T>
            << std::is_move_constructible_v<T>
            << std::is_destructible_v<T>
            << (QTypeTraits::has_operator_equal<T>::value || QTypeTraits::has_operator_less_than<T>::value)
            << QTypeTraits::has_operator_less_than<T>::value;
};

// tst_QGuiMetaType::flags2 is nearly identical to tst_QMetaType::flags2
void tst_QGuiMetaType::flags2_data()
{
    QTest::addColumn<QMetaType>("type");
    QTest::addColumn<bool>("isDefaultConstructible");
    QTest::addColumn<bool>("isCopyConstructible");
    QTest::addColumn<bool>("isMoveConstructible");
    QTest::addColumn<bool>("isDestructible");
    QTest::addColumn<bool>("isEqualityComparable");
    QTest::addColumn<bool>("isOrdered");

#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    addFlags2Row<RealType>();
QT_FOR_EACH_STATIC_GUI_CLASS(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
}

void tst_QGuiMetaType::flags2()
{
    QFETCH(QMetaType, type);
    QFETCH(bool, isDefaultConstructible);
    QFETCH(bool, isCopyConstructible);
    QFETCH(bool, isMoveConstructible);
    QFETCH(bool, isDestructible);
    QFETCH(bool, isEqualityComparable);
    QFETCH(bool, isOrdered);

    QCOMPARE(type.isDefaultConstructible(), isDefaultConstructible);
    QCOMPARE(type.isCopyConstructible(), isCopyConstructible);
    QCOMPARE(type.isMoveConstructible(), isMoveConstructible);
    QCOMPARE(type.isDestructible(), isDestructible);
    QCOMPARE(type.isEqualityComparable(), isEqualityComparable);
    QCOMPARE(type.isOrdered(), isOrdered);
}

void tst_QGuiMetaType::construct_data()
{
    create_data();
}

template <int ID>
static void testConstructHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    int size = QMetaType(ID).sizeOf();
    void *storage = qMallocAligned(size, TypeAlignment<Type>::Value);
    void *actual = QMetaType(ID).construct(storage, /*copy=*/0);
    QCOMPARE(actual, storage);
    Type *expected = DefaultValueFactory<ID>::create();
    QVERIFY2(TypeComparator<ID>::equal(*static_cast<Type *>(actual), *expected), QMetaType(ID).name());
    delete expected;
    QMetaType(ID).destruct(actual);
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
    int size = QMetaType(ID).sizeOf();
    void *storage = qMallocAligned(size, TypeAlignment<Type>::Value);
    void *actual = QMetaType(ID).construct(storage, expected);
    QCOMPARE(actual, storage);
    QVERIFY2(TypeComparator<ID>::equal(*static_cast<Type*>(actual), *expected), QMetaType(ID).name());
    QMetaType(ID).destruct(actual);
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

template <typename T>
struct StreamingTraits
{
    // Streamable by default, as currently all gui built-in types are streamable
    enum { isStreamable = 1 };
};

void tst_QGuiMetaType::saveAndLoadBuiltin_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<bool>("isStreamable");

#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(#RealType) << MetaTypeId << bool(StreamingTraits<RealType>::isStreamable);
    QT_FOR_EACH_STATIC_GUI_CLASS(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
}

void tst_QGuiMetaType::saveAndLoadBuiltin()
{
    QFETCH(int, type);
    QFETCH(bool, isStreamable);

    void *value = QMetaType(type).create();

    QByteArray ba;
    QDataStream stream(&ba, QIODevice::ReadWrite);
    QCOMPARE(QMetaType(type).save(stream, value), isStreamable);
    QCOMPARE(stream.status(), QDataStream::Ok);

    if (isStreamable)
        QVERIFY(QMetaType(type).load(stream, value));

    stream.device()->seek(0);
    stream.resetStatus();
    QCOMPARE(QMetaType(type).load(stream, value), isStreamable);
    QCOMPARE(stream.status(), QDataStream::Ok);

    if (isStreamable)
        QVERIFY(QMetaType(type).load(stream, value));

    QMetaType(type).destroy(value);
}

QTEST_MAIN(tst_QGuiMetaType)
#include "tst_qguimetatype.moc"
