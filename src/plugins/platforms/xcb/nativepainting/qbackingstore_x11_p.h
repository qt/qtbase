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

#ifndef QBACKINGSTORE_X11_H
#define QBACKINGSTORE_X11_H

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

#endif // QBACKINGSTORE_X11_H
