// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGRAPHICSFRAMECAPTURE_P_H
#define QGRAPHICSFRAMECAPTURE_P_H

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

#include <QtCore/qscopedpointer.h>
#include <QtGui/qtguiglobal.h>

QT_BEGIN_NAMESPACE

class QGraphicsFrameCapturePrivate;
class QRhi;

class Q_GUI_EXPORT QGraphicsFrameCapture
{
public:
    QGraphicsFrameCapture();
    ~QGraphicsFrameCapture();

    void setRhi(QRhi *rhi);
    void startCaptureFrame();
    void endCaptureFrame();

    QString capturePath() const;
    void setCapturePath(const QString &path);

    QString capturePrefix() const;
    void setCapturePrefix(const QString &prefix);

    QString capturedFileName();
    QStringList capturedFilesNames();

    bool isLoaded() const;
    bool isCapturing() const;
    void openCapture() const;

private:
    QScopedPointer<QGraphicsFrameCapturePrivate> d;
};

QT_END_NAMESPACE

#endif // QGRAPHICSFRAMECAPTURE_P_H
