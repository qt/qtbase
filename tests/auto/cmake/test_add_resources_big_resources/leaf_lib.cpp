// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qfile.h>

namespace leaf_lib {

bool isResourceAvailable()
{
    return QFile::exists(u":/resource2.txt"_qs);
}

} // namespace
