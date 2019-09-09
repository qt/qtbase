/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Gui module
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qrhiprofiler_p_p.h"
#include "qrhi_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QRhiProfiler
    \internal
    \inmodule QtGui

    \brief Collects resource and timing information from an active QRhi.

    A QRhiProfiler is present for each QRhi. Query it via QRhi::profiler(). The
    profiler is active only when the QRhi was created with
    QRhi::EnableProfiling. No data is collected otherwise.

    \note GPU timings are only available when QRhi indicates that
    QRhi::Timestamps is supported.

    Besides collecting data from the QRhi implementations, some additional
    values are calculated. For example, for textures and similar resources the
    profiler gives an estimate of the complete amount of memory the resource
    needs.

    \section2 Output Format

    The output is comma-separated text. Each line has a number of
    comma-separated entries and each line ends with a comma.

    For example:

    \badcode
        1,0,140446057946208,Triangle vbuf,type,0,usage,1,logical_size,84,effective_size,84,backing_gpu_buf_count,1,backing_cpu_buf_count,0,
        1,0,140446057947376,Triangle ubuf,type,2,usage,4,logical_size,68,effective_size,256,backing_gpu_buf_count,2,backing_cpu_buf_count,0,
        1,1,140446057950416,,type,0,usage,1,logical_size,112,effective_size,112,backing_gpu_buf_count,1,backing_cpu_buf_count,0,
        1,1,140446057950544,,type,0,usage,2,logical_size,12,effective_size,12,backing_gpu_buf_count,1,backing_cpu_buf_count,0,
        1,1,140446057947440,,type,2,usage,4,logical_size,68,effective_size,256,backing_gpu_buf_count,2,backing_cpu_buf_count,0,
        1,1,140446057984784,Cube vbuf (textured),type,0,usage,1,logical_size,720,effective_size,720,backing_gpu_buf_count,1,backing_cpu_buf_count,0,
        1,1,140446057982528,Cube ubuf (textured),type,2,usage,4,logical_size,68,effective_size,256,backing_gpu_buf_count,2,backing_cpu_buf_count,0,
        7,8,140446058913648,Qt texture,width,256,height,256,format,1,owns_native_resource,1,mip_count,9,layer_count,1,effective_sample_count,1,approx_byte_size,349524,
        1,8,140446058795856,Cube vbuf (textured with offscreen),type,0,usage,1,logical_size,720,effective_size,720,backing_gpu_buf_count,1,backing_cpu_buf_count,0,
        1,8,140446058947920,Cube ubuf (textured with offscreen),type,2,usage,4,logical_size,68,effective_size,256,backing_gpu_buf_count,2,backing_cpu_buf_count,0,
        7,8,140446058794928,Texture for offscreen content,width,512,height,512,format,1,owns_native_resource,1,mip_count,1,layer_count,1,effective_sample_count,1,approx_byte_size,1048576,
        1,8,140446058963904,Triangle vbuf,type,0,usage,1,logical_size,84,effective_size,84,backing_gpu_buf_count,1,backing_cpu_buf_count,0,
        1,8,140446058964560,Triangle ubuf,type,2,usage,4,logical_size,68,effective_size,256,backing_gpu_buf_count,2,backing_cpu_buf_count,0,
        5,9,140446057945392,,type,0,width,1280,height,720,effective_sample_count,1,transient_backing,0,winsys_backing,0,approx_byte_size,3686400,
        11,9,140446057944592,,width,1280,height,720,buffer_count,2,msaa_buffer_count,0,effective_sample_count,1,approx_total_byte_size,7372800,
        9,9,140446058913648,Qt texture,slot,0,size,262144,
        10,9,140446058913648,Qt texture,slot,0,
        17,2019,140446057944592,,frames_since_resize,121,min_ms_frame_delta,9,max_ms_frame_delta,33,Favg_ms_frame_delta,16.1167,
        18,2019,140446057944592,,frames_since_resize,121,min_ms_frame_build,0,max_ms_frame_build,1,Favg_ms_frame_build,0.00833333,
        17,4019,140446057944592,,frames_since_resize,241,min_ms_frame_delta,15,max_ms_frame_delta,17,Favg_ms_frame_delta,16.0583,
        18,4019,140446057944592,,frames_since_resize,241,min_ms_frame_build,0,max_ms_frame_build,0,Favg_ms_frame_build,0,
        12,5070,140446057944592,,
        2,5079,140446057947376,Triangle ubuf,
        2,5079,140446057946208,Triangle vbuf,
        2,5079,140446057947440,,
        2,5079,140446057950544,,
        2,5079,140446057950416,,
        8,5079,140446058913648,Qt texture,
        2,5079,140446057982528,Cube ubuf (textured),
        2,5079,140446057984784,Cube vbuf (textured),
        2,5079,140446058964560,Triangle ubuf,
        2,5079,140446058963904,Triangle vbuf,
        8,5079,140446058794928,Texture for offscreen content,
        2,5079,140446058947920,Cube ubuf (textured with offscreen),
        2,5079,140446058795856,Cube vbuf (textured with offscreen),
        6,5079,140446057945392,,
    \endcode

    Each line starts with \c op, \c timestamp, \c res, \c name where op is a
    value from StreamOp, timestamp is a recording timestamp in milliseconds
    (qint64), res is a number (quint64) referring to the QRhiResource the entry
    refers to, or 0 if not applicable. \c name is the value of
    QRhiResource::name() and may be empty as well. The \c name will never
    contain a comma.

    This is followed by any number of \c{key, value} pairs where \c key is an
    unspecified string and \c value is a number. If \c key starts with \c F, it
    indicates the value is a float. Otherwise assume that the value is a
    qint64.
 */

