// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [QJniObject scope]
void functionScope()
{
    QString helloString("Hello");
    jstring myJString = 0;
    {
        QJniObject string = QJniObject::fromString(helloString);
        myJString = string.object<jstring>();
    }

   // Ops! myJString is no longer valid.
}
//! [QJniObject scope]

//! [C++ native methods]
static void fromJavaOne(JNIEnv *env, jobject thiz, jint x)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    qDebug() << x << "< 100";
}

static void fromJavaTwo(JNIEnv *env, jobject thiz, jint x)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    qDebug() << x << ">= 100";
}

void foo()
{
    // register the native methods first, ideally it better be done with the app start
    const JNINativeMethod methods[] =
                {{"callNativeOne", "(I)V", reinterpret_cast<void *>(fromJavaOne)},
                 {"callNativeTwo", "(I)V", reinterpret_cast<void *>(fromJavaTwo)}};
    QJniEnvironment env;
    env.registerNativeMethods("my/java/project/FooJavaClass", methods, 2);

    // Call the java method which will calls back to the C++ functions
    QJniObject::callStaticMethod<void>("my/java/project/FooJavaClass", "foo", "(I)V", 10);  // Output: 10 < 100
    QJniObject::callStaticMethod<void>("my/java/project/FooJavaClass", "foo", "(I)V", 100); // Output: 100 >= 100
}
//! [C++ native methods]

//! [Java native methods]
class FooJavaClass
{
    public static void foo(int x)
    {
        if (x < 100)
            callNativeOne(x);
        else
            callNativeTwo(x);
    }

private static native void callNativeOne(int x);
private static native void callNativeTwo(int x);

}
//! [Java native methods]
