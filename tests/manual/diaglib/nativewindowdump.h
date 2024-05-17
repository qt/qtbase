// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef _NATIVEWINDOWDUMP_
#define _NATIVEWINDOWDUMP_

#include <QtGui/qwindowdefs.h>

namespace QtDiag {

void dumpNativeWindows(WId root = 0);
void dumpNativeQtTopLevels();

} // namespace QtDiag

#endif
