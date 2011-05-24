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

#ifndef QWSCURSOR_QWS_H
#define QWSCURSOR_QWS_H

#include <QtGui/qimage.h>
#include <QtGui/qregion.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QWSCursor
{
public:
    QWSCursor() {}
    QWSCursor(const uchar *data, const uchar *mask, int width, int height,
              int hotX, int hotY)
        { set(data, mask, width, height, hotX, hotY); }

    void set(const uchar *data, const uchar *mask,
             int width, int height, int hotX, int hotY);

    QPoint hotSpot() const { return hot; }
    QImage &image() { return cursor; }

    static QWSCursor *systemCursor(int id);

private:
    static void createSystemCursor(int id);
    void createDropShadow(int dropx, int dropy);

private:
    QPoint hot;
    QImage cursor;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QWSCURSOR_QWS_H
