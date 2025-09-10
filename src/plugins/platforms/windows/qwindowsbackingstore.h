// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSBACKINGSTORE_H
#define QWINDOWSBACKINGSTORE_H

#include <QtCore/qt_windows.h>

#include <qpa/qplatformbackingstore.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE

class QWindowsWindow;
class QWindowsNativeImage;

class QWindowsBackingStore : public QPlatformBackingStore
{
    Q_DISABLE_COPY_MOVE(QWindowsBackingStore)
public:
    QWindowsBackingStore(QWindow *window);
    ~QWindowsBackingStore() override;

    QPaintDevice *paintDevice() override;
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;
    void resize(const QSize &size, const QRegion &r) override;
    bool scroll(const QRegion &area, int dx, int dy) override;
    void beginPaint(const QRegion &) override;

    HDC getDC() const;

    QImage toImage() const override;

private:
    QScopedPointer<QWindowsNativeImage> m_image;
    bool m_alphaNeedsFill;
};

QT_END_NAMESPACE

#endif // QWINDOWSBACKINGSTORE_H
