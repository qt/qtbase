// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgraphicsframecapturerenderdoc_p_p.h"
#include <QtCore/qcoreapplication.h>
#include <QtCore/qfile.h>
#include <QtCore/qlibrary.h>
#include <QtCore/qmutex.h>
#include "QtGui/rhi/qrhi.h"
#include "QtGui/rhi/qrhi_platform.h"
#include "QtGui/qopenglcontext.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcGraphicsFrameCapture, "qt.gui.graphicsframecapture")

RENDERDOC_API_1_6_0 *QGraphicsFrameCaptureRenderDoc::s_rdocApi = nullptr;
#if QT_CONFIG(thread)
QBasicMutex QGraphicsFrameCaptureRenderDoc::s_frameCaptureMutex;
#endif

#if QT_CONFIG(opengl)
static void *glNativeContext(QOpenGLContext *context) {
    void *nctx = nullptr;
    if (context != nullptr && context->isValid()) {
#ifdef Q_OS_WIN
        nctx = context->nativeInterface<QNativeInterface::QWGLContext>()->nativeContext();
#endif

#ifdef Q_OS_LINUX
#if QT_CONFIG(egl)
        QNativeInterface::QEGLContext *eglItf = context->nativeInterface<QNativeInterface::QEGLContext>();
        if (eglItf)
            nctx = eglItf->nativeContext();
#endif

#if QT_CONFIG(xcb_glx_plugin)
        QNativeInterface::QGLXContext *glxItf = context->nativeInterface<QNativeInterface::QGLXContext>();
        if (glxItf)
            nctx = glxItf->nativeContext();
#endif
#endif

#if QT_CONFIG(metal)
        nctx = context->nativeInterface<QNativeInterface::QCocoaGLContext>()->nativeContext();
#endif
    }
    return nctx;
}
#endif // QT_CONFIG(opengl)

/*!
    \class QGraphicsFrameCaptureRenderDoc
    \internal
    \brief The QGraphicsFrameCaptureRenderDoc class provides a way to capture a record of draw calls
           for different graphics APIs.
    \since 6.6
    \inmodule QtGui

    For applications that render using graphics APIs like Vulkan or OpenGL, it would be
    convenient to have a way to check the draw calls done by the application. Specially
    for applications that make a large amount of draw calls and the output is different
    from what is expected.

    This class acts as a wrapper over \l {https://renderdoc.org/}{RenderDoc} that allows
    applications to capture a rendered frame either programmatically, by clicking a key
    on the keyboard or both. The captured frame could be viewed later using RenderDoc GUI.

    Read the \l {https://renderdoc.org/docs/index.html} {RenderDoc Documentation}
    for more information.

    \section1 API device handle

    The functions that capture a frame like QGraphicsFrameCaptureRenderDoc::startCaptureFrame takes a device
    pointer as argument. This pointer is unique for each graphics API and is associated
    with the window that will be captured. This pointer has a default value of \c nullptr.
    If no value is passed to the function the underlying API will try to find the device to
    use, but it is not guaranteed specially in a multi-window applications.

    For OpenGL, the pointer should be the OpenGL context on the platform OpenGL is being
    used. For example, on Windows it should be \c HGLRC.

    For Vulkan, the pointer should point to the dispatch table within the \c VkInstance.
*/



/*!
    Creates a new object of this class. The constructor will load RenderDoc library
    from the default path.

    Only one instance of RenderDoc library is loaded at runtime which means creating
    several instances of this class will not affect the RenderDoc initialization.
*/

QGraphicsFrameCaptureRenderDoc::QGraphicsFrameCaptureRenderDoc()
    : m_nativeHandlesSet(false)
{
    if (!s_rdocApi)
        init();
}

