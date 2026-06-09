// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSJSMAIN_H
#define QOHOSJSMAIN_H

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QtOhos {

bool isOhosNoUiChildMode();
bool isGlBackingStoreDefaultEnabled();
bool isDebugDrawQtRasterBackingStoreFlushedRegionEnabled();
bool isDebugUseBasicStyleAndThemeEnabled();
bool isNativeNodeApiKeyEventsEnabled();
bool isNativeNodeApiMouseEventsEnabled();
bool isVsyncOnSoftwareBackingStoreEnabled();

bool acquireAndCleanPendingAutoStartedInstanceWindowFlag();

void quitApplicationFromJsThread();

void updateApplicationState(int state);

bool blockEventLoopsWhenSuspended();

}

QT_END_NAMESPACE

#endif // QOHOSJSMAIN_H
