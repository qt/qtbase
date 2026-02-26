// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <render/qohossurface.h>

#include <QtCore/qscopeguard.h>
#include <QtGui/qimage.h>
#include <native_window/external_window.h>
#include <native_buffer/native_buffer.h>
#include <native_window/buffer_handle.h>
#include <poll.h>
#include <qohosplugincore.h>
#include <qohosutils.h>
#include <sys/mman.h>
#include <unistd.h>

#if QT_CONFIG(vulkan)
#include <QtGui/qvulkaninstance.h>
#endif

namespace ch = std::chrono;

QT_BEGIN_NAMESPACE

namespace
{

constexpr std::int32_t ohNativeWindowErrorCodeSuccess = 0;

void setupNativeWindowBufferUsage(::OHNativeWindow *nativeWindow, std::uint64_t usageSetBits)
{
    std::uint64_t windowBufferUsage = 0;
    auto getUsageRes = ::OH_NativeWindow_NativeWindowHandleOpt(
        nativeWindow, ::NativeWindowOperation::GET_USAGE, &windowBufferUsage);
    if (Q_UNLIKELY(getUsageRes != ohNativeWindowErrorCodeSuccess))
        qOhosReportFatalErrorAndAbort("QOhosNativeXComponent: error reading window buffer usage: %d", getUsageRes);

    std::uint64_t requestedWindowBufferUsage = windowBufferUsage | usageSetBits;
    if (requestedWindowBufferUsage != windowBufferUsage) {
        auto setUsageRes = ::OH_NativeWindow_NativeWindowHandleOpt(
            nativeWindow, ::NativeWindowOperation::SET_USAGE, requestedWindowBufferUsage);
        if (Q_UNLIKELY(setUsageRes != ohNativeWindowErrorCodeSuccess))
            qOhosReportFatalErrorAndAbort("QOhosNativeXComponent: error setting window buffer usage: %d", setUsageRes);
    }
}

void setupNativeWindowBufferFormat(::OHNativeWindow *nativeWindow)
{
    auto setFormatRes = ::OH_NativeWindow_NativeWindowHandleOpt(
        nativeWindow, ::NativeWindowOperation::SET_FORMAT, QOhosSurface::bufferFormat);
    if (Q_UNLIKELY(setFormatRes != ohNativeWindowErrorCodeSuccess)) {
        qOhosReportFatalErrorAndAbort(
            "%s: QOhosNativeXComponent: error setting window buffer format: %d",
            Q_FUNC_INFO, setFormatRes);
    }
}

std::shared_ptr<int> makeFdAutoClosingWrapper()
{
    auto fileDescriptor = std::make_shared<int>(-1);
    return std::shared_ptr<int>(
        fileDescriptor.get(),
        [fileDescriptor](auto) {
            if (*fileDescriptor != -1)
                std::ignore = ::close(*fileDescriptor);
        });
}

bool tryMapBufferHandleMemory(::BufferHandle *bufferHandle, void *&outMemory)
{
    void *bufferMemory = ::mmap(
        bufferHandle->virAddr, bufferHandle->size, PROT_WRITE, MAP_SHARED, bufferHandle->fd, 0);

    if (bufferMemory == MAP_FAILED) {
        int mmapErrno = errno;
        qCWarning(QtForOhos, "%s: mmap failed with error: %d", Q_FUNC_INFO, mmapErrno);
        return false;
    }

    outMemory = bufferMemory;
    return true;
}

bool waitForFdReadyForReadOrTimeout(int fd, const char *fdName, ch::milliseconds timeoutMs)
{
    std::array<::pollfd, 1> pollFileDescriptors = {{
        {
            .fd = fd,
            .events = POLLIN,
        },
    }};

    int pollRetCode;
    do {
        pollRetCode = ::poll(pollFileDescriptors.data(), pollFileDescriptors.size(), timeoutMs.count());
    } while (pollRetCode == -1 && (errno == EINTR || errno == EAGAIN));

    auto pollErrno = errno;

    if (pollRetCode == -1) {
        qCWarning(
            QtForOhos, "%s: polling '%s' file descriptor failed with error: %d",
            Q_FUNC_INFO, fdName, pollErrno);
    } else if (pollRetCode == 0) {
        qCWarning(
            QtForOhos, "%s: polling '%s' file descriptor timed out",
            Q_FUNC_INFO, fdName);
    }

    return pollRetCode > 0;
}

}

