/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Gui module
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
******************************************************************************/

#ifndef QRHIPROFILER_P_H
#define QRHIPROFILER_P_H

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

#include "qrhiprofiler_p.h"
#include <QElapsedTimer>
#include <QHash>

QT_BEGIN_NAMESPACE

class QRhiProfilerPrivate
{
public:
    static QRhiProfilerPrivate *get(QRhiProfiler *p) { return p->d; }

    void newBuffer(QRhiBuffer *buf, quint32 realSize, int backingGpuBufCount, int backingCpuBufCount);
    void releaseBuffer(QRhiBuffer *buf);
    void newBufferStagingArea(QRhiBuffer *buf, int slot, quint32 size);
    void releaseBufferStagingArea(QRhiBuffer *buf, int slot);

    void newRenderBuffer(QRhiRenderBuffer *rb, bool transientBacking, bool winSysBacking, int sampleCount);
    void releaseRenderBuffer(QRhiRenderBuffer *rb);

    void newTexture(QRhiTexture *tex, bool owns, int mipCount, int layerCount, int sampleCount);
    void releaseTexture(QRhiTexture *tex);
    void newTextureStagingArea(QRhiTexture *tex, int slot, quint32 size);
    void releaseTextureStagingArea(QRhiTexture *tex, int slot);

    void resizeSwapChain(QRhiSwapChain *sc, int bufferCount, int msaaBufferCount, int sampleCount);
    void releaseSwapChain(QRhiSwapChain *sc);

    void beginSwapChainFrame(QRhiSwapChain *sc);
    void endSwapChainFrame(QRhiSwapChain *sc, int frameCount);
    void swapChainFrameGpuTime(QRhiSwapChain *sc, float gpuTimeMs);

    void newReadbackBuffer(qint64 id, QRhiResource *src, quint32 size);
    void releaseReadbackBuffer(qint64 id);

    void vmemStat(uint realAllocCount, uint subAllocCount, quint32 totalSize, quint32 unusedSize);

    void startEntry(QRhiProfiler::StreamOp op, qint64 timestamp, QRhiResource *res);
    void writeInt(const char *key, qint64 v);
    void writeFloat(const char *key, float f);
    void endEntry();

    QRhiImplementation *rhiDWhenEnabled = nullptr;
    QIODevice *outputDevice = nullptr;
    QElapsedTimer ts;
    QByteArray buf;
    static const int DEFAULT_FRAME_TIMING_WRITE_INTERVAL = 120; // frames
    int frameTimingWriteInterval = DEFAULT_FRAME_TIMING_WRITE_INTERVAL;
    struct Sc {
        Sc() {
            frameToFrameSamples.reserve(DEFAULT_FRAME_TIMING_WRITE_INTERVAL);
            beginToEndSamples.reserve(DEFAULT_FRAME_TIMING_WRITE_INTERVAL);
            gpuFrameSamples.reserve(DEFAULT_FRAME_TIMING_WRITE_INTERVAL);
        }
        QElapsedTimer frameToFrameTimer;
        bool frameToFrameRunning = false;
        QElapsedTimer beginToEndTimer;
        QList<qint64> frameToFrameSamples;
        QList<qint64> beginToEndSamples;
        QList<float> gpuFrameSamples;
        QRhiProfiler::CpuTime frameToFrameTime;
        QRhiProfiler::CpuTime beginToEndFrameTime;
        QRhiProfiler::GpuTime gpuFrameTime;
    };
    QHash<QRhiSwapChain *, Sc> swapchains;
};

Q_DECLARE_TYPEINFO(QRhiProfilerPrivate::Sc, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif
