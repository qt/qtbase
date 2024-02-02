// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QtCore/QString>
#include "almostplugin.h"
#include <QtCore/qplugin.h>

QString AlmostPlugin::pluginName() const
{
    unresolvedSymbol();
    return QLatin1String("Plugin ok");
}
