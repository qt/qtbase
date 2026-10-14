// Copyright (C) 2026 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Include first the stuff that will #error out if we change the version
#include <QtCore/qdatastream.h>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

// Now pretend this plugin was built against a far-future Qt (minor version
// 255), which QLibrary will refuse to load.
#undef QT_VERSION_MINOR
#define QT_VERSION_MINOR 255
#include "futureplugin.h"

static_assert(QPluginMetaData::Header{}.qt_minor_version == 255,
              "the Qt-version faking trick did not take effect");

QString FuturePlugin::pluginName() const
{
    return QLatin1String("FuturePlugin");
}

#include "moc_futureplugin.cpp"
