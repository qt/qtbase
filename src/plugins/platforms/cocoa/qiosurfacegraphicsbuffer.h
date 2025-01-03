// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIOSURFACEGRAPHICSBUFFER_H
#define QIOSURFACEGRAPHICSBUFFER_H

#include <qpa/qplatformgraphicsbuffer.h>
#include <private/qcore_mac_p.h>

#include <CoreGraphics/CGColorSpace.h>
#include <IOSurface/IOSurface.h>

QT_BEGIN_NAMESPACE

class QIOSurfaceGraphicsBuffer : public QPlatformGraphicsBuffer
{
public:
    QIOSurfaceGraphicsBuffer(const QSize &size, const QPixelFormat &format);
    ~QIOSurfaceGraphicsBuffer();

    void setColorSpace(QCFType<CGColorSpaceRef> colorSpace);

    const uchar *data() const override;
    uchar *data() override;
    int bytesPerLine() const override;

    IOSurfaceRef surface();
    bool isInUse() const;

protected:
    bool doLock(AccessTypes access, const QRect &rect) override;
    void doUnlock() override;

private:
    QCFType<IOSurfaceRef> m_surface;

    friend QDebug operator<<(QDebug, const QIOSurfaceGraphicsBuffer *);
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug, const QIOSurfaceGraphicsBuffer *);
#endif

QT_END_NAMESPACE

#endif // QIOSURFACEGRAPHICSBUFFER_H
