// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "window.h"

Window::Window()
{
}

int Window::normalProperty()
{
    return 0;
}

int Window::newProperty()
{
    return 1;
}

void Window::normalMethod()
{
    show();
}

void Window::newMethod()
{
    // Can be hidden from users expecting the initial API
    show();
}
