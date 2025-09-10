// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOCOAMENULOADER_P_H
#define QCOCOAMENULOADER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qcore_mac_p.h>

QT_FORWARD_DECLARE_CLASS(QCocoaMenuItem);

QT_DECLARE_NAMESPACED_OBJC_INTERFACE(QCocoaMenuLoader, NSObject
+ (instancetype)sharedMenuLoader;
- (NSMenu *)menu;
- (void)ensureAppMenuInMenu:(NSMenu *)menu;
- (NSMenuItem *)quitMenuItem;
- (NSMenuItem *)preferencesMenuItem;
- (NSMenuItem *)aboutMenuItem;
- (NSMenuItem *)aboutQtMenuItem;
- (NSMenuItem *)hideMenuItem;
- (NSMenuItem *)appSpecificMenuItem:(QCocoaMenuItem *)platformItem;
- (void)qtTranslateApplicationMenu;
- (NSArray<NSMenuItem *> *)mergeable;
)

#endif // QCOCOAMENULOADER_P_H
