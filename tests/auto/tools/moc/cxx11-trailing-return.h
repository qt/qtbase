// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef CXX11_TRAILING_RETURN_H
#define CXX11_TRAILING_RETURN_H
#include <QtCore/QObject>

class CXX11TrailingReturn : public QObject
{
    Q_OBJECT
public slots:
    inline auto f() -> void;
    inline auto args(int i, char b) -> int;

    auto inlineFunc(int i) -> int { return i + 1; }
    auto inlineFuncVoid() -> void { }
    auto constFunc() const -> double { return {}; }
    auto constRefReturn() -> const CXX11TrailingReturn & { return *this; }
    auto constConstRefReturn() const -> const CXX11TrailingReturn & { return *this; }

public:
    Q_SLOT inline auto fMacro() -> void;
    Q_SLOT inline auto argsMacro(int i, char b) -> int;

    Q_SLOT auto inlineFuncMacro(int i) -> int { return i + 1; }
    Q_SLOT auto inlineFuncVoidMacro() -> void { }
    Q_SLOT auto constFuncMacro() const -> double { return {}; }
    Q_SLOT auto constRefReturnMacro() -> const CXX11TrailingReturn & { return *this; }
    Q_SLOT auto constConstRefReturnMacro() const -> const CXX11TrailingReturn & { return *this; }

signals:
    auto signal(int i) -> void;

public:
    Q_SIGNAL auto signalMacro(int i) const -> void;

    Q_INVOKABLE auto invokable(int i, double d) const -> double { return d / i; }
};

auto CXX11TrailingReturn::f() -> void
{
    return;
}

auto CXX11TrailingReturn::args(int i, char b) -> int
{
    return i + int(b);
}

auto CXX11TrailingReturn::fMacro() -> void
{
    return;
}

auto CXX11TrailingReturn::argsMacro(int i, char b) -> int
{
    return i + int(b);
}

#endif // CXX11_TRAILING_RETURN_H
