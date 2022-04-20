/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <QtTest>

#include <QtCore/qjnitypes.h>

class tst_QJniTypes : public QObject
{
    Q_OBJECT

public:
    tst_QJniTypes() = default;

private slots:
    void initTestCase();
};

struct QtJavaWrapper {};
template<>
constexpr auto QtJniTypes::typeSignature<QtJavaWrapper>()
{
    return QtJniTypes::String("Lorg/qtproject/qt/android/QtJavaWrapper;");
}

template<>
constexpr auto QtJniTypes::typeSignature<QJniObject>()
{
    return QtJniTypes::String("Ljava/lang/Object;");
}

struct QtCustomJniObject : QJniObject {};
template<>
constexpr auto QtJniTypes::typeSignature<QtCustomJniObject>()
{
    return QtJniTypes::String("Lorg/qtproject/qt/android/QtCustomJniObject;");
}

static_assert(QtJniTypes::typeSignature<QtJavaWrapper>() == "Lorg/qtproject/qt/android/QtJavaWrapper;");
static_assert(QtJniTypes::typeSignature<QtJavaWrapper>() != "Ljava/lang/Object;");
static_assert(!(QtJniTypes::typeSignature<QtJavaWrapper>() == "X"));

static_assert(QtJniTypes::fieldSignature<jint>() == "I");
static_assert(QtJniTypes::fieldSignature<jint>() != "X");
static_assert(QtJniTypes::fieldSignature<jint>() != "Ljava/lang/Object;");
static_assert(QtJniTypes::fieldSignature<jlong>() == "J");
static_assert(QtJniTypes::fieldSignature<jstring>() == "Ljava/lang/String;");
static_assert(QtJniTypes::fieldSignature<jobject>() == "Ljava/lang/Object;");
static_assert(QtJniTypes::fieldSignature<jobjectArray>() == "[Ljava/lang/Object;");
static_assert(QtJniTypes::fieldSignature<QJniObject>() == "Ljava/lang/Object;");
static_assert(QtJniTypes::fieldSignature<QtJavaWrapper>() == "Lorg/qtproject/qt/android/QtJavaWrapper;");
static_assert(QtJniTypes::fieldSignature<QtCustomJniObject>() == "Lorg/qtproject/qt/android/QtCustomJniObject;");

static_assert(QtJniTypes::methodSignature<void>() == "()V");
static_assert(QtJniTypes::methodSignature<void>() != "()X");
static_assert(QtJniTypes::methodSignature<void, jint>() == "(I)V");
static_assert(QtJniTypes::methodSignature<void, jint, jstring>() == "(ILjava/lang/String;)V");
static_assert(QtJniTypes::methodSignature<jlong, jint, jclass>() == "(ILjava/lang/Class;)J");
static_assert(QtJniTypes::methodSignature<jobject, jint, jstring>() == "(ILjava/lang/String;)Ljava/lang/Object;");

static_assert(QtJniTypes::isPrimitiveType<jint>());
static_assert(QtJniTypes::isPrimitiveType<void>());
static_assert(!QtJniTypes::isPrimitiveType<jobject>());
static_assert(!QtJniTypes::isPrimitiveType<QtCustomJniObject>());

static_assert(!QtJniTypes::isObjectType<jint>());
static_assert(!QtJniTypes::isObjectType<void>());
static_assert(QtJniTypes::isObjectType<jobject>());
static_assert(QtJniTypes::isObjectType<QtCustomJniObject>());

static_assert(QtJniTypes::String("ABCDE").startsWith("ABC"));
static_assert(QtJniTypes::String("ABCDE").startsWith("A"));
static_assert(QtJniTypes::String("ABCDE").startsWith("ABCDE"));
static_assert(!QtJniTypes::String("ABCDE").startsWith("ABCDEF"));
static_assert(!QtJniTypes::String("ABCDE").startsWith("9AB"));
static_assert(QtJniTypes::String("ABCDE").startsWith('A'));
static_assert(!QtJniTypes::String("ABCDE").startsWith('B'));

static_assert(QtJniTypes::String("ABCDE").endsWith("CDE"));
static_assert(QtJniTypes::String("ABCDE").endsWith("E"));
static_assert(QtJniTypes::String("ABCDE").endsWith("ABCDE"));
static_assert(!QtJniTypes::String("ABCDE").endsWith("DEF"));
static_assert(!QtJniTypes::String("ABCDE").endsWith("ABCDEF"));
static_assert(QtJniTypes::String("ABCDE").endsWith('E'));
static_assert(!QtJniTypes::String("ABCDE").endsWith('F'));

void tst_QJniTypes::initTestCase()
{
}

QTEST_MAIN(tst_QJniTypes)

#include "tst_qjnitypes.moc"
