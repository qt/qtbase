// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qplugin.h>

// be careful when updating to V2, the header is different on ELF systems
QT_PLUGIN_METADATA_SECTION
static const char pluginMetaData[512] = {
    'q', 'p', 'l', 'u', 'g', 'i', 'n', ' ',
    't', 'e', 's', 't', 'f', 'i', 'l', 'e'
};

extern "C" {

const void *qt_plugin_query_metadata()
{
    return pluginMetaData;
}

Q_DECL_EXPORT void *qt_plugin_instance()
{
    return nullptr;
}

}
