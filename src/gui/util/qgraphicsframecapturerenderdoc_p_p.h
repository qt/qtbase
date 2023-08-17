// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGRAPHICSFRAMECAPTURERENDERDOC_P_P_H
#define QGRAPHICSFRAMECAPTURERENDERDOC_P_P_H

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


#include <renderdoc_app.h>
#include "qgraphicsframecapture_p_p.h"

QT_BEGIN_NAMESPACE

#if QT_CONFIG(thread)
class QBasicMutex;
#endif

class QGraphicsFrameCaptureRenderDoc : public QGraphicsFrameCapturePrivate
{
public:
    QGraphicsFrameCaptureRenderDoc();
    ~QGraphicsFrameCaptureRenderDoc() = default;

    void setRhi(QRhi *rhi) override;
    void startCaptureFrame() override;
    void endCaptureFrame() override;
    bool initialized() const override;
    bool isCapturing() const override;
    void openCapture() override;

private:
    void init();
    void updateCapturePathAndTemplate();
    static RENDERDOC_API_1_5_0 *s_rdocApi;
#if QT_CONFIG(thread)
    static QBasicMutex s_frameCaptureMutex;
#endif
    bool m_nativeHandlesSet;
};

QT_END_NAMESPACE

#endif // QGRAPHICSFRAMECAPTURERENDERDOC_P_P_H
