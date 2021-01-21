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

#ifndef QIOSURFACEGRAPHICSBUFFER_H
#define QIOSURFACEGRAPHICSBUFFER_H

#include <qpa/qplatformgraphicsbuffer.h>
#include <private/qcore_mac_p.h>

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
