// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosplatformbackingstore.h"
#include "qohosplatformscreen.h"
#include "render/qohossurface.h"
#include <native_buffer/native_buffer.h>
#include <native_window/buffer_handle.h>
#include <native_window/external_window.h>
#include <qarkui/vsync.h>
#include <qohosutils.h>
#include <render/qohosview.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qendian.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <tuple>

QT_BEGIN_NAMESPACE

namespace
{

constexpr std::int32_t ohNativeWindowErrorCodeSuccess = 0;

std::uint64_t getNativeWindowBufferQueueSize(::OHNativeWindow *nativeWindow)
{
    std::int32_t bufferQueueSize;
    auto getBufferQueueSizeResult = ::OH_NativeWindow_NativeWindowHandleOpt(
        nativeWindow, ::NativeWindowOperation::GET_BUFFERQUEUE_SIZE, &bufferQueueSize);
    if (Q_UNLIKELY(getBufferQueueSizeResult != ohNativeWindowErrorCodeSuccess)) {
        qOhosReportFatalErrorAndAbort(
            "%s: failed to get buffer queue size with error code: %d",
            Q_FUNC_INFO, getBufferQueueSizeResult);
    }
    return bufferQueueSize;
}

std::size_t qImageBytesPerPixel(const QImage &image)
{
    const auto bitsPerPixel = image.depth();
    const auto bytesPerPixel = bitsPerPixel / 8;
    return bytesPerPixel;
}

QSpan<uchar> qImageScanLine(QImage &image, int y)
{
    return QSpan(image.scanLine(y), image.width() * qImageBytesPerPixel(image));
}

void copyImageRow(QSpan<const uchar> srcRow, QSpan<uchar> dstRow)
{
    const auto sizeToCopy = std::min(srcRow.size(), dstRow.size());
    std::memcpy(dstRow.data(), srcRow.data(), sizeToCopy);
}

void copyImage(QOhosPlatformBackingStore::QImageView srcImage, QImage &dstImage)
{
    const auto heightToCopy = std::min(srcImage.size().height(), dstImage.height());
    for (int i = 0; i < heightToCopy; ++i) {
        auto srcRow = srcImage.constScanLine(i);
        auto dstRow = qImageScanLine(dstImage, i);
        copyImageRow(srcRow, dstRow);
    }
}

void copyImage(QOhosPlatformBackingStore::QImageView srcImage, QImage &dstImage, const QRegion &region)
{
    const auto intersectedRegion =
        region.intersected(QRect({}, srcImage.size())).intersected(QRect({}, dstImage.size()));

    for (const auto &rect : intersectedRegion) {
        const auto xSrc = rect.x() * srcImage.bytesPerPixel();
        const auto widthSrc = rect.width() * srcImage.bytesPerPixel();
        const auto xDst = rect.x() * qImageBytesPerPixel(dstImage);
        const auto widthDst = rect.width() * qImageBytesPerPixel(dstImage);

        for (int row = 0; row < rect.height(); ++row) {
            const auto y = rect.y() + row;
            const auto srcRow = srcImage.constScanLine(y).subspan(xSrc, widthSrc);
            auto dstRow = qImageScanLine(dstImage, y).subspan(xDst, widthDst);
            copyImageRow(srcRow, dstRow);
        }
    }
}

void debugDrawFlushedQRegion(QImage &dstImage, const QRegion &region)
{
    static const QColor debugBoxColor("#7F00FF00");

    QPainter painter(&dstImage);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    for (const QRect &rect : region)
        painter.fillRect(rect, debugBoxColor);
}

std::vector<::Region::Rect> makeOhosRegionRectsForFlush(
    const QRegion &region, const QPoint &rootWindowOffset, const QSize &dstImageSize)
{
    std::vector<::Region::Rect> rects;
    if (!region.isEmpty()) {
        std::transform(
            region.begin(), region.end(), std::back_inserter(rects),
            [&](const auto &qrect) {
                return ::Region::Rect{
                    .x = qrect.x() + rootWindowOffset.x(),
                    .y = (dstImageSize.height() - qrect.y()) + rootWindowOffset.y() - qrect.height(),
                    .w = static_cast<std::uint32_t>(qrect.width()),
                    .h = static_cast<std::uint32_t>(qrect.height()),
                };
            });
    }
    return rects;
}

std::function<void()> makeVSyncFrameRequestFunc(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef, ::OHNativeWindow *nativeWindow,
    QOhosConsumer<QWindow *> qtThreadFlushFunc)
{
    auto sharedQtThreadFlushFunc = QtOhos::moveToSharedPtr(std::move(qtThreadFlushFunc));
    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &) {
            auto sharedFrameRequestFunc = QtOhos::makeProxyWithJsThreadDeleter(
                QtOhos::moveToSharedPtr(
                    QArkUi::makeVSyncFrameRequester(
                        nativeWindow,
                        [qWindowRef, sharedQtThreadFlushFunc]() {
                            qWindowRef.visitInQtThreadIfAlive(
                                [sharedQtThreadFlushFunc](QWindow &qWindow) {
                                    (*sharedQtThreadFlushFunc)(&qWindow);
                                });
                            })));

            return [sharedFrameRequestFunc]() {
                (*sharedFrameRequestFunc)();
            };
        });
}

}

