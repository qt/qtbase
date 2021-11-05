/****************************************************************************
**
** Copyright (C) 2021 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QT_VERSION_MAJOR
#  include <QtCore/qglobal.h>
#endif

extern "C" void *qt_plugin_instance()
{
    return nullptr;
}

#ifdef QT_DEBUG
static constexpr bool IsDebug = true;
#else
static constexpr bool IsDebug = false;
#endif

#ifndef PLUGIN_VERSION
#  define PLUGIN_VERSION    (QT_VERSION_MAJOR >= 7 ? 1 : 0)
#endif
#if PLUGIN_VERSION == 1
#  define PLUGIN_HEADER     1, QT_VERSION_MAJOR, 0, IsDebug ? 0x80 : 0
#else
#  define PLUGIN_HEADER     0, QT_VERSION_MAJOR, 0, IsDebug
#endif

#if defined(__ELF__) && PLUGIN_VERSION >= 1
// GCC will produce:
// fakeplugin.cpp:64:3: warning: ‘no_sanitize’ attribute ignored [-Wattributes]
__attribute__((section(".note.qt.metadata"), used, no_sanitize("address"), aligned(sizeof(void*))))
static const struct {
    unsigned n_namesz = sizeof(name);
    unsigned n_descsz = sizeof(payload);
    unsigned n_type = 0x74510001;
    char name[12] = "qt-project!";
    alignas(unsigned) unsigned char payload[2 + 4] = {
        PLUGIN_HEADER,
        0xbf,
        0xff,
    };
} qtnotemetadata;
#elif PLUGIN_VERSION >= 0
#  ifdef _MSC_VER
#    pragma section(".qtmetadata",read,shared)
__declspec(allocate(".qtmetadata"))
#  elif defined(__APPLE__)
__attribute__ ((section ("__TEXT,qtmetadata"), used))
#  else
__attribute__ ((section(".qtmetadata"), used))
#  endif
static const unsigned char qtmetadata[] = {
    'Q', 'T', 'M', 'E', 'T', 'A', 'D', 'A', 'T', 'A', ' ', '!',
    PLUGIN_HEADER,
    0xbf,
    0xff,
};
#endif
