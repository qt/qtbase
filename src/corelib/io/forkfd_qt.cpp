// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qglobal.h>

#define FORKFD_NO_SPAWNFD
#if defined(QT_NO_DEBUG) && !defined(NDEBUG)
#  define NDEBUG
#endif

#include <forkfd.h>
#include "../../3rdparty/forkfd/forkfd.c"