QImage::Format
QOhosSurface::mapNativeBufferFormatToQImageFormatOrFail(std::int32_t format)
{
    static_assert(
        Q_BYTE_ORDER == Q_LITTLE_ENDIAN,
        "Pixel format mapping is currently supported for little-endian targets only");

    QImage::Format result;

    switch (format) {
    case ::NATIVEBUFFER_PIXEL_FMT_RGB_888:
        result = QImage::Format_RGB888;
        break;
    case ::NATIVEBUFFER_PIXEL_FMT_RGBA_8888:
        result = QImage::Format_RGBA8888;
        break;
    case ::NATIVEBUFFER_PIXEL_FMT_BGRA_8888:
        result = QImage::Format_ARGB32_Premultiplied;
        break;
    default:
        qOhosReportFatalErrorAndAbort("%s: unsupported format %d", Q_FUNC_INFO, format);
    }

    return result;
}

QOhosOptional<QSize> QOhosSurface::tryGetBufferGeometryForWindow(::OHNativeWindow *nativeWindow)
{
    std::int32_t surfaceWidth = 0;
    std::int32_t surfaceHeight = 0;

    // NOTE - The parameters height and width are reversed than what is usual order
    // on purpose as required by the documentation
    std::int32_t getWindowHandleErrorCode = ::OH_NativeWindow_NativeWindowHandleOpt(
        nativeWindow, ::GET_BUFFER_GEOMETRY, &surfaceHeight, &surfaceWidth);

    return getWindowHandleErrorCode == ohNativeWindowErrorCodeSuccess
        ? QOhosOptional<QSize>({surfaceWidth, surfaceHeight})
        : makeEmptyQOhosOptional();
}

QOhosSurface::QOhosSurface(::OHNativeWindow *nativeWindow)
    : m_nativeWindow(nativeWindow), m_eglSurface()
{
    if (nativeWindow == nullptr)
        qOhosReportFatalErrorAndAbort("NativeWindow cannot be null");

    setupNativeWindowBufferUsage(
        nativeWindow,
        ::OH_NativeBuffer_Usage::NATIVEBUFFER_USAGE_CPU_READ);
}

::OHNativeWindow *QOhosSurface::nativeWindow() const
{
    return m_nativeWindow;
}

void QOhosSurface::setNativeWindowSurface(
    ::OHNativeWindow *nativeWindow, const QOhosOptional<QSize> &optSurfaceSize)
{
    if (nativeWindow == nullptr)
        qOhosReportFatalErrorAndAbort("NativeWindow cannot be null");

    m_nativeWindow = nativeWindow;

#if QT_CONFIG(vulkan)
    if (m_vulkanSurface)
        m_vulkanSurface->setNativeWindowSurface(nativeWindow);
#endif
    setupNativeWindowBufferUsage(
        nativeWindow,
        ::OH_NativeBuffer_Usage::NATIVEBUFFER_USAGE_CPU_READ);

    if (m_eglSurface) {
        m_eglSurface->setNativeWindowSurface(
            reinterpret_cast<::EGLNativeWindowType>(m_nativeWindow), optSurfaceSize);
    }
}

EGLSurface QOhosSurface::tryGetOrCreateEGLWindowSurface(EGLDisplay display, EGLConfig config, bool swappingBuffers)
{
    if (!m_eglSurface) {
        m_eglSurface = std::make_unique<QOhosEGLSurface>();
        m_eglSurface->setNativeWindowSurface(
            reinterpret_cast<::EGLNativeWindowType>(m_nativeWindow), makeEmptyQOhosOptional());
    }
    return m_eglSurface->tryGetOrCreateEGLWindowSurface(display, config, swappingBuffers, {});
}

QOhosOptional<QSize> QOhosSurface::surfaceResolution() const
{
    return tryGetBufferGeometryForWindow(m_nativeWindow);
}

