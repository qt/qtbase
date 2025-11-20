/****************************************************************************
**
** Copyright (C) 2025 The Qt Company Ltd.
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
******************************************************************************/

#if 0
#pragma qt_class(QtStdLibDetection)
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#ifndef QSTDLIBDETECTION_H
#define QSTDLIBDETECTION_H

#ifdef __cplusplus

/* If <version> exists, include it. */
/* If not, we need to include _something_, and <utility> is included by qcompilerdetection.h, too */
#if __has_include(<version>)
#  include <version>
#else
#  include <utility>
#endif

/*
   The std lib, must be one of: (Q_STL_x)

     LIBCPP       - libc++ (shipped with Clang, e.g.)
     LIBSTDCPP    - libstdc++ (shipped with GCC, e.g.)
     MSSTL        - Microsoft STL
     DINKUMWARE   - Dinkumware (shipped with QNX, VxWorks, Integrity, origin of MSSTL)
     STLPORT      - STLport (merged with SGI)
     SGI          - The original STL
     ROGUEWAVE    - RogueWave ((used to be) popular on ARM?)

   Not included:
     EASTL        - EASTL (this is not a drop-in STL, e.g. it doesn't have <vector>-style headers)

   Should be sorted most to least authoritative.
*/

#if defined(_LIBCPP_VERSION) /* libc++ */
#  define Q_STL_LIBCPP
#elif defined(__GLIBCXX__) || defined(__GLIBCPP__) /* libstdc++ */
#  define Q_STL_LIBSTDCPP
#elif defined(_MSVC_STL_VERSION) /* MSSTL (must be before Dinkumware) */
#  define Q_STL_MSSTL
#elif defined(_YVALS) || defined(_CPPLIB_VER) /* Dinkumware */
#  define Q_STL_DINKUMWARE
#elif defined(_STLPORT_VERSION) /* STLport, cf. _stlport_version.h */
#  define Q_STL_STLPORT
#elif defined(__SGI_STL) /* must be after STLport, which mimics as SGI STL */
#  define Q_STL_SGI
#elif defined(_RWSTD_VER) /* RogueWave, at least as contributed to Apache stdcxx, cf. rw/_config.h */
#  define Q_STL_ROGUEWAVE
#else
#  error Unknown std library implementation, please file a report at bugreports.qt.io.
#endif

#endif // __cplusplus

#endif // QSTDLIBDETECTION_H
