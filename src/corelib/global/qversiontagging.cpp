/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation.
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

#include "qglobal.h"

#define SYM QT_MANGLE_NAMESPACE(qt_version_tag)
//#define SSYM QT_STRINGIFY(SYM)

#if defined(Q_CC_GNU) && defined(Q_OF_ELF) && !defined(Q_OS_ANDROID)
#  define make_versioned_symbol2(sym, m, n, separator)     \
    Q_CORE_EXPORT extern const char sym ## _ ## m ## _ ## n = 0; \
    asm(".symver " QT_STRINGIFY(sym) "_" QT_STRINGIFY(m) "_" QT_STRINGIFY(n) ", " \
        QT_STRINGIFY(sym) separator "Qt_" QT_STRINGIFY(m) "." QT_STRINGIFY(n))
#else
#  define make_versioned_symbol2(sym, m, n, separator)
#endif
#define make_versioned_symbol(sym, m, n, separator)    make_versioned_symbol2(sym, m, n, separator)

extern "C" {
#if QT_VERSION_MINOR > 0
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 0, "@");
#endif
#if QT_VERSION_MINOR > 1
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 1, "@");
#endif
#if QT_VERSION_MINOR > 2
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 2, "@");
#endif
#if QT_VERSION_MINOR > 3
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 3, "@");
#endif
#if QT_VERSION_MINOR > 4
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 4, "@");
#endif
#if QT_VERSION_MINOR > 5
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 5, "@");
#endif
#if QT_VERSION_MINOR > 6
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 6, "@");
#endif
#if QT_VERSION_MINOR > 7
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 7, "@");
#endif
#if QT_VERSION_MINOR > 8
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 8, "@");
#endif
#if QT_VERSION_MINOR > 9
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 9, "@");
#endif
#if QT_VERSION_MINOR > 10
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 10, "@");
#endif
#if QT_VERSION_MINOR > 11
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 11, "@");
#endif
#if QT_VERSION_MINOR > 12
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 12, "@");
#endif
#if QT_VERSION_MINOR > 13
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 13, "@");
#endif
#if QT_VERSION_MINOR > 14
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 14, "@");
#endif
#if QT_VERSION_MINOR > 15
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 15, "@");
#endif
#if QT_VERSION_MINOR > 16
// We don't expect there will be a Qt 5.17
#  error "Please update this file with more Qt versions."
#endif

// the default version:
make_versioned_symbol(SYM, QT_VERSION_MAJOR, QT_VERSION_MINOR, "@@");
}
