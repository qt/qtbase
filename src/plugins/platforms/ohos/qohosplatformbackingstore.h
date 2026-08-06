// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPLATFORMBACKINGSTORE_H
#define QOHOSPLATFORMBACKINGSTORE_H

#include <qohosplatformwindow.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qspan.h>
#include <QtGui/qimage.h>
#include <qpa/qplatformbackingstore.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtGui/private/qrasterbackingstore_p.h>
#include <QtOpenGL/QOpenGLPaintDevice>
#include <QOpenGLContext>
#include <QScopedPointer>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <native_window/buffer_handle.h>
#include <native_window/external_window.h>
#include <optional>
#include <unordered_map>
#include <utility>

QT_BEGIN_NAMESPACE

class QOhosPlatformBackingStore final : public QRasterBackingStore
{
public:
    class QImageView
    {
    public:
        explicit QImageView(const QImage &srcImage, const QRect &subRect);

        QSize size() const;
        std::size_t bytesPerPixel() const;
        std::size_t bytesPerLine() const;
        QSpan<const uchar> constScanLine(int i) const;
    private:
        const QImage m_srcImage;
        QRect m_subRect;
    };

    class BufferRegionHandler
    {
    public:
        BufferRegionHandler(::OHNativeWindow *nativeWindow);

        std::optional<QRegion> mergeRegionForBufferHandle(::BufferHandle *bufferHandle, QRegion region) const;
        void storeRegionForBufferHandle(::BufferHandle *bufferHandle, const QRegion &region);

    private:
        std::uint64_t m_bufferQueueSize {0};
        std::uint64_t m_flushSequenceId {0};
        std::unordered_map<::BufferHandle *, std::uint64_t> m_buffersToFlushSequenceIds;
        std::deque<QRegion> m_lastFlushedRegions;
    };

    class FlushData
    {
    public:
        explicit FlushData(std::function<void()> flushRequestFunc);

        void updateDirtyRegionAndScheduleFlush(const QRegion &region, const QPoint &rootWindowOffset);
        std::pair<QRegion, QPoint> fetchAndReset();

    private:
        std::function<void()> m_flushRequestFunc;
        QRegion m_mergedRegionForFlush;
        QPoint m_lastWindowOffset;
    };

    class WindowContext
    {
    public:
        explicit WindowContext(
            ::OHNativeWindow *nativeWindow, std::function<void()> flushRequestFunc);

        BufferRegionHandler &bufferRegionHandler();
        FlushData &flushData();

    private:
        std::unique_ptr<BufferRegionHandler> m_bufferRegionHandler;
        FlushData m_flushData;
    };

    class WindowContextManager
    {
    public:
        WindowContextManager(bool vsyncEnabled, std::shared_ptr<std::function<void(QWindow *)>> flushImmediateFunc);

        WindowContext &getOrCreateWindowContext(QWindow *window, ::OHNativeWindow *nativeWindow);

    private:
        std::unordered_map<QWindow *, std::unique_ptr<WindowContext>> m_windowContexts;
        std::shared_ptr<std::function<void(QWindow *)>> m_flushFunc;
        bool m_vsyncEnabled;
    };

    struct CreateInfo
    {
        bool debugDrawFlushedRegion = false;
        bool enableVsync = false;
    };

    explicit QOhosPlatformBackingStore(QWindow *window, const CreateInfo &createInfo);
    void resize(const QSize &size, const QRegion &staticContents) override;
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) final;
    QImage::Format format() const override;
    bool scroll(const QRegion &area, int dx, int dy) override;
    void beginPaint(const QRegion &region) override;

private:
    void flushImmediate(QWindow *window);

    std::function<void()> m_windowBufferFlushedCallback;
    bool m_debugDrawFlushedRegion {false};
    bool m_vsyncEnabled {false};
    std::shared_ptr<std::function<void(QWindow *)>> m_flushFunc;
    WindowContextManager m_windowContextManager;
    bool m_reinitializeContextManager{false};
};

QT_END_NAMESPACE

#endif // QOHOSPLATFORMBACKINGSTORE_H
