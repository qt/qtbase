// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRASTERBACKINGSTORE_P_H
#define QRASTERBACKINGSTORE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qpa/qplatformbackingstore.h>
#include <private/qglobal_p.h>


QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QRasterBackingStore : public QPlatformBackingStore
{
public:
    QRasterBackingStore(QWindow *window);
    ~QRasterBackingStore();

    void resize(const QSize &size, const QRegion &staticContents) override;
    bool scroll(const QRegion &area, int dx, int dy) override;
    void beginPaint(const QRegion &region) override;

    QPaintDevice *paintDevice() override;
    QImage toImage() const override;

protected:
    virtual QImage::Format format() const;

    QImage m_image;
    QSize m_requestedSize;
};

QT_END_NAMESPACE

#endif // QRASTERBACKINGSTORE_P_H
