/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <jni.h>

#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtTest/QtTest>

static const char javaTestClass[] =
        "org/qtproject/qt/android/testdatapackage/QtJniEnvironmentTestClass";
static const char javaTestClassNoCtor[] =
        "org/qtproject/qt/android/testdatapackage/QtJniEnvironmentTestClassNoCtor";

static QString registerNativesString = QStringLiteral("Qt");
static int registerNativeInteger = 0;

class tst_QJniEnvironment : public QObject
{
    Q_OBJECT

private slots:
    void jniEnv();
    void javaVM();
    void registerNativeMethods();
    void registerNativeMethodsByJclass();
    void findMethod();
    void findStaticMethod();
    void findField();
    void findStaticField();
};

void tst_QJniEnvironment::jniEnv()
{
    QJniEnvironment env;
    JavaVM *javaVM = env.javaVM();
    QVERIFY(javaVM);

    {
        // JNI environment should now be attached to the current thread
        JNIEnv *jni = 0;
        QCOMPARE(javaVM->GetEnv((void**)&jni, JNI_VERSION_1_6), JNI_OK);

        JNIEnv *e = env.jniEnv();
        QVERIFY(e);

        QCOMPARE(env->GetVersion(), JNI_VERSION_1_6);

        // try to find an existing class
        QVERIFY(env->FindClass("java/lang/Object"));
        QVERIFY(!env->ExceptionCheck());

        // try to find a nonexistent class
        QVERIFY(!env->FindClass("this/doesnt/Exist"));
        QVERIFY(env->ExceptionCheck());
        env->ExceptionClear();

        QVERIFY(env->FindClass("java/lang/Object"));
        QVERIFY(!QJniEnvironment::checkAndClearExceptions(env.jniEnv()));

        // try to find a nonexistent class
        QVERIFY(!env->FindClass("this/doesnt/Exist"));
        QVERIFY(QJniEnvironment::checkAndClearExceptions(env.jniEnv()));

        // try to find an existing class with QJniEnvironment
        QJniEnvironment env;
        QVERIFY(env.findClass("java/lang/Object"));

        // try to find a nonexistent class
        QVERIFY(!env.findClass("this/doesnt/Exist"));

        // clear exception with member function
        QVERIFY(!env->FindClass("this/doesnt/Exist"));
        QVERIFY(env.checkAndClearExceptions());
    }

    // The env does not detach automatically, even if it goes out of scope. The only way it can
    // be detached is if it's done explicitly, or if the thread we attached to gets killed (TLS clean-up).
    JNIEnv *jni = nullptr;
    QCOMPARE(javaVM->GetEnv((void**)&jni, JNI_VERSION_1_6), JNI_OK);
}

void tst_QJniEnvironment::javaVM()
{
    QJniEnvironment env;
    JavaVM *javaVM = env.javaVM();
    QVERIFY(javaVM);

    QCOMPARE(env.javaVM(), javaVM);

    JavaVM *vm = 0;
    QCOMPARE(env->GetJavaVM(&vm), JNI_OK);
    QCOMPARE(env.javaVM(), vm);
}

static void callbackFromJava(JNIEnv *env, jobject /*thiz*/, jstring value)
{
    Q_UNUSED(env)
    registerNativesString = QJniObject(value).toString();
}

static void callbackFromJavaNoCtor(JNIEnv *env, jobject /*thiz*/, jstring value)
{
    Q_UNUSED(env)
    registerNativesString = QJniObject(value).toString();
}

void tst_QJniEnvironment::registerNativeMethods()
{
    QJniObject QtString = QJniObject::fromString(registerNativesString);
    QJniEnvironment env;

    {
        const JNINativeMethod methods[] {
          {"callbackFromJava", "(Ljava/lang/String;)V", reinterpret_cast<void *>(callbackFromJava)}
        };

        QVERIFY(env.registerNativeMethods(javaTestClass, methods, 1));

        QJniObject::callStaticMethod<void>(javaTestClass,
                                           "appendJavaToString",
                                           "(Ljava/lang/String;)V",
                                            QtString.object<jstring>());
        QTest::qWait(200);
        QVERIFY(registerNativesString == QStringLiteral("From Java: Qt"));
    }

    // No default constructor in class
    {
        const JNINativeMethod methods[] {{"callbackFromJavaNoCtor", "(Ljava/lang/String;)V",
           reinterpret_cast<void *>(callbackFromJavaNoCtor)}};

        QVERIFY(env.registerNativeMethods(javaTestClassNoCtor, methods, 1));

        QJniObject::callStaticMethod<void>(javaTestClassNoCtor,
                                           "appendJavaToString",
                                           "(Ljava/lang/String;)V",
                                            QtString.object<jstring>());
        QTest::qWait(200);
        QVERIFY(registerNativesString == QStringLiteral("From Java (no ctor): Qt"));
    }
}

