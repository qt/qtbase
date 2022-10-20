/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Gui module
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

#ifndef QRHIPROFILER_H
#define QRHIPROFILER_H

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

#include <private/qrhi_p.h>

QT_BEGIN_NAMESPACE

class QRhiProfilerPrivate;
class QIODevice;

class Q_GUI_EXPORT QRhiProfiler
{
public:
    enum StreamOp {
        NewBuffer = 1,
        ReleaseBuffer,
        NewBufferStagingArea,
        ReleaseBufferStagingArea,
        NewRenderBuffer,
        ReleaseRenderBuffer,
        NewTexture,
        ReleaseTexture,
        NewTextureStagingArea,
        ReleaseTextureStagingArea,
        ResizeSwapChain,
        ReleaseSwapChain,
        NewReadbackBuffer,
        ReleaseReadbackBuffer,
        GpuMemAllocStats,
        GpuFrameTime,
        FrameToFrameTime,
        FrameBuildTime
    };

    ~QRhiProfiler();

    void setDevice(QIODevice *device);

    void addVMemAllocatorStats();

    int frameTimingWriteInterval() const;
    void setFrameTimingWriteInterval(int frameCount);

    struct CpuTime {
        qint64 minTime = 0;
        qint64 maxTime = 0;
        float avgTime = 0;
    };

    struct GpuTime {
        float minTime = 0;
        float maxTime = 0;
        float avgTime = 0;
    };

    CpuTime frameToFrameTimes(QRhiSwapChain *sc) const;
    CpuTime frameBuildTimes(QRhiSwapChain *sc) const; // beginFrame - endFrame
    GpuTime gpuFrameTimes(QRhiSwapChain *sc) const;

private:
    Q_DISABLE_COPY(QRhiProfiler)
    QRhiProfiler();
    QRhiProfilerPrivate *d;
    friend class QRhiImplementation;
    friend class QRhiProfilerPrivate;
};

Q_DECLARE_TYPEINFO(QRhiProfiler::CpuTime, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(QRhiProfiler::GpuTime, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif
