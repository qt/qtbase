/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QOPENWFDBACKINGSTORE_H
#define QOPENWFDBACKINGSTORE_H

#include <qpa/qplatformbackingstore.h>
#include <QtGui/QImage>

class QOpenWFDBackingStore : public QPlatformBackingStore
{
public:
    QOpenWFDBackingStore(QWindow *window);

    QPaintDevice *paintDevice();

    // 'window' can be a child window, in which case 'region' is in child window coordinates and
    // offset is the (child) window's offset in relation to the window surface.
    void flush(QWindow *window, const QRegion &region, const QPoint &offset);

    void resize(const QSize &size, const QRegion &staticContents);

private:
    QImage mImage;
};

#endif // QOPENWFDBACKINGSTORE_H
