// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qstringliteral.h>

#include <utility>

auto x = QStringLiteral("test");

#if __cpp_constexpr >= 201907L

using namespace Qt::StringLiterals;

constexpr bool checkConstexprness()
{
    bool result = true;

    constexpr QString fromQStringLiteral = QStringLiteral("test");
    result &= fromQStringLiteral.size() == 4;
    result &= fromQStringLiteral.at(0) == u't';
    result &= fromQStringLiteral[1] == u'e';
    result &= fromQStringLiteral.front() == u't';
    result &= fromQStringLiteral.back() == u't';

    constexpr QString fromLiteralOperator = u"test"_s;
    result &= fromLiteralOperator.size() == 4;

    QString value = u"Hello"_s;
#if QT_CORE_INLINE_IMPL_SINCE(6, 13)
    value = value;
    result &= value.size() == 5;
#endif

    QString copyConstructed = value;
    result &= copyConstructed.size() == 5;

    QString moved = std::move(copyConstructed);
    result &= moved.size() == 5;

    moved = u"World!"_s;
    result &= moved.size() == 6;

#if QT_CORE_INLINE_IMPL_SINCE(6, 13)
    copyConstructed = moved;
    result &= copyConstructed.size() == 6;
#endif

    QString copy = moved;
    copy.clear();
    result &= copy.isNull();
    result &= moved.size() == 6;
    result &= moved.at(0) == u'W';
    result &= moved.at(1) == u'o';
    result &= moved.at(2) == u'r';
    result &= moved.at(3) == u'l';
    result &= moved.at(4) == u'd';
    result &= moved.at(5) == u'!';

    QString returned = []() constexpr {
        QString result = u"Hello"_s;
        return result;
    }();
    result &= returned.size() == 5;

    QString empty = u""_s;
    empty.clear();
    result &= empty.isNull();

    QString null;
    null.clear();
    result &= null.isNull();

    return result;
}
static_assert(checkConstexprness());

constexpr std::u16string_view stringView = u"test"_s;
static_assert(stringView == u"test");

#endif // __cpp_constexpr >= 201907L
