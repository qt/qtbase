/****************************************************************************
**
** Copyright (C) 2015 Intel Corporation.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

// qglobal.h includes this header, so keep it outside of our include guards
#include <QtCore/qglobal.h>

#if !defined(QVERSIONTAGGING_H)
#define QVERSIONTAGGING_H

QT_BEGIN_NAMESPACE

/*
 * Ugly hack warning and explanation:
 *
 * This file causes all ELF modules, be they libraries or applications, to use the
 * qt_version_tag symbol that is present in QtCore. Such symbol is versioned,
 * so the linker will automatically pull the current Qt version and add it to
 * the ELF header of the library/application. The assembly produces one section
 * called ".qtversion" containing two 32-bit values. The first is a
 * relocation to the qt_version_tag symbol (which is what causes the ELF
 * version to get used). The second value is the current Qt version at the time
 * of compilation.
 *
 * There will only be one copy of the section in the output library or application.
 */

#if defined(QT_BUILD_CORE_LIB) || defined(QT_BOOTSTRAPPED) || defined(QT_NO_VERSION_TAGGING)
// don't make tags in QtCore, bootstrapped systems or if the user asked not to
#elif defined(Q_CC_GNU) && !defined(Q_OS_ANDROID)
#  if defined(Q_PROCESSOR_X86) && (defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD_KERNEL))
#    if defined(Q_PROCESSOR_X86_64)   // x86-64 or x32
#      define QT_VERSION_TAG_RELOC(sym) ".quad " QT_STRINGIFY(QT_MANGLE_NAMESPACE(sym)) "@GOT\n"
#    else                               // x86
#      define QT_VERSION_TAG_RELOC(sym) ".long " QT_STRINGIFY(QT_MANGLE_NAMESPACE(sym)) "@GOT\n"
#    endif
#    define QT_VERSION_TAG(sym) \
    asm (   \
    ".section .qtversion, \"aG\", @progbits, qt_version_tag, comdat\n" \
    ".align 8\n" \
    QT_VERSION_TAG_RELOC(sym) \
    ".long " QT_STRINGIFY(QT_VERSION) "\n" \
    ".align 8\n" \
    ".previous" \
    )
#  endif
#endif

#if defined(QT_VERSION_TAG)
QT_VERSION_TAG(qt_version_tag);
#endif

QT_END_NAMESPACE

#endif // QVERSIONTAGGING_H
