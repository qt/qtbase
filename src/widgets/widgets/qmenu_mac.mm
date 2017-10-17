/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#include "qmenu.h"
#if QT_CONFIG(menubar)
#include "qmenubar.h"
#include "qmenubar_p.h"
#endif
#include "qmacnativewidget_mac.h"

#include <QtCore/QDebug>
#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(menu)

namespace {
// TODO use QtMacExtras copy of this function when available.
inline QPlatformNativeInterface::NativeResourceForIntegrationFunction resolvePlatformFunction(const QByteArray &functionName)
{
    QPlatformNativeInterface *nativeInterface = QGuiApplication::platformNativeInterface();
    QPlatformNativeInterface::NativeResourceForIntegrationFunction function =
        nativeInterface->nativeResourceFunctionForIntegration(functionName);
    if (Q_UNLIKELY(!function))
         qWarning("Qt could not resolve function %s from "
                  "QGuiApplication::platformNativeInterface()->nativeResourceFunctionForIntegration()",
                  functionName.constData());
    return function;
}
} //namespsace


/*!
    \since 5.2

    Returns the native NSMenu for this menu. Available on \macos only.

    \note Qt sets the delegate on the native menu. If you need to set your own
    delegate, make sure you save the original one and forward any calls to it.
*/
NSMenu *QMenu::toNSMenu()
{
    Q_D(QMenu);
    // Call into the cocoa platform plugin: qMenuToNSMenu(platformMenu())
    QPlatformNativeInterface::NativeResourceForIntegrationFunction function = resolvePlatformFunction("qmenutonsmenu");
    if (function) {
        typedef void* (*QMenuToNSMenuFunction)(QPlatformMenu *platformMenu);
        return reinterpret_cast<NSMenu *>(reinterpret_cast<QMenuToNSMenuFunction>(function)(d->createPlatformMenu()));
    }
    return nil;
}


/*!
    \since 5.2

    Set this menu to be the dock menu available by option-clicking
    on the application dock icon. Available on \macos only.
*/
void QMenu::setAsDockMenu()
{
    Q_D(QMenu);
    // Call into the cocoa platform plugin: setDockMenu(platformMenu())
    QPlatformNativeInterface::NativeResourceForIntegrationFunction function = resolvePlatformFunction("setdockmenu");
    if (function) {
        typedef void (*SetDockMenuFunction)(QPlatformMenu *platformMenu);
        reinterpret_cast<SetDockMenuFunction>(function)(d->createPlatformMenu());
    }
}


/*! \fn void qt_mac_set_dock_menu(QMenu *menu)
    \relates QMenu
    \deprecated

    Sets this \a menu to be the dock menu available by option-clicking
    on the application dock icon. Available on \macos only.

    Deprecated; use \l QMenu::setAsDockMenu() instead.
*/

void QMenuPrivate::moveWidgetToPlatformItem(QWidget *widget, QPlatformMenuItem* item)
{
    QMacNativeWidget *container = new QMacNativeWidget;
    QObject::connect(platformMenu, SIGNAL(destroyed()), container, SLOT(deleteLater()));
    container->resize(widget->sizeHint());
    widget->setParent(container);
    widget->setVisible(true);

    NSView *containerView = container->nativeView();
    QWindow *containerWindow = container->windowHandle();
    Qt::WindowFlags wf = containerWindow->flags();
    containerWindow->setFlags(wf | Qt::SubWindow);
    [(NSView *)widget->winId() setAutoresizingMask:NSViewWidthSizable];

    if (QPlatformNativeInterface::NativeResourceForIntegrationFunction function = resolvePlatformFunction("setEmbeddedInForeignView")) {
        typedef void (*SetEmbeddedInForeignViewFunction)(QPlatformWindow *window, bool embedded);
        reinterpret_cast<SetEmbeddedInForeignViewFunction>(function)(containerWindow->handle(), true);
    }

    item->setNativeContents((WId)containerView);
    container->show();
}

#endif // QT_CONFIG(menu)

#if QT_CONFIG(menubar)

/*!
    \since 5.2

    Returns the native NSMenu for this menu bar. Available on \macos only.

    \note Qt may set the delegate on the native menu bar. If you need to set your
    own delegate, make sure you save the original one and forward any calls to it.
*/
NSMenu *QMenuBar::toNSMenu()
{
    // Call into the cocoa platform plugin: qMenuBarToNSMenu(platformMenuBar())
    QPlatformNativeInterface::NativeResourceForIntegrationFunction function = resolvePlatformFunction("qmenubartonsmenu");
    if (function) {
        typedef void* (*QMenuBarToNSMenuFunction)(QPlatformMenuBar *platformMenuBar);
        return reinterpret_cast<NSMenu *>(reinterpret_cast<QMenuBarToNSMenuFunction>(function)(platformMenuBar()));
    }
    return nil;
}
#endif // QT_CONFIG(menubar)

QT_END_NAMESPACE

