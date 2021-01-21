/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
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

#ifndef QQNXRASTERWINDOWSURFACE_H
#define QQNXRASTERWINDOWSURFACE_H

#include <qpa/qplatformbackingstore.h>

#include <screen/screen.h>

QT_BEGIN_NAMESPACE

class QQnxRasterWindow;

class QQnxRasterBackingStore : public QPlatformBackingStore
{
public:
    QQnxRasterBackingStore(QWindow *window);
    ~QQnxRasterBackingStore();

    QPaintDevice *paintDevice() override;
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;
    void resize(const QSize &size, const QRegion &staticContents) override;
    bool scroll(const QRegion &area, int dx, int dy) override;
    void beginPaint(const QRegion &region) override;
    void endPaint() override;

private:
    QQnxRasterWindow *platformWindow() const;

    QWindow *m_window;
    bool m_needsPosting;
    bool m_scrolled;
};

QT_END_NAMESPACE

#endif // QQNXRASTERWINDOWSURFACE_H
