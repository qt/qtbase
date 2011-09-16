/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindow.h"

#include "qplatformwindow_qpa.h"
#include "qsurfaceformat.h"
#include "qplatformopenglcontext_qpa.h"
#include "qopenglcontext.h"
#include "qscreen.h"

#include "qwindow_p.h"
#include "qguiapplication_p.h"

#include <private/qevent_p.h>

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

QWindow::QWindow(QScreen *targetScreen)
    : QObject(*new QWindowPrivate(), 0)
    , QSurface(QSurface::Window)
{
    Q_D(QWindow);
    d->screen = targetScreen;
    if (!d->screen)
        d->screen = QGuiApplication::primaryScreen();
    QGuiApplicationPrivate::window_list.prepend(this);
}

QWindow::QWindow(QWindow *parent)
    : QObject(*new QWindowPrivate(), parent)
    , QSurface(QSurface::Window)
{
    Q_D(QWindow);
    d->parentWindow = parent;
    if (parent)
        d->screen = parent->screen();
    if (!d->screen)
        d->screen = QGuiApplication::primaryScreen();
    QGuiApplicationPrivate::window_list.prepend(this);
}

QWindow::QWindow(QWindowPrivate &dd, QWindow *parent)
    : QObject(dd, parent)
    , QSurface(QSurface::Window)
{
    Q_D(QWindow);
    d->parentWindow = parent;
    if (parent)
        d->screen = parent->screen();
    if (!d->screen)
        d->screen = QGuiApplication::primaryScreen();
    QGuiApplicationPrivate::window_list.prepend(this);
}

QWindow::~QWindow()
{
    if (QGuiApplicationPrivate::active_window == this)
        QGuiApplicationPrivate::active_window = 0;
    QGuiApplicationPrivate::window_list.removeAll(this);
    destroy();
}

QSurface::~QSurface()
{
}

void QWindow::setSurfaceType(SurfaceType surfaceType)
{
    Q_D(QWindow);
    d->surfaceType = surfaceType;
}

QWindow::SurfaceType QWindow::surfaceType() const
{
    Q_D(const QWindow);
    return d->surfaceType;
}

void QWindow::setVisible(bool visible)
{
    Q_D(QWindow);

    if (d->visible == visible)
        return;
    d->visible = visible;

    if (!d->platformWindow)
        create();

    if (visible) {
        QShowEvent showEvent;
        QGuiApplication::sendEvent(this, &showEvent);
    }

    d->platformWindow->setVisible(visible);

    if (!visible) {
        QHideEvent hideEvent;
        QGuiApplication::sendEvent(this, &hideEvent);
    }
}

bool QWindow::visible() const
{
    Q_D(const QWindow);

    return d->visible;
}

void QWindow::create()
{
    Q_D(QWindow);
    if (!d->platformWindow) {
        d->platformWindow = QGuiApplicationPrivate::platformIntegration()->createPlatformWindow(this);
        QObjectList childObjects = children();
        for (int i = 0; i < childObjects.size(); i ++) {
            QObject *object = childObjects.at(i);
            if(object->isWindowType()) {
                QWindow *window = static_cast<QWindow *>(object);
                if (window->d_func()->platformWindow)
                    window->d_func()->platformWindow->setParent(d->platformWindow);
            }
        }
    }
}

WId QWindow::winId() const
{
    Q_D(const QWindow);
    if(!d->platformWindow)
        const_cast<QWindow *>(this)->create();
    return d->platformWindow->winId();
}

QWindow *QWindow::parent() const
{
    Q_D(const QWindow);
    return d->parentWindow;
}

/**
  Sets the parent Window. This will lead to the windowing system managing the clip of the window, so it will be clipped to the parent window.
  Setting parent to be 0(NULL) means map it as a top level window. If the parent window has grabbed its window system resources, then the current window will also grab its window system resources.
  **/

