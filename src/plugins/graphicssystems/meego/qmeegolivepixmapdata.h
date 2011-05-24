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

#ifndef MLIVEPIXMAPDATA_H
#define MLIVEPIXMAPDATA_H

#include <QLinkedList>
#include <private/qpixmapdata_gl_p.h>
#include "qmeegoextensions.h"

class QMeeGoLivePixmapData;
typedef QLinkedList<QMeeGoLivePixmapData *> QMeeGoLivePixmapDataList;

class QMeeGoLivePixmapData : public QGLPixmapData
{
public:
    QMeeGoLivePixmapData(int w, int h, QImage::Format format);
    QMeeGoLivePixmapData(Qt::HANDLE h);
    ~QMeeGoLivePixmapData();

    QPixmapData *createCompatiblePixmapData() const;
    bool scroll(int dx, int dy, const QRect &rect);

    void initializeThroughEGLImage();

    QImage* lock(EGLSyncKHR fenceSync);
    bool release(QImage *img);
    Qt::HANDLE handle();

    EGLSurface getSurfaceForBackingPixmap();
    void destroySurfaceForPixmapData(QPixmapData* pmd);

    QPixmap *backingX11Pixmap;
    QImage lockedImage;
    QMeeGoLivePixmapDataList::Iterator pos;

    static void invalidateSurfaces();
};

#endif
