// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qjniobject.h"

#include "qjnihelpers_p.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qhash.h>
#include <QtCore/qreadwritelock.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QJniObject
    \inmodule QtCore
    \since 6.1
    \brief A convenience wrapper around the Java Native Interface (JNI).

    The QJniObject class wraps a reference to a Java object, ensuring it isn't
    gargage-collected and providing access to most \c JNIEnv method calls
    (member, static) and fields (setter, getter).  It eliminates much
    boiler-plate that would normally be needed, with direct JNI access, for
    every operation, including exception-handling.

    \note This API has been designed and tested for use with Android.
    It has not been tested for other platforms.

    \sa QJniEnvironment

    \section1 General Notes

    \list
        \li Class names need to be fully-qualified, for example: \c "java/lang/String".
        \li Method signatures are written as \c "(ArgumentsTypes)ReturnType", see \l {JNI Types}.
        \li All object types are returned as a QJniObject.
    \endlist

    \section1 Method Signatures

    QJniObject provides convenience functions that will use the correct signature based on the
    provided template types. For functions that only return and take \l {JNI types}, the
    signature can be generate at compile time:

    \code
    jint x = QJniObject::callMethod<jint>("getSize");
    QJniObject::callMethod<void>("touch");
    jint ret = jString1.callMethod<jint>("compareToIgnoreCase", jString2.object<jstring>());
    \endcode

    These functions are variadic templates, and the compiler will deduce the template arguments
    from the actual argument types. In many situations, only the return type needs to be provided
    explicitly.

    For functions that take other argument types, you need to supply the signature yourself. It is
    important that the signature matches the function you want to call. The example below
    demonstrates how to call different static functions:

    \code
    // Java class
    package org.qtproject.qt;
    class TestClass
    {
        static TestClass create() { ... }
        static String fromNumber(int x) { ... }
        static String[] stringArray(String s1, String s2) { ... }
    }
    \endcode

    The signature structure is \c "(ArgumentsTypes)ReturnType". Array types in the signature
    must have the \c {[} prefix, and the fully-qualified \c Object type names must have the
    \c L prefix and the \c ; suffix. The signature for the \c create function is
    \c {"()Lorg/qtproject/qt/TestClass;}. The signatures for the second and third functions
    are \c {"(I)Ljava/lang/String;"} and
    \c {"(Ljava/lang/String;Ljava/lang/String;)[Ljava/lang/String;"}, respectively.

    We can call the \c create() function like this:

    \code
    // C++ code
    QJniObject testClass = QJniObject::callStaticObjectMethod("org/qtproject/qt/TestClass",
                                                              "create",
                                                              "()Lorg/qtproject/qt/TestClass;");
    \endcode

    For the second and third function we can rely on QJniObject's template methods to create
    the implicit signature string, but we can also pass the signature string explicitly:

    \code
    // C++ code
    QJniObject stringNumber = QJniObject::callStaticObjectMethod("org/qtproject/qt/TestClass",
                                                                 "fromNumber",
                                                                 "(I)Ljava/lang/String;", 10);
    \endcode

    For the implicit signature creation to work we need to specify the return type explicitly:

    \code
    // C++ code
    QJniObject string1 = QJniObject::fromString("String1");
    QJniObject string2 = QJniObject::fromString("String2");
    QJniObject stringArray = QJniObject::callStaticObjectMethod<jstringArray>(
                                                                "org/qtproject/qt/TestClass",
                                                                "stringArray"
                                                                string1.object<jstring>(),
                                                                string2.object<jstring>());
    \endcode

    Note that while he first template parameter specifies the return type of the Java
    function, the method will still return a QJniObject.

    \section1 Handling Java Exception

    After calling Java functions that might throw exceptions, it is important
    to check for, handle and clear out any exception before continuing. All
    QJniObject functions handle exceptions internally by reporting and clearing them,
    saving client code the need to handle exceptions.

    \note The user must handle exceptions manually when doing JNI calls using \c JNIEnv directly.
    It is unsafe to make other JNI calls when exceptions are pending. For more information, see
    QJniEnvironment::checkAndClearExceptions().

    \section1 Java Native Methods

    Java native methods makes it possible to call native code from Java, this is done by creating a
    function declaration in Java and prefixing it with the \c native keyword.
    Before a native function can be called from Java, you need to map the Java native function to a
    native function in your code. Mapping functions can be done by calling
    QJniEnvironment::registerNativeMethods().

    The example below demonstrates how this could be done.

    Java implementation:
    \snippet jni/src_qjniobject.cpp Java native methods

    C++ Implementation:
    \snippet jni/src_qjniobject.cpp C++ native methods

    \section1 The Lifetime of a Java Object

    Most \l{Object types}{objects} received from Java will be local references
    and will only stay valid until you return from the native method. After that,
    the object becomes eligible for garbage collection. If your code creates
    many local references in a loop you should delete them manually with each
    iteration, otherwise you might run out of memory. For more information, see
    \l {JNI Design Overview: Global and Local References}. Local references
    created outside a native method scope must be deleted manually, since
    the garbage collector will not free them automatically because we are using
    \l {Java: AttachCurrentThread}{AttachCurrentThread}. For more information, see
    \l {JNI tips: Local and global references}.

    If you want to keep a Java object alive you need to either create a new global
    reference to the object and release it when you are done, or construct a new
    QJniObject and let it manage the lifetime of the Java object.

    \sa object()

    \note The QJniObject only manages its own references, if you construct a QJniObject from a
          global or local reference that reference will not be released by the QJniObject.

    \section1 JNI Types

    \section2 Object Types
    \table
    \header
        \li Type
        \li Signature
    \row
        \li jobject
        \li Ljava/lang/Object;
    \row
        \li jclass
        \li Ljava/lang/Class;
    \row
        \li jstring
        \li Ljava/lang/String;
    \row
        \li jthrowable
        \li Ljava/lang/Throwable;
    \row
        \li jobjectArray
        \li [Ljava/lang/Object;
    \row
        \li jarray
        \li [\e<type>
    \row
        \li jbooleanArray
        \li [Z
    \row
        \li jbyteArray
        \li [B
    \row
        \li jcharArray
        \li [C
    \row
        \li jshortArray
        \li [S
    \row
        \li jintArray
        \li [I
    \row
        \li jlongArray
        \li [J
    \row
        \li jfloatArray
        \li [F
    \row
        \li jdoubleArray
        \li [D
    \endtable

    \section2 Primitive Types
    \table
    \header
        \li Type
        \li Signature
    \row
        \li jboolean
        \li Z
    \row
        \li jbyte
        \li B
    \row
        \li jchar
        \li C
    \row
       \li jshort
       \li S
    \row
        \li jint
        \li I
    \row
        \li jlong
        \li J
    \row
        \li jfloat
        \li F
    \row
        \li jdouble
        \li D
    \endtable

    \section2 Other
    \table
    \header
        \li Type
        \li Signature
    \row
        \li void
        \li V
    \row
        \li \e{Custom type}
        \li L\e<fully-qualified-name>;
    \endtable

    For more information about JNI, see \l {Java Native Interface Specification}.
*/

/*!
    \fn bool operator==(const QJniObject &o1, const QJniObject &o2)

    \relates QJniObject

    Returns true if both objects, \a o1 and \a o2, are referencing the same Java object, or if both
    are NULL. In any other cases false will be returned.
*/

/*!
    \fn bool operator!=(const QJniObject &o1, const QJniObject &o2)
    \relates QJniObject

    Returns true if \a o1 holds a reference to a different object than \a o2.
*/

class QJniObjectPrivate
{
public:
    QJniObjectPrivate() = default;
    ~QJniObjectPrivate() {
        QJniEnvironment env;
        if (m_jobject)
            env->DeleteGlobalRef(m_jobject);
        if (m_jclass && m_own_jclass)
            env->DeleteGlobalRef(m_jclass);
    }

    friend jclass QtAndroidPrivate::findClass(const char *className, JNIEnv *env);
    static jclass loadClass(const QByteArray &className, JNIEnv *env, bool binEncoded = false)
    {
        return QJniObject::loadClass(className, env, binEncoded);
    }

    static QByteArray toBinaryEncClassName(const QByteArray &className)
    {
        return QJniObject::toBinaryEncClassName(className);
    }

    jobject m_jobject = nullptr;
    jclass m_jclass = nullptr;
    bool m_own_jclass = true;
    QByteArray m_className;
};

static inline QLatin1StringView keyBase()
{
    return "%1%2:%3"_L1;
}

static QString qt_convertJString(jstring string)
{
    QJniEnvironment env;
    int strLength = env->GetStringLength(string);
    QString res(strLength, Qt::Uninitialized);
    env->GetStringRegion(string, 0, strLength, reinterpret_cast<jchar *>(res.data()));
    return res;
}

typedef QHash<QString, jclass> JClassHash;
Q_GLOBAL_STATIC(JClassHash, cachedClasses)
Q_GLOBAL_STATIC(QReadWriteLock, cachedClassesLock)

static jclass getCachedClass(const QByteArray &classBinEnc, bool *isCached = nullptr)
{
    QReadLocker locker(cachedClassesLock);
    const QHash<QString, jclass>::const_iterator &it = cachedClasses->constFind(QString::fromLatin1(classBinEnc));
    const bool found = (it != cachedClasses->constEnd());

    if (isCached)
        *isCached = found;

    return found ? it.value() : 0;
}

QByteArray QJniObject::toBinaryEncClassName(const QByteArray &className)
{
    return QByteArray(className).replace('/', '.');
}

jclass QJniObject::loadClass(const QByteArray &className, JNIEnv *env, bool binEncoded)
{
    const QByteArray &binEncClassName = binEncoded ? className : QJniObject::toBinaryEncClassName(className);

    bool isCached = false;
    jclass clazz = getCachedClass(binEncClassName, &isCached);
    if (clazz || isCached)
        return clazz;

    QJniObject classLoader(QtAndroidPrivate::classLoader());
    if (!classLoader.isValid())
        return nullptr;

    QWriteLocker locker(cachedClassesLock);
    // did we lose the race?
    const QLatin1StringView key(binEncClassName);
    const QHash<QString, jclass>::const_iterator &it = cachedClasses->constFind(key);
    if (it != cachedClasses->constEnd())
        return it.value();

    QJniObject stringName = QJniObject::fromString(key);
    QJniObject classObject = classLoader.callObjectMethod("loadClass",
                                                          "(Ljava/lang/String;)Ljava/lang/Class;",
                                                          stringName.object());

    if (!QJniEnvironment::checkAndClearExceptions(env) && classObject.isValid())
        clazz = static_cast<jclass>(env->NewGlobalRef(classObject.object()));

    cachedClasses->insert(key, clazz);
    return clazz;
}

typedef QHash<QString, jmethodID> JMethodIDHash;
Q_GLOBAL_STATIC(JMethodIDHash, cachedMethodID)
Q_GLOBAL_STATIC(QReadWriteLock, cachedMethodIDLock)

jmethodID QJniObject::getMethodID(JNIEnv *env,
                                    jclass clazz,
                                    const char *name,
                                    const char *signature,
                                    bool isStatic)
{
    jmethodID id = isStatic ? env->GetStaticMethodID(clazz, name, signature)
                            : env->GetMethodID(clazz, name, signature);

    if (QJniEnvironment::checkAndClearExceptions(env))
        return nullptr;

    return id;
}

void QJniObject::callVoidMethodV(JNIEnv *env, jmethodID id, ...) const
{
    va_list args;
    va_start(args, id);
    callVoidMethodV(env, id, args);
    va_end(args);
}

void QJniObject::callVoidMethodV(JNIEnv *env, jmethodID id, va_list args) const
{
    env->CallVoidMethodV(d->m_jobject, id, args);
}

jmethodID QJniObject::getCachedMethodID(JNIEnv *env,
                                        jclass clazz,
                                        const QByteArray &className,
                                        const char *name,
                                        const char *signature,
                                        bool isStatic)
{
    if (className.isEmpty())
        return getMethodID(env, clazz, name, signature, isStatic);

    const QString key = keyBase().arg(QLatin1StringView(className),
                                      QLatin1StringView(name),
                                      QLatin1StringView(signature));
    QHash<QString, jmethodID>::const_iterator it;

    {
        QReadLocker locker(cachedMethodIDLock);
        it = cachedMethodID->constFind(key);
        if (it != cachedMethodID->constEnd())
            return it.value();
    }

    {
        QWriteLocker locker(cachedMethodIDLock);
        it = cachedMethodID->constFind(key);
        if (it != cachedMethodID->constEnd())
            return it.value();

        jmethodID id = getMethodID(env, clazz, name, signature, isStatic);

        cachedMethodID->insert(key, id);
        return id;
    }
}

jmethodID QJniObject::getCachedMethodID(JNIEnv *env, const char *name,
                                        const char *signature, bool isStatic) const
{
    return QJniObject::getCachedMethodID(env, d->m_jclass, d->m_className, name, signature, isStatic);
}

typedef QHash<QString, jfieldID> JFieldIDHash;
Q_GLOBAL_STATIC(JFieldIDHash, cachedFieldID)
Q_GLOBAL_STATIC(QReadWriteLock, cachedFieldIDLock)

jfieldID QJniObject::getFieldID(JNIEnv *env,
                                jclass clazz,
                                const char *name,
                                const char *signature,
                                bool isStatic)
{
    jfieldID id = isStatic ? env->GetStaticFieldID(clazz, name, signature)
                           : env->GetFieldID(clazz, name, signature);

    if (QJniEnvironment::checkAndClearExceptions(env))
        return nullptr;

    return id;
}

jfieldID QJniObject::getCachedFieldID(JNIEnv *env,
                                      jclass clazz,
                                      const QByteArray &className,
                                      const char *name,
                                      const char *signature,
                                      bool isStatic)
{
    if (className.isNull())
        return getFieldID(env, clazz, name, signature, isStatic);

    const QString key = keyBase().arg(QLatin1StringView(className),
                                      QLatin1StringView(name),
                                      QLatin1StringView(signature));
    QHash<QString, jfieldID>::const_iterator it;

    {
        QReadLocker locker(cachedFieldIDLock);
        it = cachedFieldID->constFind(key);
        if (it != cachedFieldID->constEnd())
            return it.value();
    }

    {
        QWriteLocker locker(cachedFieldIDLock);
        it = cachedFieldID->constFind(key);
        if (it != cachedFieldID->constEnd())
            return it.value();

        jfieldID id = getFieldID(env, clazz, name, signature, isStatic);

        cachedFieldID->insert(key, id);
        return id;
    }
}

jfieldID QJniObject::getCachedFieldID(JNIEnv *env,
                                      const char *name,
                                      const char *signature,
                                      bool isStatic) const
{
    return QJniObject::getCachedFieldID(env, d->m_jclass, d->m_className, name, signature, isStatic);
}

jclass QtAndroidPrivate::findClass(const char *className, JNIEnv *env)
{
    const QByteArray &classDotEnc = QJniObjectPrivate::toBinaryEncClassName(className);
    bool isCached = false;
    jclass clazz = getCachedClass(classDotEnc, &isCached);

    if (clazz || isCached)
        return clazz;

    const QLatin1StringView key(classDotEnc);
    if (env) { // We got an env. pointer (We expect this to be the right env. and call FindClass())
        QWriteLocker locker(cachedClassesLock);
        const QHash<QString, jclass>::const_iterator &it = cachedClasses->constFind(key);
        // Did we lose the race?
        if (it != cachedClasses->constEnd())
            return it.value();

        jclass fclazz = env->FindClass(className);
        if (!QJniEnvironment::checkAndClearExceptions(env)) {
            clazz = static_cast<jclass>(env->NewGlobalRef(fclazz));
            env->DeleteLocalRef(fclazz);
        }

        if (clazz)
            cachedClasses->insert(key, clazz);
    }

    if (!clazz) // We didn't get an env. pointer or we got one with the WRONG class loader...
        clazz = QJniObjectPrivate::loadClass(classDotEnc, QJniEnvironment().jniEnv(), true);

    return clazz;
}

/*!
    \fn QJniObject::QJniObject()

    Constructs an invalid JNI object.

    \sa isValid()
*/
QJniObject::QJniObject()
    : d(new QJniObjectPrivate())
{
}

/*!
    \fn QJniObject::QJniObject(const char *className)

    Constructs a new JNI object by calling the default constructor of \a className.

    \code
    QJniObject myJavaString("java/lang/String");
    \endcode
*/
QJniObject::QJniObject(const char *className)
    : d(new QJniObjectPrivate())
{
    QJniEnvironment env;
    d->m_className = toBinaryEncClassName(className);
    d->m_jclass = loadClass(d->m_className, env.jniEnv(), true);
    d->m_own_jclass = false;
    if (d->m_jclass) {
        // get default constructor
        jmethodID constructorId = getCachedMethodID(env.jniEnv(), "<init>", "()V");
        if (constructorId) {
            jobject obj = env->NewObject(d->m_jclass, constructorId);
            if (obj) {
                d->m_jobject = env->NewGlobalRef(obj);
                env->DeleteLocalRef(obj);
            }
        }
    }
}

/*!
    \fn QJniObject::QJniObject(const char *className, const char *signature, ...)

    Constructs a new JNI object by calling the constructor of \a className with
    \a signature specifying the types of any subsequent arguments.

    \code
    QJniEnvironment env;
    char* str = "Hello";
    jstring myJStringArg = env->NewStringUTF(str);
    QJniObject myNewJavaString("java/lang/String", "(Ljava/lang/String;)V", myJStringArg);
    \endcode
*/
QJniObject::QJniObject(const char *className, const char *signature, ...)
    : d(new QJniObjectPrivate())
{
    QJniEnvironment env;
    d->m_className = toBinaryEncClassName(className);
    d->m_jclass = loadClass(d->m_className, env.jniEnv(), true);
    d->m_own_jclass = false;
    if (d->m_jclass) {
        jmethodID constructorId = getCachedMethodID(env.jniEnv(), "<init>", signature);
        if (constructorId) {
            va_list args;
            va_start(args, signature);
            jobject obj = env->NewObjectV(d->m_jclass, constructorId, args);
            va_end(args);
            if (obj) {
                d->m_jobject = env->NewGlobalRef(obj);
                env->DeleteLocalRef(obj);
            }
        }
    }
}

/*!
    \fn template<typename ...Args> QJniObject::QJniObject(const char *className, Args &&...args)
    \since 6.4

    Constructs a new JNI object by calling the constructor of \a className with
    the arguments \a args. This constructor is only available if all \a args are
    known \l {JNI Types}.

    \code
    QJniEnvironment env;
    char* str = "Hello";
    jstring myJStringArg = env->NewStringUTF(str);
    QJniObject myNewJavaString("java/lang/String", myJStringArg);
    \endcode
*/

QJniObject::QJniObject(const char *className, const char *signature, const QVaListPrivate &args)
    : d(new QJniObjectPrivate())
{
    QJniEnvironment env;
    d->m_className = toBinaryEncClassName(className);
    d->m_jclass = loadClass(d->m_className, env.jniEnv(), true);
    d->m_own_jclass = false;
    if (d->m_jclass) {
        jmethodID constructorId = getCachedMethodID(env.jniEnv(), "<init>", signature);
        if (constructorId) {
            jobject obj = env->NewObjectV(d->m_jclass, constructorId, args);
            if (obj) {
                d->m_jobject = env->NewGlobalRef(obj);
                env->DeleteLocalRef(obj);
            }
        }
    }
}

/*!
    Constructs a new JNI object from \a clazz by calling the constructor with
    \a signature specifying the types of any subsequent arguments.

    \code
    QJniEnvironment env;
    jclass myClazz = env.findClass("org/qtproject/qt/TestClass");
    QJniObject(myClazz, "(I)V", 3);
    \endcode
*/
QJniObject::QJniObject(jclass clazz, const char *signature, ...)
    : d(new QJniObjectPrivate())
{
    QJniEnvironment env;
    if (clazz) {
        d->m_jclass = static_cast<jclass>(env->NewGlobalRef(clazz));
        if (d->m_jclass) {
            jmethodID constructorId = getMethodID(env.jniEnv(), d->m_jclass, "<init>", signature);
            if (constructorId) {
                va_list args;
                va_start(args, signature);
                jobject obj = env->NewObjectV(d->m_jclass, constructorId, args);
                va_end(args);
                if (obj) {
                    d->m_jobject = env->NewGlobalRef(obj);
                    env->DeleteLocalRef(obj);
                }
            }
        }
    }
}

/*!
    \fn template<typename ...Args> QJniObject::QJniObject(jclass clazz, Args &&...args)
    \since 6.4

    Constructs a new JNI object from \a clazz by calling the constructor with
    the arguments \a args. This constructor is only available if all \a args are
    known \l {JNI Types}.

    \code
    QJniEnvironment env;
    jclass myClazz = env.findClass("org/qtproject/qt/TestClass");
    QJniObject(myClazz, 3);
    \endcode
*/

/*!
    Constructs a new JNI object by calling the default constructor of \a clazz.

    \note The QJniObject will create a new reference to the class \a clazz
          and releases it again when it is destroyed. References to the class created
          outside the QJniObject need to be managed by the caller.
*/

QJniObject::QJniObject(jclass clazz)
    : d(new QJniObjectPrivate())
{
    QJniEnvironment env;
    d->m_jclass = static_cast<jclass>(env->NewGlobalRef(clazz));
    if (d->m_jclass) {
        // get default constructor
        jmethodID constructorId = getMethodID(env.jniEnv(), d->m_jclass, "<init>", "()V");
        if (constructorId) {
            jobject obj = env->NewObject(d->m_jclass, constructorId);
            if (obj) {
                d->m_jobject = env->NewGlobalRef(obj);
                env->DeleteLocalRef(obj);
            }
        }
    }
}

QJniObject::QJniObject(jclass clazz, const char *signature, const QVaListPrivate &args)
    : d(new QJniObjectPrivate())
{
    QJniEnvironment env;
    if (clazz) {
        d->m_jclass = static_cast<jclass>(env->NewGlobalRef(clazz));
        if (d->m_jclass) {
            jmethodID constructorId = getMethodID(env.jniEnv(), d->m_jclass, "<init>", signature);
            if (constructorId) {
                jobject obj = env->NewObjectV(d->m_jclass, constructorId, args);
                if (obj) {
                    d->m_jobject = env->NewGlobalRef(obj);
                    env->DeleteLocalRef(obj);
                }
            }
        }
    }
}

/*!
    Constructs a new JNI object around the Java object \a object.

    \note The QJniObject will hold a reference to the Java object \a object
    and release it when destroyed. Any references to the Java object \a object
    outside QJniObject needs to be managed by the caller. In most cases you
    should never call this function with a local reference unless you intend
    to manage the local reference yourself. See QJniObject::fromLocalRef()
    for converting a local reference to a QJniObject.

    \sa fromLocalRef()
*/
QJniObject::QJniObject(jobject object)
    : d(new QJniObjectPrivate())
{
    if (!object)
        return;

    QJniEnvironment env;
    d->m_jobject = env->NewGlobalRef(object);
    jclass cls = env->GetObjectClass(object);
    d->m_jclass = static_cast<jclass>(env->NewGlobalRef(cls));
    env->DeleteLocalRef(cls);
}

/*!
    \fn template<typename Class, typename ...Args> static inline QJniObject QJniObject::construct(Args &&...args)
    \since 6.4

    Constructs an instance of the Java class that is the equivalent of \c Class and
    returns a QJniObject containing the JNI object. The arguments in \a args are
    passed to the Java constructor.

    \code
    QJniObject javaString = QJniObject::construct<jstring>();
    \endcode

    This function is only available if all \a args are known \l {JNI Types}.
*/

/*!
    \brief Get a JNI object from a jobject variant and do the necessary
    exception clearing and delete the local reference before returning.
    The JNI object can be null if there was an exception.
*/
QJniObject QJniObject::getCleanJniObject(jobject object)
{
    if (!object)
        return QJniObject();

    QJniEnvironment env;
    if (env.checkAndClearExceptions()) {
        env->DeleteLocalRef(object);
        return QJniObject();
    }

    QJniObject res(object);
    env->DeleteLocalRef(object);
    return res;
}

/*!
    \fn QJniObject::~QJniObject()

    Destroys the JNI object and releases any references held by the JNI object.
*/
QJniObject::~QJniObject()
{}

/*!
    \fn template <typename T> T QJniObject::object() const

    Returns the object held by the QJniObject either as jobject or as type T.
    T can be one of \l {Object Types}{JNI Object Types}.

    \code
    QJniObject string = QJniObject::fromString("Hello, JNI");
    jstring jstring = string.object<jstring>();
    \endcode

    \note The returned object is still kept alive by this QJniObject. To keep the
    object alive beyond the lifetime of this QJniObject, for example to record it
    for later use, the easiest approach is to store it in another QJniObject with
    a suitable lifetime. Alternatively, you may create a new global reference to the
    object and store it, taking care to free it when you are done with it.

    \snippet jni/src_qjniobject.cpp QJniObject scope
*/
jobject QJniObject::object() const
{
    return javaObject();
}

/*!
    \fn jclass QJniObject::objectClass() const

    Returns the class object held by the QJniObject as a \c jclass.

    \note The returned object is still kept alive by this QJniObject. To keep the
    object alive beyond the lifetime of this QJniObject, for example to record it
    for later use, the easiest approach is to store it in another QJniObject with
    a suitable lifetime. Alternatively, you may create a new global reference to the
    object and store it, taking care to free it when you are done with it.

    \since 6.2
*/
jclass QJniObject::objectClass() const
{
    return d->m_jclass;
}

/*!
    \fn QByteArray QJniObject::className() const

    Returns the name of the class object held by the QJniObject as a \c QByteArray.

    \since 6.2
*/
QByteArray QJniObject::className() const
{
    return d->m_className;
}

/*!
    \fn template <typename Ret, typename ...Args> auto QJniObject::callMethod(const char *methodName, const char *signature, Args &&...args) const
    \since 6.4

    Calls the object's method \a methodName with \a signature specifying the types of any
    subsequent arguments \a args, and returns the value (unless \c Ret is \c void). If \c Ret
    is a jobject type, then the returned value will be a QJniObject.

    \code
    QJniObject myJavaStrin("org/qtproject/qt/TestClass");
    jint index = myJavaString.callMethod<jint>("indexOf", "(I)I", 0x0051);
    \endcode
*/

/*!
    \fn template <typename Ret, typename ...Args> auto QJniObject::callMethod(const char *methodName, Args &&...args) const
    \since 6.4

    Calls the method \a methodName with arguments \a args and returns the value
    (unless \c Ret is \c void). If \c Ret is a jobject type, then the returned value
    will be a QJniObject.

    \code
    QJniObject myJavaStrin("org/qtproject/qt/TestClass");
    jint size = myJavaString.callMethod<jint>("length");
    \endcode

    The method signature is deduced at compile time from \c Ret and the types of \a args.
*/

/*!
    \fn template <typename Ret, typename ...Args> auto QJniObject::callStaticMethod(const char *className, const char *methodName, const char *signature, Args &&...args)
    \since 6.4

    Calls the static method \a methodName from class \a className with \a signature
    specifying the types of any subsequent arguments \a args. Returns the result of
    the method (unless \c Ret is \c void). If \c Ret is a jobject type, then the
    returned value will be a QJniObject.

    \code
    jint a = 2;
    jint b = 4;
    jint max = QJniObject::callStaticMethod<jint>("java/lang/Math", "max", "(II)I", a, b);
    \endcode
*/

/*!
    \fn template <typename Ret, typename ...Args> auto QJniObject::callStaticMethod(const char *className, const char *methodName, Args &&...args)
    \since 6.4

    Calls the static method \a methodName on class \a className with arguments \a args,
    and returns the value of type \c Ret (unless \c Ret is \c void). If \c Ret
    is a jobject type, then the returned value will be a QJniObject.

    \code
    jint value = QJniObject::callStaticMethod<jint>("MyClass", "staticMethod");
    \endcode

    The method signature is deduced at compile time from \c Ret and the types of \a args.
*/

/*!
    \fn template <typename Ret, typename ...Args> auto QJniObject::callStaticMethod(jclass clazz, const char *methodName, const char *signature, Args &&...args)

    Calls the static method \a methodName from \a clazz with \a signature
    specifying the types of any subsequent arguments. Returns the result of
    the method (unless \c Ret is \c void). If \c Ret is a jobject type, then the
    returned value will be a QJniObject.

    \code
    QJniEnvironment env;
    jclass javaMathClass = env.findClass("java/lang/Math");
    jint a = 2;
    jint b = 4;
    jint max = QJniObject::callStaticMethod<jint>(javaMathClass, "max", "(II)I", a, b);
    \endcode
*/

/*!
    \fn template <typename Ret, typename ...Args> auto QJniObject::callStaticMethod(jclass clazz, jmethodID methodId, Args &&...args)
    \since 6.4

    Calls the static method identified by \a methodId from the class \a clazz
    with any subsequent arguments, and returns the value of type \c Ret (unless
    \c Ret is \c void). If \c Ret is a jobject type, then the returned value will
    be a QJniObject.

    Useful when \a clazz and \a methodId are already cached from previous operations.

    \code
    QJniEnvironment env;
    jclass javaMathClass = env.findClass("java/lang/Math");
    jmethodID methodId = env.findStaticMethod(javaMathClass, "max", "(II)I");
    if (methodId != 0) {
        jint a = 2;
        jint b = 4;
        jint max = QJniObject::callStaticMethod<jint>(javaMathClass, methodId, a, b);
    }
    \endcode
*/

/*!
    \fn template <typename Ret, typename ...Args> auto QJniObject::callStaticMethod(jclass clazz, const char *methodName, Args &&...args)
    \since 6.4

    Calls the static method \a methodName on \a clazz and returns the value of type \c Ret
    (unless c Ret is \c void).  If \c Ret if a jobject type, then the returned value will
    be a QJniObject.

    \code
    QJniEnvironment env;
    jclass javaMathClass = env.findClass("java/lang/Math");
    jdouble randNr = QJniObject::callStaticMethod<jdouble>(javaMathClass, "random");
    \endcode

    The method signature is deduced at compile time from \c Ret and the types of \a args.
*/

/*!
    \fn QJniObject QJniObject::callObjectMethod(const char *methodName, const char *signature, ...) const

    Calls the Java object's method \a methodName with \a signature specifying
    the types of any subsequent arguments.

    \code
    QJniObject myJavaString = QJniObject::fromString("Hello, Java");
    QJniObject mySubstring = myJavaString.callObjectMethod("substring",
                                                           "(II)Ljava/lang/String;", 7, 11);
    \endcode
*/
QJniObject QJniObject::callObjectMethod(const char *methodName, const char *signature, ...) const
{
    QJniEnvironment env;
    jmethodID id = getCachedMethodID(env.jniEnv(), methodName, signature);
    if (id) {
        va_list args;
        va_start(args, signature);
        QJniObject res = getCleanJniObject(env->CallObjectMethodV(d->m_jobject, id, args));
        va_end(args);
        return res;
    }

    return QJniObject();
}

/*!
    \fn QJniObject QJniObject::callStaticObjectMethod(const char *className, const char *methodName, const char *signature, ...)

    Calls the static method \a methodName from the class \a className with \a signature
    specifying the types of any subsequent arguments.

    \code
    QJniObject thread = QJniObject::callStaticObjectMethod("java/lang/Thread", "currentThread",
                                                           "()Ljava/lang/Thread;");
    QJniObject string = QJniObject::callStaticObjectMethod("java/lang/String", "valueOf",
                                                           "(I)Ljava/lang/String;", 10);
    \endcode
*/
QJniObject QJniObject::callStaticObjectMethod(const char *className, const char *methodName,
                                              const char *signature, ...)
{
    QJniEnvironment env;
    jclass clazz = QJniObject::loadClass(className, env.jniEnv());
    if (clazz) {
        jmethodID id = QJniObject::getCachedMethodID(env.jniEnv(), clazz,
                                         QJniObject::toBinaryEncClassName(className),
                                         methodName, signature, true);
        if (id) {
            va_list args;
            va_start(args, signature);
            QJniObject res = getCleanJniObject(env->CallStaticObjectMethodV(clazz, id, args));
            va_end(args);
            return res;
        }
    }

    return QJniObject();
}

/*!
    \fn QJniObject QJniObject::callStaticObjectMethod(jclass clazz, const char *methodName, const char *signature, ...)

    Calls the static method \a methodName from class \a clazz with \a signature
    specifying the types of any subsequent arguments.
*/
QJniObject QJniObject::callStaticObjectMethod(jclass clazz, const char *methodName,
                                              const char *signature, ...)
{
    QJniEnvironment env;
    if (clazz) {
        jmethodID id = getMethodID(env.jniEnv(), clazz, methodName, signature, true);
        if (id) {
            va_list args;
            va_start(args, signature);
            QJniObject res = getCleanJniObject(env->CallStaticObjectMethodV(clazz, id, args));
            va_end(args);
            return res;
        }
    }

    return QJniObject();
}

/*!
    \fn QJniObject QJniObject::callStaticObjectMethod(jclass clazz, jmethodID methodId, ...)

    Calls the static method identified by \a methodId from the class \a clazz
    with any subsequent arguments. Useful when \a clazz and \a methodId are
    already cached from previous operations.

    \code
    QJniEnvironment env;
    jclass clazz = env.findClass("java/lang/String");
    jmethodID methodId = env.findStaticMethod(clazz, "valueOf", "(I)Ljava/lang/String;");
    if (methodId != 0)
        QJniObject str = QJniObject::callStaticObjectMethod(clazz, methodId, 10);
    \endcode
*/
QJniObject QJniObject::callStaticObjectMethod(jclass clazz, jmethodID methodId, ...)
{
    QJniEnvironment env;
    if (clazz && methodId) {
        va_list args;
        va_start(args, methodId);
        QJniObject res = getCleanJniObject(env->CallStaticObjectMethodV(clazz, methodId, args));
        va_end(args);
        return res;
    }

    return QJniObject();
}

/*!
    \fn template<typename Ret, typename ...Args> QJniObject QJniObject::callObjectMethod(const char *methodName, Args &&...args) const
    \since 6.4

    Calls the Java objects method \a methodName with arguments \a args and returns a
    new QJniObject for the returned Java object.

    \code
    QJniObject myJavaString = QJniObject::fromString("Hello, Java");
    QJniObject myJavaString2 = myJavaString1.callObjectMethod<jstring>("toString");
    \endcode

    The method signature is deduced at compile time from \c Ret and the types of \a args.
*/

/*!
    \fn template<typename Ret, typename ...Args> QJniObject QJniObject::callStaticObjectMethod(const char *className, const char *methodName, Args &&...args)
    \since 6.4

    Calls the static method with \a methodName on the class \a className, passing
    arguments \a args, and returns a new QJniObject for the returned Java object.

    \code
    QJniObject string = QJniObject::callStaticObjectMethod<jstring>("CustomClass", "getClassName");
    \endcode

    The method signature is deduced at compile time from \c Ret and the types of \a args.
*/

/*!
    \fn template<typename Ret, typename ...Args> QJniObject QJniObject::callStaticObjectMethod(jclass clazz, const char *methodName, Args &&...args)
    \since 6.4

    Calls the static method with \a methodName on \a clazz, passing arguments \a args,
    and returns a new QJniObject for the returned Java object.
*/

/*!
    \fn template <typename T> QJniObject &QJniObject::operator=(T object)

    Replace the current object with \a object. The old Java object will be released.
*/

/*!
    \fn template <typename T> void QJniObject::setStaticField(const char *className, const char *fieldName, const char *signature, T value);

    Sets the static field \a fieldName on the class \a className to \a value
    using the setter with \a signature.

*/

/*!
    \fn template <typename T> void QJniObject::setStaticField(jclass clazz, const char *fieldName, const char *signature, T value);

    Sets the static field \a fieldName on the class \a clazz to \a value using
    the setter with \a signature.
*/

/*!
    \fn T QJniObject::getField(const char *fieldName) const

    Retrieves the value of the field \a fieldName.

    \code
    QJniObject volumeControl("org/qtproject/qt/TestClass");
    jint fieldValue = volumeControl.getField<jint>("FIELD_NAME");
    \endcode
*/

/*!
    \fn T QJniObject::getStaticField(const char *className, const char *fieldName)

    Retrieves the value from the static field \a fieldName on the class \a className.
*/

/*!
    \fn T QJniObject::getStaticField(jclass clazz, const char *fieldName)

    Retrieves the value from the static field \a fieldName on \a clazz.
*/

/*!
    \fn template <typename Klass, typename T> auto QJniObject::getStaticField(const char *fieldName)

    Retrieves the value from the static field \a fieldName for the class \c Klass.

    \c Klass needs to be a C++ type with a registered type mapping to a Java type.
*/

/*!
    \fn template <typename T> void QJniObject::setStaticField(const char *className, const char *fieldName, T value)

    Sets the static field \a fieldName of the class \a className to \a value.
*/

/*!
    \fn template <typename T> void QJniObject::setStaticField(jclass clazz, const char *fieldName, T value)

    Sets the static field \a fieldName of the class \a clazz to \a value.
*/

/*!
    \fn template <typename Klass, typename T> auto QJniObject::setStaticField(const char *fieldName, T value)

    Sets the static field \a fieldName of the class \c Klass to \a value.

    \c Klass needs to be a C++ type with a registered type mapping to a Java type.
*/

/*!
    \fn QJniObject QJniObject::getStaticObjectField(const char *className, const char *fieldName, const char *signature)

    Retrieves a JNI object from the field \a fieldName with \a signature from
    class \a className.

    \note This function can be used without a template type.

    \code
    QJniObject jobj = QJniObject::getStaticObjectField("class/with/Fields", "FIELD_NAME",
                                                       "Ljava/lang/String;");
    \endcode
*/
QJniObject QJniObject::getStaticObjectField(const char *className,
                                            const char *fieldName,
                                            const char *signature)
{
    QJniEnvironment env;
    jclass clazz = QJniObject::loadClass(className, env.jniEnv());
    if (!clazz)
        return QJniObject();
    jfieldID id = QJniObject::getCachedFieldID(env.jniEnv(), clazz,
                                   QJniObject::toBinaryEncClassName(className),
                                   fieldName,
                                   signature, true);
    if (!id)
        return QJniObject();

    return getCleanJniObject(env->GetStaticObjectField(clazz, id));
}

/*!
    \fn QJniObject QJniObject::getStaticObjectField(jclass clazz, const char *fieldName, const char *signature)

    Retrieves a JNI object from the field \a fieldName with \a signature from
    class \a clazz.

    \note This function can be used without a template type.

    \code
    QJniObject jobj = QJniObject::getStaticObjectField(clazz, "FIELD_NAME", "Ljava/lang/String;");
    \endcode
*/
QJniObject QJniObject::getStaticObjectField(jclass clazz, const char *fieldName,
                                            const char *signature)
{
    QJniEnvironment env;
    jfieldID id = getFieldID(env.jniEnv(), clazz, fieldName, signature, true);
    if (!id)
        return QJniObject();

    return getCleanJniObject(env->GetStaticObjectField(clazz, id));
}

/*!
    \fn template <typename T> void QJniObject::setField(const char *fieldName, const char *signature, T value)

    Sets the value of \a fieldName with \a signature to \a value.

    \code
    QJniObject stringArray = ...;
    QJniObject obj = ...;
    obj.setObjectField<jobjectArray>("KEY_VALUES", "([Ljava/lang/String;)V",
                               stringArray.object<jobjectArray>())
    \endcode
*/

/*!
    \fn QJniObject QJniObject::getObjectField(const char *fieldName) const

    Retrieves a JNI object from the field \a fieldName.

    \code
    QJniObject field = jniObject.getObjectField<jstring>("FIELD_NAME");
    \endcode
*/

/*!
    \fn QJniObject QJniObject::getObjectField(const char *fieldName, const char *signature) const

    Retrieves a JNI object from the field \a fieldName with \a signature.

    \note This function can be used without a template type.

    \code
    QJniObject field = jniObject.getObjectField("FIELD_NAME", "Ljava/lang/String;");
    \endcode
*/
QJniObject QJniObject::getObjectField(const char *fieldName, const char *signature) const
{
    QJniEnvironment env;
    jfieldID id = getCachedFieldID(env.jniEnv(), fieldName, signature);
    if (!id)
        return QJniObject();

    return getCleanJniObject(env->GetObjectField(d->m_jobject, id));
}

/*!
    \fn template <typename T> void QJniObject::setField(const char *fieldName, T value)

    Sets the value of \a fieldName to \a value.

    \code
    QJniObject obj;
    obj.setField<jint>("AN_INT_FIELD", 10);
    jstring myString = ...;
    obj.setField<jstring>("A_STRING_FIELD", myString);
    \endcode
*/

/*!
    \fn QJniObject QJniObject::getStaticObjectField(const char *className, const char *fieldName)

    Retrieves the object from the field \a fieldName on the class \a className.

    \code
    QJniObject jobj = QJniObject::getStaticObjectField<jstring>("class/with/Fields", "FIELD_NAME");
    \endcode
*/

/*!
    \fn QJniObject QJniObject::getStaticObjectField(jclass clazz, const char *fieldName)

    Retrieves the object from the field \a fieldName on \a clazz.

    \code
    QJniObject jobj = QJniObject::getStaticObjectField<jstring>(clazz, "FIELD_NAME");
    \endcode
*/

/*!
    \fn QJniObject QJniObject::fromString(const QString &string)

    Creates a Java string from the QString \a string and returns a QJniObject holding that string.

    \code
    QString myQString = "QString";
    QJniObject myJavaString = QJniObject::fromString(myQString);
    \endcode

    \sa toString()
*/
QJniObject QJniObject::fromString(const QString &string)
{
    QJniEnvironment env;
    return getCleanJniObject(env->NewString(reinterpret_cast<const jchar*>(string.constData()),
                                                                           string.length()));
}

/*!
    \fn QString QJniObject::toString() const

    Returns a QString with a string representation of the java object.
    Calling this function on a Java String object is a convenient way of getting the actual string
    data.

    \code
    QJniObject string = ...; //  "Hello Java"
    QString qstring = string.toString(); // "Hello Java"
    \endcode

    \sa fromString()
*/
QString QJniObject::toString() const
{
    if (!isValid())
        return QString();

    QJniObject string = callObjectMethod<jstring>("toString");
    return qt_convertJString(static_cast<jstring>(string.object()));
}

/*!
    \fn bool QJniObject::isClassAvailable(const char *className)

    Returns true if the Java class \a className is available.

    \code
    if (QJniObject::isClassAvailable("java/lang/String")) {
        // condition statement
    }
    \endcode
*/
bool QJniObject::isClassAvailable(const char *className)
{
    QJniEnvironment env;

    if (!env.jniEnv())
        return false;

    return loadClass(className, env.jniEnv());;
}

/*!
    \fn bool QJniObject::isValid() const

    Returns true if this instance holds a valid Java object.

    \code
    QJniObject qjniObject;                        // ==> isValid() == false
    QJniObject qjniObject(0)                      // ==> isValid() == false
    QJniObject qjniObject("could/not/find/Class") // ==> isValid() == false
    \endcode
*/
bool QJniObject::isValid() const
{
    return d->m_jobject;
}

/*!
    \fn QJniObject QJniObject::fromLocalRef(jobject localRef)

    Creates a QJniObject from the local JNI reference \a localRef.
    This function takes ownership of \a localRef and frees it before returning.

    \note Only call this function with a local JNI reference. For example, most raw JNI calls,
    through the JNI environment, return local references to a java object.

    \code
    jobject localRef = env->GetObjectArrayElement(array, index);
    QJniObject element = QJniObject::fromLocalRef(localRef);
    \endcode
*/
QJniObject QJniObject::fromLocalRef(jobject lref)
{
    QJniObject obj(lref);
    QJniEnvironment()->DeleteLocalRef(lref);
    return obj;
}

bool QJniObject::isSameObject(jobject obj) const
{
    return QJniEnvironment()->IsSameObject(d->m_jobject, obj);
}

bool QJniObject::isSameObject(const QJniObject &other) const
{
    return isSameObject(other.d->m_jobject);
}

void QJniObject::assign(jobject obj)
{
    if (isSameObject(obj))
        return;

    jobject jobj = static_cast<jobject>(obj);
    d = QSharedPointer<QJniObjectPrivate>::create();
    if (obj) {
        QJniEnvironment env;
        d->m_jobject = env->NewGlobalRef(jobj);
        jclass objectClass = env->GetObjectClass(jobj);
        d->m_jclass = static_cast<jclass>(env->NewGlobalRef(objectClass));
        env->DeleteLocalRef(objectClass);
    }
}

jobject QJniObject::javaObject() const
{
    return d->m_jobject;
}

QT_END_NAMESPACE
