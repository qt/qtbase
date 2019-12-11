/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

inline uint qHash(const QWindowsPixmapCursorCacheKey &k, uint seed) noexcept
{
    return (uint(k.bitmapCacheKey) + uint(k.maskCacheKey)) ^ seed;
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
