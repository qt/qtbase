// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QUIWINDOW_H
#define QUIWINDOW_H

#include <UIKit/UIWindow.h>

@interface QUIWindow : UIWindow
@property (nonatomic, readonly) BOOL sendingEvent;
@end

#endif // QUIWINDOW_H
