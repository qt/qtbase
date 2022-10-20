/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qjnienvironment.h"
#include "qjniobject.h"
#include "qjnihelpers_p.h"

#include <QtCore/QThread>
#include <QtCore/QThreadStorage>

QT_BEGIN_NAMESPACE

/*!
    \class QJniEnvironment
    \inmodule QtCore
    \since 6.1
    \brief The QJniEnvironment class provides access to the JNI Environment (JNIEnv).

    When using JNI, the \l {JNI tips: JavaVM and JNIEnv}{JNIEnv} class is a pointer to a function
    table and a member function for each JNI function that indirects through the table. \c JNIEnv
    provides most of the JNI functions. Every C++ native function receives a \c JNIEnv as the first
    argument. The JNI environment cannot be shared between threads.

    Since \c JNIEnv doesn't do much error checking, such as exception checking and clearing,
    QJniEnvironment allows you to do that easily.

    For more information about JNIEnv, see \l {Java: Interface Function Table}.

    \note This API has been designed and tested for use with Android.
    It has not been tested for other platforms.
*/

static const char qJniThreadName[] = "QtThread";

class QJniEnvironmentPrivate
{
public:
    JNIEnv *jniEnv = nullptr;
};

class QJniEnvironmentPrivateTLS
{
public:
    inline ~QJniEnvironmentPrivateTLS()
    {
        QtAndroidPrivate::javaVM()->DetachCurrentThread();
    }
};

struct QJniLocalRefDeleterPrivate
{
    static void cleanup(jobject obj)
    {
        if (!obj)
            return;

        QJniEnvironment env;
        env->DeleteLocalRef(obj);
    }
};

// To simplify this we only define it for jobjects.
typedef QScopedPointer<_jobject, QJniLocalRefDeleterPrivate> QJniScopedLocalRefPrivate;


Q_GLOBAL_STATIC(QThreadStorage<QJniEnvironmentPrivateTLS *>, jniEnvTLS)



/*!
    \fn QJniEnvironment::QJniEnvironment()

    Constructs a new JNI Environment object and attaches the current thread to the Java VM.
*/
QJniEnvironment::QJniEnvironment()
    : d(new QJniEnvironmentPrivate{})
{
    JavaVM *vm = QtAndroidPrivate::javaVM();
    const jint ret = vm->GetEnv((void**)&d->jniEnv, JNI_VERSION_1_6);
    if (ret == JNI_OK) // Already attached
        return;

    if (ret == JNI_EDETACHED) { // We need to (re-)attach
        JavaVMAttachArgs args = { JNI_VERSION_1_6, qJniThreadName, nullptr };
        if (vm->AttachCurrentThread(&d->jniEnv, &args) != JNI_OK)
            return;

        if (!jniEnvTLS->hasLocalData()) // If we attached the thread we own it.
            jniEnvTLS->setLocalData(new QJniEnvironmentPrivateTLS);
    }
}

/*!
    \fn QJniEnvironment::~QJniEnvironment()

    Detaches the current thread from the Java VM and destroys the QJniEnvironment object.
    This will clear any pending exception by calling checkAndClearExceptions().
*/
QJniEnvironment::~QJniEnvironment()
{
    checkAndClearExceptions();
}

/*!
    Returns \c true if this instance holds a valid JNIEnv object.

    \since 6.2
*/
bool QJniEnvironment::isValid() const
{
    return d->jniEnv;
}

/*!
    \fn JNIEnv *QJniEnvironment::operator->() const

    Provides access to the JNI Environment's \c JNIEnv pointer.
*/
JNIEnv *QJniEnvironment::operator->() const
{
    return d->jniEnv;
}

/*!
    \fn JNIEnv &QJniEnvironment::operator*() const

    Returns the JNI Environment's \c JNIEnv object.
*/
JNIEnv &QJniEnvironment::operator*() const
{
    return *d->jniEnv;
}

/*!
    \fn JNIEnv *QJniEnvironment::jniEnv() const

    Returns the JNI Environment's \c JNIEnv pointer.
*/
JNIEnv *QJniEnvironment::jniEnv() const
{
    return d->jniEnv;
}