/*!
    \enum QRhiProfiler::StreamOp
    Describes an entry in the profiler's output stream.

    \value NewBuffer A buffer is created
    \value ReleaseBuffer A buffer is destroyed
    \value NewBufferStagingArea A staging buffer for buffer upload is created
    \value ReleaseBufferStagingArea A staging buffer for buffer upload is destroyed
    \value NewRenderBuffer A renderbuffer is created
    \value ReleaseRenderBuffer A renderbuffer is destroyed
    \value NewTexture A texture is created
    \value ReleaseTexture A texture is destroyed
    \value NewTextureStagingArea A staging buffer for texture upload is created
    \value ReleaseTextureStagingArea A staging buffer for texture upload is destroyed
    \value ResizeSwapChain A swapchain is created or resized
    \value ReleaseSwapChain A swapchain is destroyed
    \value NewReadbackBuffer A staging buffer for readback is created
    \value ReleaseReadbackBuffer A staging buffer for readback is destroyed
    \value GpuMemAllocStats GPU memory allocator statistics
    \value GpuFrameTime GPU frame times
    \value FrameToFrameTime CPU frame-to-frame times
    \value FrameBuildTime CPU beginFrame-endFrame times
 */

/*!
    \class QRhiProfiler::CpuTime
    \internal
    \inmodule QtGui
    \brief Contains CPU-side frame timings.

    Once sufficient number of frames have been rendered, the minimum, maximum,
    and average values (in milliseconds) from various measurements are made
    available in this struct queriable from QRhiProfiler::frameToFrameTimes()
    and QRhiProfiler::frameBuildTimes().

    \sa QRhiProfiler::setFrameTimingWriteInterval()
 */

/*!
    \class QRhiProfiler::GpuTime
    \internal
    \inmodule QtGui
    \brief Contains GPU-side frame timings.

    Once sufficient number of frames have been rendered, the minimum, maximum,
    and average values (in milliseconds) calculated from GPU command buffer
    timestamps are made available in this struct queriable from
    QRhiProfiler::gpuFrameTimes().

    \sa QRhiProfiler::setFrameTimingWriteInterval()
 */

