// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QtCore/qjnitypes.h>
#include <QtCore/qjniarray.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class tst_QJniArray : public QObject
{
    Q_OBJECT

public:
    tst_QJniArray() = default;

private slots:
    void construct();
    void copyAndMove();
    void invalidArraysAreEmpty();
    void size();
    void operators();
    void ordering();
    void toContainer();
    void pointerToValue();
    void mutate();
};

using namespace QtJniTypes;

// fake type so that we can compile-time test and assert correct type mappings
Q_DECLARE_JNI_CLASS(List, "qt/test/List");

// verify that we get the return type we expect for the specified return type
#define VERIFY_RETURN_FOR_TYPE(In, Out) \
static_assert(std::is_same_v<decltype(List::callStaticMethod<In>("toByteArray")), Out>)

VERIFY_RETURN_FOR_TYPE(jobjectArray, QJniObject);
VERIFY_RETURN_FOR_TYPE(jobject[], QJniArray<jobject>);
//VERIFY_RETURN_FOR_TYPE(QList<QJniObject>, QList<QJniObject>);
VERIFY_RETURN_FOR_TYPE(QList<jobject>, QList<jobject>);

VERIFY_RETURN_FOR_TYPE(jbyteArray, QJniObject);
VERIFY_RETURN_FOR_TYPE(jbyte[], QJniArray<jbyte>);
VERIFY_RETURN_FOR_TYPE(QByteArray, QByteArray);

VERIFY_RETURN_FOR_TYPE(jbooleanArray, QJniObject);
VERIFY_RETURN_FOR_TYPE(jboolean[], QJniArray<jboolean>);
VERIFY_RETURN_FOR_TYPE(QList<jboolean>, QList<jboolean>);
VERIFY_RETURN_FOR_TYPE(QList<bool>, QList<jboolean>);

VERIFY_RETURN_FOR_TYPE(jcharArray, QJniObject);
VERIFY_RETURN_FOR_TYPE(jchar[], QJniArray<jchar>);
VERIFY_RETURN_FOR_TYPE(QList<jchar>, QList<jchar>);

VERIFY_RETURN_FOR_TYPE(jshortArray, QJniObject);
VERIFY_RETURN_FOR_TYPE(jshort[], QJniArray<jshort>);
VERIFY_RETURN_FOR_TYPE(QList<jshort>, QList<jshort>);
VERIFY_RETURN_FOR_TYPE(QList<short>, QList<short>);

VERIFY_RETURN_FOR_TYPE(jintArray, QJniObject);
VERIFY_RETURN_FOR_TYPE(jint[], QJniArray<jint>);
VERIFY_RETURN_FOR_TYPE(QList<jint>, QList<jint>);
VERIFY_RETURN_FOR_TYPE(QList<int>, QList<int>);

VERIFY_RETURN_FOR_TYPE(jlongArray, QJniObject);
VERIFY_RETURN_FOR_TYPE(jlong[], QJniArray<jlong>);
VERIFY_RETURN_FOR_TYPE(QList<jlong>, QList<jlong>);
// VERIFY_RETURN_FOR_TYPE(QList<long>, QList<long>); // assumes long is 64bit

VERIFY_RETURN_FOR_TYPE(jfloatArray, QJniObject);
VERIFY_RETURN_FOR_TYPE(jfloat[], QJniArray<jfloat>);
VERIFY_RETURN_FOR_TYPE(QList<jfloat>, QList<jfloat>);
VERIFY_RETURN_FOR_TYPE(QList<float>, QList<float>);

VERIFY_RETURN_FOR_TYPE(jdoubleArray, QJniObject);
VERIFY_RETURN_FOR_TYPE(jdouble[], QJniArray<jdouble>);
VERIFY_RETURN_FOR_TYPE(QList<jdouble>, QList<jdouble>);
VERIFY_RETURN_FOR_TYPE(QList<double>, QList<double>);

VERIFY_RETURN_FOR_TYPE(QString, QString);
VERIFY_RETURN_FOR_TYPE(QJniArray<QString>, QJniArray<QString>);

VERIFY_RETURN_FOR_TYPE(List, List);
VERIFY_RETURN_FOR_TYPE(List[], QJniArray<List>);
VERIFY_RETURN_FOR_TYPE(QJniArray<List>, QJniArray<List>);

