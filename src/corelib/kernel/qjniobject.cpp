// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qjniobject.h"

#include "qjnihelpers_p.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qhash.h>
#include <QtCore/qreadwritelock.h>
#include <QtCore/qloggingcategory.h>


QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcJniThreadCheck, "qt.core.jni.threadcheck")

using namespace Qt::StringLiterals;

/*!
    \class QJniObject
    \inmodule QtCore
    \since 6.1
    \brief A convenience wrapper around the Java Native Interface (JNI).

    The QJniObject class wraps a reference to a Java object, ensuring it isn't
    garbage-collected and providing access to most \c JNIEnv method calls
    (member, static) and fields (setter, getter).  It eliminates much
    boiler-plate that would normally be needed, with direct JNI access, for
    every operation, including exception-handling.

    \note This API has been designed and tested for use with Android.
    It has not been tested for other platforms.

    \sa QJniEnvironment

    \section1 Method Signatures

    QJniObject provides convenience functions that will use the correct signature based on the
    provided or deduced template arguments.

    \code
    jint x = QJniObject::callMethod<jint>("getSize");
    QJniObject::callMethod<void>("touch");
    jint ret = jString1.callMethod<jint>("compareToIgnoreCase", jString2.object<jstring>());
    \endcode

    These functions are variadic templates, and the compiler will deduce the
    signature from the actual argument types. Only the return type needs to be
    provided explicitly. QJniObject can deduce the signature string for
    functions that take \l {JNI types}, and for types that have been declared
    with the QtJniTypes type mapping.

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

    \code
    // C++ code
    Q_DECLARE_JNI_CLASS(TestClass, "org/qtproject/qt/TestClass")

    // ...
    using namespace QtJniTypes;
    TestClass testClass = TestClass::callStaticMethod<TestClass>("create");
    \endcode

    This allows working with arbitrary Java and Android types in C++ code, without having to
    create JNI signature strings explicitly.

    \section2 Explicit JNI Signatures

    It is possible to supply the signature yourself. In that case, it is important
    that the signature matches the function you want to call.

    \list
        \li Class names need to be fully-qualified, for example: \c "java/lang/String".
        \li Method signatures are written as \c "(ArgumentsTypes)ReturnType", see \l {JNI Types}.
        \li All object types are returned as a QJniObject.
    \endlist

    The example below demonstrates how to call different static functions:

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
    QJniObject stringArray = QJniObject::callStaticObjectMethod<jobjectArray>(
                                                                "org/qtproject/qt/TestClass",
                                                                "stringArray",
                                                                string1.object<jstring>(),
                                                                string2.object<jstring>());
    \endcode

    Note that while the first template parameter specifies the return type of the Java
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
    QJniObjectPrivate()
    {
    }
    ~QJniObjectPrivate() {
        JNIEnv *env = QJniEnvironment::getJniEnv();
        if (m_jobject)
            env->DeleteGlobalRef(m_jobject);
        if (m_jclass && m_own_jclass)
            env->DeleteGlobalRef(m_jclass);
    }

    template <typename ...Args>
    void construct(const char *signature = nullptr, Args &&...args)
    {
        if (m_jclass) {
            JNIEnv *env = QJniEnvironment::getJniEnv();
            // get default constructor
            jmethodID constructorId = QJniObject::getCachedMethodID(env, m_jclass, m_className, "<init>",
                                                                    signature ? signature : "()V");
            if (constructorId) {
                jobject obj = nullptr;
                if constexpr (sizeof...(Args) == 0)
                    obj = env->NewObject(m_jclass, constructorId);
                else
                    obj = env->NewObjectV(m_jclass, constructorId, std::forward<Args>(args)...);
                if (obj) {
                    m_jobject = env->NewGlobalRef(obj);
                    env->DeleteLocalRef(obj);
                }
            }
        }
    }

    bool isJString(JNIEnv *env) const;

    QByteArray m_className;
    jobject m_jobject = nullptr;
    jclass m_jclass = nullptr;
    bool m_own_jclass = true;
    enum class IsJString : quint8 {
        Unknown,
        Yes,
        No,
    };
    mutable IsJString m_is_jstring = IsJString::Unknown;
};

template <typename ...Args>
static inline QByteArray cacheKey(Args &&...args)
{
    return (QByteArrayView(":") + ... + QByteArrayView(args));
}

typedef QHash<QByteArray, jclass> JClassHash;
Q_GLOBAL_STATIC(JClassHash, cachedClasses)
Q_GLOBAL_STATIC(QReadWriteLock, cachedClassesLock)

static jclass getCachedClass(const QByteArray &className)
{
    QReadLocker locker(cachedClassesLock);
    const auto &it = cachedClasses->constFind(className);
    return it != cachedClasses->constEnd() ? it.value() : nullptr;
}


bool QJniObjectPrivate::isJString(JNIEnv *env) const
{
    Q_ASSERT(m_jobject);
    if (m_is_jstring != IsJString::Unknown)
        return m_is_jstring == IsJString::Yes;

    static constexpr auto stringSignature = QtJniTypes::Traits<jstring>::className();
    if (!m_className.isEmpty()) {
        m_is_jstring = m_className == stringSignature
                     ? IsJString::Yes
                     : IsJString::No;
    } else {
        const jclass strClass = getCachedClass(stringSignature.data());
        const bool isStringInstance = strClass
                                    ? env->IsInstanceOf(m_jobject, strClass)
                                    : false;
        m_is_jstring = isStringInstance ? IsJString::Yes : IsJString::No;
    }

    return m_is_jstring == IsJString::Yes;
}

/*!
    \internal

    Get a JNI object from a jobject variant and do the necessary
    exception clearing and delete the local reference before returning.
    The JNI object can be null if there was an exception.
*/
static QJniObject getCleanJniObject(jobject object, JNIEnv *env)
{
    if (QJniEnvironment::checkAndClearExceptions(env) || !object) {
        if (object)
            env->DeleteLocalRef(object);
        return QJniObject();
    }

    QJniObject res(object);
    env->DeleteLocalRef(object);
    return res;
}

