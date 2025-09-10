// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgraphicsframecapture_p.h"
#if (defined (Q_OS_WIN) || defined(Q_OS_LINUX)) && QT_CONFIG(library)
#include "qgraphicsframecapturerenderdoc_p_p.h"
#elif QT_CONFIG(metal)
#include "qgraphicsframecapturemetal_p_p.h"
#else
#include "qgraphicsframecapture_p_p.h"
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
#if (defined (Q_OS_WIN) || defined(Q_OS_LINUX)) && QT_CONFIG(library)
    d.reset(new QGraphicsFrameCaptureRenderDoc);
#elif QT_CONFIG(metal)
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

QString QGraphicsFrameCapture::capturePath() const
{
    if (!d.isNull())
        return d->capturePath();
    return QString();
}

void QGraphicsFrameCapture::setCapturePath(const QString &path)
{
    if (!d.isNull())
        d->setCapturePath(path);
}

QString QGraphicsFrameCapture::capturePrefix() const
{
    if (!d.isNull())
        return d->capturePrefix();
    return QString();
}

void QGraphicsFrameCapture::setCapturePrefix(const QString &prefix)
{
    if (!d.isNull())
        d->setCapturePrefix(prefix);
}

QString QGraphicsFrameCapture::capturedFileName()
{
    if (!d.isNull())
        return d->capturedFileName();

    return QString();
}

QStringList QGraphicsFrameCapture::capturedFilesNames()
{
    if (!d.isNull())
        return d->capturedFilesNames();

    return QStringList();
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
