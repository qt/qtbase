/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QBACKINGSTORE_X11_H
#define QBACKINGSTORE_X11_H

#include <qpa/qplatformbackingstore.h>

typedef struct _XDisplay Display;

QT_BEGIN_NAMESPACE

class QXcbWindow;

class QXcbNativeBackingStore : public QPlatformBackingStore
{
public:
    QXcbNativeBackingStore(QWindow *window);
    ~QXcbNativeBackingStore();

    QPaintDevice *paintDevice() override;
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;

    QImage toImage() const override;

    void resize(const QSize &size, const QRegion &staticContents) override;
    bool scroll(const QRegion &area, int dx, int dy) override;

    void beginPaint(const QRegion &region) override;

private:
    Display *display() const;

    QPixmap m_pixmap;
    bool m_translucentBackground;
};

QT_END_NAMESPACE

#endif // QBACKINGSTORE_X11_H
