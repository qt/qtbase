// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QUNIQUEHANDLE_P_H
#define QUNIQUEHANDLE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qassert.h>
#include <QtCore/qcompare.h>
#include <QtCore/qfunctionaltools_impl.h>
#include <QtCore/qswap.h>
#include <QtCore/qtclasshelpermacros.h>

#include <memory>
#include <utility>
#include <type_traits>

QT_BEGIN_NAMESPACE

/*! \internal QUniqueHandle is a general purpose RAII wrapper intended
    for interfacing with resource-allocating C-style APIs, for example
    operating system APIs, database engine APIs, or any other scenario
    where resources are allocated and released, and where pointer
    semantics does not seem a perfect fit.

    QUniqueHandle does not support copying, because it is intended to
    maintain ownership of resources that can not be copied. This makes
    it safer to use than naked handle types, since ownership is
    maintained by design.

    The underlying handle object is described using a client supplied
    HandleTraits object that is implemented per resource type. The
    traits struct must describe two properties of a handle:

    1) What value is considered invalid
    2) How to close a resource.

    Example 1:

        struct InvalidHandleTraits {
            using Type = HANDLE;

            static Type invalidValue() {
                return INVALID_HANDLE_VALUE;
            }

            static bool close(Type handle) {
                return CloseHandle(handle) != 0;
            }
        }

        using FileHandle = QUniqueHandle<InvalidHandleTraits>;

    Usage:

        // Takes ownership of returned handle.
        FileHandle handle{ CreateFile(...) };

        if (!handle.isValid()) {
            qDebug() << GetLastError()
            return;
        }

        ...

    Example 2:

        struct SqLiteTraits {
            using Type = sqlite3*;

            static Type invalidValue() {
                return nullptr;
            }

            static bool close(Type handle) {
                sqlite3_close(handle);
                return true;
            }
        }

        using DbHandle = QUniqueHandle<SqLiteTraits>;

    Usage:

        DbHandle h;

        // Take ownership of returned handle.
        int result = sqlite3_open(":memory:", &h);

        ...

    Example 3:

        struct TempFileTraits {
            using Type = FILE*;

            static Type invalidValue() {
                return nullptr;
            }

            static bool close(Type handle) {
                return fclose(handle) == 0;
            }
        };

        struct TempFileDeleter {
            using Type = TempFileTraits::Type;

            void operator()(Type handle) {
                if (handle != TempFileTraits::invalidValue()) {
                    TempFileTraits::close(handle);
                    if (path)
                        remove(path);
                }
            }

            const char* path{ nullptr };
        };

        using TempFileHandle = QUniqueHandle<TempFileTraits, TempFileDeleter>;

    Usage:

        TempFileHandle tempFile(fopen("temp.bin", "wb"), TempFileDeleter{ "my_temp.bin" });

        ...

    NOTE: The QUniqueHandle assumes that closing a resource is
    guaranteed to succeed, and provides no support for handling failure
    to close a resource. It is therefore only recommended for use cases
    where failure to close a resource is either not an error, or an
    unrecoverable error.
*/

// clang-format off

namespace QtUniqueHandleTraits {

template <typename HandleTraits>
struct DefaultDeleter
{
    using Type = typename HandleTraits::Type;

    void operator()(Type handle) const noexcept
    {
        if (handle != HandleTraits::invalidValue()) {
            const bool success = HandleTraits::close(handle);
            Q_ASSERT(success);
        }
    }
};

} // namespace QtUniqueHandleTraits

template <typename HandleTraits, typename Deleter = QtUniqueHandleTraits::DefaultDeleter<HandleTraits>>
class QUniqueHandle : private QtPrivate::CompactStorage<Deleter>
{
    using Storage = QtPrivate::CompactStorage<Deleter>;

