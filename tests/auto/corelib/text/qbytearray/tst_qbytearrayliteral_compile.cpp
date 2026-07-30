// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qbytearray.h>

#include <utility>

auto x = QByteArrayLiteral("test");

#if __cpp_constexpr >= 201907L

constexpr QByteArray y = QByteArrayLiteral("test");
static_assert(y.size() == 4);

using namespace Qt::StringLiterals;

constexpr QByteArray z = "test"_ba;
static_assert(z.size() == 4);

constexpr bool checkConstexprness()
{
    bool result = true;

    QByteArray value = "Hello"_ba;
#if QT_CORE_INLINE_IMPL_SINCE(6, 13)
    value = value;
    result &= value.size() == 5;
#endif

    QByteArray copy = value;
    result &= copy.size() == 5;

    QByteArray moved = std::move(copy);
    result &= moved.size() == 5;

    moved = "World!"_ba;
    result &= moved.size() == 6;

#if QT_CORE_INLINE_IMPL_SINCE(6, 13)
    copy = moved;
    result &= copy.size() == 6;
#endif

    QByteArray returned = []() constexpr {
        QByteArray result = "Hello"_ba;
        return result;
    }();
    result &= returned.size() == 5;
    return result;
}
static_assert(checkConstexprness());

#endif // __cpp_constexpr >= 201907L
