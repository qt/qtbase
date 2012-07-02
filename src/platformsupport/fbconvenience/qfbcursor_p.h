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

#ifndef QFBCURSOR_P_H
#define QFBCURSOR_P_H

#include <qpa/qplatformcursor.h>

QT_BEGIN_NAMESPACE

class QFbCursor : public QPlatformCursor
{
public:
    QFbCursor(QPlatformScreen * scr);

    // output methods
    QRect dirtyRect();
    virtual QRect drawCursor(QPainter & painter);

    // input methods
    virtual void pointerEvent(const QMouseEvent & event);
    virtual void changeCursor(QCursor * widgetCursor, QWindow *window);

    virtual void setDirty() { dirty = true; /* screen->setDirty(QRect()); */ }
    virtual bool isDirty() { return dirty; }
    virtual bool isOnScreen() { return onScreen; }
    virtual QRect lastPainted() { return prevRect; }

protected:
    QPlatformCursorImage *graphic;

private:
    void setCursor(const uchar *data, const uchar *mask, int width, int height, int hotX, int hotY);
    void setCursor(Qt::CursorShape shape);
    void setCursor(const QImage &image, int hotx, int hoty);

    QPlatformScreen *screen;
    QRect currentRect;      // next place to draw the cursor
    QRect prevRect;         // last place the cursor was drawn
    QRect getCurrentRect();
    bool dirty;
    bool onScreen;
};

QT_END_NAMESPACE

#endif // QFBCURSOR_P_H

