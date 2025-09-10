// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/*
 * This is a precompiled header file for use in Xcode / Mac GCC /
 * GCC >= 3.4 / VC to greatly speed the building of Qt. It may also be
 * of use to people developing their own project, but it is probably
 * better to define your own header.  Use of this header is currently
 * UNSUPPORTED.
 */


#if defined __cplusplus
// for rand_s, _CRT_RAND_S must be #defined before #including stdlib.h.
// put it at the beginning so some indirect inclusion doesn't break it
#ifndef _CRT_RAND_S
#  define _CRT_RAND_S
#endif
#include <stdlib.h>
#include <qglobal.h>
#ifdef Q_OS_WIN
#  ifdef Q_CC_MINGW
// <unistd.h> must be included before any other header pulls in <time.h>.
#    include <unistd.h> // Define _POSIX_THREAD_SAFE_FUNCTIONS to obtain localtime_r()
#  endif // Q_CC_MINGW
#  define _POSIX_
#  include <limits.h>
#  undef _POSIX_
#endif // Q_OS_WIN
#include <qcoreapplication.h>
#include <qcoreevent.h>
#include <qiodevice.h>
#include <qlist.h>
#include <qvariant.h> /* All moc generated code has this include */
#include <qobject.h>
#if QT_CONFIG(regularexpression)
#  include <qregularexpression.h>
#endif
#include <qscopedpointer.h>
#include <qshareddata.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtimer.h>
#endif
