// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGRAPHICSFRAMEDECAPTUREMETAL_P_P_H
#define QGRAPHICSFRAMEDECAPTUREMETAL_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qgraphicsframecapture_p_p.h"
#include <QtCore/qmutex.h>
#ifdef Q_OS_MACOS
#include <QtCore/qprocess.h>
#endif

Q_FORWARD_DECLARE_OBJC_CLASS(MTLCaptureManager);
Q_FORWARD_DECLARE_OBJC_CLASS(MTLCaptureDescriptor);
Q_FORWARD_DECLARE_OBJC_CLASS(NSURL);

QT_BEGIN_NAMESPACE

class QGraphicsFrameCaptureMetal : public QGraphicsFrameCapturePrivate
{
public:
    QGraphicsFrameCaptureMetal();
    ~QGraphicsFrameCaptureMetal();

    void setRhi(QRhi *rhi) override;
    void startCaptureFrame() override;
    void endCaptureFrame() override;
    bool initialized() const override;
    bool isCapturing() const override;
    void openCapture() override;

private:
    void updateCaptureFileName();
#if defined(Q_OS_MACOS) && QT_CONFIG(process)
    QProcess *m_process = nullptr;
#endif
    MTLCaptureManager *m_captureManager = nullptr;
    MTLCaptureDescriptor *m_captureDescriptor = nullptr;
    NSURL *m_traceURL = nullptr;
    bool m_initialized = false;
    static uint frameNumber;
};

QT_END_NAMESPACE

#endif // QGRAPHICSFRAMEDECAPTUREMETAL_P_P_H
