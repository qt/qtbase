// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef UNTERMINATED_FUNCTION_MACRO_H
#define UNTERMINATED_FUNCTION_MACRO_H

class Dummy : public QObject {
    Q_OBJECT
}

#define MACRO(arg) do_something(arg)

static void foo() {
    MACRO(foo
}

#endif // UNTERMINATED_FUNCTION_MACRO_H
