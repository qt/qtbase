// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

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
