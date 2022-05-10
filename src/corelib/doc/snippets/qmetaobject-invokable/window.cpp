// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "window.h"

Window::Window()
{
}

void Window::normalMethod()
{
    // Cannot be called by the meta-object system.
    show();
}

void Window::invokableMethod()
{
    // Can be called by the meta-object system.
    show();
}
