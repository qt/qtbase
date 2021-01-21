/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
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
    bool m_backingStoreSet = false;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMBACKINGSTORE_H