void QWindow::setParent(QWindow *parent)
{
    Q_D(QWindow);

    QObject::setParent(parent);

    if (d->platformWindow) {
        if (parent && parent->d_func()->platformWindow) {
            d->platformWindow->setParent(parent->d_func()->platformWindow);
        } else {
            d->platformWindow->setParent(0);
        }
    }

    d->parentWindow = parent;
}

/*!
   Returns whether the window is top level, i.e. has no parent window.
 */
bool QWindow::isTopLevel() const
{
    Q_D(const QWindow);
    return d->parentWindow == 0;
}

bool QWindow::isModal() const
{
    Q_D(const QWindow);
    return d->modality != Qt::NonModal;
}

Qt::WindowModality QWindow::windowModality() const
{
    Q_D(const QWindow);
    return d->modality;
}

void QWindow::setWindowModality(Qt::WindowModality windowModality)
{
    Q_D(QWindow);
    d->modality = windowModality;
}

void QWindow::setFormat(const QSurfaceFormat &format)
{
    Q_D(QWindow);
    d->requestedFormat = format;
}

QSurfaceFormat QWindow::format() const
{
    Q_D(const QWindow);
    if (d->platformWindow)
        return d->platformWindow->format();
    return d->requestedFormat;
}

void QWindow::setWindowFlags(Qt::WindowFlags flags)
{
    Q_D(QWindow);
    if (d->platformWindow)
        d->windowFlags = d->platformWindow->setWindowFlags(flags);
    else
        d->windowFlags = flags;
}

Qt::WindowFlags QWindow::windowFlags() const
{
    Q_D(const QWindow);
    return d->windowFlags;
}

Qt::WindowType QWindow::windowType() const
{
    Q_D(const QWindow);
    return static_cast<Qt::WindowType>(int(d->windowFlags & Qt::WindowType_Mask));
}

void QWindow::setWindowTitle(const QString &title)
{
    Q_D(QWindow);
    d->windowTitle = title;
    if (d->platformWindow) {
        d->platformWindow->setWindowTitle(title);
    }
}

QString QWindow::windowTitle() const
{
    Q_D(const QWindow);
    return d->windowTitle;
}

void QWindow::raise()
{
    Q_D(QWindow);
    if (d->platformWindow) {
        d->platformWindow->raise();
    }
}

void QWindow::lower()
{
    Q_D(QWindow);
    if (d->platformWindow) {
        d->platformWindow->lower();
    }
}

void QWindow::setOpacity(qreal level)
{
    Q_D(QWindow);
    if (d->platformWindow) {
        d->platformWindow->setOpacity(level);
    }
}

void QWindow::requestActivateWindow()
{
    Q_D(QWindow);
    QGuiApplicationPrivate::active_window = this;
    if (d->platformWindow) {
        d->platformWindow->requestActivateWindow();
    }
}

Qt::WindowState QWindow::windowState() const
{
    Q_D(const QWindow);
    return d->windowState;
}

void QWindow::setWindowState(Qt::WindowState state)
{
    if (state == Qt::WindowActive) {
        requestActivateWindow();
        return;
    }

    Q_D(QWindow);
    if (d->platformWindow)
        d->windowState = d->platformWindow->setWindowState(state);
    else
        d->windowState = state;
}

/*!
  Sets the transient parent, which is a hint to the window manager that this window is a dialog or pop-up on behalf of the given window.
*/
void QWindow::setTransientParent(QWindow *parent)
{
    Q_D(QWindow);
    d->transientParent = parent;
}

QWindow *QWindow::transientParent() const
{
    Q_D(const QWindow);
    return d->transientParent.data();
}

QSize QWindow::minimumSize() const
{
    Q_D(const QWindow);
    return d->minimumSize;
}

QSize QWindow::maximumSize() const
{
    Q_D(const QWindow);
    return d->maximumSize;
}

QSize QWindow::baseSize() const
{
    Q_D(const QWindow);
    return d->baseSize;
}

QSize QWindow::sizeIncrement() const
{
    Q_D(const QWindow);
    return d->sizeIncrement;
}

