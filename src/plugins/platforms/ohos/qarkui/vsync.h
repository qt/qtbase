// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QARKUI__VSYNC_H
#define QARKUI__VSYNC_H

#include <QtCore/qglobal.h>
#include <functional>
#include <native_window/external_window.h>

QT_BEGIN_NAMESPACE

namespace QArkUi {

std::function<void()> makeVSyncFrameRequester(
    ::OHNativeWindow *nativeWindow, std::function<void()> vsyncFrameReadyFunc);

}

QT_END_NAMESPACE

#endif
