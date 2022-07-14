// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "leaf_lib.h"

#include <QtCore/qfile.h>

namespace intermediate_lib {

bool isLeafLibResourceAvailable()
{
    return leaf_lib::isResourceAvailable();
}

bool isResourceAvailable()
{
    return QFile::exists(u":/resource3.txt"_qs);
}

} // namespace
