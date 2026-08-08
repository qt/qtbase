// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qbytearray.h>

#include <string_view>
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
    result &= y.at(0) == 't';
    result &= y[1] == 'e';
    result &= y.front() == 't';
    result &= y.back() == 't';
    result &= y.data()[2] == 's';
    result &= y.constData()[3] == 't';
    result &= y.begin() == y.data();
    result &= y.cbegin() == y.data();
    result &= y.constBegin() == y.data();
    result &= y.end() == y.data() + y.size();
    result &= y.cend() == y.data() + y.size();
    result &= y.constEnd() == y.data() + y.size();
    result &= y.rbegin().base() == y.end();
    result &= y.rend().base() == y.begin();
    result &= y.crbegin().base() == y.end();
    result &= y.crend().base() == y.begin();

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

constexpr std::string_view stringView = "test"_ba;
static_assert(stringView == "test");

#endif // __cpp_constexpr >= 201907L
