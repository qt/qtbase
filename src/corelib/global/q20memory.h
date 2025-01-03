// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef Q20MEMORY_H
#define Q20MEMORY_H

#include <QtCore/qtconfigmacros.h>

#include <memory>
#include <utility>

#include <type_traits>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. Types and functions defined in this
// file can reliably be replaced by their std counterparts, once available.
// You may use these definitions in your own code, but be aware that we
// will remove them once Qt depends on the C++ version that supports
// them in namespace std. There will be NO deprecation warning, the
// definitions will JUST go away.
//
// If you can't agree to these terms, don't use these definitions!
//
// We mean it.
//

QT_BEGIN_NAMESPACE

// like std::construct_at (but not whitelisted for constexpr)
namespace q20 {
#ifdef __cpp_lib_constexpr_dynamic_alloc
using std::construct_at;
#else
template <typename T,
          typename... Args,
          typename Enable = std::void_t<decltype(::new (std::declval<void *>()) T(std::declval<Args>()...))> >
T *construct_at(T *ptr, Args && ... args)
{
    return ::new (const_cast<void *>(static_cast<const volatile void *>(ptr)))
                                                                T(std::forward<Args>(args)...);
}
#endif // __cpp_lib_constexpr_dynamic_alloc
} // namespace q20

QT_END_NAMESPACE

#endif /* Q20MEMORY_H */