#define VERIFY_CONTAINER_FOR_TYPE(In, Out) \
    static_assert(std::is_same_v<decltype(std::declval<In>().toContainer()), Out>);

#define VERIFY_ARRAY_FOR_CONTAINER(In, Out) \
    static_assert(std::is_same_v<decltype(QJniArrayBase::fromContainer(std::declval<In>())), Out>);

// primitive types, both JNI and C++ equivalent types
VERIFY_CONTAINER_FOR_TYPE(QJniArray<jchar>, QList<jchar>);
VERIFY_ARRAY_FOR_CONTAINER(QList<jchar>, QJniArray<jchar>);
VERIFY_CONTAINER_FOR_TYPE(QJniArray<jint>, QList<jint>);
VERIFY_ARRAY_FOR_CONTAINER(QList<jint>, QJniArray<jint>);
VERIFY_CONTAINER_FOR_TYPE(QJniArray<int>, QList<int>);
VERIFY_ARRAY_FOR_CONTAINER(QList<int>, QJniArray<int>);
VERIFY_CONTAINER_FOR_TYPE(QJniArray<jfloat>, QList<jfloat>);
VERIFY_ARRAY_FOR_CONTAINER(QList<jfloat>, QJniArray<jfloat>);
VERIFY_CONTAINER_FOR_TYPE(QJniArray<float>, QList<float>);
VERIFY_ARRAY_FOR_CONTAINER(QList<float>, QJniArray<float>);
VERIFY_CONTAINER_FOR_TYPE(QJniArray<jdouble>, QList<jdouble>);
VERIFY_ARRAY_FOR_CONTAINER(QList<jdouble>, QJniArray<jdouble>);
VERIFY_CONTAINER_FOR_TYPE(QJniArray<double>, QList<double>);
VERIFY_ARRAY_FOR_CONTAINER(QList<double>, QJniArray<double>);

// jobject and derivatives
VERIFY_CONTAINER_FOR_TYPE(QJniArray<jobject>, QList<jobject>);
VERIFY_ARRAY_FOR_CONTAINER(QList<jobject>, QJniArray<jobject>);
VERIFY_CONTAINER_FOR_TYPE(QJniArray<jclass>, QList<jclass>);
VERIFY_ARRAY_FOR_CONTAINER(QList<jclass>, QJniArray<jclass>);
VERIFY_CONTAINER_FOR_TYPE(QJniArray<jthrowable>, QList<jthrowable>);
VERIFY_ARRAY_FOR_CONTAINER(QList<jthrowable>, QJniArray<jthrowable>);

// QJniObject-ish classes
VERIFY_CONTAINER_FOR_TYPE(QJniArray<QJniObject>, QList<QJniObject>);
VERIFY_ARRAY_FOR_CONTAINER(QList<QJniObject>, QJniArray<QJniObject>);
VERIFY_CONTAINER_FOR_TYPE(QJniArray<List>, QList<List>);
VERIFY_ARRAY_FOR_CONTAINER(QList<List>, QJniArray<List>);

// Special case: jbyte, (u)char, and QByteArray
VERIFY_CONTAINER_FOR_TYPE(QJniArray<jbyte>, QByteArray);
VERIFY_ARRAY_FOR_CONTAINER(QByteArray, QJniArray<jbyte>);
VERIFY_ARRAY_FOR_CONTAINER(QList<jbyte>, QJniArray<jbyte>);
VERIFY_CONTAINER_FOR_TYPE(QJniArray<char>, QByteArray);
VERIFY_ARRAY_FOR_CONTAINER(QList<char>, QJniArray<jbyte>);
VERIFY_CONTAINER_FOR_TYPE(QJniArray<uchar>, QList<uchar>);

// Special case: jstring, QString, and QStringList
VERIFY_CONTAINER_FOR_TYPE(QJniArray<jstring>, QStringList);
VERIFY_ARRAY_FOR_CONTAINER(QStringList, QJniArray<QString>);
VERIFY_CONTAINER_FOR_TYPE(QJniArray<QString>, QStringList);
VERIFY_ARRAY_FOR_CONTAINER(QList<jstring>, QJniArray<jstring>);

