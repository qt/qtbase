/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QABSTRACTPLATFORMMENUBAR_P_H
#define QABSTRACTPLATFORMMENUBAR_P_H

#include <qfactoryinterface.h>
#include <qglobal.h>
#include <qplugin.h>

#ifndef QT_NO_MENUBAR

QT_BEGIN_NAMESPACE

class QAction;
class QActionEvent;
class QEvent;
class QMenuBar;
class QObject;
class QWidget;

class QAbstractPlatformMenuBar;

struct QPlatformMenuBarFactoryInterface : public QFactoryInterface
{
    virtual QAbstractPlatformMenuBar *create() = 0;
};

#define QPlatformMenuBarFactoryInterface_iid "com.nokia.qt.QPlatformMenuBarFactoryInterface"
Q_DECLARE_INTERFACE(QPlatformMenuBarFactoryInterface, QPlatformMenuBarFactoryInterface_iid)

/*!
    The platform-specific implementation of a menubar
*/
class QAbstractPlatformMenuBar
{
public:
    virtual ~QAbstractPlatformMenuBar() {}

    virtual void init(QMenuBar *) = 0;

    virtual void setVisible(bool visible) = 0;

    virtual void actionEvent(QActionEvent *) = 0;

    virtual void handleReparent(QWidget *oldParent, QWidget *newParent, QWidget *oldWindow, QWidget *newWindow) = 0;

    virtual bool allowCornerWidgets() const = 0;

    virtual void popupAction(QAction *) = 0;

    virtual void setNativeMenuBar(bool) = 0;

    virtual bool isNativeMenuBar() const = 0;

    /*!
        Return true if the native menubar is capable of listening to the
        shortcut keys. If false is returned, QMenuBar will trigger actions on
        shortcut itself.
    */
    virtual bool shortcutsHandledByNativeMenuBar() const = 0;

    virtual bool menuBarEventFilter(QObject *, QEvent *event) = 0;
};

QT_END_NAMESPACE

#endif // QT_NO_MENUBAR

#endif // QABSTRACTPLATFORMMENUBAR_P_H
