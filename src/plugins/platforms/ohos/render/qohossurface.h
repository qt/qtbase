// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSURFACE_H
#define QOHOSSURFACE_H

#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qsize.h>
#include <QtGui/qimage.h>
#include <memory>
#include <native_buffer/native_buffer.h>
#include <native_window/external_window.h>
#include <qohosplugincore.h>
#include <render/qohosegl.h>

#if QT_CONFIG(vulkan)
#include <render/qohosvulkansurface.h>
class QVulkanInstance;
#endif

QT_BEGIN_NAMESPACE

class QOhosSurface final
{
public:
    static constexpr ::OH_NativeBuffer_Format bufferFormat = ::NATIVEBUFFER_PIXEL_FMT_BGRA_8888;
    static QImage::Format mapNativeBufferFormatToQImageFormatOrFail(std::int32_t format);
    static QOhosOptional<QSize> tryGetBufferGeometryForWindow(::OHNativeWindow *nativeWindow);

    explicit QOhosSurface(::OHNativeWindow *nativeWindow);

    ::OHNativeWindow *nativeWindow() const;
    void setNativeWindowSurface(::OHNativeWindow *nativeWindow, const QOhosOptional<QSize> &optSurfaceSize);
    EGLSurface tryGetOrCreateEGLWindowSurface(EGLDisplay display, EGLConfig config);
    QOhosOptional<QSize> surfaceResolution() const;

    void paintOnNativeWindowSurface(
        std::function<std::vector<::Region::Rect>(QImage &, ::BufferHandle *)> paintFunc,
        std::function<void(::BufferHandle *)> onFlushSuccessFunc);
    void clearNativeWindowSurface();

    void setExtraUsageBitsForNativeWindowBuffer(std::uint64_t usageSetBits);

#if QT_CONFIG(vulkan)
    VkSurfaceKHR *tryGetOrCreateVulkanWindowSurface(QVulkanInstance *instance);
#endif

private:
    ::OHNativeWindow *m_nativeWindow;
    std::unique_ptr<QOhosEGLSurface> m_eglSurface;
#if QT_CONFIG(vulkan)
    std::unique_ptr<QOhosVulkanSurface> m_vulkanSurface;
#endif
};

QT_END_NAMESPACE

#endif