void tst_QJniArray::construct()
{
    // explicit
    {
        QStringList strings;
        for (int i = 0; i < 10000; ++i)
            strings << QString::number(i);
        QJniArray<QString> list(strings);
        QCOMPARE(list.size(), 10000);
        QCOMPARE(list.at(500), QString::number(500));
        QCOMPARE(list.toContainer(), strings);
    }
    {
        constexpr qsizetype size = 5;
        const QJniArray<jstring> list(size);
        const QStringList strings = list.toContainer();
        QCOMPARE(strings.at(0), QString());
        QCOMPARE(strings.size(), size);
    }
    {
        QJniArray bytes = QJniArrayBase::fromContainer(QByteArray("abc"));
        static_assert(std::is_same_v<decltype(bytes)::value_type, jbyte>);
        QCOMPARE(bytes.size(), 3);
    }
    {
        QJniArray list{1, 2, 3};
        static_assert(std::is_same_v<decltype(list), QJniArray<int>>);
        QCOMPARE(list.size(), 3);
        list = {4, 5};
        QCOMPARE(list.size(), 2);
    }
    {
        QJniArray<jint> list(QList<int>{1, 2, 3});
        QCOMPARE(list.size(), 3);
    }
    // CTAD with deduction guide
    {
        QJniArray list(QList<int>{1, 2, 3});
        QCOMPARE(list.size(), 3);
    }
    {
        QJniArray bytes(QByteArray("abc"));
        static_assert(std::is_same_v<decltype(bytes)::value_type, jbyte>);
        QCOMPARE(bytes.size(), 3);
    }
    {
        QStringList strings{"a", "b", "c"};
        QJniArray list(strings);
        QCOMPARE(list.size(), 3);
    }
    {
        QJniArray<jint> list{QList<int>{1, 2, 3}};
        QCOMPARE(list.size(), 3);
    }

    // non-contiguous container
    {
       std::list list({1, 2, 3});
       QJniArray array(list);
       QCOMPARE(array.size(), 3);
    }
    {
        std::list list({QString(), QString(), QString()});
        QJniArray array(list);
        QCOMPARE(array.size(), 3);
    }
}

// Verify that we can convert QJniArrays into each other as long as element types
// are convertible without narrowing.
template <typename From, typename To>
using CanConstructDetector = decltype(QJniArray<To>(std::declval<QJniArray<From>>()));
template <typename From, typename To>
using CanAssignDetector = decltype(std::declval<QJniArray<To>>().operator=(std::declval<QJniArray<From>>()));

template <typename From, typename To>
static constexpr bool canConstruct = qxp::is_detected_v<CanConstructDetector, From, To>;
template <typename From, typename To>
static constexpr bool canAssign = qxp::is_detected_v<CanAssignDetector, From, To>;

static_assert(canConstruct<jshort, jint> && canAssign<jshort, jint>);
static_assert(!canConstruct<jint, jshort> && !canAssign<jint, jshort>);
static_assert(canConstruct<jstring, jobject> && canAssign<jstring, jobject>);
static_assert(!canConstruct<jobject, jstring> && !canAssign<jobject, jstring>);

// exercise the QJniArray(QJniArray<Other> &&other) constructor
void tst_QJniArray::copyAndMove()
{
    QJniArray<jshort> tempShortArray({1, 2, 3});

    // copy - both arrays remain valid and reference the same object
    {
        QJniArray<jshort> shortArrayCopy(tempShortArray);
        QVERIFY(tempShortArray.isValid());
        QVERIFY(shortArrayCopy.isValid());
        QCOMPARE(tempShortArray, shortArrayCopy);
    }

    // moving QJniArray<T> to QJniArray<T> leaves the moved-from object invalid
    QJniArray<jshort> shortArray(std::move(tempShortArray));
    QVERIFY(!tempShortArray.isValid());
    QVERIFY(shortArray.isValid());

    tempShortArray = shortArray;

    // copying QJniArray<short> to QJniArray<int> works
    QJniArray<jint> intArray(shortArray);
    QVERIFY(shortArray.isValid());
    QVERIFY(intArray.isValid());
    QCOMPARE(intArray, shortArray);

    // moving QJniArray<short> to QJniArray<int> leaves the moved-from array invalid
    QJniArray<jlong> longArray(std::move(shortArray));
    QVERIFY(!shortArray.isValid());
    QVERIFY(longArray.isValid());
    QCOMPARE(longArray, intArray);
    QCOMPARE_NE(longArray, shortArray); // we can compare a moved-from object

    longArray = intArray;

    // not possible due to narrowing conversion, covered by static_asserts above
    // QJniArray<jshort> shortArray2(longArray);
    // intArray = longArray;
}

