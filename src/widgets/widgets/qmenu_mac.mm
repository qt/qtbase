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

#include <qtwidgetsglobal.h>

QT_USE_NAMESPACE

#include "qmenu.h"
#if QT_CONFIG(menubar)
#include "qmenubar.h"
#include "qmenubar_p.h"
#endif

#include <QtCore/QDebug>
#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformmenu_p.h>

using namespace QNativeInterface::Private;

QT_BEGIN_NAMESPACE

#if QT_CONFIG(menu)

/*!
    \fn NSMenu *QMenu::toNSMenu()
    \since 5.2

    Returns the native NSMenu for this menu. Available on \macos only.

    \note Qt sets the delegate on the native menu. If you need to set your own
    delegate, make sure you save the original one and forward any calls to it.
*/
NSMenu *QMenu::toNSMenu()
{
    Q_D(QMenu);
    if (auto *cocoaPlatformMenu = dynamic_cast<QCocoaMenu *>(d->createPlatformMenu()))
        return cocoaPlatformMenu->nsMenu();

    return nil;
}


/*!
    \fn void QMenu::setAsDockMenu()
    \since 5.2

    Set this menu to be the dock menu available by option-clicking
    on the application dock icon. Available on \macos only.
*/
void QMenu::setAsDockMenu()
{
    Q_D(QMenu);
    if (auto *cocoaPlatformMenu = dynamic_cast<QCocoaMenu *>(d->createPlatformMenu()))
        cocoaPlatformMenu->setAsDockMenu();
}

void QMenuPrivate::moveWidgetToPlatformItem(QWidget *widget, QPlatformMenuItem* item)
{
    // Hide the widget before we mess with it
    widget->hide();

    // Move out of QMenu, since this widget will live in the native menu item
    widget->setParent(nullptr);

    // Make sure the widget doesn't prevent quitting the application,
    // just because it's a parent-less (top level) window.
    widget->setAttribute(Qt::WA_QuitOnClose, false);

    // And that it blends nicely with the native menu background
    widget->setAttribute(Qt::WA_TranslucentBackground);

    // Trigger creation of the backing QWindow, the platform window, and its
    // underlying NSView and NSWindow. At this point the widget is still hidden,
    // so the corresponding NSWindow that is created is not shown.
    widget->setAttribute(Qt::WA_NativeWindow);
    QWindow *widgetWindow = widget->windowHandle();
    widgetWindow->create();

    // Inform the window that it's actually a sub-window. This
    // ensures that we dispose of the NSWindow when the widget is
    // finally shown. We need to do this on a QWindow level, as
    // QWidget will ignore the flag if there is no parentWidget().
    // And we need to do it after creating the platform window, as
    // QWidget will overwrite the window flags during creation.
    widgetWindow->setFlag(Qt::SubWindow);

    // Finally, we can associate the underlying NSView with the menu item,
    // and show it. This will dispose of the created NSWindow, due to
    // the Qt::SubWindow flag above. The widget will not actually be
    // visible until it's re-parented into the NSMenu hierarchy.
    item->setNativeContents(WId(widgetWindow->winId()));
    widget->show();
}

#endif // QT_CONFIG(menu)

#if QT_CONFIG(menubar)

/*!
    \fn NSMenu *QMenuBar::toNSMenu()
    \since 5.2

    Returns the native NSMenu for this menu bar. Available on \macos only.

    \note Qt may set the delegate on the native menu bar. If you need to set your
    own delegate, make sure you save the original one and forward any calls to it.
*/
NSMenu *QMenuBar::toNSMenu()
{
    if (auto *cocoaMenuBar = dynamic_cast<QCocoaMenuBar *>(platformMenuBar()))
        return cocoaMenuBar->nsMenu();

    return nil;
}
#endif // QT_CONFIG(menubar)

QT_END_NAMESPACE