/*!
    Searches for \a className using all available class loaders. Qt on Android
    uses a custom class loader to load all the .jar files and it must be used
    to find any classes that are created by that class loader because these
    classes are not visible when using the default class loader.

    Returns the class pointer or null if \a className is not found.

    A use case for this function is searching for a class to call a JNI method
    that takes a \c jclass. This can be useful when doing multiple JNI calls on
    the same class object which can a bit faster than using a class name in each
    call. Additionally, this call looks for internally cached classes first before
    doing a JNI call, and returns such a class if found. The following code snippet
    creates an instance of the class \c CustomClass and then calls the
    \c printFromJava() method:

    \code
    QJniEnvironment env;
    jclass javaClass = env.findClass("org/qtproject/example/android/CustomClass");
    QJniObject javaMessage = QJniObject::fromString("findClass example");
    QJniObject::callStaticMethod<void>(javaClass, "printFromJava",
                                       "(Ljava/lang/String;)V", javaMessage.object<jstring>());
    \endcode

    \note This call returns a global reference to the class object from the
    internally cached classes.
*/
jclass QJniEnvironment::findClass(const char *className)
{
    return QtAndroidPrivate::findClass(className, d->jniEnv);
}

/*!
    Searches for an instance method of a class \a clazz. The method is specified
    by its \a methodName and \a signature.

    Returns the method ID or \c nullptr if the method is not found.

    A usecase for this method is searching for class methods and caching their
    IDs, so that they could later be used for calling the methods.

    \since 6.2
*/
jmethodID QJniEnvironment::findMethod(jclass clazz, const char *methodName, const char *signature)
{
    if (clazz) {
        jmethodID id = d->jniEnv->GetMethodID(clazz, methodName, signature);
        if (!checkAndClearExceptions(d->jniEnv))
            return id;
    }

    return nullptr;
}

/*!
    Searches for a static method of a class \a clazz. The method is specified
    by its \a methodName and \a signature.

    Returns the method ID or \c nullptr if the method is not found.

    A usecase for this method is searching for class methods and caching their
    IDs, so that they could later be used for calling the methods.

    \code
    QJniEnvironment env;
    jclass javaClass = env.findClass("org/qtproject/example/android/CustomClass");
    jmethodID methodId = env.findStaticMethod(javaClass,
                                              "staticJavaMethod",
                                              "(Ljava/lang/String;)V");
    QJniObject javaMessage = QJniObject::fromString("findStaticMethod example");
    QJniObject::callStaticMethod<void>(javaClass,
                                       methodId,
                                       javaMessage.object<jstring>());
    \endcode

    \since 6.2
*/
jmethodID QJniEnvironment::findStaticMethod(jclass clazz, const char *methodName, const char *signature)
{
    if (clazz) {
        jmethodID id = d->jniEnv->GetStaticMethodID(clazz, methodName, signature);
        if (!checkAndClearExceptions(d->jniEnv))
            return id;
    }

    return nullptr;
}


/*!
    Searches for an member field of a class \a clazz. The field is specified
    by its \a fieldName and \a signature.

    Returns the field ID or \c nullptr if the field is not found.

    A usecase for this method is searching for class fields and caching their
    IDs, so that they could later be used for getting/setting the fields.

    \since 6.2
*/
jfieldID QJniEnvironment::findField(jclass clazz, const char *fieldName, const char *signature)
{
    if (clazz) {
        jfieldID id = d->jniEnv->GetFieldID(clazz, fieldName, signature);
        if (!checkAndClearExceptions())
            return id;
    }

    return nullptr;
}

/*!
    Searches for a static field of a class \a clazz. The field is specified
    by its \a fieldName and \a signature.

    Returns the field ID or \c nullptr if the field is not found.

    A usecase for this method is searching for class fields and caching their
    IDs, so that they could later be used for getting/setting the fields.

    \since 6.2
*/
jfieldID QJniEnvironment::findStaticField(jclass clazz, const char *fieldName, const char *signature)
{
    if (clazz) {
        jfieldID id = d->jniEnv->GetStaticFieldID(clazz, fieldName, signature);
        if (!checkAndClearExceptions())
            return id;
    }

    return nullptr;
}

/*!
    \fn JavaVM *QJniEnvironment::javaVM()

    Returns the Java VM interface for the current process. Although it might
    be possible to have multiple Java VMs per process, Android allows only one.

*/
JavaVM *QJniEnvironment::javaVM()
{
    return QtAndroidPrivate::javaVM();
}

