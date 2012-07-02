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

#include "qfbcursor_p.h"
#include <QtGui/QPainter>

QT_BEGIN_NAMESPACE

QFbCursor::QFbCursor(QPlatformScreen *scr)
        : screen(scr), currentRect(QRect()), prevRect(QRect())
{
    graphic = new QPlatformCursorImage(0, 0, 0, 0, 0, 0);
    setCursor(Qt::ArrowCursor);
}

QRect QFbCursor::getCurrentRect()
{
    QRect rect = graphic->image()->rect().translated(-graphic->hotspot().x(),
                                                     -graphic->hotspot().y());
    rect.translate(QCursor::pos());
    QPoint screenOffset = screen->geometry().topLeft();
    rect.translate(-screenOffset);  // global to local translation
    return rect;
}


void QFbCursor::pointerEvent(const QMouseEvent & e)
{
    Q_UNUSED(e);
    QPoint screenOffset = screen->geometry().topLeft();
    currentRect = getCurrentRect();
    // global to local translation
    if (onScreen || screen->geometry().intersects(currentRect.translated(screenOffset))) {
        setDirty();
    }
}

QRect QFbCursor::drawCursor(QPainter & painter)
{
    dirty = false;
    if (currentRect.isNull())
        return QRect();

    // We need this because the cursor might be dirty due to moving off screen
    QPoint screenOffset = screen->geometry().topLeft();
    // global to local translation
    if (!currentRect.translated(screenOffset).intersects(screen->geometry()))
        return QRect();

    prevRect = currentRect;
    painter.drawImage(prevRect, *graphic->image());
    onScreen = true;
    return prevRect;
}

QRect QFbCursor::dirtyRect()
{
    if (onScreen) {
        onScreen = false;
        return prevRect;
    }
    return QRect();
}

void QFbCursor::setCursor(Qt::CursorShape shape)
{
    graphic->set(shape);
}

void QFbCursor::setCursor(const QImage &image, int hotx, int hoty)
{
    graphic->set(image, hotx, hoty);
}

void QFbCursor::setCursor(const uchar *data, const uchar *mask, int width, int height, int hotX, int hotY)
{
    graphic->set(data, mask, width, height, hotX, hotY);
}

void QFbCursor::changeCursor(QCursor * widgetCursor, QWindow *window)
{
    Q_UNUSED(window);
    Qt::CursorShape shape = widgetCursor->shape();

    if (shape == Qt::BitmapCursor) {
        // application supplied cursor
        QPoint spot = widgetCursor->hotSpot();
        setCursor(widgetCursor->pixmap().toImage(), spot.x(), spot.y());
    } else {
        // system cursor
        setCursor(shape);
    }
    currentRect = getCurrentRect();
    QPoint screenOffset = screen->geometry().topLeft(); // global to local translation
    if (onScreen || screen->geometry().intersects(currentRect.translated(screenOffset)))
        setDirty();
}

QT_END_NAMESPACE

