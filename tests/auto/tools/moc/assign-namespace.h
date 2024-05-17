// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef ASSIGN_NAMESPACE_H
#define ASSIGN_NAMESPACE_H

namespace A
{
    namespace Nested
    {
        namespace Space {}
    }
}

namespace Mine = Qt;
namespace Theirs = A::Nested::Space;

#endif // ASSIGN_NAMESPACE_H
