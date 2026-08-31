// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../shared/examplefw.h"
#include <QRandomGenerator>

// Buffer-to-buffer copy example. A compute shader writes to a storage buffer,
// the contents of which are then copied, on the GPU, into a separate vertex
// buffer with copyBuffer(). That vertex buffer is what the graphics pass
// consumes. This is the same particle movement as in the computebuffer
// example, except that there the one and same buffer has both StorageBuffer
// and VertexBuffer usage.
//
// Combining the two usages, like computebuffer does, is perfectly valid and is
// often the better choice, so the split here is for demonstration purposes.
// Real uses of copyBuffer() are cases where the single buffer approach is not
// an option:
//
// - The buffer holding the compute results is not ours. It comes from another
//   component, library, or buffer pool, and was created with StorageBuffer
//   usage only, or was imported from a native buffer. Its usage flags cannot be
//   changed retroactively, so getting the data into a vertex (or index) buffer
//   we own needs a copy.
//
// - A snapshot, or per-frame versions of the data, are needed. The compute pass
//   keeps updating its working buffer, while the graphics pass wants the
//   contents as of one particular frame. Note also that a QRhiBuffer with
//   StorageBuffer usage has just one native buffer with all backends, whereas a
//   Static or Immutable buffer without that usage may be backed by multiple,
//   one per frame in flight. Copying into such a buffer is therefore also a way
//   to get frame-slotted data out of a compute pass.
//
// - Only a part of the data is needed. The compute shader generates one large
//   structure, and the draw call consumes a slice of it, with a smaller,
//   differently sized buffer. sourceOffset(), destinationOffset(), and size()
//   in QRhiBufferCopyDescription are meant exactly for this.
//
// There can be performance considerations as well, in both directions. Marking
// a buffer as shader-writable (which is what StorageBuffer usage implies) may
// affect where and how the buffer is allocated, and a buffer with both usages
// has to transition between shader-write and vertex-read state in every frame.
// Copying to a plain vertex buffer avoids that, at the cost of the copy itself.
// Which one wins is highly dependent on the 3D API, GPU, and the amount of
// data, so this is not a reason on its own to prefer one over the other.

// Note that the example relies on gl_PointSize which is not supported
// everywhere. So in some cases the points will be of size 1.

struct {
    QList<QRhiResource *> releasePool;
    QRhiBuffer *sbuf = nullptr;
    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *computeUniBuf = nullptr;
    QRhiShaderResourceBindings *computeBindings = nullptr;
    QRhiComputePipeline *computePipeline = nullptr;
    QRhiShaderResourceBindings *graphicsBindings = nullptr;
    QRhiGraphicsPipeline *graphicsPipeline = nullptr;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    float step = 0.2f;
} d;

// these struct must match the std140 packing rules
struct Data {
    float pos[2];
    float dir;
    quint32 pad[1];
};
struct ComputeUBuf {
    float step;
    quint32 count;
};

const int DATA_COUNT = 256 * 128;

const int COMPUTE_UBUF_SIZE = 8;

