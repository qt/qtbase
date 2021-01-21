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
