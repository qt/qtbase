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

#include "qwindow.h"

#include "qplatformwindow_qpa.h"
#include "qwindowformat_qpa.h"
#include "qplatformglcontext_qpa.h"
#include "qwindowcontext_qpa.h"

#include "qwindow_p.h"
#include "qapplication_p.h"

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

QWindow::QWindow(QWindow *parent)
    : QObject(*new QWindowPrivate())
{
    if (parent) {
        setParent(parent);
    }
}

QWindow::~QWindow()
{
    destroy();
}

void QWindow::setVisible(bool visible)
{
    Q_D(QWindow);
    if (!d->platformWindow) {
        create();
    }
    d->platformWindow->setVisible(visible);
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
        d->platformWindow = QApplicationPrivate::platformIntegration()->createPlatformWindow(this);
        d->windowFlags = d->platformWindow->setWindowFlags(d->windowFlags);
        if (!d->windowTitle.isNull())
            d->platformWindow->setWindowTitle(d->windowTitle);
    }
    Q_ASSERT(d->platformWindow);

    QObjectList childObjects = children();
    for (int i = 0; i < childObjects.size(); i ++) {
        QObject *object = childObjects.at(i);
        if(object->isWindowType()) {
            QWindow *window = static_cast<QWindow *>(object);
            window->setParent(this);
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

/**
  Sets the parent Window. This will lead to the windowing system managing the clip of the window, so it will be clipped to the parent window.
  Setting parent to be 0(NULL) means map it as a top level window. If the parent window has grabbed its window system resources, then the current window will also grab its window system resources.
  **/

void QWindow::setParent(QWindow *parent)
{
    Q_D(QWindow);

    if (d->parent == parent)
        return;

    QObject::setParent(parent);

    if (parent) {
        if (parent->d_func()->platformWindow) {
            if(!d->platformWindow) {
                create();
            }
            d->platformWindow->setParent(parent->d_func()->platformWindow);
            d->parent = parent;
        }
    } else  {
        d->parent = 0;
        if (d->parentWindow) {
            d->platformWindow->setParent(0);
        }
    }

}

void QWindow::setWindowFormat(const QWindowFormat &format)
{
    Q_D(QWindow);
    d->requestedFormat = format;
}

QWindowFormat QWindow::requestedWindowFormat() const
{
    Q_D(const QWindow);
    return d->requestedFormat;
}

QWindowFormat QWindow::actualWindowFormat() const
{
    return glContext()->handle()->windowFormat();
}

void QWindow::setSurfaceType(SurfaceType type)
{
    Q_D(QWindow);
    d->surfaceType = type;
}

QWindow::SurfaceType QWindow::surfaceType() const
{
    Q_D(const QWindow);
    return d->surfaceType;
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
    if (d->platformWindow) {
        d->platformWindow->requestActivateWindow();
    }
}

Qt::WindowStates QWindow::windowState() const
{
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
    return Qt::WindowNoState;
}

void QWindow::setWindowState(Qt::WindowStates state)
{
    Q_UNUSED(state);
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
}

QSize QWindow::minimumSize() const
{
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
    return QSize();
}

QSize QWindow::maximumSize() const
{
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
    return QSize();
}

void QWindow::setMinimumSize(const QSize &size) const
{
    Q_UNUSED(size);
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
}

void QWindow::setMaximumSize(const QSize &size) const
{
    Q_UNUSED(size);
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
}

void QWindow::setGeometry(const QRect &rect)
{
    Q_D(QWindow);
    d->geometry = rect;
    if (d->platformWindow) {
        d->platformWindow->setGeometry(rect);
    }
}

QRect QWindow::geometry() const
{
    Q_D(const QWindow);
    return d->geometry;
}

void QWindow::setWindowIcon(const QImage &icon) const
{
    Q_UNUSED(icon);
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
}

QWindowContext * QWindow::glContext() const
{
    Q_D(const QWindow);
    if (d->platformWindow && !d->glContext)
        const_cast<QWindowPrivate *>(d)->glContext = new QWindowContext(const_cast<QWindow *>(this));
    return d->glContext;
}

void QWindow::setRequestFormat(const QWindowFormat &format)
{
    Q_D(QWindow);
    d->requestedFormat = format;
}

QWindowFormat QWindow::format() const
{
    return QWindowFormat();
}

void QWindow::destroy()
{
    Q_D(QWindow);
    if (d->glContext) {
        d->glContext->deleteQGLContext();
    }
    delete d->glContext;
    d->glContext = 0;
    delete d->platformWindow;
    d->platformWindow = 0;
}

QPlatformWindow *QWindow::handle() const
{
    Q_D(const QWindow);
    return d->platformWindow;
}

QWindowSurface *QWindow::surface() const
{
    Q_D(const QWindow);
    return d->surface;
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

void QWindow::resizeEvent(QResizeEvent *)
{
}

void QWindow::showEvent(QShowEvent *)
{
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
}

void QWindow::hideEvent(QHideEvent *)
{
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
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

    case QEvent::Close:
        destroy();
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

QT_END_NAMESPACE
