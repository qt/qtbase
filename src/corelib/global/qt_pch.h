/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
#define _CRT_RAND_S
#endif
#include <stdlib.h>
#include <qglobal.h>
#ifdef Q_OS_WIN
# ifdef Q_CC_MINGW
// <unistd.h> must be included before any other header pulls in <time.h>.
#  include <unistd.h> // Define _POSIX_THREAD_SAFE_FUNCTIONS to obtain localtime_r()
# endif
# define _POSIX_
# include <limits.h>
# undef _POSIX_
#endif
#include <qcoreapplication.h>
#include <qcoreevent.h>
#include <qiodevice.h>
#include <qlist.h>
#include <qvariant.h>  /* All moc genereated code has this include */
#include <qobject.h>
#include <qregexp.h>
#include <qscopedpointer.h>
#include <qshareddata.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qvector.h>
#if QT_CONFIG(textcodec)
#include <qtextcodec.h>
#endif
#endif
