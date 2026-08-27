// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qaccessiblewindow_p.h"

#if QT_CONFIG(accessibility)

#include <QtGui/qguiapplication.h>
#include <QtGui/qwindow.h>
#include <QtGui/private/qwindow_p.h>

QT_BEGIN_NAMESPACE

/*
    Accessibility interface for a QWindow.

    Reports the window itself, with the window's child QWindows as its
    children, so that content Qt hosts in a child window is reachable even
    on a platform bridge that does not know about native windows.

    A window reaches the accessibility tree only when concrete QWindow
    subclasses opt in, by registering an accessible factory, and implementing
    accessibleRoot(). A plain QWindow does not do this, which keeps Qt's own
    implementation-detail windows out of the tree.

    The node reports window(). A bridge that does know about native windows
    reads it to find the native handle, and substitutes that handle for the
    node, in which case the native element supplies its own children.
*/
QAccessibleWindow::QAccessibleWindow(QWindow *window)
    : QAccessibleObject(window)
{
}

QWindow *QAccessibleWindow::window() const
{
    return static_cast<QWindow *>(object());
}

/*
    Resolves the interfaces of the window's child QWindows.

    Each child is resolved through its own accessibleRoot(), so a child that
    has not opted into the tree contributes nothing, and one that represents
    something else contributes whatever represents it.

    The children come out in stacking order, bottom first, as a result of the
    order QWindow::raise() and QWindow::lower() maintains.
*/
QList<QAccessibleInterface *> QAccessibleWindow::childInterfaces() const
{
    QList<QAccessibleInterface *> interfaces;

    if (!isValid())
        return interfaces;

    const auto childWindows = window()->findChildren<QWindow *>(Qt::FindDirectChildrenOnly);
    interfaces.reserve(childWindows.size());
    for (QWindow *childWindow : childWindows) {
        if (QAccessibleInterface *iface = childWindow->accessibleRoot())
            interfaces.append(iface);
    }

    return interfaces;
}

int QAccessibleWindow::childCount() const
{
    return childInterfaces().size();
}

int QAccessibleWindow::indexOfChild(const QAccessibleInterface *child) const
{
    if (!child)
        return -1;

    // Compares interfaces, not objects, as a child window's interface is
    // not necessarily an interface for the window itself.
    return childInterfaces().indexOf(const_cast<QAccessibleInterface *>(child));
}

QAccessibleInterface *QAccessibleWindow::child(int index) const
{
    const QList<QAccessibleInterface *> children = childInterfaces();
    if (index >= 0 && index < children.size())
        return children.at(index);
    return nullptr;
}

QAccessibleInterface *QAccessibleWindow::parent() const
{
    if (!isValid())
        return nullptr;

    // A window hosted by a window container reflects the container
    if (QObject *a11yParent = QWindowPrivate::get(window())->accessibleParent)
        return QAccessible::queryAccessibleInterface(a11yParent);

    // A child window with no container parent belongs to the window
    // it is a child of, if that window is in the tree as well.
    if (QWindow *parentWindow = window()->parent()) {
        if (QAccessibleInterface *iface = parentWindow->accessibleRoot())
            return iface;
    }

    return QAccessible::queryAccessibleInterface(qApp);
}

QAccessibleInterface *QAccessibleWindow::focusChild() const
{
    const QWindow *focusWindow = QGuiApplication::focusWindow();

    for (QAccessibleInterface *child : childInterfaces()) {
        if (QAccessibleInterface *focus = child->focusChild())
            return focus;
        // The child window may hold the focus without reporting anything
        // within it as focused, in which case the child is the answer.
        if (child->window() == focusWindow)
            return child;
    }

    return nullptr;
}

QString QAccessibleWindow::text(QAccessible::Text text) const
{
    if (!isValid())
        return {};

    if (text == QAccessible::Name && window()->isTopLevel())
        return window()->title();

    return {};
}

QAccessible::Role QAccessibleWindow::role() const
{
    if (isValid() && window()->isTopLevel())
        return QAccessible::Window;

    // A window that is not top level is hosted by something, and presents
    // a client area rather than a window of its own.
    return QAccessible::Client;
}

QAccessible::State QAccessibleWindow::state() const
{
    QAccessible::State state;

    if (!isValid()) {
        state.invalid = true;
        return state;
    }

    state.invisible = !window()->isVisible();
    state.focused = window() == QGuiApplication::focusWindow();
    state.active = window()->isActive();

    return state;
}

QRect QAccessibleWindow::rect() const
{
    if (!isValid())
        return {};

    return QRect(window()->mapToGlobal(QPoint(0, 0)), window()->size());
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
