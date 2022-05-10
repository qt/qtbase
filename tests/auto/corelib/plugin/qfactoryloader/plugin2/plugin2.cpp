// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtCore/qstring.h>
#include "plugin2.h"

QString Plugin2::pluginName() const
{
    return QLatin1String("Plugin2 ok");
}
