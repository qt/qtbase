// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QOBJECT_H
#error Do not include qobject_impl.h directly
#include <QtCore/qobjectdefs_impl.h>
#endif

#if 0
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE


namespace QtPrivate {
    namespace { struct UniqueType; }
    /*
        Logic to statically generate the array of qMetaTypeId
        ConnectionTypes<FunctionPointer<Signal>::Arguments>::types() returns an array
        of int that is suitable for the types arguments of the connection functions.

        The array only exist if all the types are valid for QMetaType, detected
        using qTryMetaTypeInterfaceForType(). If any type is not valid at this
        point (non-const references and forward-declared types), the function
        returns nullptr. If the issue is a forward-declared type, the function
        can be used in a queued connection if the type is registered elsewhere
        before the signal is emitted. If the type is a non-const reference, it
        cannot be use in queued connections at all.
    */
    template <typename... Args> struct ConnectionTypesHelper
    {
        static const int *types()
        {
            if constexpr ((TypeIsSuitableForMetaType<Args, UniqueType> && ...)) {
                static const int t[] = { qMetaTypeId<Args>()..., 0 };
                return t;
            } else {
                return nullptr;
            }
        }
    };

    template <typename ArgList> struct ConnectionTypes;
    template <> struct ConnectionTypes<List<>>
    { static const int *types() { return nullptr; } };
    template <typename... Args> struct ConnectionTypes<List<Args...>>
            : public ConnectionTypesHelper<typename MetatypeDecay<Args>::type...>
    {
    };
}


QT_END_NAMESPACE
