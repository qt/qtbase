// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#import <AppKit/AppKit.h>

#include <QMenu>
#include <QTest>

void tst_qmenu_QTBUG_37933_ampersands()
{
    QMenu m;
    QFETCH(QString, title);
    QFETCH(QString, visibleTitle);
    m.addAction(title);

    NSMenu* nativeMenu = m.toNSMenu();
    Q_ASSERT(nativeMenu != 0);
    NSMenuItem* item = [nativeMenu itemAtIndex:0];
    Q_ASSERT(item != 0);
    QCOMPARE(QString::fromUtf8([[item title] UTF8String]), visibleTitle);
}
