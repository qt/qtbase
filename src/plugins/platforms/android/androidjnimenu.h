/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef ANDROIDJNIMENU_H
#define ANDROIDJNIMENU_H

#include <jni.h>
#include <qglobal.h>

QT_BEGIN_NAMESPACE

class QAndroidPlatformMenuBar;
class QAndroidPlatformMenu;
class QAndroidPlatformMenuItem;
class QWindow;
class QRect;
class QPoint;

namespace QtAndroidMenu
{
    // Menu support
    void openOptionsMenu();
    void showContextMenu(QAndroidPlatformMenu *menu, const QRect &anchorRect, JNIEnv *env);
    void hideContextMenu(QAndroidPlatformMenu *menu);
    void syncMenu(QAndroidPlatformMenu *menu);
    void androidPlatformMenuDestroyed(QAndroidPlatformMenu *menu);

    void setMenuBar(QAndroidPlatformMenuBar *menuBar, QWindow *window);
    void setActiveTopLevelWindow(QWindow *window);
    void addMenuBar(QAndroidPlatformMenuBar *menuBar);
    void removeMenuBar(QAndroidPlatformMenuBar *menuBar);

    // Menu support
    bool registerNatives(JNIEnv *env);
}

QT_END_NAMESPACE

#endif // ANDROIDJNIMENU_H