void QOhosSurface::paintOnNativeWindowSurface(
    std::function<std::vector<::Region::Rect>(QImage &, ::BufferHandle *)> paintFunc,
    std::function<void(::BufferHandle *)> onFlushSuccessFunc)
{
    ::OHNativeWindowBuffer *nativeWindowBuffer = nullptr;
    auto fenceFileDescriptor = makeFdAutoClosingWrapper();

    setupNativeWindowBufferFormat(m_nativeWindow);

    auto requestBufferEc = ::OH_NativeWindow_NativeWindowRequestBuffer(
        m_nativeWindow, &nativeWindowBuffer, fenceFileDescriptor.get());
    if (requestBufferEc != ohNativeWindowErrorCodeSuccess) {
        qCWarning(QtForOhos, "%s: NativeWindowRequestBuffer failed with error code: %d", Q_FUNC_INFO, requestBufferEc);
        return;
    }

    auto nativeWindowAbortBufferGuard = qScopeGuard([nativeWindow = m_nativeWindow, nativeWindowBuffer](){
        auto abortBufferEc = ::OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, nativeWindowBuffer);
        if (abortBufferEc != ohNativeWindowErrorCodeSuccess)
            qCWarning(QtForOhos, "%s: NativeWindowAbortBuffer failed with error code: %d", Q_FUNC_INFO, abortBufferEc);
    });

    ::BufferHandle *bufferHandle = ::OH_NativeWindow_GetBufferHandleFromNative(nativeWindowBuffer);
    if (bufferHandle == nullptr) {
        qCWarning(QtForOhos, "%s: GetBufferHandleFromNative returned null", Q_FUNC_INFO);
        return;
    }

    QImage::Format dstImageFormat = QOhosSurface::mapNativeBufferFormatToQImageFormatOrFail(bufferHandle->format);
    QSize dstImageSize{bufferHandle->width, bufferHandle->height};

    void *bufferMemory = nullptr;
    if (!tryMapBufferHandleMemory(bufferHandle, bufferMemory))
        return;

    auto unmapMemoryGuard = qScopeGuard([bufferMemory, bufferMemorySize = bufferHandle->size]() {
        int result = ::munmap(bufferMemory, bufferMemorySize);
        if (result != 0) {
            int munmapErrno = errno;
            qCWarning(QtForOhos, "%s: munmap failed with error: %d", Q_FUNC_INFO, munmapErrno);
        }
    });

    if (*fenceFileDescriptor != -1) {
        if (!waitForFdReadyForReadOrTimeout(*fenceFileDescriptor, "window buffer fence", ch::seconds(3)))
            return;
    }

    QImage dstImage(
        reinterpret_cast<uchar *>(bufferMemory),
        dstImageSize.width(), dstImageSize.height(),
        bufferHandle->stride,
        dstImageFormat);

    auto rects = paintFunc(dstImage, bufferHandle);

    const std::int32_t acquireFenceFileDescriptor = -1;
    auto flushBufferEc = ::OH_NativeWindow_NativeWindowFlushBuffer(
        m_nativeWindow, nativeWindowBuffer, acquireFenceFileDescriptor,
        ::Region{
            .rects = rects.empty() ? nullptr : rects.data(),
            .rectNumber = static_cast<std::int32_t>(rects.size()),
        });
    if (flushBufferEc != ohNativeWindowErrorCodeSuccess) {
        qCWarning(QtForOhos, "%s: failed to flush buffer with error code: %d", Q_FUNC_INFO, flushBufferEc);
        return;
    }
    onFlushSuccessFunc(bufferHandle);
    nativeWindowAbortBufferGuard.dismiss();
}

void QOhosSurface::clearNativeWindowSurface()
{
    paintOnNativeWindowSurface(
        [](QImage &dstImage, ::BufferHandle *) {
            dstImage.fill(Qt::transparent);
            return std::vector<::Region::Rect>();
        },
        [](::BufferHandle *) {});
}

void QOhosSurface::setExtraUsageBitsForNativeWindowBuffer(std::uint64_t usageSetBits)
{
    setupNativeWindowBufferUsage(m_nativeWindow, usageSetBits);
}

#if QT_CONFIG(vulkan)

VkSurfaceKHR *QOhosSurface::tryGetOrCreateVulkanWindowSurface(QVulkanInstance *instance)
{
    if (!m_vulkanSurface) {
        m_vulkanSurface = std::make_unique<QOhosVulkanSurface>();
        m_vulkanSurface->setNativeWindowSurface(m_nativeWindow);
        setExtraUsageBitsForNativeWindowBuffer(NATIVEBUFFER_USAGE_HW_RENDER | NATIVEBUFFER_USAGE_HW_TEXTURE);
    }
    auto createFn = reinterpret_cast<PFN_vkCreateSurfaceOHOS>(
        instance->getInstanceProcAddr("vkCreateSurfaceOHOS"));
    auto destroyFn = reinterpret_cast<PFN_vkDestroySurfaceKHR>(
        instance->getInstanceProcAddr("vkDestroySurfaceKHR"));
    return m_vulkanSurface->tryGetOrCreateVulkanWindowSurface(instance->vkInstance(), createFn, destroyFn);
}

#endif // QT_CONFIG(vulkan)

QT_END_NAMESPACE
