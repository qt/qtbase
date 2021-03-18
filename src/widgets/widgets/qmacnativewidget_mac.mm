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
#include "qmacnativewidget_mac.h"

#include <QtCore/qdebug.h>
#include <QtGui/qwindow.h>
#include <QtGui/qguiapplication.h>
#include <qpa/qplatformnativeinterface.h>

/*!
    \class QMacNativeWidget
    \since 4.5
    \brief The QMacNativeWidget class provides a widget for \macos that provides
    a way to put Qt widgets into Cocoa hierarchies.

    \ingroup advanced
    \inmodule QtWidgets

    On \macos, there is a difference between a window and view;
    normally expressed as widgets in Qt.  Qt makes assumptions about its
    parent-child hierarchy that make it complex to put an arbitrary Qt widget
    into a hierarchy of "normal" views from Apple frameworks. QMacNativeWidget
    bridges the gap between views and windows and makes it possible to put a
    hierarchy of Qt widgets into a non-Qt window or view.

    QMacNativeWidget pretends it is a window (i.e. isWindow() will return true),
    but it cannot be shown on its own. It needs to be put into a window
    when it is created or later through a native call.

    Here is an example showing how to put a QPushButton into a NSWindow:

    \snippet qmacnativewidget/main.mm 0

    Note that QMacNativeWidget requires knowledge of Cocoa. All it
    does is get the Qt hierarchy into a window not owned by Qt. It is then up
    to the programmer to ensure it is placed correctly in the window and
    responds correctly to events.
*/

QT_BEGIN_NAMESPACE

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

NSView *getEmbeddableView(QWindow *qtWindow)
{
    // Make sure the platform window is created
    qtWindow->create();

    // Inform the window that it's a subwindow of a non-Qt window. This must be
    // done after create() because we need to have a QPlatformWindow instance.
    // The corresponding NSWindow will not be shown and can be deleted later.
    typedef void (*SetEmbeddedInForeignViewFunction)(QPlatformWindow *window, bool embedded);
    reinterpret_cast<SetEmbeddedInForeignViewFunction>(resolvePlatformFunction("setEmbeddedInForeignView"))(qtWindow->handle(), true);

    // Get the Qt content NSView for the QWindow from the Qt platform plugin
    QPlatformNativeInterface *platformNativeInterface = QGuiApplication::platformNativeInterface();
    NSView *qtView = (NSView *)platformNativeInterface->nativeResourceForWindow("nsview", qtWindow);
    return qtView; // qtView is ready for use.
}

/*!
    Create a QMacNativeWidget with \a parentView as its "superview" (i.e.,
    parent). The \a parentView is  a NSView pointer.
*/
QMacNativeWidget::QMacNativeWidget(NSView *parentView)
    : QWidget(0)
{
    Q_UNUSED(parentView);

    //d_func()->topData()->embedded = true;
    setPalette(QPalette(Qt::transparent));
    setAttribute(Qt::WA_SetPalette, false);
    setAttribute(Qt::WA_LayoutUsesWidgetRect);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground, false);
}

/*!
    Destroy the QMacNativeWidget.
*/
QMacNativeWidget::~QMacNativeWidget()
{
}

/*!
    \reimp
*/
QSize QMacNativeWidget::sizeHint() const
{
    // QMacNativeWidget really does not have any other choice
    // than to fill its designated area.
    if (windowHandle())
        return windowHandle()->size();
    return QWidget::sizeHint();
}

/*!
    Returns the native view backing the QMacNativeWidget.

*/
NSView *QMacNativeWidget::nativeView() const
{
    winId();
    return getEmbeddableView(windowHandle());
}

/*!
    \reimp
*/
bool QMacNativeWidget::event(QEvent *ev)
{
    return QWidget::event(ev);
}

QT_END_NAMESPACE
