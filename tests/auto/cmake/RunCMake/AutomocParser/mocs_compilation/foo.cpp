// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Intentionally does NOT #include "moc_foo.cpp".
#include "foo.h"

Foo *makeFoo()
{
    return new Foo;
}