static void intCallbackFromJava(JNIEnv *env, jobject /*thiz*/, jint value)
{
    Q_UNUSED(env)
    registerNativeInteger = static_cast<int>(value);
}

void tst_QJniEnvironment::registerNativeMethodsByJclass()
{
    const JNINativeMethod methods[] {
        { "intCallbackFromJava", "(I)V", reinterpret_cast<void *>(intCallbackFromJava) }
    };

    QJniEnvironment env;
    jclass clazz = env.findClass(javaTestClass);
    QVERIFY(clazz != 0);
    QVERIFY(env.registerNativeMethods(clazz, methods, 1));

    QCOMPARE(registerNativeInteger, 0);

    QJniObject parameter = QJniObject::fromString(QString("123"));
    QJniObject::callStaticMethod<void>(clazz, "convertToInt", "(Ljava/lang/String;)V",
                                       parameter.object<jstring>());
    QTest::qWait(200);
    QCOMPARE(registerNativeInteger, 123);
}

void tst_QJniEnvironment::findMethod()
{
    QJniEnvironment env;
    jclass clazz = env.findClass("java/lang/Integer");
    QVERIFY(clazz != nullptr);

    // existing method
    jmethodID methodId = env.findMethod(clazz, "toString", "()Ljava/lang/String;");
    QVERIFY(methodId != nullptr);

    // invalid signature
    jmethodID invalid = env.findMethod(clazz, "unknown", "()I");
    QVERIFY(invalid == nullptr);
    // check that all exceptions are already cleared
    QVERIFY(!env.checkAndClearExceptions());
}

void tst_QJniEnvironment::findStaticMethod()
{
    QJniEnvironment env;
    jclass clazz = env.findClass("java/lang/Integer");
    QVERIFY(clazz != nullptr);

    // existing method
    jmethodID staticMethodId = env.findStaticMethod(clazz, "parseInt", "(Ljava/lang/String;)I");
    QVERIFY(staticMethodId != nullptr);

    QJniObject parameter = QJniObject::fromString("123");
    jint result = QJniObject::callStaticMethod<jint>(clazz, staticMethodId,
                                                     parameter.object<jstring>());
    QCOMPARE(result, 123);

    // invalid method
    jmethodID invalid = env.findStaticMethod(clazz, "unknown", "()I");
    QVERIFY(invalid == nullptr);
    // check that all exceptions are already cleared
    QVERIFY(!env.checkAndClearExceptions());
}

void tst_QJniEnvironment::findField()
{
    QJniEnvironment env;
    jclass clazz = env.findClass(javaTestClass);
    QVERIFY(clazz != nullptr);

    // valid field
    jfieldID validId = env.findField(clazz, "INT_FIELD", "I");
    QVERIFY(validId != nullptr);

    jmethodID constructorId = env.findMethod(clazz, "<init>", "()V");
    QVERIFY(constructorId != nullptr);
    jobject obj = env->NewObject(clazz, constructorId);
    QVERIFY(!env.checkAndClearExceptions());
    int value = env->GetIntField(obj, validId);
    QVERIFY(!env.checkAndClearExceptions());
    QVERIFY(value == 123);

    // invalid signature
    jfieldID invalidId = env.findField(clazz, "unknown", "I");
    QVERIFY(invalidId == nullptr);
    // check that all exceptions are already cleared
    QVERIFY(!env.checkAndClearExceptions());
}

void tst_QJniEnvironment::findStaticField()
{
    QJniEnvironment env;
    jclass clazz = env.findClass(javaTestClass);
    QVERIFY(clazz != nullptr);

    // valid field
    jfieldID validId = env.findStaticField(clazz, "S_INT_FIELD", "I");
    QVERIFY(validId != nullptr);

    int size = env->GetStaticIntField(clazz, validId);
    QVERIFY(!env.checkAndClearExceptions());
    QVERIFY(size == 321);

    // invalid signature
    jfieldID invalidId = env.findStaticField(clazz, "unknown", "I");
    QVERIFY(invalidId == nullptr);
    // check that all exceptions are already cleared
    QVERIFY(!env.checkAndClearExceptions());
}

QTEST_MAIN(tst_QJniEnvironment)

#include "tst_qjnienvironment.moc"