/*!
    \internal
 */
QRhiProfiler::QRhiProfiler()
    : d(new QRhiProfilerPrivate)
{
    d->ts.start();
}

/*!
    Destructor.
 */
QRhiProfiler::~QRhiProfiler()
{
    // Flush because there is a high chance we have writes that were made since
    // the event loop last ran. (esp. relevant for network devices like QTcpSocket)
    if (d->outputDevice)
        d->outputDevice->waitForBytesWritten(1000);

    delete d;
}

/*!
    Sets the output \a device.

    \note No output will be generated when QRhi::EnableProfiling was not set.
 */
void QRhiProfiler::setDevice(QIODevice *device)
{
    d->outputDevice = device;
}

/*!
    Requests writing a GpuMemAllocStats entry into the output, when applicable.
    Backends that do not support this will ignore the request. This is an
    explicit request since getting the allocator status and statistics may be
    an expensive operation.
 */
void QRhiProfiler::addVMemAllocatorStats()
{
    if (d->rhiDWhenEnabled)
        d->rhiDWhenEnabled->sendVMemStatsToProfiler();
}

/*!
    \return the currently set frame timing writeout interval.
 */
int QRhiProfiler::frameTimingWriteInterval() const
{
    return d->frameTimingWriteInterval;
}

/*!
    Sets the number of frames that need to be rendered before the collected CPU
    and GPU timings are processed (min, max, average are calculated) to \a
    frameCount.

    The default value is 120.
 */
void QRhiProfiler::setFrameTimingWriteInterval(int frameCount)
{
    if (frameCount > 0)
        d->frameTimingWriteInterval = frameCount;
}

/*!
   \return min, max, and avg in milliseconds for the time that elapsed between two
   QRhi::endFrame() calls.

   \note The values are all 0 until at least frameTimingWriteInterval() frames
   have been rendered.
 */
QRhiProfiler::CpuTime QRhiProfiler::frameToFrameTimes(QRhiSwapChain *sc) const
{
    auto it = d->swapchains.constFind(sc);
    if (it != d->swapchains.constEnd())
        return it->frameToFrameTime;

    return QRhiProfiler::CpuTime();
}

/*!
   \return min, max, and avg in milliseconds for the time that elapsed between
   a QRhi::beginFrame() and QRhi::endFrame().

   \note The values are all 0 until at least frameTimingWriteInterval() frames
   have been rendered.
 */
QRhiProfiler::CpuTime QRhiProfiler::frameBuildTimes(QRhiSwapChain *sc) const
{
    auto it = d->swapchains.constFind(sc);
    if (it != d->swapchains.constEnd())
        return it->beginToEndFrameTime;

    return QRhiProfiler::CpuTime();
}

/*!
   \return min, max, and avg in milliseconds for the GPU time that is spent on
   one frame.

   \note The values are all 0 until at least frameTimingWriteInterval() frames
   have been rendered.

   The GPU times should only be compared between runs on the same GPU of the
   same system with the same backend. Comparing times for different graphics
   cards or for different backends can give misleading results. The numbers are
   not meant to be comparable that way.

   \note Some backends have no support for this, and even for those that have,
   it is not guaranteed that the driver will support it at run time. Support
   can be checked via QRhi::Timestamps.
 */
QRhiProfiler::GpuTime QRhiProfiler::gpuFrameTimes(QRhiSwapChain *sc) const
{
    auto it = d->swapchains.constFind(sc);
    if (it != d->swapchains.constEnd())
        return it->gpuFrameTime;

    return QRhiProfiler::GpuTime();
}

void QRhiProfilerPrivate::startEntry(QRhiProfiler::StreamOp op, qint64 timestamp, QRhiResource *res)
{
    buf.clear();
    buf.append(QByteArray::number(op));
    buf.append(',');
    buf.append(QByteArray::number(timestamp));
    buf.append(',');
    buf.append(QByteArray::number(quint64(quintptr(res))));
    buf.append(',');
    if (res)
        buf.append(res->name());
    buf.append(',');
}

