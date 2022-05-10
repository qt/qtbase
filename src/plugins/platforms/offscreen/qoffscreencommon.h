// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#include <qjsonobject.h>
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
    QImage toImage() const override { return m_image; }

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
    QOffscreenPlatformNativeInterface(QOffscreenIntegration *integration);
    ~QOffscreenPlatformNativeInterface();

    static void setConfiguration(const QJsonObject &configuration, QOffscreenPlatformNativeInterface *iface);
    static QJsonObject configuration(QOffscreenPlatformNativeInterface *iface);

    void *nativeResourceForIntegration(const QByteArray &resource) override;
private:
    QOffscreenIntegration *m_integration;
};

QT_END_NAMESPACE

#endif
