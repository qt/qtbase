// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef USING_NAMESPACES_H
#define USING_NAMESPACES_H

namespace Foo {}
namespace Bar
{
    namespace Huh
    {
    }
}
namespace Top
{
}

using namespace Foo;
using namespace Bar::Huh;
using namespace ::Top;

#endif // USING_NAMESPACES_H