    template <typename D>
    using if_default_constructible = std::enable_if_t<std::is_default_constructible_v<D>, bool>;

public:
    using Type = typename HandleTraits::Type;
    static_assert(std::is_nothrow_default_constructible_v<Type>);
    static_assert(std::is_nothrow_constructible_v<Type>);
    static_assert(std::is_nothrow_copy_constructible_v<Type>);
    static_assert(std::is_nothrow_move_constructible_v<Type>);
    static_assert(std::is_nothrow_copy_assignable_v<Type>);
    static_assert(std::is_nothrow_move_assignable_v<Type>);
    static_assert(std::is_nothrow_destructible_v<Type>);
    static_assert(std::is_nothrow_copy_constructible_v<Deleter>);
    static_assert(std::is_nothrow_move_constructible_v<Deleter>);
    static_assert(std::is_nothrow_copy_assignable_v<Deleter>);
    static_assert(std::is_nothrow_move_assignable_v<Deleter>);
    static_assert(std::is_nothrow_destructible_v<Deleter>);
    static_assert(noexcept(std::declval<Type>() == std::declval<Type>()));
    static_assert(noexcept(std::declval<Type>() != std::declval<Type>()));
    static_assert(noexcept(std::declval<Type>() < std::declval<Type>()));
    static_assert(noexcept(std::declval<Type>() <= std::declval<Type>()));
    static_assert(noexcept(std::declval<Type>() > std::declval<Type>()));
    static_assert(noexcept(std::declval<Type>() >= std::declval<Type>()));

    template <if_default_constructible<Deleter> = true>
    QUniqueHandle() noexcept
    {}

    template <if_default_constructible<Deleter> = true>
    explicit QUniqueHandle(const Type &handle) noexcept
        : m_handle{ handle }
    {}

    QUniqueHandle(const Type &handle, const Deleter &deleter) noexcept
        : Storage{ deleter }, m_handle{ handle }
    {}

    QUniqueHandle(const Type &handle, Deleter &&deleter) noexcept
        : Storage{ std::move(deleter) }, m_handle{ handle }
    {}

    QUniqueHandle(QUniqueHandle &&other) noexcept
        : Storage{ std::move(other.deleter()) }, m_handle{ other.release() }
    {
    }

    ~QUniqueHandle() noexcept
    {
        close();
    }

    void swap(QUniqueHandle &other) noexcept
    {
        qSwap(m_handle, other.m_handle);
        qSwap(deleter(), other.deleter());
    }

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QUniqueHandle)

    QUniqueHandle(const QUniqueHandle &) = delete;
    QUniqueHandle &operator=(const QUniqueHandle &) = delete;


    [[nodiscard]] bool isValid() const noexcept
    {
        return m_handle != HandleTraits::invalidValue();
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return isValid();
    }

    [[nodiscard]] Type get() const noexcept
    {
        return m_handle;
    }

    [[nodiscard]] Deleter& deleter() noexcept
    {
        return Storage::object();
    }

    [[nodiscard]] const Deleter& deleter() const noexcept
    {
        return Storage::object();
    }

    void reset(const Type& handle = HandleTraits::invalidValue()) noexcept
    {
        if (handle == m_handle)
            return;

        close();
        m_handle = handle;
    }

    [[nodiscard]] Type release() noexcept
    {
        return std::exchange(m_handle, HandleTraits::invalidValue());
    }

    [[nodiscard]] Type *operator&() noexcept  // NOLINT(google-runtime-operator)
    {
        Q_ASSERT(!isValid());
        return &m_handle;
    }

    void close() noexcept
    {
        if (!isValid())
            return;

        deleter()(m_handle);

        m_handle = HandleTraits::invalidValue();
    }

private:
    friend bool comparesEqual(const QUniqueHandle& lhs, const QUniqueHandle& rhs) noexcept
    {
        return lhs.get() == rhs.get();
    }

    friend Qt::strong_ordering compareThreeWay(const QUniqueHandle& lhs,
                                               const QUniqueHandle& rhs) noexcept
    {
        if constexpr (std::is_pointer_v<Type>)
            return qCompareThreeWay(Qt::totally_ordered_wrapper{ lhs.get() },
                                    Qt::totally_ordered_wrapper{ rhs.get() });
        else
            return qCompareThreeWay(lhs.get(), rhs.get());
    }

    Q_DECLARE_STRONGLY_ORDERED(QUniqueHandle)

    Type m_handle{ HandleTraits::invalidValue() };
};

// clang-format on

template <typename Trait, typename Deleter>
void swap(QUniqueHandle<Trait, Deleter> &lhs, QUniqueHandle<Trait, Deleter> &rhs) noexcept
{
    lhs.swap(rhs);
}


QT_END_NAMESPACE

#endif