void QWindow::setMinimumSize(const QSize &size)
{
    Q_D(QWindow);
    QSize adjustedSize = QSize(qBound(0, size.width(), QWINDOWSIZE_MAX), qBound(0, size.height(), QWINDOWSIZE_MAX));
    if (d->minimumSize == adjustedSize)
        return;
    d->minimumSize = adjustedSize;
    if (d->platformWindow && isTopLevel())
        d->platformWindow->propagateSizeHints();
}

void QWindow::setMaximumSize(const QSize &size)
{
    Q_D(QWindow);
    QSize adjustedSize = QSize(qBound(0, size.width(), QWINDOWSIZE_MAX), qBound(0, size.height(), QWINDOWSIZE_MAX));
    if (d->maximumSize == adjustedSize)
        return;
    d->maximumSize = adjustedSize;
    if (d->platformWindow && isTopLevel())
        d->platformWindow->propagateSizeHints();
}

void QWindow::setBaseSize(const QSize &size)
{
    Q_D(QWindow);
    if (d->baseSize == size)
        return;
    d->baseSize = size;
    if (d->platformWindow && isTopLevel())
        d->platformWindow->propagateSizeHints();
}

void QWindow::setSizeIncrement(const QSize &size)
{
    Q_D(QWindow);
    if (d->sizeIncrement == size)
        return;
    d->sizeIncrement = size;
    if (d->platformWindow && isTopLevel())
        d->platformWindow->propagateSizeHints();
}

void QWindow::setGeometry(const QRect &rect)
{
    Q_D(QWindow);
    if (d->platformWindow) {
        d->platformWindow->setGeometry(rect);
    } else {
        d->geometry = rect;
    }
}

QRect QWindow::geometry() const
{
    Q_D(const QWindow);
    return d->geometry;
}

QMargins QWindow::frameMargins() const
{
    Q_D(const QWindow);
    if (d->platformWindow)
        return d->platformWindow->frameMargins();
    return QMargins();
}

void QWindow::setWindowIcon(const QImage &icon) const
{
    Q_UNUSED(icon);
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
}

void QWindow::destroy()
{
    Q_D(QWindow);
    setVisible(false);
    delete d->platformWindow;
    d->platformWindow = 0;
}

QPlatformWindow *QWindow::handle() const
{
    Q_D(const QWindow);
    return d->platformWindow;
}

QPlatformSurface *QWindow::surfaceHandle() const
{
    Q_D(const QWindow);
    return d->platformWindow;
}

bool QWindow::setKeyboardGrabEnabled(bool grab)
{
    Q_D(QWindow);
    if (d->platformWindow)
        return d->platformWindow->setKeyboardGrabEnabled(grab);
    return false;
}

bool QWindow::setMouseGrabEnabled(bool grab)
{
    Q_D(QWindow);
    if (d->platformWindow)
        return d->platformWindow->setMouseGrabEnabled(grab);
    return false;
}

QScreen *QWindow::screen() const
{
    Q_D(const QWindow);
    return d->screen;
}

void QWindow::setScreen(QScreen *newScreen)
{
    Q_D(QWindow);
    bool wasCreated = d->platformWindow != 0;
    if (wasCreated)
        destroy();
    d->screen = newScreen ? newScreen : QGuiApplication::primaryScreen();
    if (wasCreated)
        create();
}

/*!
  Returns the accessibility interface for the object that the window represents
  */
QAccessibleInterface *QWindow::accessibleRoot() const
{
    return 0;
}


void QWindow::showMinimized()
{
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
}

void QWindow::showMaximized()
{
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
}

void QWindow::showFullScreen()
{
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
}

void QWindow::showNormal()
{
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
}

bool QWindow::close()
{
    //should we have close?
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
    return true;
}

void QWindow::exposeEvent(QExposeEvent *)
{
}

void QWindow::resizeEvent(QResizeEvent *)
{
}

void QWindow::showEvent(QShowEvent *)
{
}