QOhosPlatformBackingStore::QOhosPlatformBackingStore(QWindow *window, const CreateInfo &createInfo)
    : QRasterBackingStore(window)
    , m_debugDrawFlushedRegion(createInfo.debugDrawFlushedRegion)
    , m_vsyncEnabled(createInfo.enableVsync)
    , m_windowContextManager(
        createInfo.enableVsync,
        [this](QWindow *qWindow) {
            flushImmediate(qWindow);
        })
{
}

void QOhosPlatformBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    if (m_reinitializeContextManager) {
        m_windowContextManager = WindowContextManager(
            m_vsyncEnabled,
            [this](QWindow *qWindow) {
                flushImmediate(qWindow);
            });
        m_reinitializeContextManager = false;
    }

    auto *platformWindow = QOhosPlatformWindow::fromQWindowOrNull(window);
    auto *surface = platformWindow != nullptr ? platformWindow->ownedSurfaceOrNull() : nullptr;
    auto bounds = region.translated(offset).boundingRect() & m_image.rect();

    if (bounds.isEmpty())
        return;

    if (surface == nullptr) {
        qOhosPrintfDebug("Window %p has no surface", window);
        return;
    }

    m_windowContextManager
        .getOrCreateWindowContext(window, surface->nativeWindow())
        .flushData().updateDirtyRegionAndScheduleFlush(region, offset);
}

void QOhosPlatformBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    m_reinitializeContextManager = (size != m_requestedSize);
    QRasterBackingStore::resize(size, staticContents);
}

QImage::Format QOhosPlatformBackingStore::format() const
{
    return QOhosSurface::mapNativeBufferFormatToQImageFormatOrFail(QOhosSurface::bufferFormat);
}

QOhosPlatformBackingStore::QImageView::QImageView(const QImage &srcImage, const QRect &subRect)
    : m_srcImage(srcImage)
    , m_subRect(subRect)
{
    auto imageRect = QRect(QPoint{}, m_srcImage.size());
    if (!imageRect.contains(m_subRect)) {
        qOhosReportFatalErrorAndAbort(
            "image rect: (%d, %d, %d, %d) does not contain sub-rect: (%d, %d, %d, %d)",
            imageRect.x(), imageRect.y(), imageRect.width(), imageRect.height(),
            m_subRect.x(), m_subRect.y(), m_subRect.width(), m_subRect.height());
    }

    constexpr auto minAcceptedImageDepth = 8;
    if (srcImage.depth() < minAcceptedImageDepth)
        qOhosReportFatalErrorAndAbort("QImageView is not supported for <8bpp QImages");
}

std::size_t QOhosPlatformBackingStore::QImageView::bytesPerPixel() const
{
    return qImageBytesPerPixel(m_srcImage);
}

QSpan<const uchar> QOhosPlatformBackingStore::QImageView::constScanLine(int i) const
{
    Q_ASSERT(i >= 0 && i < m_subRect.height());
    auto srcScanLine = QSpan(
        m_srcImage.constScanLine(m_subRect.y() + i), m_srcImage.bytesPerLine());
    return srcScanLine.subspan(
        m_subRect.x() * bytesPerPixel(), bytesPerLine());
}

