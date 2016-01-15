/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

#import <AppKit/AppKit.h>
#include <QtCore/private/qcore_mac_p.h>

@interface QT_MANGLE_NAMESPACE(QCocoaMenuLoader) : NSResponder
{
    IBOutlet NSMenu *theMenu;
    IBOutlet NSMenu *appMenu;
    IBOutlet NSMenuItem *quitItem;
    IBOutlet NSMenuItem *preferencesItem;
    IBOutlet NSMenuItem *aboutItem;
    IBOutlet NSMenuItem *aboutQtItem;
    IBOutlet NSMenuItem *hideItem;
    NSMenuItem *lastAppSpecificItem;
    NSMenuItem *servicesItem;
    NSMenuItem *hideAllOthersItem;
    NSMenuItem *showAllItem;
}
- (void)ensureAppMenuInMenu:(NSMenu *)menu;
- (void)removeActionsFromAppMenu;
- (NSMenu *)applicationMenu;
- (NSMenu *)menu;
- (NSMenuItem *)quitMenuItem;
- (NSMenuItem *)preferencesMenuItem;
- (NSMenuItem *)aboutMenuItem;
- (NSMenuItem *)aboutQtMenuItem;
- (NSMenuItem *)hideMenuItem;
- (NSMenuItem *)appSpecificMenuItem:(NSInteger)tag;
- (IBAction)terminate:(id)sender;
- (IBAction)orderFrontStandardAboutPanel:(id)sender;
- (IBAction)hideOtherApplications:(id)sender;
- (IBAction)unhideAllApplications:(id)sender;
- (IBAction)hide:(id)sender;
- (IBAction)qtDispatcherToQPAMenuItem:(id)sender;
- (void)orderFrontCharacterPalette:(id)sender;
- (BOOL)validateMenuItem:(NSMenuItem*)menuItem;
- (void)qtTranslateApplicationMenu;
- (NSArray *)mergeable;
@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QCocoaMenuLoader);

QT_BEGIN_NAMESPACE

void qt_mac_loadMenuNib(QCocoaMenuLoader *qtMenuLoader);

QT_END_NAMESPACE

#endif // QCOCOAMENULOADER_P_H
