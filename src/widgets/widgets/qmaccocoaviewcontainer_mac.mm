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

#import <AppKit/AppKit.h>
#include "qmaccocoaviewcontainer_mac.h"

#include <QtCore/QDebug>
#include <QtGui/QWindow>
#include <qpa/qplatformnativeinterface.h>
#include <private/qwidget_p.h>
#include <private/qwindow_p.h>

/*!
    \class QMacCocoaViewContainer
    \since 4.5

    \brief The QMacCocoaViewContainer class provides a widget for \macos that can be used to wrap arbitrary
    Cocoa views (i.e., NSView subclasses) and insert them into Qt hierarchies.

    \ingroup advanced
    \inmodule QtWidgets

    While Qt offers a lot of classes for writing your application, Apple's
    Cocoa frameworks offer functionality that is not currently available (or
    may never end up) in Qt. Using QMacCocoaViewContainer, it is possible to take an
    arbitrary NSView-derived class from Cocoa and put it in a Qt widgets hierarchy.
    Depending on the level of integration you need, you can use QMacCocoaViewContainer
    directly or subclass it to wrap more functionality of the underlying NSView.

    It should be also noted that, at the Cocoa level, there is a difference
    between top-level windows and views (widgets that are inside a window).
    For this reason, make sure that the NSView that you are wrapping doesn't
    end up as a top-level window. The best way to ensure this is to make sure
    QMacCocoaViewContainer's parent widget is not null.

    If you are using QMacCocoaViewContainer as a subclass and are accessing Cocoa API,
    it is probably simpler to have your file end with \tt{.mm} instead of \tt{.cpp}.
    Most Apple tools will correctly identify the source as Objective-C++.

    QMacCocoaViewContainer requires knowledge of how Cocoa works, especially in
    regard to its reference counting (retain/release) nature. It is noted in
    the functions below if there is any change in the reference count. Cocoa
    views often generate temporary objects that are released by an autorelease
    pool. If this is done outside of a running event loop, it is up to the
    developer to provide the autorelease pool.

    The following is a snippet showing how to subclass QMacCocoaViewContainer
    to wrap an NSSearchField.
    \snippet macmainwindow.mm 0

*/

QT_BEGIN_NAMESPACE

class QMacCocoaViewContainerPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QMacCocoaViewContainer)
public:
    NSView *nsview;
    QMacCocoaViewContainerPrivate();
    ~QMacCocoaViewContainerPrivate();
};

QMacCocoaViewContainerPrivate::QMacCocoaViewContainerPrivate()
     : nsview(0)
{
}

QMacCocoaViewContainerPrivate::~QMacCocoaViewContainerPrivate()
{
    [nsview release];
}

/*!
    Create a new QMacCocoaViewContainer using the NSView pointer in
    the \a view with parent, \a parent. QMacCocoaViewContainer will
    retain the \a view.

*/
QMacCocoaViewContainer::QMacCocoaViewContainer(NSView *view, QWidget *parent)
   : QWidget(*new QMacCocoaViewContainerPrivate, parent, 0)
{
    // Ensures that we have a QWindow, even if we're not a top level widget
    setAttribute(Qt::WA_NativeWindow);

    setCocoaView(view);
}

/*!
    Destroy the QMacCocoaViewContainer and release the wrapped view.
*/
QMacCocoaViewContainer::~QMacCocoaViewContainer()
{

}

/*!
    Returns the NSView that has been set on this container.
*/
NSView *QMacCocoaViewContainer::cocoaView() const
{
    Q_D(const QMacCocoaViewContainer);
    return d->nsview;
}

/*!
    Sets \a view as the NSView to contain and retains it. If this
    container already had a view set, it will release the previously set view.
*/
void QMacCocoaViewContainer::setCocoaView(NSView *view)
{
    Q_D(QMacCocoaViewContainer);
    NSView *oldView = d->nsview;
    [view retain];
    d->nsview = view;

    // Get rid of QWindow completely, and re-create a new vanilla one, which
    // we will then re-configure to be a foreign window.
    destroy();
    create();

    // Can't use QWindow::fromWinId() here due to QWidget controlling its
    // QWindow, and can't use QWidget::createWindowContainer() due to the
    // QMacCocoaViewContainer class being public API instead of a factory
    // function.
    QWindow *window = windowHandle();
    window->destroy();
    qt_window_private(window)->create(false, WId(view));
    Q_ASSERT(window->handle());

    [oldView release];

    // The QWindow::destroy()) call above will explicitly hide this widget.
    // Clear the hidden state here so it can be implicitly shown again.
    setAttribute(Qt::WA_WState_Hidden, false);

}

QT_END_NAMESPACE