void QGraphicsFrameCaptureRenderDoc::setRhi(QRhi *rhi)
{
    if (!rhi)
        return;

    QRhi::Implementation backend = rhi->backend();
    const QRhiNativeHandles *nh = rhi->nativeHandles();

    switch (backend) {
    case QRhi::Implementation::D3D11: {
#ifdef Q_OS_WIN
        const  QRhiD3D11NativeHandles *d3d11nh = static_cast<const QRhiD3D11NativeHandles *>(nh);
        m_nativeHandle = d3d11nh->dev;
        break;
#endif
        qCWarning(lcGraphicsFrameCapture) << "Could not find valid handles for D3D11. Check platform support";
        break;
    }
    case QRhi::Implementation::D3D12: {
#ifdef Q_OS_WIN
        const  QRhiD3D12NativeHandles *d3d12nh = static_cast<const QRhiD3D12NativeHandles *>(nh);
        m_nativeHandle = d3d12nh->dev;
        break;
#endif
        qCWarning(lcGraphicsFrameCapture) << "Could not find valid handles for D3D12. Check platform support";
        break;
    }
    case QRhi::Implementation::Vulkan: {
#if QT_CONFIG(vulkan)
        const  QRhiVulkanNativeHandles *vknh = static_cast<const QRhiVulkanNativeHandles *>(nh);
        m_nativeHandle = RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(vknh->inst->vkInstance());
        break;
#endif
        qCWarning(lcGraphicsFrameCapture) << "Could not find valid handles for Vulkan. Check platform support";
        break;
    }
    case QRhi::Implementation::OpenGLES2: {
#ifndef QT_NO_OPENGL
        const  QRhiGles2NativeHandles *glnh = static_cast<const QRhiGles2NativeHandles *>(nh);
        m_nativeHandle = glNativeContext(glnh->context);
        if (m_nativeHandle)
            break;
#endif
        qCWarning(lcGraphicsFrameCapture) << "Could not find valid handles for OpenGL. Check platform support";
        break;
    }
    case QRhi::Implementation::Metal:
    case QRhi::Implementation::Null:
        qCWarning(lcGraphicsFrameCapture) << "Invalid handles were provided."
                                             " Metal and Null backends are not supported with RenderDoc";
        break;
    }

    if (m_nativeHandle)
        m_nativeHandlesSet = true;
}

/*!
    Starts a frame capture using the set native handles provided through QGraphicsFrameCaptureRenderDoc::setRhi
    \a device. This function must be called before QGraphicsFrameCaptureRenderDoc::endCaptureFrame.
    \sa {API device handle}
*/
void QGraphicsFrameCaptureRenderDoc::startCaptureFrame()
{

    if (!initialized()) {
        qCWarning(lcGraphicsFrameCapture) << "RenderDoc was not initialized."
                                             " Starting capturing can not be done.";
        return;
    }

#if QT_CONFIG(thread)
    // There is a single instance of RenderDoc library and it needs mutex for multithreading access.
    QMutexLocker locker(&s_frameCaptureMutex);
#endif
    if (s_rdocApi->IsFrameCapturing()) {
        qCWarning(lcGraphicsFrameCapture) << "A frame capture is already in progress, "
                                             "will not initiate another one until"
                                             " QGraphicsFrameCapture::endCaptureFrame is called.";
        return;
    }

    qCInfo(lcGraphicsFrameCapture) << "A frame capture is going to start.";
    updateCapturePathAndTemplate();
    s_rdocApi->StartFrameCapture(m_nativeHandle, nullptr);
}

/*!
    Ends a frame capture started by a call to QGraphicsFrameCaptureRenderDoc::startCaptureFrame
    using the set native handles provided through QGraphicsFrameCaptureRenderDoc::setRhi.
    This function must be called after QGraphicsFrameCaptureRenderDoc::startCaptureFrame.
    Otherwise, a warning message will be printend and nothing will happen.
    \sa {API device handle}
*/
void QGraphicsFrameCaptureRenderDoc::endCaptureFrame()
{
    if (!initialized()) {
        qCWarning(lcGraphicsFrameCapture) << "RenderDoc was not initialized."
                                             " End capturing can not be done.";
        return;
    }

#if QT_CONFIG(thread)
    // There is a single instance of RenderDoc library and it needs mutex for multithreading access.
    QMutexLocker locker(&s_frameCaptureMutex);
#endif
    if (!s_rdocApi->IsFrameCapturing()) {
        qCWarning(lcGraphicsFrameCapture) << "A call to QGraphicsFrameCapture::endCaptureFrame can not be done"
                                             " without a call to QGraphicsFrameCapture::startCaptureFrame";
        return;
    }

    qCInfo(lcGraphicsFrameCapture) << "A frame capture is going to end.";
    uint32_t result = s_rdocApi->EndFrameCapture(m_nativeHandle, nullptr);

    if (result) {
        uint32_t count = s_rdocApi->GetNumCaptures();
        uint32_t pathLength = 0;
        s_rdocApi->GetCapture(count - 1, nullptr, &pathLength, nullptr);
        if (pathLength > 0) {
            QVarLengthArray<char> name(pathLength, 0);
            s_rdocApi->GetCapture(count - 1, name.data(), &pathLength, nullptr);
            m_capturedFilesNames.append(QString::fromUtf8(name.data(), -1));
        }
    }
}

