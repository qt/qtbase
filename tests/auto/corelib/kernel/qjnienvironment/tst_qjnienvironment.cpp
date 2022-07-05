// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
        QVERIFY(env.findClass<jstring>());

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
Q_DECLARE_JNI_NATIVE_METHOD(callbackFromJava);

static void tediouslyLongNamed_callbackFromJava(JNIEnv *env, jobject /*thiz*/, jstring value)
{
    Q_UNUSED(env)
    registerNativesString = QJniObject(value).toString();
}
Q_DECLARE_JNI_NATIVE_METHOD(tediouslyLongNamed_callbackFromJava, namedCallbackFromJava)

static void callbackFromJavaNoCtor(JNIEnv *env, jobject /*thiz*/, jstring value)
{
    Q_UNUSED(env)
    registerNativesString = QJniObject(value).toString();
}
Q_DECLARE_JNI_NATIVE_METHOD(callbackFromJavaNoCtor);

class CallbackClass {
public:
    static void memberCallbackFromJava(JNIEnv *env, jobject /*thiz*/, jstring value)
    {
        Q_UNUSED(env)
        registerNativesString = QJniObject(value).toString();
    }
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(memberCallbackFromJava)

    static void tediouslyLongNamed_memberCallbackFromJava(JNIEnv *env, jobject /*thiz*/,
                                                          jstring value)
    {
        Q_UNUSED(env)
        registerNativesString = QJniObject(value).toString();
    }
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(tediouslyLongNamed_memberCallbackFromJava,
                                                 namedMemberCallbackFromJava)
};

namespace CallbackNamespace {
    static void namespaceCallbackFromJava(JNIEnv *env, jobject /*thiz*/, jstring value)
    {
        Q_UNUSED(env)
        registerNativesString = QJniObject(value).toString();
    }
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(namespaceCallbackFromJava)
}

void tst_QJniEnvironment::registerNativeMethods()
{
    QJniObject QtString = QJniObject::fromString(registerNativesString);
    QJniEnvironment env;

    {
        QVERIFY(env.registerNativeMethods(javaTestClass, {
            Q_JNI_NATIVE_METHOD(callbackFromJava)
        }));

        QJniObject::callStaticMethod<void>(javaTestClass,
                                           "appendJavaToString",
                                           "(Ljava/lang/String;)V",
                                            QtString.object<jstring>());
        QTest::qWait(200);
        QVERIFY(registerNativesString == QStringLiteral("From Java: Qt"));
    }

    // Named native function
    {
        QVERIFY(env.registerNativeMethods(javaTestClass, {
            Q_JNI_NATIVE_METHOD(tediouslyLongNamed_callbackFromJava)
        }));

        QJniObject::callStaticMethod<void>(javaTestClass,
                                           "namedAppendJavaToString",
                                           "(Ljava/lang/String;)V",
                                            QtString.object<jstring>());
        QTest::qWait(200);
        QVERIFY(registerNativesString == QStringLiteral("From Java (named): Qt"));
    }

    // Static class member as callback
    {
        QVERIFY(env.registerNativeMethods(javaTestClass, {
            Q_JNI_NATIVE_SCOPED_METHOD(memberCallbackFromJava, CallbackClass)
        }));

        QJniObject::callStaticMethod<void>(javaTestClass,
                                           "memberAppendJavaToString",
                                           "(Ljava/lang/String;)V",
                                            QtString.object<jstring>());
        QTest::qWait(200);
        QVERIFY(registerNativesString == QStringLiteral("From Java (member): Qt"));
    }

    // Static named class member as callback
    {
        QVERIFY(env.registerNativeMethods(javaTestClass, {
            Q_JNI_NATIVE_SCOPED_METHOD(tediouslyLongNamed_memberCallbackFromJava,
                                       CallbackClass)
        }));

        QJniObject::callStaticMethod<void>(javaTestClass,
                                           "namedMemberAppendJavaToString",
                                           "(Ljava/lang/String;)V",
                                            QtString.object<jstring>());
        QTest::qWait(200);
        QVERIFY(registerNativesString == QStringLiteral("From Java (named member): Qt"));
    }

    // Function generally just in namespace as callback
    {
        QVERIFY(env.registerNativeMethods(javaTestClass, {
            Q_JNI_NATIVE_SCOPED_METHOD(namespaceCallbackFromJava, CallbackNamespace)
        }));

        QJniObject::callStaticMethod<void>(javaTestClass,
                                           "namespaceAppendJavaToString",
                                           "(Ljava/lang/String;)V",
                                            QtString.object<jstring>());
        QTest::qWait(200);
        QVERIFY(registerNativesString == QStringLiteral("From Java (namespace): Qt"));
    }

    // No default constructor in class
    {
        QVERIFY(env.registerNativeMethods(javaTestClassNoCtor, {
            Q_JNI_NATIVE_METHOD(callbackFromJavaNoCtor)
        }));

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
Q_DECLARE_JNI_NATIVE_METHOD(intCallbackFromJava);

void tst_QJniEnvironment::registerNativeMethodsByJclass()
{
    QJniEnvironment env;
    jclass clazz = env.findClass(javaTestClass);
    QVERIFY(clazz != 0);
    QVERIFY(env.registerNativeMethods(clazz, {
        Q_JNI_NATIVE_METHOD(intCallbackFromJava)
    }));

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

    // existing method
    methodId = env.findMethod<jstring>(clazz, "toString");
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

    // existing method
    staticMethodId = env.findStaticMethod<jint, jstring>(clazz, "parseInt");
    QVERIFY(staticMethodId != nullptr);

    QJniObject parameter = QJniObject::fromString("123");
    jint result = QJniObject::callStaticMethod<jint>(clazz, staticMethodId,
                                                     parameter.object<jstring>());
    QCOMPARE(result, 123);

    // invalid method
    jmethodID invalid = env.findStaticMethod(clazz, "unknown", "()I");
    QVERIFY(invalid == nullptr);
    invalid = env.findStaticMethod<jint>(clazz, "unknown");
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
    validId = env.findField<jint>(clazz, "INT_FIELD");
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
    validId = env.findStaticField<jint>(clazz, "S_INT_FIELD");
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
