/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
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

#ifndef QHAIKURASTERWINDOWSURFACE_H
#define QHAIKURASTERWINDOWSURFACE_H

#include <qpa/qplatformbackingstore.h>

#include "qhaikubuffer.h"

QT_BEGIN_NAMESPACE

class BBitmap;
class QHaikuRasterWindow;

class QHaikuRasterBackingStore : public QPlatformBackingStore
{
public:
    explicit QHaikuRasterBackingStore(QWindow *window);
    ~QHaikuRasterBackingStore();

    QPaintDevice *paintDevice() override;
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;
    void resize(const QSize &size, const QRegion &staticContents) override;

private:
    BBitmap *m_bitmap;
    QHaikuBuffer m_buffer;
    QSize m_bufferSize;
};

QT_END_NAMESPACE

#endif