void QGraphicsFrameCaptureRenderDoc::updateCapturePathAndTemplate()
{
    if (!initialized()) {
        qCWarning(lcGraphicsFrameCapture) << "RenderDoc was not initialized."
                                             " Updating save location can not be done.";
        return;
    }


    QString rdocFilePathTemplate = m_capturePath + QStringLiteral("/") + m_capturePrefix;
    s_rdocApi->SetCaptureFilePathTemplate(rdocFilePathTemplate.toUtf8().constData());
}

/*!
    Returns true if the API is loaded and can capture frames or not.
*/
bool QGraphicsFrameCaptureRenderDoc::initialized() const
{
    return s_rdocApi && m_nativeHandlesSet;
}

bool QGraphicsFrameCaptureRenderDoc::isCapturing() const
{
    if (!initialized()) {
        qCWarning(lcGraphicsFrameCapture) << "RenderDoc was not initialized."
                                             " Can not query if capturing is in progress or not.";
        return false;
    }

    return s_rdocApi->IsFrameCapturing();
}

void QGraphicsFrameCaptureRenderDoc::openCapture()
{
    if (!initialized()) {
        qCWarning(lcGraphicsFrameCapture) << "RenderDoc was not initialized."
                                             " Can not open RenderDoc UI tool.";
        return;
    }

#if QT_CONFIG(thread)
    // There is a single instance of RenderDoc library and it needs mutex for multithreading access.
    QMutexLocker locker(&s_frameCaptureMutex);
#endif
    if (s_rdocApi->IsTargetControlConnected())
        s_rdocApi->ShowReplayUI();
    else
        s_rdocApi->LaunchReplayUI(1, nullptr);
}

void QGraphicsFrameCaptureRenderDoc::init()
{
#if QT_CONFIG(thread)
    // There is a single instance of RenderDoc library and it needs mutex for multithreading access.
    QMutexLocker locker(&s_frameCaptureMutex);
#endif

    QLibrary renderDocLib(QStringLiteral("renderdoc"));
    pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI) renderDocLib.resolve("RENDERDOC_GetAPI");
    if (!renderDocLib.isLoaded() || (RENDERDOC_GetAPI == nullptr)) {
        qCWarning(lcGraphicsFrameCapture) << renderDocLib.errorString().toLatin1();
        return;
    }

    int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, static_cast<void **>(static_cast<void *>(&s_rdocApi)));

    if (ret == 0) {
        qCWarning(lcGraphicsFrameCapture) << "The requested RenderDoc API is invalid or not supported";
        return;
    }

    s_rdocApi->MaskOverlayBits(RENDERDOC_OverlayBits::eRENDERDOC_Overlay_None,
                               RENDERDOC_OverlayBits::eRENDERDOC_Overlay_None);
    s_rdocApi->SetCaptureKeys(nullptr, 0);
    s_rdocApi->SetFocusToggleKeys(nullptr, 0);

    QString rdocFilePathTemplate = m_capturePath + QStringLiteral("/") + m_capturePrefix;
    s_rdocApi->SetCaptureFilePathTemplate(rdocFilePathTemplate.toUtf8().constData());
}

QT_END_NAMESPACE
