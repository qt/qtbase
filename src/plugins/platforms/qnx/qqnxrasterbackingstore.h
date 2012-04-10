/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
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

#ifndef QQNXRASTERWINDOWSURFACE_H
#define QQNXRASTERWINDOWSURFACE_H

#include <QtGui/qplatformbackingstore_qpa.h>

#include <screen/screen.h>

QT_BEGIN_NAMESPACE

class QQnxWindow;

class QQnxRasterBackingStore : public QPlatformBackingStore
{
public:
    QQnxRasterBackingStore(QWindow *window);
    ~QQnxRasterBackingStore();

    QPaintDevice *paintDevice();
    void flush(QWindow *window, const QRegion &region, const QPoint &offset);
    void resize(const QSize &size, const QRegion &staticContents);
    bool scroll(const QRegion &area, int dx, int dy);
    void beginPaint(const QRegion &region);
    void endPaint(const QRegion &region);

private:
    class ScrollOp {
    public:
        ScrollOp(const QRegion &a, int x, int y) : totalArea(a), dx(x), dy(y) {}
        QRegion totalArea;
        int dx;
        int dy;
    };

    QQnxWindow *m_platformWindow;
    QList<ScrollOp> m_scrollOpList;
};

QT_END_NAMESPACE

#endif // QQNXRASTERWINDOWSURFACE_H
