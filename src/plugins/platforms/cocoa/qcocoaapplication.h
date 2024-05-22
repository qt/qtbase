// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (c) 2007-2008, Apple, Inc.
// SPDX-License-Identifier: BSD-3-Clause

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp, qcolor_x11.cpp, qfiledialog.cpp
// and many other.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#ifndef QCOCOAAPPLICATION_H
#define QCOCOAAPPLICATION_H

#include <qglobal.h>
#include <QtCore/private/qcore_mac_p.h>

QT_DECLARE_NAMESPACED_OBJC_INTERFACE(QNSApplication, NSApplication)

QT_BEGIN_NAMESPACE

void qt_redirectNSApplicationSendEvent();
void qt_resetNSApplicationSendEvent();

QT_END_NAMESPACE

#endif // QCOCOAAPPLICATION_H
