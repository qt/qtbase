// Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Sérgio Martins <sergio.martins@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
void myComplexCodeWithMultipleReturnPoints(int v)
{
    // The lambda will be executed right before your function returns
    auto cleanup = qScopeGuard([] { code you want executed goes HERE; });

    if (v == -1)
        return;

    int v2 = code_that_might_throw_exceptions();

    if (v2 == -1)
        return;

    (...)
}
//! [0]
