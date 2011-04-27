/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QGUIAPPLICATION_QPA_P_H
#define QGUIAPPLICATION_QPA_P_H

#include <QtGui/qguiapplication_qpa.h>

#include <QtCore/private/qcoreapplication_p.h>

#include <QtCore/private/qthread_p.h>

#include <QWindowSystemInterface>
#include "private/qwindowsysteminterface_qpa_p.h"
#include "QtGui/qplatformintegration_qpa.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QPlatformIntegration;

class Q_GUI_EXPORT QGuiApplicationPrivate : public QCoreApplicationPrivate
{
    Q_DECLARE_PUBLIC(QGuiApplication)
public:
    QGuiApplicationPrivate(int &argc, char **argv, int flags);
    ~QGuiApplicationPrivate();

    static int keyboard_input_time;
    static int mouse_double_click_time;

    static Qt::KeyboardModifiers modifier_buttons;
    static Qt::MouseButtons mouse_buttons;

    static QPlatformIntegration *platform_integration;

    static QPlatformIntegration *platformIntegration()
    { return platform_integration; }

    static QAbstractEventDispatcher *qt_qpa_core_dispatcher()
    { return QCoreApplication::instance()->d_func()->threadData->eventDispatcher; }

    static void processMouseEvent(QWindowSystemInterfacePrivate::MouseEvent *e);
    static void processKeyEvent(QWindowSystemInterfacePrivate::KeyEvent *e);
    static void processWheelEvent(QWindowSystemInterfacePrivate::WheelEvent *e);
    static void processTouchEvent(QWindowSystemInterfacePrivate::TouchEvent *e);

    static void processCloseEvent(QWindowSystemInterfacePrivate::CloseEvent *e);

    static void processGeometryChangeEvent(QWindowSystemInterfacePrivate::GeometryChangeEvent *e);

    static void processEnterEvent(QWindowSystemInterfacePrivate::EnterEvent *e);
    static void processLeaveEvent(QWindowSystemInterfacePrivate::LeaveEvent *e);

    static void processActivatedEvent(QWindowSystemInterfacePrivate::ActivatedWindowEvent *e);

    static void processWindowSystemEvent(QWindowSystemInterfacePrivate::WindowSystemEvent *e);

    static void reportScreenCount(QWindowSystemInterfacePrivate::ScreenCountEvent *e);
    static void reportGeometryChange(QWindowSystemInterfacePrivate::ScreenGeometryEvent *e);
    static void reportAvailableGeometryChange(QWindowSystemInterfacePrivate::ScreenAvailableGeometryEvent *e);

    static bool app_do_modal;

    static QPointer<QWidget> qt_last_mouse_receiver;

    static QWidgetList qt_modal_stack;

    static Qt::MouseButtons buttons;
    static ulong mousePressTime;
    static Qt::MouseButton mousePressButton;
    static int mousePressX;
    static int mousePressY;
    static int mouse_double_click_distance;

private:
    void init();

    static QGuiApplicationPrivate *self;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QGUIAPPLICATION_QPA_P_H
