// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

