// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSOPENGLTESTER_H
#define QWINDOWSOPENGLTESTER_H

#include <QtCore/qbytearray.h>
#include <QtCore/qflags.h>
#include <QtCore/qlist.h>
#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

class QDebug;
class QVariant;

struct GpuDescription
{
    static GpuDescription detect();
    static QList<GpuDescription> detectAll();
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
        SoftwareRasterizer      = 0x0020,
        RendererMask            = 0x00FF,
        DisableRotationFlag     = 0x0100,
        DisableProgramCacheFlag = 0x0200
    };
    Q_DECLARE_FLAGS(Renderers, Renderer)

    static Renderer requestedRenderer();

    static QWindowsOpenGLTester::Renderers  supportedRenderers(Renderer requested);

private:
    static Renderers detectSupportedRenderers(const GpuDescription &gpu, Renderer requested);
    static bool testDesktopGL();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QWindowsOpenGLTester::Renderers)

QT_END_NAMESPACE

#endif // QWINDOWSOPENGLTESTER_H
