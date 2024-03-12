// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QtCore/qstring.h>
#include "plugin1.h"

QString Plugin1::pluginName() const
{
    return QLatin1String("Plugin1 ok");
}

#include "moc_plugin1.cpp"