void QRhiProfilerPrivate::writeInt(const char *key, qint64 v)
{
    Q_ASSERT(key[0] != 'F');
    buf.append(key);
    buf.append(',');
    buf.append(QByteArray::number(v));
    buf.append(',');
}

void QRhiProfilerPrivate::writeFloat(const char *key, float f)
{
    Q_ASSERT(key[0] == 'F');
    buf.append(key);
    buf.append(',');
    buf.append(QByteArray::number(double(f)));
    buf.append(',');
}

void QRhiProfilerPrivate::endEntry()
{
    buf.append('\n');
    outputDevice->write(buf);
}

void QRhiProfilerPrivate::newBuffer(QRhiBuffer *buf, quint32 realSize, int backingGpuBufCount, int backingCpuBufCount)
{
    if (!outputDevice)
        return;

    startEntry(QRhiProfiler::NewBuffer, ts.elapsed(), buf);
    writeInt("type", buf->type());
    writeInt("usage", buf->usage());
    writeInt("logical_size", buf->size());
    writeInt("effective_size", realSize);
    writeInt("backing_gpu_buf_count", backingGpuBufCount);
    writeInt("backing_cpu_buf_count", backingCpuBufCount);
    endEntry();
}

void QRhiProfilerPrivate::releaseBuffer(QRhiBuffer *buf)
{
    if (!outputDevice)
        return;

    startEntry(QRhiProfiler::ReleaseBuffer, ts.elapsed(), buf);
    endEntry();
}

void QRhiProfilerPrivate::newBufferStagingArea(QRhiBuffer *buf, int slot, quint32 size)
{
    if (!outputDevice)
        return;

    startEntry(QRhiProfiler::NewBufferStagingArea, ts.elapsed(), buf);
    writeInt("slot", slot);
    writeInt("size", size);
    endEntry();
}

void QRhiProfilerPrivate::releaseBufferStagingArea(QRhiBuffer *buf, int slot)
{
    if (!outputDevice)
        return;

    startEntry(QRhiProfiler::ReleaseBufferStagingArea, ts.elapsed(), buf);
    writeInt("slot", slot);
    endEntry();
}

void QRhiProfilerPrivate::newRenderBuffer(QRhiRenderBuffer *rb, bool transientBacking, bool winSysBacking, int sampleCount)
{
    if (!outputDevice)
        return;

    const QRhiRenderBuffer::Type type = rb->type();
    const QSize sz = rb->pixelSize();
    // just make up something, ds is likely D24S8 while color is RGBA8 or similar
    const QRhiTexture::Format assumedFormat = type == QRhiRenderBuffer::DepthStencil ? QRhiTexture::D32F : QRhiTexture::RGBA8;
    quint32 byteSize = rhiDWhenEnabled->approxByteSizeForTexture(assumedFormat, sz, 1, 1);
    if (sampleCount > 1)
        byteSize *= uint(sampleCount);

    startEntry(QRhiProfiler::NewRenderBuffer, ts.elapsed(), rb);
    writeInt("type", type);
    writeInt("width", sz.width());
    writeInt("height", sz.height());
    writeInt("effective_sample_count", sampleCount);
    writeInt("transient_backing", transientBacking);
    writeInt("winsys_backing", winSysBacking);
    writeInt("approx_byte_size", byteSize);
    endEntry();
}

void QRhiProfilerPrivate::releaseRenderBuffer(QRhiRenderBuffer *rb)
{
    if (!outputDevice)
        return;

    startEntry(QRhiProfiler::ReleaseRenderBuffer, ts.elapsed(), rb);
    endEntry();
}

