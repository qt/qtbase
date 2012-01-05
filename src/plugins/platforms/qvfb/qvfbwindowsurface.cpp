/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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


#include "qvfbwindowsurface.h"
#include "qvfbintegration.h"
#include <QtCore/qdebug.h>
#include <QtGui/qpainter.h>
#include <private/qapplication_p.h>

QT_BEGIN_NAMESPACE

QVFbWindowSurface::QVFbWindowSurface(//QVFbIntegration *graphicsSystem,
                                     QVFbScreen *screen, QWidget *window)
    : QWindowSurface(window),
      mScreen(screen)
{
}

QVFbWindowSurface::~QVFbWindowSurface()
{
}

QPaintDevice *QVFbWindowSurface::paintDevice()
{
    return mScreen->screenImage();
}

void QVFbWindowSurface::flush(QWidget *widget, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(widget);
    Q_UNUSED(offset);

//    QRect rect = geometry();
//    QPoint topLeft = rect.topLeft();

    mScreen->setDirty(region.boundingRect());
}

void QVFbWindowSurface::resize(const QSize&)
{

// any size you like as long as it's full-screen...

    QRect rect(mScreen->availableGeometry());
    QWindowSurface::resize(rect.size());
}


QVFbWindow::QVFbWindow(QVFbScreen *screen, QWidget *window)
    : QPlatformWindow(window),
      mScreen(screen)
{
}


void QVFbWindow::setGeometry(const QRect &)
{

// any size you like as long as it's full-screen...

    QRect rect(mScreen->availableGeometry());
    QWindowSystemInterface::handleGeometryChange(this->widget(), rect);

    QPlatformWindow::setGeometry(rect);
}



QT_END_NAMESPACE