void QWindow::hideEvent(QHideEvent *)
{
}

bool QWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseMove:
        mouseMoveEvent(static_cast<QMouseEvent*>(event));
        break;

    case QEvent::MouseButtonPress:
        mousePressEvent(static_cast<QMouseEvent*>(event));
        break;

    case QEvent::MouseButtonRelease:
        mouseReleaseEvent(static_cast<QMouseEvent*>(event));
        break;

    case QEvent::MouseButtonDblClick:
        mouseDoubleClickEvent(static_cast<QMouseEvent*>(event));
        break;

    case QEvent::Resize:
        resizeEvent(static_cast<QResizeEvent*>(event));
        break;

    case QEvent::KeyPress:
        keyPressEvent(static_cast<QKeyEvent *>(event));
        break;

    case QEvent::KeyRelease:
        keyReleaseEvent(static_cast<QKeyEvent *>(event));
        break;

#ifndef QT_NO_WHEELEVENT
    case QEvent::Wheel:
        wheelEvent(static_cast<QWheelEvent*>(event));
        break;
#endif

    case QEvent::Close: {
        Q_D(QWindow);
        bool wasVisible = visible();
        destroy();
        if (wasVisible)
            d->maybeQuitOnLastWindowClosed();
        break; }

    case QEvent::Expose:
        exposeEvent(static_cast<QExposeEvent *>(event));
        break;

    case QEvent::Show:
        showEvent(static_cast<QShowEvent *>(event));
        break;

    case QEvent::Hide:
        hideEvent(static_cast<QHideEvent *>(event));
        break;

    default:
        return QObject::event(event);
    }
    return true;
}

void QWindow::keyPressEvent(QKeyEvent *)
{
}

void QWindow::keyReleaseEvent(QKeyEvent *)
{
}

void QWindow::inputMethodEvent(QInputMethodEvent *)
{
}

void QWindow::mousePressEvent(QMouseEvent *)
{
}

void QWindow::mouseReleaseEvent(QMouseEvent *)
{
}

void QWindow::mouseDoubleClickEvent(QMouseEvent *)
{
}

void QWindow::mouseMoveEvent(QMouseEvent *)
{
}

#ifndef QT_NO_WHEELEVENT
void QWindow::wheelEvent(QWheelEvent *)
{
}
#endif //QT_NO_WHEELEVENT



/*!
    \fn QPoint QWindow::mapToGlobal(const QPoint &pos) const

    Translates the window coordinate \a pos to global screen
    coordinates. For example, \c{mapToGlobal(QPoint(0,0))} would give
    the global coordinates of the top-left pixel of the window.

    \sa mapFromGlobal()
*/

QPoint QWindow::mapToGlobal(const QPoint &pos) const
{
    return pos + d_func()->globalPosition();
}



/*!
    \fn QPoint QWindow::mapFromGlobal(const QPoint &pos) const

    Translates the global screen coordinate \a pos to window
    coordinates.

    \sa mapToGlobal()
*/

QPoint QWindow::mapFromGlobal(const QPoint &pos) const
{
    return pos - d_func()->globalPosition();
}



Q_GUI_EXPORT QWindowPrivate *qt_window_private(QWindow *window)
{
    return window->d_func();
}

void QWindowPrivate::maybeQuitOnLastWindowClosed()
{
    Q_Q(QWindow);

    // Attempt to close the application only if this has WA_QuitOnClose set and a non-visible parent
    bool quitOnClose = QGuiApplication::quitOnLastWindowClosed() && !q->parent();

    if (quitOnClose) {
        QWindowList list = QGuiApplication::topLevelWindows();
        bool lastWindowClosed = true;
        for (int i = 0; i < list.size(); ++i) {
            QWindow *w = list.at(i);
            if (!w->visible() || w->parent())
                continue;
            lastWindowClosed = false;
            break;
        }
        if (lastWindowClosed)
            QGuiApplicationPrivate::emitLastWindowClosed();
    }

}

QT_END_NAMESPACE
