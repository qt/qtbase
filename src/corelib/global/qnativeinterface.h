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

#ifndef QNATIVEINTERFACE_H
#define QNATIVEINTERFACE_H

#include <QtCore/qglobal.h>
#include <QtCore/qloggingcategory.h>

#include <typeinfo>

#ifndef QT_STATIC
#  define Q_NATIVE_INTERFACE_EXPORT Q_DECL_EXPORT
#  define Q_NATIVE_INTERFACE_IMPORT Q_DECL_IMPORT
#else
#  define Q_NATIVE_INTERFACE_EXPORT
#  define Q_NATIVE_INTERFACE_IMPORT
#endif

QT_BEGIN_NAMESPACE

// We declare a virtual non-inline function in the form
// of the destructor, making it the key function. This
// ensures that the typeinfo of the class is exported.
// By being protected, we also ensure that pointers to
// the interface can't be deleted.
#define QT_DECLARE_NATIVE_INTERFACE_3(NativeInterface, Revision, BaseType) \
    protected: \
        virtual ~NativeInterface(); \
        struct TypeInfo { \
            using baseType = BaseType; \
            static constexpr int revision = Revision; \
        }; \
    public: \

// Revisioned interfaces only make sense when exposed through a base
// type via QT_DECLARE_NATIVE_INTERFACE_ACCESSOR, as the revision
// checks happen at that level (and not for normal dynamic_casts).
#define QT_DECLARE_NATIVE_INTERFACE_2(NativeInterface, Revision) \
    static_assert(false, "Must provide a base type when specifying revision");

#define QT_DECLARE_NATIVE_INTERFACE_1(NativeInterface) \
    QT_DECLARE_NATIVE_INTERFACE_3(NativeInterface, 0, void)

#define QT_DECLARE_NATIVE_INTERFACE(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_NATIVE_INTERFACE, __VA_ARGS__)

namespace QNativeInterface::Private {
    template <typename NativeInterface>
    struct TypeInfo : private NativeInterface
    {
        static constexpr int revision() { return NativeInterface::TypeInfo::revision; }

        template<typename BaseType>
        static constexpr bool isCompatibleWith =
            std::is_base_of<typename NativeInterface::TypeInfo::baseType, BaseType>::value;
    };

    template <typename T>
    Q_NATIVE_INTERFACE_IMPORT void *resolveInterface(const T *that, const std::type_info &type, int revision);

    Q_CORE_EXPORT Q_DECLARE_LOGGING_CATEGORY(lcNativeInterface)
}

// Declares an accessor for the native interface
#define QT_DECLARE_NATIVE_INTERFACE_ACCESSOR \
    template <typename I> \
    I *nativeInterface() const \
    { \
        using T = std::decay_t<decltype(*this)>; \
        static_assert(QNativeInterface::Private::TypeInfo<I>::template isCompatibleWith<T>, \
            "T::nativeInterface<I>() requires that native interface I is compatible with T"); \
        \
        return static_cast<I*>(QNativeInterface::Private::resolveInterface(this, typeid(I), QNativeInterface::Private::TypeInfo<I>::revision())); \
    }

// Provides a definition for the interface destructor
#define QT_DEFINE_NATIVE_INTERFACE_2(Namespace, InterfaceClass) \
    QT_PREPEND_NAMESPACE(Namespace)::InterfaceClass::~InterfaceClass() = default

#define QT_DEFINE_NATIVE_INTERFACE(...) QT_OVERLOADED_MACRO(QT_DEFINE_NATIVE_INTERFACE, QNativeInterface, __VA_ARGS__)
#define QT_DEFINE_PRIVATE_NATIVE_INTERFACE(...) QT_OVERLOADED_MACRO(QT_DEFINE_NATIVE_INTERFACE, QNativeInterface::Private, __VA_ARGS__)

#define QT_NATIVE_INTERFACE_RETURN_IF(NativeInterface, baseType) \
    using QNativeInterface::Private::lcNativeInterface; \
    qCDebug(lcNativeInterface, "Comparing requested type id %s with available %s", \
            type.name(), typeid(NativeInterface).name()); \
    if (type == typeid(NativeInterface)) { \
        qCDebug(lcNativeInterface, "Match for type id %s. Comparing revisions (requested %d / available %d)", \
            type.name(), revision, TypeInfo<NativeInterface>::revision()); \
        if (revision == TypeInfo<NativeInterface>::revision()) { \
            qCDebug(lcNativeInterface) << "Full match. Returning dynamic cast of" << baseType; \
            return dynamic_cast<NativeInterface*>(baseType); \
        } else { \
            qCWarning(lcNativeInterface, "Native interface revision mismatch (requested %d / available %d) for interface %s", \
                revision, TypeInfo<NativeInterface>::revision(), type.name()); \
            return nullptr; \
        } \
    }

QT_END_NAMESPACE

#endif // QNATIVEINTERFACE_H