/*!
    \internal
    \a className must be slash-encoded
*/
jclass QtAndroidPrivate::findClass(const char *className, JNIEnv *env)
{
    Q_ASSERT(env);
    QByteArray classNameArray(className);
#ifdef QT_DEBUG
    if (classNameArray.contains('.')) {
        qWarning("QtAndroidPrivate::findClass: className '%s' should use slash separators!",
                 className);
    }
#endif
    classNameArray.replace('.', '/');
    jclass clazz = getCachedClass(classNameArray);
    if (clazz)
        return clazz;

    QWriteLocker locker(cachedClassesLock);
    // Check again; another thread might have added the class to the cache after
    // our call to getCachedClass and before we acquired the lock.
    const auto &it = cachedClasses->constFind(classNameArray);
    if (it != cachedClasses->constEnd())
        return it.value();

    // JNIEnv::FindClass wants "a fully-qualified class name or an array type signature"
    // which is a slash-separated class name.
    jclass localClazz = env->FindClass(classNameArray.constData());
    if (localClazz) {
        clazz = static_cast<jclass>(env->NewGlobalRef(localClazz));
        env->DeleteLocalRef(localClazz);
    } else {
        // Clear the exception silently; we are going to try the ClassLoader next,
        // so no need for warning unless that fails as well.
        env->ExceptionClear();
    }

    if (!clazz) {
        // Wrong class loader, try our own
        QJniObject classLoader(QtAndroidPrivate::classLoader());
        if (!classLoader.isValid())
            return nullptr;

        // ClassLoader::loadClass on the other hand wants the binary name of the class,
        // e.g. dot-separated. In testing it works also with /, but better to stick to
        // the specification.
        const QString binaryClassName = QString::fromLatin1(className).replace(u'/', u'.');
        jstring classNameObject = env->NewString(reinterpret_cast<const jchar*>(binaryClassName.constData()),
                                                                                binaryClassName.length());
        QJniObject classObject = classLoader.callMethod<jclass>("loadClass", classNameObject);
        env->DeleteLocalRef(classNameObject);

        if (!QJniEnvironment::checkAndClearExceptions(env) && classObject.isValid())
            clazz = static_cast<jclass>(env->NewGlobalRef(classObject.object()));
    }

    if (clazz)
        cachedClasses->insert(classNameArray, clazz);

    return clazz;
}

jclass QJniObject::loadClass(const QByteArray &className, JNIEnv *env)
{
    return QtAndroidPrivate::findClass(className, env);
}

typedef QHash<QByteArray, jmethodID> JMethodIDHash;
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
    env->CallVoidMethodV(d->m_jobject, id, args);
    va_end(args);
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

    const QByteArray key = cacheKey(className, name, signature);
    QHash<QByteArray, jmethodID>::const_iterator it;

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

