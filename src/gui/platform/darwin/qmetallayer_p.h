// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMETALLAYER_P_H
#define QMETALLAYER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/qtguiglobal.h>

#include <QtCore/qreadwritelock.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/private/qcore_mac_p.h>

#include <QuartzCore/CAMetalLayer.h>

QT_BEGIN_NAMESPACE
class QReadWriteLock;

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(lcMetalLayer, Q_GUI_EXPORT)

QT_END_NAMESPACE

#if defined(__OBJC__)
Q_GUI_EXPORT
#endif
QT_DECLARE_NAMESPACED_OBJC_INTERFACE(QMetalLayer, CAMetalLayer
@property (nonatomic, readonly) QT_PREPEND_NAMESPACE(QReadWriteLock) &displayLock;
@property (atomic, copy) void (^mainThreadPresentation)();
)

#endif // QMETALLAYER_P_H
