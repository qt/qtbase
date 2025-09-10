// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgraphicsframecapturemetal_p_p.h"
#include <QtCore/qurl.h>
#include "Metal/Metal.h"
#include "qglobal.h"
#include <QtGui/rhi/qrhi.h>
#include <QtGui/rhi/qrhi_platform.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcGraphicsFrameCapture, "qt.gui.graphicsframecapture")

#if __has_feature(objc_arc)
#error ARC not supported
#endif

uint QGraphicsFrameCaptureMetal::frameNumber = 0;

QGraphicsFrameCaptureMetal::QGraphicsFrameCaptureMetal()
{
    qputenv("METAL_CAPTURE_ENABLED", QByteArrayLiteral("1"));

    m_captureDescriptor = [MTLCaptureDescriptor new];
}

QGraphicsFrameCaptureMetal::~QGraphicsFrameCaptureMetal()
{
#if defined(Q_OS_MACOS) && QT_CONFIG(process)
    if (m_process) {
        m_process->terminate();
        delete m_process;
    }
#endif
    [m_captureDescriptor release];
}

void QGraphicsFrameCaptureMetal::setRhi(QRhi *rhi)
{
    if (!rhi)
        return;

    QRhi::Implementation backend = rhi->backend();
    const QRhiNativeHandles *nh = rhi->nativeHandles();

    switch (backend) {
    case QRhi::Implementation::Metal: {
        const  QRhiMetalNativeHandles *mtlnh = static_cast<const QRhiMetalNativeHandles *>(nh);
        if (mtlnh->cmdQueue) {
            m_captureDescriptor.captureObject = mtlnh->cmdQueue;
        } else if (mtlnh->dev) {
            m_captureDescriptor.captureObject = mtlnh->dev;
        } else {
            qCWarning(lcGraphicsFrameCapture) << "No valid Metal Device or Metal Command Queue found";
            m_initialized = false;
            return;
        }
        break;
    }
    default: {
        qCWarning(lcGraphicsFrameCapture) << "Invalid handles were provided. MTLCaptureManager works only with Metal API";
        m_initialized = false;
        return;
        break;
    }
    }

    if (!m_captureManager) {
        m_captureManager = MTLCaptureManager.sharedCaptureManager;
        bool supportDocs = [m_captureManager supportsDestination:MTLCaptureDestinationGPUTraceDocument];
        if (supportDocs) {
            m_captureDescriptor.destination = MTLCaptureDestinationGPUTraceDocument;
            m_initialized = true;
        }
    }
}

void QGraphicsFrameCaptureMetal::startCaptureFrame()
{
    if (!initialized()) {
        qCWarning(lcGraphicsFrameCapture) << "Capturing on Metal was not initialized. Starting capturing can not be done.";
        return;
    }

    if (isCapturing()) {
        qCWarning(lcGraphicsFrameCapture) << "A frame capture is already in progress,"
                                             "will not initiate another one until QGraphicsFrameCapture::endCaptureFrame is called.";
        return;
    }

    updateCaptureFileName();
    NSError *error;
    if (![m_captureManager startCaptureWithDescriptor:m_captureDescriptor error:&error]) {
        QString errorMsg = QString::fromNSString(error.localizedDescription);
        qCWarning(lcGraphicsFrameCapture, "Failed to start capture : %s", qPrintable(errorMsg));
    }
}

void QGraphicsFrameCaptureMetal::endCaptureFrame()
{
    if (!initialized()) {
        qCWarning(lcGraphicsFrameCapture) << "Capturing on Metal was not initialized. End capturing can not be done.";
        return;
    }

    if (!isCapturing()) {
        qCWarning(lcGraphicsFrameCapture) << "A call to QGraphicsFrameCapture::endCaptureFrame can not be done"
                                             " without a call to QGraphicsFrameCapture::startCaptureFrame";
        return;
    }

    [m_captureManager stopCapture];
    m_capturedFilesNames.append(QString::fromNSString(m_traceURL.path));
    frameNumber++;
}

bool QGraphicsFrameCaptureMetal::initialized() const
{
    return m_initialized;
}

bool QGraphicsFrameCaptureMetal::isCapturing() const
{
    if (!initialized()) {
        qCWarning(lcGraphicsFrameCapture) << "Capturing on Metal was not initialized. Can not query if capturing is in progress or not.";
        return false;
    }

    return [m_captureManager isCapturing];
}

void QGraphicsFrameCaptureMetal::openCapture()
{
#if defined(Q_OS_MACOS)
#if !QT_CONFIG(process)
    qFatal("QGraphicsFrameCapture requires QProcess on macOS");
#else
    if (!initialized()) {
        qCWarning(lcGraphicsFrameCapture) << "Capturing on Metal was not initialized. Can not open XCode with a valid capture.";
        return;
    }

    if (!m_process) {
        m_process = new QProcess();
        m_process->setProgram(QStringLiteral("xed"));
        QStringList args;
        args.append(QUrl::fromNSURL(m_traceURL).toLocalFile());
        m_process->setArguments(args);
    }

    m_process->kill();
    m_process->start();
#endif
#endif
}

void QGraphicsFrameCaptureMetal::updateCaptureFileName()
{
    m_traceURL = QUrl::fromLocalFile(m_capturePath + u"/" + m_capturePrefix + u"_"
                                     + QString::number(frameNumber) + u".gputrace")
                         .toNSURL();
    // We need to remove the trace file if it already existed else MTLCaptureManager
    // will fail to.
    if ([NSFileManager.defaultManager fileExistsAtPath:m_traceURL.path])
        [NSFileManager.defaultManager removeItemAtURL:m_traceURL error:nil];

    m_captureDescriptor.outputURL = m_traceURL;
}

QT_END_NAMESPACE
