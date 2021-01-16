/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the qmake application of the Qt Toolkit.
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
****************************************************************************/

#ifndef QMAKE_PCH_H
#define QMAKE_PCH_H
// for rand_s, _CRT_RAND_S must be #defined before #including stdlib.h.
// put it at the beginning so some indirect inclusion doesn't break it
#ifndef _CRT_RAND_S
#define _CRT_RAND_S
#endif
#include <qglobal.h>
#ifdef Q_OS_WIN
# define _POSIX_
# include <limits.h>
# undef _POSIX_
#endif

#include <stdio.h>
//#include "makefile.h"
//#include "meta.h"
#include <qfile.h>
//#include "winmakefile.h"
//#include <qtextstream.h>
//#include "project.h"
#include <qstring.h>
#include <qstringlist.h>
#include <qhash.h>
#include <time.h>
#include <stdlib.h>
#include <qregexp.h>

//#include <qdir.h>
//#include "option.h"

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

#endif
