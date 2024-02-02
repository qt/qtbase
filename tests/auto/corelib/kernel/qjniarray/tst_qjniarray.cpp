// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest>

#include <QtCore/qjnitypes.h>
#include <QtCore/qjniarray.h>

using namespace Qt::StringLiterals;

class tst_QJniArray : public QObject
{
    Q_OBJECT

public:
    tst_QJniArray() = default;

private slots:
    void size();
    void operators();
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

void tst_QJniArray::size()
{
    QJniArray<jint> array;
    QVERIFY(!array.isValid());
    QCOMPARE(array.size(), 0);

    QList<int> intList;
    intList.resize(10);
    auto intArray = QJniArrayBase::fromContainer(intList);
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

QTEST_MAIN(tst_QJniArray)

#include "tst_qjniarray.moc"
