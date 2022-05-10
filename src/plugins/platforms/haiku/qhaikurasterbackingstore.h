// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