std::size_t QOhosPlatformBackingStore::QImageView::bytesPerLine() const
{
    return bytesPerPixel() * m_subRect.width();
}

QSize QOhosPlatformBackingStore::QImageView::size() const
{
    return m_subRect.size();
}

QOhosPlatformBackingStore::BufferRegionHandler::BufferRegionHandler(::OHNativeWindow *nativeWindow)
    : m_bufferQueueSize(getNativeWindowBufferQueueSize(nativeWindow))
{
}

QOhosOptional<QRegion> QOhosPlatformBackingStore::BufferRegionHandler::mergeRegionForBufferHandle(
    ::BufferHandle *bufferHandle,
    QRegion region) const
{
    const auto it = m_buffersToFlushSequenceIds.find(bufferHandle);
    if (it == m_buffersToFlushSequenceIds.end())
        return {};

    std::uint64_t numberOfRegions = m_flushSequenceId - it->second;
    if (numberOfRegions > m_bufferQueueSize)
        return {};

    for (auto it = m_lastFlushedRegions.cend() - numberOfRegions; it != m_lastFlushedRegions.cend(); ++it)
        region = region.united(*it);

    return makeQOhosOptional(region);
}

void QOhosPlatformBackingStore::BufferRegionHandler::storeRegionForBufferHandle(
    ::BufferHandle *bufferHandle,
    const QRegion &region)
{
    m_buffersToFlushSequenceIds[bufferHandle] = m_flushSequenceId;

    if (m_buffersToFlushSequenceIds.size() > m_bufferQueueSize) {
        m_buffersToFlushSequenceIds.clear();
        m_lastFlushedRegions.clear();
        m_flushSequenceId = 0;
        return;
    }

    m_lastFlushedRegions.push_back(region);
    if (m_lastFlushedRegions.size() > m_bufferQueueSize)
        m_lastFlushedRegions.pop_front();

    ++m_flushSequenceId;
}

QOhosPlatformBackingStore::FlushData::FlushData(std::function<void()> flushRequestFunc)
    : m_flushRequestFunc(std::move(flushRequestFunc))
{
}

std::pair<QRegion, QPoint> QOhosPlatformBackingStore::FlushData::fetchAndReset()
{
    return std::make_pair(
        std::exchange(m_mergedRegionForFlush, QRegion {}),
        std::exchange(m_lastWindowOffset, QPoint {}));
}

void QOhosPlatformBackingStore::FlushData::updateDirtyRegionAndScheduleFlush(
    const QRegion &region, const QPoint &rootWindowOffset)
{
    m_mergedRegionForFlush = m_mergedRegionForFlush.united(region);
    m_lastWindowOffset = rootWindowOffset;
    m_flushRequestFunc();
}

QOhosPlatformBackingStore::WindowContext::WindowContext(
    ::OHNativeWindow *nativeWindow, std::function<void()> flushRequestFunc)
    : m_bufferRegionHandler(std::make_unique<BufferRegionHandler>(nativeWindow))
    , m_flushData(std::move(flushRequestFunc))
{
}

QOhosPlatformBackingStore::BufferRegionHandler &
QOhosPlatformBackingStore::WindowContext::bufferRegionHandler()
{
    return *m_bufferRegionHandler;
}

QOhosPlatformBackingStore::FlushData &QOhosPlatformBackingStore::WindowContext::flushData()
{
    return m_flushData;
}

QOhosPlatformBackingStore::WindowContextManager::WindowContextManager(
    bool vsyncEnabled,
    std::function<void(QWindow *)> flushImmediateFunc)
    : m_flushFunc(QtOhos::moveToSharedPtr(std::move(flushImmediateFunc)))
    , m_vsyncEnabled(vsyncEnabled)
{
}

