// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>

#include <QtCore/qjnitypes.h>
#include <QtCore/qjniarray.h>

using namespace Qt::StringLiterals;

class tst_QJniArray : public QObject
{
    Q_OBJECT

public:
    tst_QJniArray() = default;
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

QTEST_MAIN(tst_QJniArray)

#include "tst_qjniarray.moc"
