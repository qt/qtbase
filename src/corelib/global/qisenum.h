/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtCore/qglobal.h>

#ifndef QISENUM_H
#define QISENUM_H

#ifndef Q_IS_ENUM
#  if defined(Q_CC_GNU) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
#    define Q_IS_ENUM(x) __is_enum(x)
#  elif defined(Q_CC_MSVC) && defined(_MSC_FULL_VER) && (_MSC_FULL_VER >=140050215)
#    define Q_IS_ENUM(x) __is_enum(x)
#  elif defined(Q_CC_CLANG)
#    if __has_extension(is_enum)
#      define Q_IS_ENUM(x) __is_enum(x)
#    endif
#  endif
#endif

#ifndef Q_IS_ENUM
#  include <QtCore/qtypetraits.h>
#  define Q_IS_ENUM(x) QtPrivate::is_enum<x>::value
#endif

// shut up syncqt
QT_BEGIN_NAMESPACE
QT_END_NAMESPACE
#endif // QISENUM_H
