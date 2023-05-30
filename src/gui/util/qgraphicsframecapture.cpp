// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgraphicsframecapture_p.h"
#if defined (Q_OS_WIN) || defined(Q_OS_LINUX)
#include "qgraphicsframecapturerenderdoc_p_p.h"
#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
#include "qgraphicsframecapturemetal_p_p.h"
#endif

#include <QtCore/qstandardpaths.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

QGraphicsFrameCapturePrivate::QGraphicsFrameCapturePrivate()
    : m_capturePath(QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                    QStringLiteral("/") + QCoreApplication::applicationName() +
                    QStringLiteral("/captures")),
      m_capturePrefix(QCoreApplication::applicationName())
{

}

QGraphicsFrameCapture::QGraphicsFrameCapture()
{
#if defined (Q_OS_WIN) || defined(Q_OS_LINUX)
    d.reset(new QGraphicsFrameCaptureRenderDoc);
#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    d.reset(new QGraphicsFrameCaptureMetal);
#endif
}

QGraphicsFrameCapture::~QGraphicsFrameCapture()
{

}

void QGraphicsFrameCapture::setRhi(QRhi *rhi)
{
    if (!d.isNull())
        d->setRhi(rhi);
}

void QGraphicsFrameCapture::startCaptureFrame()
{
    if (!d.isNull())
        d->startCaptureFrame();
}

void QGraphicsFrameCapture::endCaptureFrame()
{
    if (!d.isNull())
        d->endCaptureFrame();
}

void QGraphicsFrameCapture::setCapturePath(const QString &path)
{
    if (!d.isNull())
        d->setCapturePath(path);
}

void QGraphicsFrameCapture::setCapturePrefix(const QString &prefix)
{
    if (!d.isNull())
        d->setCapturePrefix(prefix);
}

bool QGraphicsFrameCapture::isLoaded() const
{
    if (!d.isNull())
        return d->initialized();

    return false;
}

bool QGraphicsFrameCapture::isCapturing() const
{
    if (!d.isNull())
        return d->isCapturing();

    return false;
}

void QGraphicsFrameCapture::openCapture() const
{
    if (!d.isNull())
        d->openCapture();
}

QT_END_NAMESPACE
