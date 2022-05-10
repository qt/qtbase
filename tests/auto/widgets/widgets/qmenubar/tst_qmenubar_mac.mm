// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#import <Cocoa/Cocoa.h>

#include <QMenuBar>
#include <QTest>

bool tst_qmenubar_taskQTBUG56275(QMenuBar *menubar)
{
    NSMenu *mainMenu = menubar->toNSMenu();
    return mainMenu.numberOfItems == 2
        && [[mainMenu itemAtIndex:1].title isEqualToString:@"menu"];
}
