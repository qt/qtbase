// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CXX11_TRAILING_RETURN_H
#define CXX11_TRAILING_RETURN_H
#include <QtCore/QObject>

class CXX11TrailingReturn : public QObject
{
    Q_OBJECT
public slots:
    inline auto fun() -> void;
    inline auto arguments(int i, char b) -> int;
    inline auto inlineFunc(int i) -> int
    {
        return i + 1;
    }

    inline auto constRefReturn() -> const CXX11TrailingReturn &
    {
        return *this;
    }

    inline auto constConstRefReturn() const -> const CXX11TrailingReturn &
    {
        return *this;
    }

signals:
    auto trailingSignalReturn(int i) -> void;
};

auto CXX11TrailingReturn::fun() -> void
{
    return;
}

auto CXX11TrailingReturn::arguments(int i, char b) -> int
{
    return i + int(b);
}

#endif // CXX11_TRAILING_RETURN_H
