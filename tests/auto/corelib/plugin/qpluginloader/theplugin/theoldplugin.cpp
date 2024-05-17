// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include "theoldplugin.h"
#include <QtCore/QString>
#include <QtCore/qplugin.h>

QString TheOldPlugin::pluginName() const
{
    return QLatin1String("Plugin ok");
}

static int pluginVariable = 0xc0ffee;
extern "C" Q_DECL_EXPORT int *pointerAddress()
{
    return &pluginVariable;
}

// This hardcodes the old plugin metadata from before Qt 6.2
QT_PLUGIN_METADATA_SECTION
static constexpr unsigned char qt_pluginMetaData_ThePlugin[] = {
    'Q', 'T', 'M', 'E', 'T', 'A', 'D', 'A', 'T', 'A', ' ', '!',
    // metadata version, Qt version, architectural requirements
    0, QT_VERSION_MAJOR, QT_VERSION_MINOR, qPluginArchRequirements(),
    0xbf,
    // "IID"
    0x02,  0x78,  0x2b,  'o',  'r',  'g',  '.',  'q',
    't',  '-',  'p',  'r',  'o',  'j',  'e',  'c',
    't',  '.',  'Q',  't',  '.',  'a',  'u',  't',
    'o',  't',  'e',  's',  't',  's',  '.',  'p',
    'l',  'u',  'g',  'i',  'n',  'i',  'n',  't',
    'e',  'r',  'f',  'a',  'c',  'e',
    // "className"
    0x03,  0x69,  'T',  'h',  'e',  'P',  'l',  'u',
    'g',  'i',  'n',
    // "MetaData"
    0x04,  0xa2,  0x67,  'K',  'P',  'l',  'u',  'g',
    'i',  'n',  0xa8,  0x64,  'N',  'a',  'm',  'e',
    0x6e,  'W',  'i',  'n',  'd',  'o',  'w',  'G',
    'e',  'o',  'm',  'e',  't',  'r',  'y',  0x68,
    'N',  'a',  'm',  'e',  '[',  'm',  'r',  ']',
    0x78,  0x1f,  uchar('\xe0'), uchar('\xa4'), uchar('\x9a'), uchar('\xe0'), uchar('\xa5'), uchar('\x8c'),
    uchar('\xe0'), uchar('\xa4'), uchar('\x95'), uchar('\xe0'), uchar('\xa4'), uchar('\x9f'), ' ',  uchar('\xe0'),
    uchar('\xa4'), uchar('\xad'), uchar('\xe0'), uchar('\xa5'), uchar('\x82'), uchar('\xe0'), uchar('\xa4'), uchar('\xae'),
    uchar('\xe0'), uchar('\xa4'), uchar('\xbf'), uchar('\xe0'), uchar('\xa4'), uchar('\xa4'), uchar('\xe0'), uchar('\xa5'),
    uchar('\x80'), 0x68,  'N',  'a',  'm',  'e',  '[',  'p',
    'a',  ']',  0x78,  0x24,  uchar('\xe0'), uchar('\xa8'), uchar('\xb5'), uchar('\xe0'),
    uchar('\xa8'), uchar('\xbf'), uchar('\xe0'), uchar('\xa9'), uchar('\xb0'), uchar('\xe0'), uchar('\xa8'), uchar('\xa1'),
    uchar('\xe0'), uchar('\xa9'), uchar('\x8b'), uchar('\xe0'), uchar('\xa8'), uchar('\x9c'), uchar('\xe0'), uchar('\xa9'),
    uchar('\x81'), uchar('\xe0'), uchar('\xa8'), uchar('\xae'), uchar('\xe0'), uchar('\xa9'), uchar('\x88'), uchar('\xe0'),
    uchar('\xa8'), uchar('\x9f'), uchar('\xe0'), uchar('\xa8'), uchar('\xb0'), uchar('\xe0'), uchar('\xa9'), uchar('\x80'),
    0x68,  'N',  'a',  'm',  'e',  '[',  't',  'h',
    ']',  0x78,  0x39,  uchar('\xe0'), uchar('\xb8'), uchar('\xa1'), uchar('\xe0'), uchar('\xb8'),
    uchar('\xb4'), uchar('\xe0'), uchar('\xb8'), uchar('\x95'), uchar('\xe0'), uchar('\xb8'), uchar('\xb4'), uchar('\xe0'),
    uchar('\xb8'), uchar('\x82'), uchar('\xe0'), uchar('\xb8'), uchar('\x99'), uchar('\xe0'), uchar('\xb8'), uchar('\xb2'),
    uchar('\xe0'), uchar('\xb8'), uchar('\x94'), uchar('\xe0'), uchar('\xb8'), uchar('\x82'), uchar('\xe0'), uchar('\xb8'),
    uchar('\xad'), uchar('\xe0'), uchar('\xb8'), uchar('\x87'), uchar('\xe0'), uchar('\xb8'), uchar('\xab'), uchar('\xe0'),
    uchar('\xb8'), uchar('\x99'), uchar('\xe0'), uchar('\xb9'), uchar('\x89'), uchar('\xe0'), uchar('\xb8'), uchar('\xb2'),
    uchar('\xe0'), uchar('\xb8'), uchar('\x95'), uchar('\xe0'), uchar('\xb9'), uchar('\x88'), uchar('\xe0'), uchar('\xb8'),
    uchar('\xb2'), uchar('\xe0'), uchar('\xb8'), uchar('\x87'), 0x68,  'N',  'a',  'm',
    'e',  '[',  'u',  'k',  ']',  0x78,  0x19,  uchar('\xd0'),
    uchar('\xa0'), uchar('\xd0'), uchar('\xbe'), uchar('\xd0'), uchar('\xb7'), uchar('\xd0'), uchar('\xbc'), uchar('\xd1'),
    uchar('\x96'), uchar('\xd1'), uchar('\x80'), uchar('\xd0'), uchar('\xb8'), ' ',  uchar('\xd0'), uchar('\xb2'),
    uchar('\xd1'), uchar('\x96'), uchar('\xd0'), uchar('\xba'), uchar('\xd0'), uchar('\xbd'), uchar('\xd0'), uchar('\xb0'),
    0x6b,  'N',  'a',  'm',  'e',  '[',  'z',  'h',
    '_',  'C',  'N',  ']',  0x6c,  uchar('\xe7'), uchar('\xaa'), uchar('\x97'),
    uchar('\xe5'), uchar('\x8f'), uchar('\xa3'), uchar('\xe5'), uchar('\xbd'), uchar('\xa2'), uchar('\xe7'), uchar('\x8a'),
    uchar('\xb6'), 0x6b,  'N',  'a',  'm',  'e',  '[',  'z',
    'h',  '_',  'T',  'W',  ']',  0x6c,  uchar('\xe8'), uchar('\xa6'),
    uchar('\x96'), uchar('\xe7'), uchar('\xaa'), uchar('\x97'), uchar('\xe4'), uchar('\xbd'), uchar('\x8d'), uchar('\xe7'),
    uchar('\xbd'), uchar('\xae'), 0x6c,  'S',  'e',  'r',  'v',  'i',
    'c',  'e',  'T',  'y',  'p',  'e',  's',  0x81,
    0x68,  'K',  'C',  'M',  'o',  'd',  'u',  'l',
    'e',  0x76,  'X',  '-',  'K',  'D',  'E',  '-',
    'P',  'a',  'r',  'e',  'n',  't',  'C',  'o',
    'm',  'p',  'o',  'n',  'e',  'n',  't',  's',
    0x81,  0x6e,  'w',  'i',  'n',  'd',  'o',  'w',
    'g',  'e',  'o',  'm',  'e',  't',  'r',  'y',
    0xff,
};
QT_MOC_EXPORT_PLUGIN(TheOldPlugin, ThePlugin)
