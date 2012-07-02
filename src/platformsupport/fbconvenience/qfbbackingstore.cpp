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

#include "qfbbackingstore_p.h"
#include "qfbwindow_p.h"
#include "qfbscreen_p.h"

#include <qpa/qplatformwindow.h>

QT_BEGIN_NAMESPACE

QFbBackingStore::QFbBackingStore(QFbScreen *screen, QWindow *window)
    : QPlatformBackingStore(window),
      mScreen(screen)
{
    mImage = QImage(window->size(), mScreen->format());

    platformWindow = static_cast<QFbWindow*>(window->handle());
    platformWindow->surface = this;
}

QFbBackingStore::~QFbBackingStore()
{
}

void QFbBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(window);
    Q_UNUSED(offset);

    platformWindow->repaint(region);
}

void QFbBackingStore::resize(const QSize &size, const QRegion &region)
{
    Q_UNUSED(region);
    // change the widget's QImage if this is a resize
    if (mImage.size() != size)
        mImage = QImage(size, mScreen->format());
    // QPlatformBackingStore::resize(size);
}

bool QFbBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    return QPlatformBackingStore::scroll(area, dx, dy);
}

void QFbBackingStore::beginPaint(const QRegion &region)
{
    Q_UNUSED(region);
}

void QFbBackingStore::endPaint(const QRegion &region)
{
    Q_UNUSED(region);
}

QT_END_NAMESPACE

