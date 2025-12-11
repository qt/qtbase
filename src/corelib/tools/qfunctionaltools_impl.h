// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#if 0
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#ifndef QFUNCTIONALTOOLS_IMPL_H
#define QFUNCTIONALTOOLS_IMPL_H

#include <QtCore/qtconfigmacros.h>

#include <type_traits>
#include <utility>

QT_BEGIN_NAMESPACE

/*!
    \class QtPrivate::CompactStorage
    \internal
    CompactStorage is a small utility class that stores an object in the
    most space-efficient way possible. If the stored type is empty and not
    marked as \c final, CompactStorage inherits from it directly, allowing
    the compiler to apply the Empty Base Optimization (EBO). Otherwise,
    the object is stored as a regular data member.

    This class is typically used as a private base class for utility types
    that need to optionally carry extra data without increasing their size.
    The stored object can be accessed through the \c object() accessor,
    which returns the contained object in any const/reference combination.

    Example 1:

        // FileHandle carries a lightweight context describing
        // the underlying OS resource.
        struct Context {
            int fd = -1;
            bool isTemporary = false;
        };

        struct FileHandle : private QtPrivate::CompactStorage<Context>
        {
            using Storage = QtPrivate::CompactStorage<Context>;

            FileHandle(int descriptor, bool temp)
                : Storage(Context{descriptor, temp})
            {}

            void open(int descriptor, bool temp) noexcept {
                object().fd = descriptor;
                object().isTemporary = temp;
            }

            void close() noexcept {
                if (object().isTemporary)
                    ::close(object().fd);
            }
        };

    Example 2:

        // When the stored type is empty, EBO removes any overhead.
        struct EmptyContext {};

        struct LightweightHandle : private QtPrivate::CompactStorage<EmptyContext>
        {
            using Storage = QtPrivate::CompactStorage<EmptyContext>;
            int handle = -1;

            void reset(int h) noexcept { handle = h; }
        };

        static_assert(sizeof(LightweightHandle) == sizeof(int));
*/

namespace QtPrivate {

namespace detail {

#define FOR_EACH_CVREF(op) \
    op(&) \
    op(const &) \
    op(&&) \
    op(const &&) \
    /* end */


template <typename Object, typename = void>
struct StorageByValue
{
    Object o;
#define MAKE_GETTER(cvref) \
    constexpr Object cvref object() cvref noexcept \
    { return static_cast<Object cvref>(o); }
    FOR_EACH_CVREF(MAKE_GETTER)
#undef MAKE_GETTER
};

template <typename Object, typename Tag = void>
struct StorageEmptyBaseClassOptimization : Object
{
    StorageEmptyBaseClassOptimization() = default;
    StorageEmptyBaseClassOptimization(Object &&o)
        : Object(std::move(o))
    {}
    StorageEmptyBaseClassOptimization(const Object &o)
        : Object(o)
    {}

#define MAKE_GETTER(cvref) \
    constexpr Object cvref object() cvref noexcept \
    { return static_cast<Object cvref>(*this); }
    FOR_EACH_CVREF(MAKE_GETTER)
#undef MAKE_GETTER
};
} // namespace detail

template <typename Object, typename Tag = void>
using CompactStorage = typename std::conditional_t<
        std::conjunction_v<
            std::is_empty<Object>,
            std::negation<std::is_final<Object>>
        >,
        detail::StorageEmptyBaseClassOptimization<Object, Tag>,
        detail::StorageByValue<Object, Tag>
    >;

} // namespace QtPrivate

#undef FOR_EACH_CVREF

QT_END_NAMESPACE

#endif // QFUNCTIONALTOOLS_IMPL_H
