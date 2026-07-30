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

    QString returned = []() constexpr {
        QString result = u"Hello"_s;
        return result;
    }();
    result &= returned.size() == 5;
    return result;
}
static_assert(checkConstexprness());

#endif // __cpp_constexpr >= 201907L
