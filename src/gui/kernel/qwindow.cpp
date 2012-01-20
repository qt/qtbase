/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#include <QStyleHints>

QT_BEGIN_NAMESPACE

/*!
    \class QWindow
    \brief The QWindow class encapsulates an independent window in a Windowing System.

    A window that is supplied a parent become a native child window of
    their parent window.

    Windows can potentially use a lot of memory. A usual measurement is
    width * height * depth. A window might also include multiple buffers
    to support double and triple buffering. To release a windows memory
    resources, the destroy() function.

 */

QWindow::QWindow(QScreen *targetScreen)
    : QObject(*new QWindowPrivate(), 0)
    , QSurface(QSurface::Window)
{
    Q_D(QWindow);
    d->screen = targetScreen;
    if (!d->screen)
        d->screen = QGuiApplication::primaryScreen();

    //if your applications aborts here, then chances are your creating a QWindow before the
    //screen list is populated.
    Q_ASSERT(d->screen);

    connect(d->screen, SIGNAL(destroyed(QObject *)), this, SLOT(screenDestroyed(QObject *)));
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
    if (QGuiApplicationPrivate::focus_window == this)
        QGuiApplicationPrivate::focus_window = 0;
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
    emit visibleChanged(visible);

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


/*!
    Returns the requested surfaceformat of this window.

    If the requested format was not supported by the platform implementation,
    the requestedFormat will differ from the actual window format.

    \sa format.
 */
QSurfaceFormat QWindow::requestedFormat() const
{
    Q_D(const QWindow);
    return d->requestedFormat;
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
    if (d->platformWindow)
        d->platformWindow->requestActivateWindow();
}

/*!
    Returns true if the window should appear active from a style perspective.

    This is the case for the window that has input focus as well as windows
    that are in the same parent / transient parent chain as the focus window.

    To get the window that currently has focus, use QGuiApplication::focusWindow().
*/
bool QWindow::isActive() const
{
    Q_D(const QWindow);
    if (!d->platformWindow)
        return false;

    QWindow *focus = QGuiApplication::focusWindow();
    if (focus == this)
        return true;

    if (!parent() && !transientParent()) {
        return isAncestorOf(focus);
    } else {
        return (parent() && parent()->isActive()) || (transientParent() && transientParent()->isActive());
    }
}

/*!
  Returns the window's currently set orientation.

  The default value is Qt::UnknownOrientation.

  \sa setOrientation(), QScreen::currentOrientation()
*/
Qt::ScreenOrientation QWindow::orientation() const
{
    Q_D(const QWindow);
    return d->orientation;
}

/*!
  Set the orientation of the window's contents.

  This is a hint to the window manager in case it needs to display
  additional content like popups, dialogs, status bars, or similar
  in relation to the window.

  The recommended orientation is QScreen::currentOrientation() but
  an application doesn't have to support all possible orientations,
  and thus can opt to ignore the current screen orientation.

  \sa QScreen::currentOrientation()
*/
void QWindow::setOrientation(Qt::ScreenOrientation orientation)
{
    Q_D(QWindow);
    if (orientation == d->orientation)
        return;

    d->orientation = orientation;
    if (d->platformWindow) {
        d->platformWindow->setOrientation(orientation);
    }
    emit orientationChanged(orientation);
}

Qt::WindowState QWindow::windowState() const
{
    Q_D(const QWindow);
    return d->windowState;
}

void QWindow::setWindowState(Qt::WindowState state)
{
    if (state == Qt::WindowActive) {
        qWarning() << "QWindow::setWindowState does not accept Qt::WindowActive";
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

/*!
    \enum QWindow::AncestorMode

    This enum is used to control whether or not transient parents
    should be considered ancestors.

    \value ExcludeTransients Transient parents are not considered ancestors.
    \value IncludeTransients Transient parents are considered ancestors.
*/

/*!
  Returns true if the window is an ancestor of the given child. If mode is
  IncludeTransients transient parents are also considered ancestors.
*/
bool QWindow::isAncestorOf(const QWindow *child, AncestorMode mode) const
{
    if (child->parent() == this || (mode == IncludeTransients && child->transientParent() == this))
        return true;

    return (child->parent() && isAncestorOf(child->parent(), mode))
        || (mode == IncludeTransients && child->transientParent() && isAncestorOf(child->transientParent(), mode));
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

/*!
    Sets the geometry of the window excluding its window frame.
*/
void QWindow::setGeometry(const QRect &rect)
{
    Q_D(QWindow);
    if (rect == geometry())
        return;
    QRect oldRect = geometry();

    d->positionPolicy = QWindowPrivate::WindowFrameExclusive;
    if (d->platformWindow) {
        d->platformWindow->setGeometry(rect);
    } else {
        d->geometry = rect;
    }

    if (rect.x() != oldRect.x())
        emit xChanged(rect.x());
    if (rect.y() != oldRect.y())
        emit yChanged(rect.y());
    if (rect.width() != oldRect.width())
        emit widthChanged(rect.width());
    if (rect.height() != oldRect.height())
        emit heightChanged(rect.height());
}

/*!
    Returns the geometry of the window excluding its window frame.
*/
QRect QWindow::geometry() const
{
    Q_D(const QWindow);
    if (d->platformWindow)
        return d->platformWindow->geometry();
    return d->geometry;
}

/*!
    Returns the window frame margins surrounding the window.
*/
QMargins QWindow::frameMargins() const
{
    Q_D(const QWindow);
    if (d->platformWindow)
        return d->platformWindow->frameMargins();
    return QMargins();
}

/*!
    Returns the geometry of the window including its window frame.
*/
QRect QWindow::frameGeometry() const
{
    Q_D(const QWindow);
    if (d->platformWindow) {
        QMargins m = frameMargins();
        return d->platformWindow->geometry().adjusted(-m.left(), -m.top(), m.right(), m.bottom());
    }
    return d->geometry;
}

/*!
    Returns the top left position of the window including its window frame.
*/
QPoint QWindow::framePos() const
{
    Q_D(const QWindow);
    if (d->platformWindow) {
        QMargins margins = frameMargins();
        return d->platformWindow->geometry().topLeft() - QPoint(margins.left(), margins.top());
    }
    return d->geometry.topLeft();
}

/*!
    Sets the upper left position of the window including its window frame.
*/
void QWindow::setFramePos(const QPoint &point)
{
    Q_D(QWindow);
    d->positionPolicy = QWindowPrivate::WindowFrameInclusive;
    if (d->platformWindow) {
        d->platformWindow->setGeometry(QRect(point, size()));
    } else {
        d->geometry.setTopLeft(point);
    }
}

void QWindow::resize(const QSize &newSize)
{
    Q_D(QWindow);
    if (d->platformWindow) {
        d->platformWindow->setGeometry(QRect(pos(), newSize));
    } else {
        d->geometry.setSize(newSize);
    }
}

void QWindow::setWindowIcon(const QImage &icon) const
{
    Q_UNUSED(icon);
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
}



/*!
    Releases the native platform resources associated with this window.
 */

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
    if (!newScreen)
        newScreen = QGuiApplication::primaryScreen();
    if (newScreen != screen()) {
        const bool wasCreated = d->platformWindow != 0;
        if (wasCreated)
            destroy();
        if (d->screen)
            disconnect(d->screen, SIGNAL(destroyed(QObject *)), this, SLOT(screenDestroyed(QObject *)));
        d->screen = newScreen;
        if (newScreen) {
            connect(d->screen, SIGNAL(destroyed(QObject *)), this, SLOT(screenDestroyed(QObject *)));
            if (wasCreated)
                create();
        }
        emit screenChanged(newScreen);
    }
}

void QWindow::screenDestroyed(QObject *object)
{
    Q_D(QWindow);
    if (object == static_cast<QObject *>(d->screen))
        setScreen(0);
}

/*!
    \fn QWindow::screenChanged(QScreen *screen)

    This signal is emitted when a window's screen changes, either
    by being set explicitly with setScreen(), or automatically when
    the window's screen is removed.
*/

/*!
  Returns the accessibility interface for the object that the window represents
  \preliminary
  \sa QAccessible
  */
QAccessibleInterface *QWindow::accessibleRoot() const
{
    return 0;
}

/*!
  Returns the QObject that will be the final receiver of events tied focus, such
  as key events.
*/
QObject *QWindow::focusObject() const
{
    return const_cast<QWindow *>(this);
}

void QWindow::show()
{
    if (qApp->styleHints()->showIsFullScreen())
        showFullScreen();
    else
        showNormal();
}

void QWindow::hide()
{
    setVisible(false);
}

void QWindow::showMinimized()
{
    setWindowState(Qt::WindowMinimized);
    setVisible(true);
}

void QWindow::showMaximized()
{
    setWindowState(Qt::WindowMaximized);
    setVisible(true);
}

void QWindow::showFullScreen()
{
    setWindowState(Qt::WindowFullScreen);
    setVisible(true);
    requestActivateWindow();
}

void QWindow::showNormal()
{
    setWindowState(Qt::WindowNoState);
    setVisible(true);
}

bool QWindow::close()
{
    //should we have close?
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
    return true;
}

void QWindow::exposeEvent(QExposeEvent *ev)
{
    ev->ignore();
}

void QWindow::moveEvent(QMoveEvent *ev)
{
    ev->ignore();
}

void QWindow::resizeEvent(QResizeEvent *ev)
{
    ev->ignore();
}

void QWindow::showEvent(QShowEvent *ev)
{
    ev->ignore();
}

void QWindow::hideEvent(QHideEvent *ev)
{
    ev->ignore();
}

bool QWindow::event(QEvent *ev)
{
    switch (ev->type()) {
    case QEvent::MouseMove:
        mouseMoveEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::MouseButtonPress:
        mousePressEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::MouseButtonRelease:
        mouseReleaseEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::MouseButtonDblClick:
        mouseDoubleClickEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        touchEvent(static_cast<QTouchEvent *>(ev));
        break;

    case QEvent::Move:
        moveEvent(static_cast<QMoveEvent*>(ev));
        break;

    case QEvent::Resize:
        resizeEvent(static_cast<QResizeEvent*>(ev));
        break;

    case QEvent::KeyPress:
        keyPressEvent(static_cast<QKeyEvent *>(ev));
        break;

    case QEvent::KeyRelease:
        keyReleaseEvent(static_cast<QKeyEvent *>(ev));
        break;

    case QEvent::FocusIn:
        focusInEvent(static_cast<QFocusEvent *>(ev));
        break;

    case QEvent::FocusOut:
        focusOutEvent(static_cast<QFocusEvent *>(ev));
        break;

#ifndef QT_NO_WHEELEVENT
    case QEvent::Wheel:
        wheelEvent(static_cast<QWheelEvent*>(ev));
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
        exposeEvent(static_cast<QExposeEvent *>(ev));
        break;

    case QEvent::Show:
        showEvent(static_cast<QShowEvent *>(ev));
        break;

    case QEvent::Hide:
        hideEvent(static_cast<QHideEvent *>(ev));
        break;

    default:
        return QObject::event(ev);
    }
    return true;
}

void QWindow::keyPressEvent(QKeyEvent *ev)
{
    ev->ignore();
}

void QWindow::keyReleaseEvent(QKeyEvent *ev)
{
    ev->ignore();
}

void QWindow::focusInEvent(QFocusEvent *ev)
{
    ev->ignore();
}

void QWindow::focusOutEvent(QFocusEvent *ev)
{
    ev->ignore();
}

void QWindow::mousePressEvent(QMouseEvent *ev)
{
    ev->ignore();
}

void QWindow::mouseReleaseEvent(QMouseEvent *ev)
{
    ev->ignore();
}

void QWindow::mouseDoubleClickEvent(QMouseEvent *ev)
{
    ev->ignore();
}

void QWindow::mouseMoveEvent(QMouseEvent *ev)
{
    ev->ignore();
}

#ifndef QT_NO_WHEELEVENT
void QWindow::wheelEvent(QWheelEvent *ev)
{
    ev->ignore();
}
#endif //QT_NO_WHEELEVENT

void QWindow::touchEvent(QTouchEvent *ev)
{
    ev->ignore();
}

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
