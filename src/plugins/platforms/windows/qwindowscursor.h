// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSCURSOR_H
#define QWINDOWSCURSOR_H

#include <QtCore/qt_windows.h>

#include <qpa/qplatformcursor.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE

struct QWindowsPixmapCursorCacheKey
{
    explicit QWindowsPixmapCursorCacheKey(const QCursor &c);

    qint64 bitmapCacheKey;
    qint64 maskCacheKey;
};

inline bool operator==(const QWindowsPixmapCursorCacheKey &k1, const QWindowsPixmapCursorCacheKey &k2)
{
    return k1.bitmapCacheKey == k2.bitmapCacheKey && k1.maskCacheKey == k2.maskCacheKey;
}

inline size_t qHash(const QWindowsPixmapCursorCacheKey &k, size_t seed) noexcept
{
    return (size_t(k.bitmapCacheKey) + size_t(k.maskCacheKey)) ^ seed;
}

class CursorHandle
{
    Q_DISABLE_COPY_MOVE(CursorHandle)
public:
    explicit CursorHandle(HCURSOR hcursor = nullptr) : m_hcursor(hcursor) {}
    ~CursorHandle()
    {
        if (m_hcursor)
            DestroyCursor(m_hcursor);
    }

    bool isNull() const { return !m_hcursor; }
    HCURSOR handle() const { return m_hcursor; }

private:
    const HCURSOR m_hcursor;
};

using CursorHandlePtr = QSharedPointer<CursorHandle>;

class QWindowsCursor : public QPlatformCursor
{
public:
    enum class State {
        Showing,
        Hidden,
        Suppressed // Cursor suppressed by touch interaction (Windows 8).
    };

    struct PixmapCursor {
        explicit PixmapCursor(const QPixmap &pix = QPixmap(), const QPoint &h = QPoint()) : pixmap(pix), hotSpot(h) {}

        QPixmap pixmap;
        QPoint hotSpot;
    };

    explicit QWindowsCursor(const QPlatformScreen *screen);

    void changeCursor(QCursor * widgetCursor, QWindow * widget) override;
    void setOverrideCursor(const QCursor &cursor) override;
    void clearOverrideCursor() override;
    static void enforceOverrideCursor();
    static bool hasOverrideCursor() { return m_overriddenCursor != nullptr; }

    QPoint pos() const override;
    void setPos(const QPoint &pos) override;

    QSize size() const override;

    static HCURSOR createPixmapCursor(QPixmap pixmap, const QPoint &hotSpot, qreal scaleFactor = 1);
    static HCURSOR createPixmapCursor(const PixmapCursor &pc, qreal scaleFactor = 1) { return createPixmapCursor(pc.pixmap, pc.hotSpot, scaleFactor); }
    static PixmapCursor customCursor(Qt::CursorShape cursorShape, const QPlatformScreen *screen = nullptr);

    static HCURSOR createCursorFromShape(Qt::CursorShape cursorShape, const QPlatformScreen *screen = nullptr);
    static QPoint mousePosition();
    static State cursorState();

    CursorHandlePtr standardWindowCursor(Qt::CursorShape s = Qt::ArrowCursor);
    CursorHandlePtr pixmapWindowCursor(const QCursor &c);

    QPixmap dragDefaultCursor(Qt::DropAction action) const;

    HCURSOR hCursor(const QCursor &c) const;

private:
    typedef QHash<Qt::CursorShape, CursorHandlePtr> StandardCursorCache;
    typedef QHash<QWindowsPixmapCursorCacheKey, CursorHandlePtr> PixmapCursorCache;

    CursorHandlePtr cursorHandle(const QCursor &c);

    const QPlatformScreen *const m_screen;
    StandardCursorCache m_standardCursorCache;
    PixmapCursorCache m_pixmapCursorCache;

    mutable QPixmap m_copyDragCursor;
    mutable QPixmap m_moveDragCursor;
    mutable QPixmap m_linkDragCursor;
    mutable QPixmap m_ignoreDragCursor;

    static HCURSOR m_overriddenCursor;
    static HCURSOR m_overrideCursor;
};

QT_END_NAMESPACE

#endif // QWINDOWSCURSOR_H