/*!
    Registers the Java methods in the array \a methods of size \a size, each of
    which can call native C++ functions from class \a className. These methods
    must be registered before any attempt to call them.

    Returns \c true if the registration is successful, otherwise \c false.

    Each element in the methods array consists of:
    \list
        \li The Java method name
        \li Method signature
        \li The C++ functions that will be executed
    \endlist

    \code
    const JNINativeMethod methods[] =
                            {{"callNativeOne", "(I)V", reinterpret_cast<void *>(fromJavaOne)},
                            {"callNativeTwo", "(I)V", reinterpret_cast<void *>(fromJavaTwo)}};
    QJniEnvironment env;
    env.registerNativeMethods("org/qtproject/android/TestJavaClass", methods, 2);
    \endcode
*/
bool QJniEnvironment::registerNativeMethods(const char *className, const JNINativeMethod methods[],
                                            int size)
{
    const jclass clazz = findClass(className);

    if (!clazz)
        return false;

    return registerNativeMethods(clazz, methods, size);
}
#if QT_DEPRECATED_SINCE(6, 2)
/*!
    \overload
    \deprecated [6.2] Use the overload with a const JNINativeMethod[] instead.

    Registers the Java methods in the array \a methods of size \a size, each of
    which can call native C++ functions from class \a className. These methods
    must be registered before any attempt to call them.

    Returns \c true if the registration is successful, otherwise \c false.

    Each element in the methods array consists of:
    \list
        \li The Java method name
        \li Method signature
        \li The C++ functions that will be executed
    \endlist

    \code
    JNINativeMethod methods[] = {{"callNativeOne", "(I)V", reinterpret_cast<void *>(fromJavaOne)},
                                 {"callNativeTwo", "(I)V", reinterpret_cast<void *>(fromJavaTwo)}};
    QJniEnvironment env;
    env.registerNativeMethods("org/qtproject/android/TestJavaClass", methods, 2);
    \endcode
*/
bool QJniEnvironment::registerNativeMethods(const char *className, JNINativeMethod methods[],
                                            int size)
{
    return registerNativeMethods(className, const_cast<const JNINativeMethod*>(methods), size);
}
#endif
/*!
    \overload

    This overload uses a previously cached jclass instance \a clazz.

    \code
    JNINativeMethod methods[] {{"callNativeOne", "(I)V", reinterpret_cast<void *>(fromJavaOne)},
                               {"callNativeTwo", "(I)V", reinterpret_cast<void *>(fromJavaTwo)}};
    QJniEnvironment env;
    jclass clazz = env.findClass("org/qtproject/android/TestJavaClass");
    env.registerNativeMethods(clazz, methods, 2);
    \endcode
*/
bool QJniEnvironment::registerNativeMethods(jclass clazz, const JNINativeMethod methods[],
                                            int size)
{
    if (d->jniEnv->RegisterNatives(clazz, methods, size) < 0) {
        checkAndClearExceptions();
        return false;
    }
    return true;
}

/*!
    \enum QJniEnvironment::OutputMode

    \value Silent The exceptions are cleaned silently
    \value Verbose Prints the exceptions and their stack backtrace as an error
           to \c stderr stream.
*/

/*!
    \fn QJniEnvironment::checkAndClearExceptions(OutputMode outputMode = OutputMode::Verbose)

    Cleans any pending exceptions either silently or reporting stack backtrace,
    depending on the \a outputMode.

    In contrast to \l QJniObject, which handles exceptions internally, if you
    make JNI calls directly via \c JNIEnv, you need to clear any potential
    exceptions after the call using this function. For more information about
    \c JNIEnv calls that can throw an exception, see \l {Java: JNI Functions}{JNI Functions}.

    \return \c true when a pending exception was cleared.
*/
bool QJniEnvironment::checkAndClearExceptions(QJniEnvironment::OutputMode outputMode)
{
    if (Q_UNLIKELY(d->jniEnv->ExceptionCheck())) {
        if (outputMode != OutputMode::Silent)
            d->jniEnv->ExceptionDescribe();
        d->jniEnv->ExceptionClear();

        return true;
    }

    return false;
}

/*!
    \fn QJniEnvironment::checkAndClearExceptions(JNIEnv *env, OutputMode outputMode = OutputMode::Verbose)

    Cleans any pending exceptions for \a env, either silently or reporting
    stack backtrace, depending on the \a outputMode. This is useful when you
    already have a \c JNIEnv pointer such as in a native function implementation.

    In contrast to \l QJniObject, which handles exceptions internally, if you
    make JNI calls directly via \c JNIEnv, you need to clear any potential
    exceptions after the call using this function. For more information about
    \c JNIEnv calls that can throw an exception, see \l {Java: JNI Functions}{JNI Functions}.

    \return \c true when a pending exception was cleared.
*/
bool QJniEnvironment::checkAndClearExceptions(JNIEnv *env, QJniEnvironment::OutputMode outputMode)
{
    if (Q_UNLIKELY(env->ExceptionCheck())) {
        if (outputMode != OutputMode::Silent)
            env->ExceptionDescribe();
        env->ExceptionClear();

        return true;
    }

    return false;
}

QT_END_NAMESPACE