void Window::customInit()
{
    if (!m_r->isFeatureSupported(QRhi::Compute))
        qFatal("Compute is not supported");

    if (!m_r->isFeatureSupported(QRhi::BufferToBufferCopy))
        qFatal("Buffer-to-buffer copy is not supported");

    if (!m_r->isFeatureSupported(QRhi::VertexShaderPointSize))
        qWarning("Point sizes other than 1 not supported");

    // compute pass

    d.sbuf = m_r->newBuffer(QRhiBuffer::Static, QRhiBuffer::StorageBuffer, sizeof(Data) * DATA_COUNT);
    d.sbuf->create();
    d.releasePool << d.sbuf;

    // The destination of the copy. Note that it has no StorageBuffer usage:
    // the compute shader never sees this buffer, the data gets here purely via
    // copyBuffer().
    d.vbuf = m_r->newBuffer(QRhiBuffer::Static, QRhiBuffer::VertexBuffer, sizeof(Data) * DATA_COUNT);
    d.vbuf->create();
    d.releasePool << d.vbuf;

    d.computeUniBuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, COMPUTE_UBUF_SIZE);
    d.computeUniBuf->create();
    d.releasePool << d.computeUniBuf;

    d.initialUpdates = m_r->nextResourceUpdateBatch();

    QByteArray data;
    data.resize(sizeof(Data) * DATA_COUNT);
    Data *p = reinterpret_cast<Data *>(data.data());
    QRandomGenerator *rgen = QRandomGenerator::global();
    for (int i = 0; i < DATA_COUNT; ++i) {
        p->pos[0] = rgen->bounded(1000) / 500.0f - 1.0f;
        p->pos[1] = rgen->bounded(1000) / 500.0f - 1.0f;
        p->dir = rgen->bounded(2) ? 1 : -1;
        ++p;
    }
    d.initialUpdates->uploadStaticBuffer(d.sbuf, data.constData());

    ComputeUBuf ud;
    ud.step = d.step;
    ud.count = DATA_COUNT;
    d.initialUpdates->updateDynamicBuffer(d.computeUniBuf, 0, COMPUTE_UBUF_SIZE, &ud);

    d.computeBindings = m_r->newShaderResourceBindings();
    d.computeBindings->setBindings({
                                       QRhiShaderResourceBinding::bufferLoadStore(0, QRhiShaderResourceBinding::ComputeStage, d.sbuf),
                                       QRhiShaderResourceBinding::uniformBuffer(1, QRhiShaderResourceBinding::ComputeStage, d.computeUniBuf)
                                   });
    d.computeBindings->create();
    d.releasePool << d.computeBindings;

    d.computePipeline = m_r->newComputePipeline();
    d.computePipeline->setShaderResourceBindings(d.computeBindings);
    d.computePipeline->setShaderStage({ QRhiShaderStage::Compute, getShader(QLatin1String(":/buffer.comp.qsb")) });
    d.computePipeline->create();
    d.releasePool << d.computePipeline;

    // graphics pass

    d.graphicsBindings = m_r->newShaderResourceBindings();
    d.graphicsBindings->create();
    d.releasePool << d.graphicsBindings;

    d.graphicsPipeline = m_r->newGraphicsPipeline();
    d.graphicsPipeline->setTopology(QRhiGraphicsPipeline::Points);
    d.graphicsPipeline->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/main.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/main.frag.qsb")) }
    });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { sizeof(Data) } // the stride is the full struct, only pos is used as vertex input
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
    });
    d.graphicsPipeline->setVertexInputLayout(inputLayout);
    d.graphicsPipeline->setShaderResourceBindings(d.graphicsBindings);
    d.graphicsPipeline->setRenderPassDescriptor(m_rp);
    d.graphicsPipeline->create();
    d.releasePool << d.graphicsPipeline;
}

void Window::customRelease()
{
    qDeleteAll(d.releasePool);
    d.releasePool.clear();
}

void Window::customRender()
{
    const QSize outputSizeInPixels = m_sc->currentPixelSize();
    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch();
    if (d.initialUpdates) {
        u->merge(d.initialUpdates);
        d.initialUpdates->release();
        d.initialUpdates = nullptr;
    }

    // compute pass
    cb->beginComputePass(u);
    cb->setComputePipeline(d.computePipeline);
    cb->setShaderResources();
    cb->dispatch(DATA_COUNT / 256, 1, 1);

    // Get the compute results into the vertex buffer. Passing the batch to
    // endComputePass() ensures the copy is recorded after the dispatch.
    QRhiResourceUpdateBatch *copyBatch = m_r->nextResourceUpdateBatch();
    copyBatch->copyBuffer(d.vbuf, d.sbuf);
    cb->endComputePass(copyBatch);

    // graphics pass
    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 });
    cb->setGraphicsPipeline(d.graphicsPipeline);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(DATA_COUNT);
    cb->endPass();
}
