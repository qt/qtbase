// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtCore/QString>
#include "theplugin.h"
#include <QtCore/qplugin.h>

QString ThePlugin::pluginName() const
{
    return QLatin1String("Plugin ok");
}

static int pluginVariable = 0xc0ffee;
extern "C" Q_DECL_EXPORT int *pointerAddress()
{
    return &pluginVariable;
}
