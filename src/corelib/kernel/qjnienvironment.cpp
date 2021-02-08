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
    This will clear any pending exception by calling exceptionCheckAndClear().
*/
QJniEnvironment::~QJniEnvironment()
{
    exceptionCheckAndClear();
}

/*!
    \fn JNIEnv *QJniEnvironment::operator->()

    Provides access to the QJniEnvironment's JNIEnv pointer.
*/
JNIEnv *QJniEnvironment::operator->()
{
    return d->jniEnv;
}

/*!
    \fn QJniEnvironment::operator JNIEnv *() const

    Returns the JNI Environment pointer.
*/
QJniEnvironment::operator JNIEnv* () const
{
    return d->jniEnv;
}

/*!
    \fn jclass QJniEnvironment::findClass(const char *className)

    Searches for \a className using all available class loaders. Qt on Android
    uses a custom class loader to load all the .jar files and it must be used
    to find any classes that are created by that class loader because these
    classes are not visible when using the default class loader.

    Returns the class pointer or null if is not found.

    A use case for this function is searching for a custom class then calling
    its member method. The following code snippet creates an instance of the
    class \c CustomClass and then calls the \c printFromJava() method:

    \code
    QJniEnvironment env;
    jclass javaClass = env.findClass("org/qtproject/example/android/CustomClass");
    QJniObject classObject(javaClass);

    QJniObject javaMessage = QJniObject::fromString("findClass example");
    classObject.callMethod<void>("printFromJava",
                                 "(Ljava/lang/String;)V",
                                 javaMessage.object<jstring>());
    \endcode
*/
jclass QJniEnvironment::findClass(const char *className)
{
    return QtAndroidPrivate::findClass(className, d->jniEnv);
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
    \fn bool QJniEnvironment::registerNativeMethods(const char *className, JNINativeMethod methods[], int size)

    Registers the Java methods in the array \a methods of size \a size, each of
    which can call native C++ functions from class \a className. These methods
    must be registered before any attempt to call them.

    Returns True if the registration is successful, otherwise False.

    Each element in the methods array consists of:
    \list
        \li The Java method name
        \li Method signature
        \li The C++ functions that will be executed
    \endlist

    \code
    JNINativeMethod methods[] {{"callNativeOne", "(I)V", reinterpret_cast<void *>(fromJavaOne)},
                               {"callNativeTwo", "(I)V", reinterpret_cast<void *>(fromJavaTwo)}};
    QJniEnvironment env;
    env.registerNativeMethods("org/qtproject/android/TestJavaClass", methods, 2);
    \endcode
*/
bool QJniEnvironment::registerNativeMethods(const char *className, JNINativeMethod methods[], int size)
{
    QJniObject classObject(className);

    if (!classObject.isValid())
        return false;

    jclass clazz = d->jniEnv->GetObjectClass(classObject.object());
    if (d->jniEnv->RegisterNatives(clazz, methods, size) < 0) {
        exceptionCheckAndClear();
        d->jniEnv->DeleteLocalRef(clazz);
        return false;
    }

    d->jniEnv->DeleteLocalRef(clazz);

    return true;
}

/*!
    \enum QJniEnvironment::OutputMode

    \value Silent The exceptions are cleaned silently
    \value Verbose Prints the exceptions and their stack backtrace as an error
           to \c stderr stream.
*/

/*!
    \fn QJniEnvironment::exceptionCheckAndClear(OutputMode outputMode = OutputMode::Verbose)

    Cleans any pending exceptions either silently or reporting stack backtrace,
    depending on the \a outputMode.

    In contrast to \l QJniObject, which handles exceptions internally, if you
    make JNI calls directly via \c JNIEnv, you need to clear any potential
    exceptions after the call using this function. For more information about
    \c JNIEnv calls that can throw an exception, see \l {Oracle: JNI Functions}{JNI Functions}.
*/
bool QJniEnvironment::exceptionCheckAndClear(QJniEnvironment::OutputMode outputMode)
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
    \fn QJniEnvironment::exceptionCheckAndClear(JNIEnv *env, OutputMode outputMode = OutputMode::Verbose)

    Cleans any pending exceptions for \a env, either silently or reporting
    stack backtrace, depending on the \a outputMode. This is useful when you
    already have a \c JNIEnv pointer such as in a native function implementation.

    In contrast to \l QJniObject, which handles exceptions internally, if you
    make JNI calls directly via \c JNIEnv, you need to clear any potential
    exceptions after the call using this function. For more information about
    \c JNIEnv calls that can throw an exception, see \l {Oracle: JNI Functions}{JNI Functions}.
*/
bool QJniEnvironment::exceptionCheckAndClear(JNIEnv *env, QJniEnvironment::OutputMode outputMode)
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