typedef QHash<QByteArray, jfieldID> JFieldIDHash;
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

    const QByteArray key = cacheKey(className, name, signature);
    QHash<QByteArray, jfieldID>::const_iterator it;

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
    d->m_className = className;
    d->m_jclass = loadClass(d->m_className, jniEnv());
    d->m_own_jclass = false;

    d->construct();
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
    d->m_className = className;
    d->m_jclass = loadClass(d->m_className, jniEnv());
    d->m_own_jclass = false;

    va_list args;
    va_start(args, signature);
    d->construct(signature, args);
    va_end(args);
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
    if (clazz) {
        d->m_jclass = static_cast<jclass>(jniEnv()->NewGlobalRef(clazz));
        va_list args;
        va_start(args, signature);
        d->construct(signature, args);
        va_end(args);
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
    : QJniObject(clazz, "()V")
{
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

    JNIEnv *env = jniEnv();
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
    \fn QJniObject::~QJniObject()

    Destroys the JNI object and releases any references held by the JNI object.
*/
QJniObject::~QJniObject()
{}

/*!
    \fn void QJniObject::swap(QJniObject &other)
    \since 6.8
    \memberswap{object}
*/


namespace {
QByteArray getClassNameHelper(JNIEnv *env, const QJniObjectPrivate *d)
{
    if (env->PushLocalFrame(3) != JNI_OK) // JVM out of memory
        return QByteArray();

    jmethodID mid = env->GetMethodID(d->m_jclass, "getClass", "()Ljava/lang/Class;");
    jobject classObject = env->CallObjectMethod(d->m_jobject, mid);
    jclass classObjectClass = env->GetObjectClass(classObject);
    mid = env->GetMethodID(classObjectClass, "getName", "()Ljava/lang/String;");
    jstring stringObject = static_cast<jstring>(env->CallObjectMethod(classObject, mid));
    const jsize length = env->GetStringUTFLength(stringObject);
    const char* nameString = env->GetStringUTFChars(stringObject, NULL);
    const QByteArray result = QByteArray::fromRawData(nameString, length).replace('.', '/');
    env->ReleaseStringUTFChars(stringObject, nameString);
    env->PopLocalFrame(nullptr);
    return result;
}
}

/*! \internal

    Returns the JNIEnv of the calling thread.
*/
JNIEnv *QJniObject::jniEnv() const noexcept
{
    return QJniEnvironment::getJniEnv();
}

/*!
    \fn jobject QJniObject::object() const
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
    if (d->m_className.isEmpty() && d->m_jclass && d->m_jobject) {
        JNIEnv *env = jniEnv();
        d->m_className = getClassNameHelper(env, d.get());
    }
    return d->m_className;
}

/*!
    \fn template <typename Ret, typename ...Args> auto QJniObject::callMethod(const char *methodName, const char *signature, Args &&...args) const
    \since 6.4

    Calls the object's method \a methodName with \a signature specifying the types of any
    subsequent arguments \a args, and returns the value (unless \c Ret is \c void). If \c Ret
    is a jobject type, then the returned value will be a QJniObject.

    \code
    QJniObject myJavaString("org/qtproject/qt/TestClass");
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
    QJniObject myJavaString("org/qtproject/qt/TestClass");
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
    (unless \c Ret is \c void).  If \c Ret is a jobject type, then the returned value will
    be a QJniObject.

    \code
    QJniEnvironment env;
    jclass javaMathClass = env.findClass("java/lang/Math");
    jdouble randNr = QJniObject::callStaticMethod<jdouble>(javaMathClass, "random");
    \endcode

    The method signature is deduced at compile time from \c Ret and the types of \a args.
*/

/*!
    \fn template <typename Klass, typename Ret, typename ...Args> auto QJniObject::callStaticMethod(const char *methodName, Args &&...args)
    \since 6.7

    Calls the static method \a methodName on the class \c Klass and returns the value of type
    \c Ret (unless \c Ret is \c void).  If \c Ret is a jobject type, then the returned value will
    be a QJniObject.

    The method signature is deduced at compile time from \c Ret and the types of \a args.
    \c Klass needs to be a C++ type with a registered type mapping to a Java type.
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
    JNIEnv *env = jniEnv();
    jmethodID id = getCachedMethodID(env, methodName, signature);
    if (id) {
        va_list args;
        va_start(args, signature);
        QJniObject res = getCleanJniObject(env->CallObjectMethodV(d->m_jobject, id, args), env);
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
    JNIEnv *env = QJniEnvironment::getJniEnv();
    jclass clazz = QJniObject::loadClass(className, env);
    if (clazz) {
        jmethodID id = QJniObject::getCachedMethodID(env, clazz,
                                         className,
                                         methodName, signature, true);
        if (id) {
            va_list args;
            va_start(args, signature);
            QJniObject res = getCleanJniObject(env->CallStaticObjectMethodV(clazz, id, args), env);
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
    if (clazz) {
        QJniEnvironment env;
        jmethodID id = getMethodID(env.jniEnv(), clazz, methodName, signature, true);
        if (id) {
            va_list args;
            va_start(args, signature);
            QJniObject res = getCleanJniObject(env->CallStaticObjectMethodV(clazz, id, args), env.jniEnv());
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
    if (clazz && methodId) {
        JNIEnv *env = QJniEnvironment::getJniEnv();
        va_list args;
        va_start(args, methodId);
        QJniObject res = getCleanJniObject(env->CallStaticObjectMethodV(clazz, methodId, args), env);
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
    \fn template <typename T, std::enable_if_t<std::is_convertible_v<T, jobject>, bool> = true> QJniObject &QJniObject::operator=(T object)

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
    \fn template<typename T> T QJniObject::getField(const char *fieldName) const

    Retrieves the value of the field \a fieldName.

    \code
    QJniObject volumeControl("org/qtproject/qt/TestClass");
    jint fieldValue = volumeControl.getField<jint>("FIELD_NAME");
    \endcode
*/

/*!
    \fn template<typename T> T QJniObject::getStaticField(const char *className, const char *fieldName)

    Retrieves the value from the static field \a fieldName on the class \a className.
*/

/*!
    \fn template<typename T> T QJniObject::getStaticField(jclass clazz, const char *fieldName)

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
    JNIEnv *env = QJniEnvironment::getJniEnv();
    jclass clazz = QJniObject::loadClass(className, env);
    if (!clazz)
        return QJniObject();
    jfieldID id = QJniObject::getCachedFieldID(env, clazz,
                                   className,
                                   fieldName,
                                   signature, true);
    if (!id)
        return QJniObject();

    return getCleanJniObject(env->GetStaticObjectField(clazz, id), env);
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
    JNIEnv *env = QJniEnvironment::getJniEnv();
    jfieldID id = getFieldID(env, clazz, fieldName, signature, true);
    return getCleanJniObject(env->GetStaticObjectField(clazz, id), env);
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
    \fn template<typename T> QJniObject QJniObject::getObjectField(const char *fieldName) const

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
    JNIEnv *env = jniEnv();
    jfieldID id = getCachedFieldID(env, fieldName, signature);
    if (!id)
        return QJniObject();

    return getCleanJniObject(env->GetObjectField(d->m_jobject, id), env);
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
    \fn template<typename T> QJniObject QJniObject::getStaticObjectField(const char *className, const char *fieldName)

    Retrieves the object from the field \a fieldName on the class \a className.

    \code
    QJniObject jobj = QJniObject::getStaticObjectField<jstring>("class/with/Fields", "FIELD_NAME");
    \endcode
*/

/*!
    \fn template<typename T> QJniObject QJniObject::getStaticObjectField(jclass clazz, const char *fieldName)

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
    JNIEnv *env = QJniEnvironment::getJniEnv();
    jstring stringRef = QtJniTypes::Detail::fromQString(string, env);
    QJniObject stringObject = getCleanJniObject(stringRef, env);
    stringObject.d->m_className = QtJniTypes::Traits<jstring>::className();
    stringObject.d->m_is_jstring = QJniObjectPrivate::IsJString::Yes;
    return stringObject;
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

    JNIEnv *env = jniEnv();
    if (d->isJString(env))
        return QtJniTypes::Detail::toQString(object<jstring>(), env);

    const QJniObject string = callMethod<jstring>("toString");
    return QtJniTypes::Detail::toQString(string.object<jstring>(), env);
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
    JNIEnv *env = QJniEnvironment::getJniEnv();

    if (!env)
        return false;

    return loadClass(className, env);
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
    return d && d->m_jobject;
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
    obj.jniEnv()->DeleteLocalRef(lref);
    return obj;
}

bool QJniObject::isSameObject(jobject obj) const
{
    if (!d)
        return obj == nullptr;
    if (d->m_jobject == obj)
        return true;
    if (!d->m_jobject || !obj)
        return false;
    return jniEnv()->IsSameObject(d->m_jobject, obj);
}

bool QJniObject::isSameObject(const QJniObject &other) const
{
    if (!other.isValid())
        return !isValid();
    return isSameObject(other.d->m_jobject);
}

void QJniObject::assign(jobject obj)
{
    if (d && isSameObject(obj))
        return;

    d = QSharedPointer<QJniObjectPrivate>::create();
    if (obj) {
        JNIEnv *env = jniEnv();
        d->m_jobject = env->NewGlobalRef(obj);
        jclass objectClass = env->GetObjectClass(obj);
        d->m_jclass = static_cast<jclass>(env->NewGlobalRef(objectClass));
        env->DeleteLocalRef(objectClass);
    }
}

jobject QJniObject::javaObject() const
{
    return d->m_jobject;
}

QT_END_NAMESPACE
