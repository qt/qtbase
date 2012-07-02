/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qfbwindow_p.h"
#include "qfbscreen_p.h"

QT_BEGIN_NAMESPACE

void QFbWindow::setGeometry(const QRect &rect)
{
// store previous geometry for screen update
    oldGeometry = geometry();


    QList<QFbScreen *>::const_iterator i = mScreens.constBegin();
    QList<QFbScreen *>::const_iterator end = mScreens.constEnd();
    while (i != end) {
        (*i)->invalidateRectCache();
        ++i;
    }
//###    QWindowSystemInterface::handleGeometryChange(window(), rect);

    QPlatformWindow::setGeometry(rect);
}

void QFbWindow::setVisible(bool visible)
{
    visibleFlag = visible;
    QList<QFbScreen *>::const_iterator i = mScreens.constBegin();
    QList<QFbScreen *>::const_iterator end = mScreens.constEnd();
    while (i != end) {
        (*i)->invalidateRectCache();
        (*i)->setDirty(geometry());
        ++i;
    }
}

Qt::WindowFlags QFbWindow::setWindowFlags(Qt::WindowFlags type)
{
    flags = type;
    QList<QFbScreen *>::const_iterator i = mScreens.constBegin();
    QList<QFbScreen *>::const_iterator end = mScreens.constEnd();
    while (i != end) {
        (*i)->invalidateRectCache();
        ++i;
    }
    return flags;
}

Qt::WindowFlags QFbWindow::windowFlags() const
{
    return flags;
}

QFbWindow::QFbWindow(QWindow *window)
    : QPlatformWindow(window), visibleFlag(false)
{
    static QAtomicInt winIdGenerator(1);
    windowId = winIdGenerator.fetchAndAddRelaxed(1);
}

QFbWindow::~QFbWindow()
{
    QList<QFbScreen *>::const_iterator i = mScreens.constBegin();
    QList<QFbScreen *>::const_iterator end = mScreens.constEnd();
    while (i != end) {
        (*i)->removeWindow(this);
        ++i;
    }
}

void QFbWindow::raise()
{
    QList<QFbScreen *>::const_iterator i = mScreens.constBegin();
    QList<QFbScreen *>::const_iterator end = mScreens.constEnd();
    while (i != end) {
        (*i)->raise(this);
        ++i;
    }
}

void QFbWindow::lower()
{
    QList<QFbScreen *>::const_iterator i = mScreens.constBegin();
    QList<QFbScreen *>::const_iterator end = mScreens.constEnd();
    while (i != end) {
        (*i)->lower(this);
        ++i;
    }
}

void QFbWindow::repaint(const QRegion &region)
{
    QRect currentGeometry = geometry();

    QRect dirtyClient = region.boundingRect();
    QRect dirtyRegion(currentGeometry.left() + dirtyClient.left(),
                      currentGeometry.top() + dirtyClient.top(),
                      dirtyClient.width(),
                      dirtyClient.height());
    QList<QFbScreen *>::const_iterator i = mScreens.constBegin();
    QList<QFbScreen *>::const_iterator end = mScreens.constEnd();
    QRect oldGeometryLocal = oldGeometry;
    oldGeometry = currentGeometry;
    while (i != end) {
        // If this is a move, redraw the previous location
        if (oldGeometryLocal != currentGeometry) {
            (*i)->setDirty(oldGeometryLocal);
        }
        (*i)->setDirty(dirtyRegion);
        ++i;
    }
}


QT_END_NAMESPACE

