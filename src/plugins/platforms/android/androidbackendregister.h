// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef ANDROIDBACKENDREGISTER_H
#define ANDROIDBACKENDREGISTER_H

#include <type_traits>

#include <QtCore/qjnienvironment.h>
#include <QtCore/qjnitypes.h>
#include <QtCore/qjniobject.h>

#include <QtCore/qstring.h>
#include <QtCore/qmap.h>
#include <QtCore/qmutex.h>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcAndroidBackendRegister)

/*
    \internal

    This class is used to [un]register QJniObjects which implement specific interfaces. These
    objects can then be fetched or directly called by using that interface.

    This is intended to decouple the Qt C++ code from the underlying Java implementation, as Qt now
    has multiple separate usecases, each of which may have different implementations and support
    different features.

    To use this register, the interface must be declared as a JNI class via Q_DECLARE_JNI_CLASS:

    Q_DECLARE_JNI_CLASS(ImaginaryInterface, "org/qtproject/qt/android/ImaginaryInterface")

    Where ImaginaryInterface is a Java interface like this:

    @UsedFromNativeCode
    interface ImaginaryInterface {
        void doSomething(int imaginary, int imaginary2);
    }

    After that, the features provided by that interface can be used in the C++ code in two ways:

    Use the convenience method callInterface() to call a method directly:

    AndroidBackendRegister *reg = QtAndroid::backendRegister();
    int imaginary, imaginary2;
    reg->callInterface<QtJniTypes::ImaginaryInterface, void>("doSomething", imaginary, imaginary2);

    Or get the QJniObject directly and use it as you would any other QJniObject:
    AndroidBackendRegister *reg = QtAndroid::backendRegister();
    auto imaginary = reg->getInterface<QtJniTypes::ImaginaryInterface>();
    // ... do whatever with QJniObject

    In order to register a new interface on the Java side, the BackendRegister class must be used,
    with its native functions registerBackend() and unregisterBackend():

    BackendRegister.registerBackend(ImaginaryInterface.class, imaginaryInterfaceObject);

    and

    BackendRegister.unregisterBackend(ImaginaryInterface.class);

    Note that only one object can be registered for each interface. If multiple objects are
    registered, only the latest one is kept. Thus, you only need to declare the interface you want
    to unregister, not the object that implements the interface as well.

    If the interface needs to be available as soon as possible, it should be registered immediately
    after Qt has started, by using the QtNative app state listener functionality.
*/

template <typename T>
using ValidInterfaceType = std::enable_if_t<std::is_base_of_v<QtJniTypes::JObjectBase, T>, bool>;

class AndroidBackendRegister
{
public:
    static bool registerNatives();

    /*
        \internal

        Returns a QJniObject which is registered for the given interface.
        Requires the type of the requested interface to be registered via
        Q_DECLARE_JNI_CLASS. (see ValidInterfaceType).
    */
    template <typename T, ValidInterfaceType<T> = true>
    [[nodiscard]] T getInterface()
    {
        QMutexLocker lock(&m_registerMutex);
        return m_register.value(QString(QtJniTypes::Traits<T>::className().data()));
    }

    template <typename Object>
    using IsObjectType =
            typename std::disjunction<std::is_base_of<QJniObject, Object>,
                                      std::is_base_of<QtJniTypes::JObjectBase, Object>>;

    /*
        \internal

        Convenience function that calls getInterface<Interface>() and then QJniObject::callMethod()
        on the resulting object, forwarding the rest of the parameters to that function.

        If the interface is not registered, a warning is printed and an empty object is returned.
    */
    template <typename Interface, typename Ret = void, typename... Args,
              ValidInterfaceType<Interface> = true>
    auto callInterface(const char *func, Args... args)
    {
        if (const auto obj = getInterface<Interface>(); obj.isValid()) {
            return obj.template callMethod<Ret, Args...>(func, std::forward<Args>(args)...);
        } else {
            qWarning() << "No interface with className"
                       << QtJniTypes::Traits<Interface>::className() << "has been registered.";
        }

        if constexpr (IsObjectType<Ret>::value)
            return Ret(QJniObject());
        if constexpr (!std::is_same_v<Ret, void>)
            return Ret{};
    }

private:
    QMutex m_registerMutex;
    QMap<QString, QJniObject> m_register;

    static jboolean isNull(JNIEnv *, jclass);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(isNull)
    static void registerBackend(JNIEnv *, jclass, jclass interfaceClass, jobject interface);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(registerBackend)
    static void unregisterBackend(JNIEnv *, jclass, jclass interfaceClass);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(unregisterBackend)
};

QT_END_NAMESPACE

#endif // ANDROIDBACKENDREGISTER_H
