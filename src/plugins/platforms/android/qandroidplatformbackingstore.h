// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDPLATFORMBACKINGSTORE_H
#define QANDROIDPLATFORMBACKINGSTORE_H

#include <qpa/qplatformbackingstore.h>
#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

class QAndroidPlatformBackingStore : public QPlatformBackingStore
{
public:
    explicit QAndroidPlatformBackingStore(QWindow *window);
    QPaintDevice *paintDevice() override;
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;
    void resize(const QSize &size, const QRegion &staticContents) override;
    QImage toImage() const override { return m_image; }
    void setBackingStore(QWindow *window);
protected:
    QImage m_image;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMBACKINGSTORE_H
