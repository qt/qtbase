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
    \brief The QJniEnvironment provides access to the JNI Environment.
    \since 6.1

    \note This API has been tested and meant to be mainly used for Android and it hasn't been tested
    for other platforms.
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

    Constructs a new QJniEnvironment object and attaches the current thread to the Java VM.
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
    classes are not visible in the default class loader.

    Returns the class pointer or null if is not found.

    A use case for this function is searching for a custom class then calling
    its memeber method. The following code snippet create an instance of the
    class \c CustomClass and then calls \c printFromJava() method:

    \code
    QJniEnvironment env;
    jclass javaClass = env.findClass("org/qtproject/example/android/CustomClass");
    QJniObject classObject(javaClass);

    QJniObject javaMessage = QJniObject::fromString("findClass example");
    classObject.callMethod<void>("printFromJava",
                                 "(Ljava/lang/String;)V",
                                 javaMessage.object<jstring>());
    \endcode

    \since Qt 6.1
*/
jclass QJniEnvironment::findClass(const char *className)
{
    return QtAndroidPrivate::findClass(className, d->jniEnv);
}

/*!
    \fn JavaVM *QJniEnvironment::javaVM()

    Returns the Java VM interface.

    \since Qt 6.1
*/
JavaVM *QJniEnvironment::javaVM()
{
    return QtAndroidPrivate::javaVM();
}

/*!
    \fn bool QJniEnvironment::registerNativeMethods(const char *className, JNINativeMethod methods[])

    Registers the Java methods \a methods that can call native C++ functions from class \a
    className. These methods must be registered before any attempt to call them.

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
    env.registerNativeMethods("org/qtproject/android/TestJavaClass", methods);
    \endcode

    \since Qt 6.1
*/
bool QJniEnvironment::registerNativeMethods(const char *className, JNINativeMethod methods[], int size)
{
    jclass clazz = findClass(className);

    if (!clazz)
        return false;

    jclass gClazz = static_cast<jclass>(d->jniEnv->NewGlobalRef(clazz));

    if (d->jniEnv->RegisterNatives(gClazz, methods, size / sizeof(methods[0])) < 0) {
        exceptionCheckAndClear();
        return false;
    }

    d->jniEnv->DeleteLocalRef(gClazz);

    return true;
}

/*!
    \enum QJniExceptionCleaner::OutputMode

    \value Silent the exceptions are cleaned silently
    \value Verbose describes the exceptions before cleaning them
*/

/*!
    \fn QJniEnvironment::exceptionCheckAndClear(OutputMode outputMode = OutputMode::Silent)

    Cleans any pending exceptions either silently or with descriptions, depending on the \a outputMode.

    \since 6.1
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
    \fn QJniEnvironment::exceptionCheckAndClear(JNIEnv *env, OutputMode outputMode = OutputMode::Silent)

    Cleans any pending exceptions for \a env, either silently or with descriptions, depending on the \a outputMode.

    \since 6.1
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
