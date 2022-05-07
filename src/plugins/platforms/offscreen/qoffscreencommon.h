/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QOFFSCREENCOMMON_H
#define QOFFSCREENCOMMON_H

#include <qpa/qplatformbackingstore.h>
#if QT_CONFIG(draganddrop)
#include <qpa/qplatformdrag.h>
#endif
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qplatformwindow.h>

#include <qscopedpointer.h>
#include <qimage.h>
#include <qhash.h>

QT_BEGIN_NAMESPACE

class QOffscreenIntegration;
class QOffscreenScreen : public QPlatformScreen
{
public:
    QOffscreenScreen(const QOffscreenIntegration *integration);

    QRect geometry() const override { return m_geometry; }
    int depth() const override { return 32; }
    QImage::Format format() const override { return QImage::Format_RGB32; }
    QDpi logicalDpi() const override { return QDpi(m_logicalDpi, m_logicalDpi); }
    QDpi logicalBaseDpi() const override { return QDpi(m_logicalBaseDpi, m_logicalBaseDpi); }
    qreal devicePixelRatio() const override { return m_dpr; }
    QString name() const override { return m_name; }
    QPlatformCursor *cursor() const override { return m_cursor.data(); }
    QList<QPlatformScreen *> virtualSiblings() const override;

    QPixmap grabWindow(WId window, int x, int y, int width, int height) const override;

    static QPlatformWindow *windowContainingCursor;

public:
    QString m_name;
    QRect m_geometry;
    int m_logicalDpi = 96;
    int m_logicalBaseDpi= 96;
    qreal m_dpr = 1;
    QScopedPointer<QPlatformCursor> m_cursor;
    const QOffscreenIntegration *m_integration;
};

#if QT_CONFIG(draganddrop)
class QOffscreenDrag : public QPlatformDrag
{
public:
    Qt::DropAction drag(QDrag *) override { return Qt::IgnoreAction; }
};
#endif

class QOffscreenBackingStore : public QPlatformBackingStore
{
public:
    QOffscreenBackingStore(QWindow *window);
    ~QOffscreenBackingStore();

    QPaintDevice *paintDevice() override;
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;
    void resize(const QSize &size, const QRegion &staticContents) override;
    bool scroll(const QRegion &area, int dx, int dy) override;

    QPixmap grabWindow(WId window, const QRect &rect) const;

    static QOffscreenBackingStore *backingStoreForWinId(WId id);

private:
    void clearHash();

    QImage m_image;
    QHash<WId, QRect> m_windowAreaHash;

    static QHash<WId, QOffscreenBackingStore *> m_backingStoreForWinIdHash;
};

class QOffscreenPlatformNativeInterface : public QPlatformNativeInterface
{
public:
    ~QOffscreenPlatformNativeInterface();
};

QT_END_NAMESPACE

#endif
