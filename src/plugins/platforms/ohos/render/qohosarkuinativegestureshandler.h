// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSARKUINATIVEGESTURESHANDLER_H
#define QOHOSARKUINATIVEGESTURESHANDLER_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtGui/qwindow.h>
#include <qarkui/qembeddedwindownode.h>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

QOhosConsumer<const QArkUi::NativeGestureInfo &> makeQOhosArkUiNativeGesturesHandler(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef);

QT_END_NAMESPACE

#endif