void tst_QJniArray::invalidArraysAreEmpty()
{
    QJniArray<jchar> invalid;
    QVERIFY(!invalid.isValid());
    QCOMPARE(invalid.object(), nullptr);
    QVERIFY(invalid.isEmpty());

    QCOMPARE(invalid.begin(), invalid.end());
    QCOMPARE(invalid.rbegin(), invalid.rend());

    QList<jchar> data;
    // safe to iterate
    for (const auto &e : invalid)
        data.emplace_back(e);
    QVERIFY(data.empty());

    // safe to convert
    data = invalid.toContainer();
    QVERIFY(data.empty());

    // unsafe to access
    // auto element = invalid.at(0);
}

void tst_QJniArray::size()
{
    QJniArray<jint> array;
    QVERIFY(!array.isValid());
    QCOMPARE(array.size(), 0);

    QList<int> intList;
    intList.resize(10);
    auto intArray = QJniArray(intList);
    QCOMPARE(intArray.size(), 10);
}

void tst_QJniArray::operators()
{
    QByteArray bytes("abcde");
    QJniArray<jbyte> array(bytes);
    QVERIFY(array.isValid());

    {
        auto it = array.begin();
        QCOMPARE(*it, 'a');
        QCOMPARE(*++it, 'b');
        QCOMPARE(*it++, 'b');
        QCOMPARE(*it, 'c');
        ++it;
        it++;
        QCOMPARE(*it, 'e');
        QCOMPARE(++it, array.end());

        it -= array.size();
        QCOMPARE(it, array.begin());
        it += 2;
        QCOMPARE(*it, 'c');

        const auto it2 = it + 2;
        QCOMPARE(*it2, 'e');
        QCOMPARE(it2 - it, 2);

        it = it2 - 2;
        QCOMPARE(*it, 'c');

        it = 1 + it;
        QCOMPARE(*it, 'd');
        it = array.size() - it;
        QCOMPARE(*it, 'c');

        QCOMPARE(it[1], 'd');
        QCOMPARE(it[-1], 'b');
    }
    {
        auto it = array.rbegin();
        QCOMPARE(*it, 'e');
        QCOMPARE(*++it, 'd');
        QCOMPARE(*it++, 'd');
        QCOMPARE(*it, 'c');
        ++it;
        it++;
        QCOMPARE(*it, 'a');
        QCOMPARE(++it, array.rend());
    }
    {
        QJniArray<jbyte>::const_iterator it = {};
        QCOMPARE(it, QJniArray<jbyte>::const_iterator{});
        QCOMPARE_NE(array.begin(), array.end());

        it = array.begin();
        QCOMPARE(it, array.begin());
    }

    QCOMPARE(std::distance(array.begin(), array.end()), array.size());

    qsizetype index = 0;
    for (const auto &value : array) {
        QCOMPARE(value, bytes.at(index));
        ++index;
    }
}

void tst_QJniArray::ordering()
{
    QByteArray bytes("abcde");
    QJniArray array(bytes);

    auto arrayBegin = array.begin();
    auto arrayEnd = array.end();
    QCOMPARE(arrayBegin, arrayBegin);
    QCOMPARE_LT(arrayBegin, arrayEnd);
    QCOMPARE_GT(arrayEnd, arrayBegin);
}

template <typename T, typename C>
using ToContainerTest = decltype(std::declval<QJniArray<T>>().toContainer(std::declval<C>()));

template <typename T, typename C>
static constexpr bool hasToContainer = qxp::is_detected_v<ToContainerTest, T, C>;

static_assert(hasToContainer<jint, QList<jint>>);
static_assert(hasToContainer<jint, QList<int>>);

