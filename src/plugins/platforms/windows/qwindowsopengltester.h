/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINDOWSOPENGLTESTER_H
#define QWINDOWSOPENGLTESTER_H

#include <QtCore/QByteArray>
#include <QtCore/QFlags>
#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

class QDebug;
class QVariant;

struct GpuDescription
{
    GpuDescription() :  vendorId(0), deviceId(0), revision(0), subSysId(0) {}

    static GpuDescription detect();
    QString toString() const;
    QVariant toVariant() const;

    uint vendorId;
    uint deviceId;
    uint revision;
    uint subSysId;
    QVersionNumber driverVersion;
    QByteArray driverName;
    QByteArray description;
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
        DisableRotationFlag     = 0x0100
    };
    Q_DECLARE_FLAGS(Renderers, Renderer)

    static Renderer requestedGlesRenderer();
    static Renderer requestedRenderer();

    static Renderers supportedGlesRenderers();
    static Renderers supportedRenderers();

private:
    static QWindowsOpenGLTester::Renderers detectSupportedRenderers(const GpuDescription &gpu, bool glesOnly);
    static bool testDesktopGL();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QWindowsOpenGLTester::Renderers)

QT_END_NAMESPACE

#endif // QWINDOWSOPENGLTESTER_H
