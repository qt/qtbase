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

#include "qwindow_qpa.h"

#include "qplatformwindow_qpa.h"
#include "qwindowformat_qpa.h"

#include "qapplication_p.h"

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

class QWindowPrivate : public QObjectPrivate
{
public:
    QWindowPrivate(QWindow::WindowTypes types)
        : QObjectPrivate()
        , windowTypes(types)
        , platformWindow(0)
        , glContext(0)
        , widget(0)
    {

    }

    ~QWindowPrivate()
    {

    }

    QWindow::WindowTypes windowTypes;
    QPlatformWindow *platformWindow;
    QWindowFormat requestedFormat;
    QString windowTitle;
    QRect geometry;
    QGLContext *glContext;
    QWidget *widget;
};

QWindow::QWindow(WindowTypes types, QWindow *parent)
    : QObject(*new QWindowPrivate(types), parent)
{

}

QWidget *QWindow::widget() const
{
    Q_D(const QWindow);
    return d->widget;
}

void QWindow::setWidget(QWidget *widget)
{
    Q_D(QWindow);
    d->widget = widget;
}

void QWindow::setVisible(bool visible)
{
    Q_D(QWindow);
    if (!d->platformWindow) {
        create();
    }
    d->platformWindow->setVisible(visible);
}

void QWindow::create()
{
    Q_D(QWindow);
    d->platformWindow = QApplicationPrivate::platformIntegration()->createPlatformWindow(this,d->requestedFormat);
    Q_ASSERT(d->platformWindow);
}

WId QWindow::winId() const
{
    Q_D(const QWindow);
    if(d->platformWindow) {
        return d->platformWindow->winId();
    }
    return 0;
}

void QWindow::setParent(const QWindow *parent)
{
    Q_D(QWindow);
    //How should we support lazy init when setting parent
    if (!parent->d_func()->platformWindow) {
        const_cast<QWindow *>(parent)->create();
    }

    if(!d->platformWindow) {
        create();
    }
    d->platformWindow->setParent(parent->d_func()->platformWindow);
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
    Q_D(const QWindow);
    return d->requestedFormat;
}

QWindow::WindowTypes QWindow::types() const
{
    Q_D(const QWindow);
    return d->windowTypes;
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

QGLContext * QWindow::glContext() const
{
    Q_D(const QWindow);
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
    //JA, this will be solved later....
    //    if (QGLContext *context = extra->topextra->window->glContext()) {
    //                context->deleteQGLContext();
    Q_ASSERT(false);
    //    }
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
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
}

void QWindow::showEvent(QShowEvent *)
{
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
}

void QWindow::hideEvent(QHideEvent *)
{
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
}

bool QWindow::event(QEvent *)
{
    qDebug() << "unimplemented:" << __FILE__ << __LINE__;
    return false;
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