static_assert(hasToContainer<jbyte, QByteArray>);
static_assert(hasToContainer<jstring, QStringList>);
static_assert(hasToContainer<String, QStringList>);
static_assert(hasToContainer<jchar, std::list<jchar>>);
static_assert(hasToContainer<jbyte, std::vector<jbyte>>);
// different types but doesn't narrow
static_assert(hasToContainer<jshort, QList<int>>);
static_assert(hasToContainer<jfloat, QList<jdouble>>);
// would narrow
static_assert(!hasToContainer<jlong, QList<short>>);
static_assert(!hasToContainer<jdouble, QList<jfloat>>);
// incompatible types
static_assert(!hasToContainer<jstring, QByteArray>);

void tst_QJniArray::toContainer()
{
    std::vector<jchar> charVector{u'a', u'b', u'c'};
    QJniArray<jchar> charArray(charVector);

    std::vector<jchar> vector;
    charArray.toContainer(vector);

    QCOMPARE(vector, charVector);
    QCOMPARE(charArray.toContainer<std::vector<jchar>>(), charVector);

    // non-contiguous container of primitive elements
    std::list charList = charArray.toContainer<std::list<jchar>>();
    QCOMPARE(charList.size(), charVector.size());
}

void tst_QJniArray::pointerToValue()
{
    const QJniArray stringArray{u"one"_s, u"two"_s, u"three"_s};
    auto it = std::find(stringArray.begin(), stringArray.end(), u"two"_s);
    QCOMPARE_NE(it, stringArray.end());
    QCOMPARE(it->size(), 3);
    QCOMPARE((++it)->size(), 5);
}

void tst_QJniArray::mutate()
{
    {
        QJniArray array{1, 2, 3};
        auto it = std::find(array.begin(), array.end(), 2);
        QCOMPARE_NE(it, array.end());
        *it = 4;
        QCOMPARE(array.at(1), 4);

        auto rit = array.rbegin();
        QCOMPARE(*rit, 3);
        *rit = 1;
        QCOMPARE(array[2], 1);
    }
    {
        QJniArray strings{u"one"_s, u"two"_s, u"three"_s};
        strings[1] = u"TWO"_s;
        QCOMPARE(strings.at(1), u"TWO"_s);

        // not possible as we cannot overload operator.()
        // strings[1].assign(u"two"_s)

        // not allowed as the modification won't be written back
        //strings[1]->assign(u"two"_s);
    }
    {
        QJniArray objects{QJniObject::fromString(u"one"_s),
                          QJniObject::fromString(u"two"_s),
                          QJniObject::fromString(u"three"_s)};
        objects[1] = QJniObject::fromString(u"TWO"_s);
        QCOMPARE(objects.at(1).toString(), u"TWO"_s);
    }
    {
        QJniArray<jint> emptyArray(5);
        QCOMPARE(emptyArray.size(), 5);
        QCOMPARE(emptyArray[0], 0);
    }
    {
        QJniArray<QString> emptyArray(5);
        QCOMPARE(emptyArray.size(), 5);
        QCOMPARE(emptyArray[0], QString());

        const QString null = u"null"_s;
        emptyArray[0] = null;
        QCOMPARE(emptyArray[0], null);

        emptyArray[1] = emptyArray[0];
        QCOMPARE(emptyArray[1], null);
        emptyArray[2] = *emptyArray.cbegin();
        QCOMPARE(emptyArray[2], null);

        int i = 0;
        for (auto str : emptyArray) {
            String strCopy(str);
            QCOMPARE(strCopy.toString(), str);
            QCOMPARE(strCopy.toString(), emptyArray[i]);
            ++i;
        }

        i = 0;
        for (auto string : emptyArray)
            string = u"Row %1"_s.arg(i++);
        QCOMPARE(emptyArray.at(0), "Row 0");
        QVERIFY(!(*emptyArray[1]).isEmpty());
    }
    {
        const QJniArray<QString> source{"one", "two", "three"};
        QJniArray<QString> target(3);

        auto s = source.begin();
        for (auto t : target) {
            t = *s;
            ++s;
        }
        QCOMPARE(source.toContainer(), target.toContainer());
    }
}

QT_END_NAMESPACE

QTEST_MAIN(tst_QJniArray)

#include "tst_qjniarray.moc"
