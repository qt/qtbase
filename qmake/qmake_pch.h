// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

//#include <qdir.h>
//#include "option.h"

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

#endif