void QRhiProfilerPrivate::newTexture(QRhiTexture *tex, bool owns, int mipCount, int layerCount, int sampleCount)
{
    if (!outputDevice)
        return;

    const QRhiTexture::Format format = tex->format();
    const QSize sz = tex->pixelSize();
    quint32 byteSize = rhiDWhenEnabled->approxByteSizeForTexture(format, sz, mipCount, layerCount);
    if (sampleCount > 1)
        byteSize *= uint(sampleCount);

    startEntry(QRhiProfiler::NewTexture, ts.elapsed(), tex);
    writeInt("width", sz.width());
    writeInt("height", sz.height());
    writeInt("format", format);
    writeInt("owns_native_resource", owns);
    writeInt("mip_count", mipCount);
    writeInt("layer_count", layerCount);
    writeInt("effective_sample_count", sampleCount);
    writeInt("approx_byte_size", byteSize);
    endEntry();
}

void QRhiProfilerPrivate::releaseTexture(QRhiTexture *tex)
{
    if (!outputDevice)
        return;

    startEntry(QRhiProfiler::ReleaseTexture, ts.elapsed(), tex);
    endEntry();
}

void QRhiProfilerPrivate::newTextureStagingArea(QRhiTexture *tex, int slot, quint32 size)
{
    if (!outputDevice)
        return;

    startEntry(QRhiProfiler::NewTextureStagingArea, ts.elapsed(), tex);
    writeInt("slot", slot);
    writeInt("size", size);
    endEntry();
}

void QRhiProfilerPrivate::releaseTextureStagingArea(QRhiTexture *tex, int slot)
{
    if (!outputDevice)
        return;

    startEntry(QRhiProfiler::ReleaseTextureStagingArea, ts.elapsed(), tex);
    writeInt("slot", slot);
    endEntry();
}

void QRhiProfilerPrivate::resizeSwapChain(QRhiSwapChain *sc, int bufferCount, int msaaBufferCount, int sampleCount)
{
    if (!outputDevice)
        return;

    const QSize sz = sc->currentPixelSize();
    quint32 byteSize = rhiDWhenEnabled->approxByteSizeForTexture(QRhiTexture::BGRA8, sz, 1, 1);
    byteSize = byteSize * uint(bufferCount) + byteSize * uint(msaaBufferCount) * uint(sampleCount);

    startEntry(QRhiProfiler::ResizeSwapChain, ts.elapsed(), sc);
    writeInt("width", sz.width());
    writeInt("height", sz.height());
    writeInt("buffer_count", bufferCount);
    writeInt("msaa_buffer_count", msaaBufferCount);
    writeInt("effective_sample_count", sampleCount);
    writeInt("approx_total_byte_size", byteSize);
    endEntry();
}

void QRhiProfilerPrivate::releaseSwapChain(QRhiSwapChain *sc)
{
    if (!outputDevice)
        return;

    startEntry(QRhiProfiler::ReleaseSwapChain, ts.elapsed(), sc);
    endEntry();
}

template<typename T>
void calcTiming(QVector<T> *vec, T *minDelta, T *maxDelta, float *avgDelta)
{
    if (vec->isEmpty())
        return;

    *minDelta = *maxDelta = 0;
    float totalDelta = 0;
    for (T delta : qAsConst(*vec)) {
        totalDelta += float(delta);
        if (*minDelta == 0 || delta < *minDelta)
            *minDelta = delta;
        if (*maxDelta == 0 || delta > *maxDelta)
            *maxDelta = delta;
    }
    *avgDelta = totalDelta / vec->count();

    vec->clear();
}

void QRhiProfilerPrivate::beginSwapChainFrame(QRhiSwapChain *sc)
{
    Sc &scd(swapchains[sc]);
    scd.beginToEndTimer.start();
}

