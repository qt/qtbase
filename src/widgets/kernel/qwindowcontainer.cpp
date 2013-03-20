/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowcontainer_p.h"
#include "qwidget_p.h"
#include <QtGui/qwindow.h>

QT_BEGIN_NAMESPACE

class QWindowContainerPrivate : public QWidgetPrivate
{
public:
    Q_DECLARE_PUBLIC(QWindowContainer)

    QWindowContainerPrivate() : window(0), oldFocusWindow(0) { }
    ~QWindowContainerPrivate() { }

    QPointer<QWindow> window;
    QWindow *oldFocusWindow;
};



/*!
    \fn QWidget *QWidget::createWindowContainer(QWindow *window, QWidget *parent, Qt::WindowFlags flags);

    Creates a QWidget that makes it possible to embed \a window into
    a QWidget-based application.

    The window container is created as a child of \a parent and with
    window flags \a flags.

    Once the window has been embedded into the container, the
    container will control the window's geometry and
    visibility. Explicit calls to QWindow::setGeometry(),
    QWindow::show() or QWindow::hide() on an embedded window is not
    recommended.

    The container takes over ownership of \a window. The window can
    be removed from the window container with a call to
    QWindow::setParent().

    The window container has a number of known limitations:

    \list

    \li Stacking order; The embedded window will stack on top of the
    widget hierarchy as an opaque box. The stacking order of multiple
    overlapping window container instances is undefined.

    \li Window Handles; The window container will explicitly invoke
    winId() which will force the use of native window handles
    inside the application. See \l {Native Widgets vs Alien Widgets}
    {QWidget documentation} for more details.

    \li Rendering Integration; The window container does not interoperate
    with QGraphicsProxyWidget, QWidget::render() or similar functionality.

    \li Focus Handling; It is possible to let the window container
    instance have any focus policy and it will delegate focus to the
    window via a call to QWindow::requestActivate(). However,
    returning to the normal focus chain from the QWindow instance will
    be up to the QWindow instance implementation itself. For instance,
    when entering a Qt Quick based window with tab focus, it is quite
    likely that further tab presses will only cycle inside the QML
    application. Also, whether QWindow::requestActivate() actually
    gives the window focus, is platform dependent.

    \li Using many window container instances in a QWidget-based
    application can greatly hurt the overall performance of the
    application.

    \endlist
 */

QWidget *QWidget::createWindowContainer(QWindow *window, QWidget *parent, Qt::WindowFlags flags)
{
    return new QWindowContainer(window, parent, flags);
}



/*!
    \internal
 */

QWindowContainer::QWindowContainer(QWindow *embeddedWindow, QWidget *parent, Qt::WindowFlags flags)
    : QWidget(*new QWindowContainerPrivate, parent, flags)
{
    Q_D(QWindowContainer);
    if (!embeddedWindow) {
        qWarning("QWindowContainer: embedded window cannot be null");
        return;
    }

    d->window = embeddedWindow;

    // We force this window to become a native window and reparent the
    // window directly to it. This is done so that the order in which
    // the QWindowContainer is added to a QWidget tree and when it
    // gets a window does not matter.
    winId();
    d->window->setParent(windowHandle());

    connect(QGuiApplication::instance(), SIGNAL(focusWindowChanged(QWindow *)), this, SLOT(focusWindowChanged(QWindow *)));
}



/*!
    \internal
 */

QWindowContainer::~QWindowContainer()
{
    Q_D(QWindowContainer);
    delete d->window;
}



/*!
    \internal
 */

void QWindowContainer::focusWindowChanged(QWindow *focusWindow)
{
    Q_D(QWindowContainer);
    d->oldFocusWindow = focusWindow;
}



/*!
    \internal
 */

bool QWindowContainer::event(QEvent *e)
{
    Q_D(QWindowContainer);
    if (!d->window)
        return QWidget::event(e);

    QEvent::Type type = e->type();
    switch (type) {
    case QEvent::ChildRemoved: {
        QChildEvent *ce = static_cast<QChildEvent *>(e);
        if (ce->child() == d->window)
            d->window = 0;
        break;
    }
    // The only thing we are interested in is making sure our sizes stay
    // in sync, so do a catch-all case.
    case QEvent::Resize:
    case QEvent::Move:
    case QEvent::PolishRequest:
        d->window->setGeometry(0, 0, width(), height());
        break;
    case QEvent::Show:
        d->window->show();
        break;
    case QEvent::Hide:
        d->window->hide();
        break;
    case QEvent::FocusIn:
        if (d->oldFocusWindow != d->window) {
            d->window->requestActivate();
        } else {
            QWidget *next = nextInFocusChain();
            next->setFocus();
        }
        break;
    default:
        break;
    }

    return QWidget::event(e);
}

QT_END_NAMESPACE
