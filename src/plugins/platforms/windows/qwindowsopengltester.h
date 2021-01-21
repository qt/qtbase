/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QWINDOWSOPENGLTESTER_H
#define QWINDOWSOPENGLTESTER_H

#include <QtCore/qbytearray.h>
#include <QtCore/qflags.h>
#include <QtCore/qvector.h>
#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

class QDebug;
class QVariant;

struct GpuDescription
{
    static GpuDescription detect();
    static QVector<GpuDescription> detectAll();
    QString toString() const;
    QVariant toVariant() const;

    uint vendorId = 0;
    uint deviceId = 0;
    uint revision = 0;
    uint subSysId = 0;
    QVersionNumber driverVersion;
    QByteArray driverName;
    QByteArray description;
    QString gpuSuitableScreen;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const GpuDescription &gd);
#endif

class QWindowsOpenGLTester
{
public:
    enum Renderer {
        InvalidRenderer         = 0x0000,
        DesktopGl               = 0x0001,
        AngleRendererD3d11      = 0x0002,
        AngleRendererD3d9       = 0x0004,
        AngleRendererD3d11Warp  = 0x0008, // "Windows Advanced Rasterization Platform"
        AngleBackendMask        = AngleRendererD3d11 | AngleRendererD3d9 | AngleRendererD3d11Warp,
        Gles                    = 0x0010, // ANGLE/unspecified or Generic GLES for Windows CE.
        GlesMask                = Gles | AngleBackendMask,
        SoftwareRasterizer      = 0x0020,
        RendererMask            = 0x00FF,
        DisableRotationFlag     = 0x0100,
        DisableProgramCacheFlag = 0x0200
    };
    Q_DECLARE_FLAGS(Renderers, Renderer)

    static Renderer requestedGlesRenderer();
    static Renderer requestedRenderer();

    static QWindowsOpenGLTester::Renderers  supportedRenderers(Renderer requested);

private:
    static Renderers detectSupportedRenderers(const GpuDescription &gpu, Renderer requested);
    static bool testDesktopGL();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QWindowsOpenGLTester::Renderers)

QT_END_NAMESPACE

#endif // QWINDOWSOPENGLTESTER_H