QOhosPlatformBackingStore::WindowContext &
QOhosPlatformBackingStore::WindowContextManager::getOrCreateWindowContext(
    QWindow *window,
    ::OHNativeWindow *nativeWindow)
{
    auto handlerIter = m_windowContexts.find(window);
    if (handlerIter == m_windowContexts.end()) {

        std::function<void()> flushFunc;

        if (m_vsyncEnabled) {
            auto weakQtFlushFunc = QtOhos::makeWeakPtr(m_flushFunc);
            auto qWindowRef = QtOhos::makeQThreadSafeRef(window);
            flushFunc = makeVSyncFrameRequestFunc(
                qWindowRef, nativeWindow,
                [weakQtFlushFunc](QWindow *qWindow) {
                    auto sharedQtThreadFlushFunc = weakQtFlushFunc.lock();
                    if (sharedQtThreadFlushFunc)
                        (*sharedQtThreadFlushFunc)(qWindow);
                });
        } else {
            flushFunc = [window, flushFunc = m_flushFunc]() {
                (*flushFunc)(window);
            };
        }

        std::tie(handlerIter, std::ignore) = m_windowContexts.emplace(
            window, std::make_unique<WindowContext>(
                nativeWindow, std::move(flushFunc)));
    }
    return *(handlerIter->second);
}

bool QOhosPlatformBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    Q_GUI_EXPORT void qt_scrollRectInImage(QImage &img, const QRect &rect, const QPoint &offset);

    const qreal devicePixelRatio = m_image.devicePixelRatio();
    const QPoint delta(
        static_cast<int>(dx * devicePixelRatio),
        static_cast<int>(dy * devicePixelRatio));

    for (const QRect &rect : area) {
        qt_scrollRectInImage(
            m_image,
            QRect(rect.topLeft() * devicePixelRatio, rect.size() * devicePixelRatio), delta);
    }

    return true;
}

void QOhosPlatformBackingStore::flushImmediate(QWindow *window)
{
    auto *platformWindow = QOhosPlatformWindow::fromQWindowOrNull(window);
    auto *surface = platformWindow != nullptr ? platformWindow->ownedSurfaceOrNull() : nullptr;

    if (surface == nullptr) {
        qOhosPrintfDebug("Window %p has no surface", window);
        return;
    }

    bool isRootWindow = window == this->window();

    ::OHNativeWindow *nativeWindow = surface->nativeWindow();

    auto &windowContext = m_windowContextManager.getOrCreateWindowContext(window, nativeWindow);
    QRegion region;
    QPoint rootWindowOffset;
    std::tie(region, rootWindowOffset) = windowContext.flushData().fetchAndReset();

    if (region.isEmpty())
        return;

    auto &bufferRegionHandler = m_windowContextManager
        .getOrCreateWindowContext(window, nativeWindow).bufferRegionHandler();

    auto srcImageRect = isRootWindow
        ? QRect({}, m_image.size())
        : QRect(rootWindowOffset, platformWindow->geometry().size()).intersected(QRect({}, m_image.size()));
    if (srcImageRect.isEmpty()) {
        qOhosPrintfDebug("Cannot get source image rect, ignore flush call");
        return;
    }

    QImageView srcImage = QImageView(m_image, srcImageRect);

    surface->paintOnNativeWindowSurface(
        [&](QImage &dstImage, ::BufferHandle *bufferHandle) {
            if (srcImage.bytesPerPixel() != qImageBytesPerPixel(dstImage)) {
                qOhosReportFatalErrorAndAbort(
                    "%s: bytes per pixel in src and dst image mismatch. Image formats are not the same.", Q_FUNC_INFO);
            }

            const auto& mergedRegionOpt = bufferRegionHandler.mergeRegionForBufferHandle(bufferHandle, region);
            if (mergedRegionOpt.hasValue())
                copyImage(srcImage, dstImage, mergedRegionOpt.value());
            else
                copyImage(srcImage, dstImage);

            if (m_debugDrawFlushedRegion)
                debugDrawFlushedQRegion(dstImage, region);

            return makeOhosRegionRectsForFlush(
                mergedRegionOpt.valueOr(QRegion()), isRootWindow ? QPoint{} : rootWindowOffset, dstImage.size());
        },
        [&](::BufferHandle *bufferHandle) {
            bufferRegionHandler.storeRegionForBufferHandle(bufferHandle, region);
        });

    if (isRootWindow) {
        auto *view = platformWindow->ownedViewOrNull();
        if (view != nullptr)
            view->handleSurfaceContentsUpdated();
    }
}

QT_END_NAMESPACE
