// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGRAPHICSFRAMECAPTURE_P_P_H
#define QGRAPHICSFRAMECAPTURE_P_P_H

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

#include <QtCore/qnamespace.h>
#include <QtCore/qstring.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcGraphicsFrameCapture)

class QRhi;
struct QRhiNativeHandles;

class QGraphicsFrameCapturePrivate
{
public:
    QGraphicsFrameCapturePrivate() ;
    virtual ~QGraphicsFrameCapturePrivate() = default;

    virtual void setRhi(QRhi *rhi) = 0;
    virtual void startCaptureFrame() = 0;
    virtual void endCaptureFrame() = 0;

    QString capturePath() const { return m_capturePath; };
    virtual void setCapturePath(const QString &path) { m_capturePath = path; }

    QString capturePrefix() const { return m_capturePrefix; }
    virtual void setCapturePrefix(const QString &prefix) { m_capturePrefix = prefix; }

    virtual QString capturedFileName() const
    {
        return !m_capturedFilesNames.isEmpty() ? m_capturedFilesNames.last() : QString();
    }
    virtual QStringList capturedFilesNames() const { return m_capturedFilesNames; }

    virtual bool initialized() const = 0;
    virtual bool isCapturing() const = 0;
    virtual void openCapture() = 0;

protected:
    QRhi *m_rhi = nullptr;
    QRhiNativeHandles *m_rhiHandles = nullptr;
    void *m_nativeHandle = nullptr;
    QString m_capturePath;
    QString m_capturePrefix;
    QStringList m_capturedFilesNames;
};

QT_END_NAMESPACE

#endif // QGRAPHICSFRAMECAPTURE_P_P_H
