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

#ifndef QSHAPEDPIXMAPDNDWINDOW_H
#define QSHAPEDPIXMAPDNDWINDOW_H

#include <QtGui/QWindow>
#include <QtGui/QPixmap>
#include <QtGui/QBackingStore>

QT_BEGIN_NAMESPACE

QT_BEGIN_HEADER

class QShapedPixmapWindow : public QWindow
{
public:
    QShapedPixmapWindow();

    void render();

    void setPixmap(const QPixmap &pixmap);
    void setHotspot(const QPoint &hotspot);

    void updateGeometry();

protected:
    void exposeEvent(QExposeEvent *);

private:
    QBackingStore *m_backingStore;
    QPixmap m_pixmap;
    QPoint m_hotSpot;
};

QT_END_HEADER

QT_END_NAMESPACE

#endif // QSHAPEDPIXMAPDNDWINDOW_H
