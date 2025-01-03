// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWIN10HELPERS_H
#define QWIN10HELPERS_H

#include <QtCore/qglobal.h>
#include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

bool qt_windowsIsTabletMode(HWND hwnd);

QT_END_NAMESPACE

#endif // QWIN10HELPERS_H