void QRhiProfilerPrivate::endSwapChainFrame(QRhiSwapChain *sc, int frameCount)
{
    Sc &scd(swapchains[sc]);
    if (!scd.frameToFrameRunning) {
        scd.frameToFrameTimer.start();
        scd.frameToFrameRunning = true;
        return;
    }

    scd.frameToFrameSamples.append(scd.frameToFrameTimer.restart());
    if (scd.frameToFrameSamples.count() >= frameTimingWriteInterval) {
        calcTiming(&scd.frameToFrameSamples,
                   &scd.frameToFrameTime.minTime, &scd.frameToFrameTime.maxTime, &scd.frameToFrameTime.avgTime);
        if (outputDevice) {
            startEntry(QRhiProfiler::FrameToFrameTime, ts.elapsed(), sc);
            writeInt("frames_since_resize", frameCount);
            writeInt("min_ms_frame_delta", scd.frameToFrameTime.minTime);
            writeInt("max_ms_frame_delta", scd.frameToFrameTime.maxTime);
            writeFloat("Favg_ms_frame_delta", scd.frameToFrameTime.avgTime);
            endEntry();
        }
    }

    scd.beginToEndSamples.append(scd.beginToEndTimer.elapsed());
    if (scd.beginToEndSamples.count() >= frameTimingWriteInterval) {
        calcTiming(&scd.beginToEndSamples,
                   &scd.beginToEndFrameTime.minTime, &scd.beginToEndFrameTime.maxTime, &scd.beginToEndFrameTime.avgTime);
        if (outputDevice) {
            startEntry(QRhiProfiler::FrameBuildTime, ts.elapsed(), sc);
            writeInt("frames_since_resize", frameCount);
            writeInt("min_ms_frame_build", scd.beginToEndFrameTime.minTime);
            writeInt("max_ms_frame_build", scd.beginToEndFrameTime.maxTime);
            writeFloat("Favg_ms_frame_build", scd.beginToEndFrameTime.avgTime);
            endEntry();
        }
    }
}

void QRhiProfilerPrivate::swapChainFrameGpuTime(QRhiSwapChain *sc, float gpuTime)
{
    Sc &scd(swapchains[sc]);
    scd.gpuFrameSamples.append(gpuTime);
    if (scd.gpuFrameSamples.count() >= frameTimingWriteInterval) {
        calcTiming(&scd.gpuFrameSamples,
                   &scd.gpuFrameTime.minTime, &scd.gpuFrameTime.maxTime, &scd.gpuFrameTime.avgTime);
        if (outputDevice) {
            startEntry(QRhiProfiler::GpuFrameTime, ts.elapsed(), sc);
            writeFloat("Fmin_ms_gpu_frame_time", scd.gpuFrameTime.minTime);
            writeFloat("Fmax_ms_gpu_frame_time", scd.gpuFrameTime.maxTime);
            writeFloat("Favg_ms_gpu_frame_time", scd.gpuFrameTime.avgTime);
            endEntry();
        }
    }
}

void QRhiProfilerPrivate::newReadbackBuffer(qint64 id, QRhiResource *src, quint32 size)
{
    if (!outputDevice)
        return;

    startEntry(QRhiProfiler::NewReadbackBuffer, ts.elapsed(), src);
    writeInt("id", id);
    writeInt("size", size);
    endEntry();
}

void QRhiProfilerPrivate::releaseReadbackBuffer(qint64 id)
{
    if (!outputDevice)
        return;

    startEntry(QRhiProfiler::ReleaseReadbackBuffer, ts.elapsed(), nullptr);
    writeInt("id", id);
    endEntry();
}

void QRhiProfilerPrivate::vmemStat(uint realAllocCount, uint subAllocCount, quint32 totalSize, quint32 unusedSize)
{
    if (!outputDevice)
        return;

    startEntry(QRhiProfiler::GpuMemAllocStats, ts.elapsed(), nullptr);
    writeInt("real_alloc_count", realAllocCount);
    writeInt("sub_alloc_count", subAllocCount);
    writeInt("total_size", totalSize);
    writeInt("unused_size", unusedSize);
    endEntry();
}

QT_END_NAMESPACE
