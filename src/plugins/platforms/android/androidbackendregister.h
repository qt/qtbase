// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

template <typename T>
using ValidInterfaceType = std::enable_if_t<std::is_base_of_v<QtJniTypes::JObjectBase, T>, bool>;

class AndroidBackendRegister
{
public:
    static bool registerNatives();

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

    template <typename Interface, typename Ret, typename... Args,
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
