/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfswindow.h"

#include <QtGui/QWindowSystemInterface>

QT_BEGIN_NAMESPACE

QEglFSWindow::QEglFSWindow(QWidget *w, QEglFSScreen *screen)
    : QPlatformWindow(w), m_screen(screen)
{
    static int serialNo = 0;
    m_winid  = ++serialNo;
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglWindow %p: %p %p 0x%x\n", this, w, screen, uint(m_winid));
#endif
}


void QEglFSWindow::setGeometry(const QRect &)
{
    // We only support full-screen windows
    QRect rect(m_screen->availableGeometry());
    QWindowSystemInterface::handleGeometryChange(this->widget(), rect);

    // Since toplevels are fullscreen, propegate the screen size back to the widget
    widget()->setGeometry(rect);

    QPlatformWindow::setGeometry(rect);
}

WId QEglFSWindow::winId() const
{
    return m_winid;
}



QPlatformGLContext *QEglFSWindow::glContext() const
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglWindow::glContext %p\n", m_screen->platformContext());
#endif
    Q_ASSERT(m_screen);
     return m_screen->platformContext();
}

QT_END_NAMESPACE
