/****************************************************************************
**
** Copyright (C) 2018 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPLUGIN_P_H
#define QPLUGIN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

enum class QtPluginMetaDataKeys {
    QtVersion,
    Requirements,
    IID,
    ClassName,
    MetaData,
    URI,
    IsDebug,
};

// F(IntKey, StringKey, Description)
// Keep this list sorted in the order moc should output.
#define QT_PLUGIN_FOREACH_METADATA(F) \
    F(QtPluginMetaDataKeys::IID, "IID", "Plugin's Interface ID")                \
    F(QtPluginMetaDataKeys::ClassName, "className", "Plugin class name")        \
    F(QtPluginMetaDataKeys::MetaData, "MetaData", "Other meta data")            \
    F(QtPluginMetaDataKeys::URI, "URI", "Plugin URI")                           \
    /* not output by moc in CBOR */                                             \
    F(QtPluginMetaDataKeys::QtVersion, "version", "Qt version")                 \
    F(QtPluginMetaDataKeys::Requirements, "archlevel", "Architectural level")   \
    F(QtPluginMetaDataKeys::IsDebug, "debug", "Debug-mode plugin")              \
    /**/

namespace {
struct DecodedArchRequirements
{
    quint8 level;
    bool isDebug;
    friend constexpr bool operator==(DecodedArchRequirements r1, DecodedArchRequirements r2)
    {
        return r1.level == r2.level && r1.isDebug == r2.isDebug;
    }
};

static constexpr DecodedArchRequirements decodeVersion0ArchRequirements(quint8 value)
{
    // see qPluginArchRequirements() and QPluginMetaDataV2::archRequirements()
    DecodedArchRequirements r = {};
#ifdef Q_PROCESSOR_X86
    if (value & 4)
        r.level = 4;            // AVX512F -> x86-64-v4
    else if (value & 2)
        r.level = 3;            // AVX2 -> x86-64-v3
#endif
    if (value & 1)
        r.isDebug = true;
    return r;
}
// self checks
static_assert(decodeVersion0ArchRequirements(0) == DecodedArchRequirements{ 0, false });
static_assert(decodeVersion0ArchRequirements(1) == DecodedArchRequirements{ 0, true });
#ifdef Q_PROCESSOR_X86
static_assert(decodeVersion0ArchRequirements(2) == DecodedArchRequirements{ 3, false });
static_assert(decodeVersion0ArchRequirements(3) == DecodedArchRequirements{ 3, true });
static_assert(decodeVersion0ArchRequirements(4) == DecodedArchRequirements{ 4, false });
static_assert(decodeVersion0ArchRequirements(5) == DecodedArchRequirements{ 4, true });
#endif

static constexpr DecodedArchRequirements decodeVersion1ArchRequirements(quint8 value)
{
    return { quint8(value & 0x7f), bool(value & 0x80) };
}
// self checks
static_assert(decodeVersion1ArchRequirements(0) == DecodedArchRequirements{ 0, false });
static_assert(decodeVersion1ArchRequirements(0x80) == DecodedArchRequirements{ 0, true });
#ifdef Q_PROCESSOR_X86
static_assert(decodeVersion1ArchRequirements(1) == DecodedArchRequirements{ 1, false });
static_assert(decodeVersion1ArchRequirements(3) == DecodedArchRequirements{ 3, false});
static_assert(decodeVersion1ArchRequirements(4) == DecodedArchRequirements{ 4, false });
static_assert(decodeVersion1ArchRequirements(0x82) == DecodedArchRequirements{ 2, true });
static_assert(decodeVersion1ArchRequirements(0x84) == DecodedArchRequirements{ 4, true });
#endif
} // unnamed namespace

QT_END_NAMESPACE

#endif // QPLUGIN_P_H
