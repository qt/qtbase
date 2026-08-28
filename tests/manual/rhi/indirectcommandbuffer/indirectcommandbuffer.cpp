// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#define EXAMPLEFW_PREINIT
#define EXAMPLEFW_IMGUI
#include "../shared/examplefw.h"

// Exercises QRhiIndirectCommandBuffer both ways: with the commands recorded on
// the CPU and in a compute shader on the GPU. The two are interchangeable at
// runtime from the GUI, and must produce exactly the same image.
//
// The scene is OBJECT_COUNT small objects spread over a sphere, cycling through
// a triangle, a quad and a cube. Every object has its own baked vertex and
// index range, so each indirect command has a different indexCount, firstIndex
// and vertexOffset - which is what multi-draw indirect exists for.
//
// Which objects are drawn is decided by hashing the object index against a
// density slider. The same hash runs on the CPU and in the compute shader, so
// switching between the two paths is a visual no-op. With the count buffer
// enabled the surviving count is only known to the device, and commandCount()
// is then just an upper bound - the GUI shows this, together with the actual
// count read back from the count buffer.

static const quint32 OBJECT_COUNT = 1200;

struct Vertex {
    float pos[3];
    float color[4];
};

struct Object {
    quint32 indexCount;
    quint32 firstIndex;
    qint32 vertexOffset;
};

struct {
    QList<QRhiResource *> releasePool;

    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ibuf = nullptr;
    QRhiBuffer *ubuf = nullptr;

    QRhiShaderResourceBindings *srb = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;

    QRhiIndirectCommandBuffer *icb = nullptr;

    // The GPU-driven path.
    QRhiBuffer *srcCmdBuf = nullptr;
    QRhiBuffer *gpuCmdBuf = nullptr;
    QRhiBuffer *countBuf = nullptr;
    QRhiBuffer *paramsBuf = nullptr;
    QRhiShaderResourceBindings *computeSrb = nullptr;
    QRhiComputePipeline *computePs = nullptr;

    QList<QRhiIndexedIndirectDrawCommand> cmds; // one per object, in order

    QRhiResourceUpdateBatch *initialUpdates = nullptr;

    bool computeSupported = false;
    bool countSupported = false;
    bool readbackSupported = false;

    // GUI state.
    int mode = 0; // 0 = record on the CPU, 1 = build on the GPU
    bool useCountBuffer = false;
    int density = 1000;
    bool rotate = true;

    int recordedForDensity = -1;
    int countedForDensity = -1;
    quint32 visibleOnCpu = 0;

    // The device-side draw count, pulled out of the count buffer so that the
    // GUI can show the number executeIndirect() ends up using.
    QRhiReadbackResult countReadback;
    bool countReadbackPending = false;
    bool haveDeviceCount = false;
    quint32 deviceCount = 0;

    float rotation = 0.0f;
    int frame = 0;
} d;

static bool visible(quint32 i, quint32 density)
{
    // Must match buildcmds.comp.
    quint32 h = i * 2654435761u;
    h ^= h >> 15;
    return (h % 1000u) < density;
}

static void appendTriangle(QList<Vertex> *v, QList<quint16> *idx, const QVector3D &c,
                           float s, const QColor &col)
{
    const float r = float(col.redF()), g = float(col.greenF()), b = float(col.blueF());
    const float p[3][2] = { { 0.0f, 0.62f }, { -0.55f, -0.35f }, { 0.55f, -0.35f } };
    for (const auto &xy : p)
        *v << Vertex { { c.x() + xy[0] * s, c.y() + xy[1] * s, c.z() }, { r, g, b, 1.0f } };
    *idx << 0 << 1 << 2;
}

static void appendQuad(QList<Vertex> *v, QList<quint16> *idx, const QVector3D &c,
                       float s, const QColor &col, float z = 0.0f)
{
    const float r = float(col.redF()), g = float(col.greenF()), b = float(col.blueF());
    const float p[4][2] = { { -0.45f, -0.45f }, { 0.45f, -0.45f }, { 0.45f, 0.45f }, { -0.45f, 0.45f } };
    for (const auto &xy : p)
        *v << Vertex { { c.x() + xy[0] * s, c.y() + xy[1] * s, c.z() + z }, { r, g, b, 1.0f } };
    *idx << 0 << 1 << 2 << 2 << 3 << 0;
}

static void appendCube(QList<Vertex> *v, QList<quint16> *idx, const QVector3D &c,
                       float s, const QColor &col)
{
    // normal, tangent, bitangent per face; the normal only picks the shade here.
    static const float basis[6][9] = {
        {  0, 0, 1,   1, 0, 0,   0, 1, 0 },
        {  0, 0,-1,  -1, 0, 0,   0, 1, 0 },
        {  1, 0, 0,   0, 0,-1,   0, 1, 0 },
        { -1, 0, 0,   0, 0, 1,   0, 1, 0 },
        {  0, 1, 0,   1, 0, 0,   0, 0,-1 },
        {  0,-1, 0,   1, 0, 0,   0, 0, 1 },
    };
    const float h = 0.42f * s;
    for (int f = 0; f < 6; ++f) {
        const QVector3D n(basis[f][0], basis[f][1], basis[f][2]);
        const QVector3D t(basis[f][3], basis[f][4], basis[f][5]);
        const QVector3D bt(basis[f][6], basis[f][7], basis[f][8]);
        // No lighting in the shader, so bake a per-face factor to give the
        // cubes some definition.
        const float shade = 0.55f + 0.45f * float(QVector3D::dotProduct(n, QVector3D(0.3f, 0.75f, 0.59f)) * 0.5f + 0.5f);
        const float r = float(col.redF()) * shade;
        const float g = float(col.greenF()) * shade;
        const float b = float(col.blueF()) * shade;
        for (int corner = 0; corner < 4; ++corner) {
            const float u = (corner == 0 || corner == 3) ? -1.0f : 1.0f;
            const float w = (corner < 2) ? -1.0f : 1.0f;
            const QVector3D p = c + n * h + t * (u * h) + bt * (w * h);
            *v << Vertex { { p.x(), p.y(), p.z() }, { r, g, b, 1.0f } };
        }
        const quint16 base = quint16(f * 4);
        *idx << base << quint16(base + 1) << quint16(base + 2)
             << quint16(base + 2) << quint16(base + 3) << base;
    }
}

// Bakes every object into one vertex and one index buffer. Indices stay
// object-local, so 16 bits are plenty and vertexOffset picks the object.
static void buildScene(QList<Vertex> *verts, QList<quint16> *indices, QList<QRhiIndexedIndirectDrawCommand> *cmds)
{
    for (quint32 i = 0; i < OBJECT_COUNT; ++i) {
        // Golden-angle spiral, which spreads the objects evenly over a sphere.
        const float y = 1.0f - 2.0f * (float(i) + 0.5f) / float(OBJECT_COUNT);
        const float r = std::sqrt(qMax(0.0f, 1.0f - y * y));
        const float theta = float(i) * 2.399963f;
        const QVector3D center(9.0f * r * std::cos(theta), 9.0f * y, 9.0f * r * std::sin(theta));
        const QColor color = QColor::fromHsvF(std::fmod(float(i) * 0.017f, 1.0f), 0.7f, 0.95f);

        QList<Vertex> v;
        QList<quint16> idx;
        switch (i % 3) {
        case 0:
            appendTriangle(&v, &idx, center, 0.8f, color);
            break;
        case 1:
            appendQuad(&v, &idx, center, 0.8f, color);
            break;
        default:
            appendCube(&v, &idx, center, 0.8f, color);
            break;
        }

        QRhiIndexedIndirectDrawCommand cmd = {};
        cmd.indexCount = quint32(idx.count());
        cmd.instanceCount = 1;
        cmd.firstIndex = quint32(indices->count());
        cmd.vertexOffset = qint32(verts->count());
        cmd.firstInstance = 0;
        *cmds << cmd;

        *verts << v;
        *indices << idx;
    }
}

void preInit()
{
    sampleCount = 4; // MSAA swapchain
}

void Window::customInit()
{
    if (!m_r->isFeatureSupported(QRhi::DrawIndirect))
        qFatal("DrawIndirect is not supported");
    if (!m_r->isFeatureSupported(QRhi::BaseVertex))
        qFatal("BaseVertex is not supported");

    d.computeSupported = m_r->isFeatureSupported(QRhi::Compute);
    d.countSupported = m_r->isFeatureSupported(QRhi::DrawIndirectCount);
    d.readbackSupported = m_r->isFeatureSupported(QRhi::ReadBackNonUniformBuffer);

    d.countReadback.completed = []() {
        Q_ASSERT(d.countReadback.data.size() >= int(sizeof(quint32)));
        memcpy(&d.deviceCount, d.countReadback.data.constData(), sizeof(quint32));
        d.haveDeviceCount = true;
        d.countReadbackPending = false;
    };

    QList<Vertex> verts;
    QList<quint16> indices;
    buildScene(&verts, &indices, &d.cmds);

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                            quint32(verts.count() * sizeof(Vertex)));
    if (!d.vbuf->create())
        qFatal("failed to create d.vbuf");
    d.releasePool << d.vbuf;

    d.ibuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer,
                            quint32(indices.count() * sizeof(quint16)));
    if (!d.ibuf->create())
        qFatal("failed to create d.ibuf");
    d.releasePool << d.ibuf;

    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64);
    if (!d.ubuf->create())
        qFatal("failed to create d.ubuf");
    d.releasePool << d.ubuf;

    d.initialUpdates = m_r->nextResourceUpdateBatch();
    d.initialUpdates->uploadStaticBuffer(d.vbuf, verts.constData());
    d.initialUpdates->uploadStaticBuffer(d.ibuf, indices.constData());

    d.icb = m_r->newIndirectCommandBuffer(QRhiIndirectCommandBuffer::IndexedDraws, OBJECT_COUNT);
    if (!d.icb->create())
        qFatal("failed to create d.icb");
    d.releasePool << d.icb;

    d.srb = m_r->newShaderResourceBindings();
    d.srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, d.ubuf)
    });
    if (!d.srb->create())
        qFatal("failed to create d.srb");
    d.releasePool << d.srb;

    d.ps = m_r->newGraphicsPipeline();
    d.ps->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/quads.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/quads.frag.qsb")) }
    });
    QRhiVertexInputLayout vlayout;
    vlayout.setBindings({ { sizeof(Vertex) } });
    vlayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, offsetof(Vertex, pos) },
        { 0, 1, QRhiVertexInputAttribute::Float4, offsetof(Vertex, color) },
    });
    d.ps->setVertexInputLayout(vlayout);
    d.ps->setShaderResourceBindings(d.srb);
    d.ps->setRenderPassDescriptor(m_rp);
    d.ps->setSampleCount(sampleCount);
    d.ps->setDepthTest(true);
    d.ps->setDepthWrite(true);
    d.ps->setDepthOp(QRhiGraphicsPipeline::Less);
    // Without this the Metal backend cannot use a native indirect command
    // buffer and falls back to replaying the commands one by one.
    d.ps->setFlags(QRhiGraphicsPipeline::UsesIndirectDraws);
    if (!d.ps->create())
        qFatal("failed to create d.ps");
    d.releasePool << d.ps;

    if (d.computeSupported) {
        const quint32 cmdBufSize = OBJECT_COUNT * sizeof(QRhiIndexedIndirectDrawCommand);

        d.srcCmdBuf = m_r->newBuffer(QRhiBuffer::Static, QRhiBuffer::StorageBuffer, cmdBufSize);
        if (!d.srcCmdBuf->create())
            qFatal("failed to create d.srcCmdBuf");
        d.releasePool << d.srcCmdBuf;
        d.initialUpdates->uploadStaticBuffer(d.srcCmdBuf, d.cmds.constData());

        d.gpuCmdBuf = m_r->newBuffer(QRhiBuffer::Static,
                                     QRhiBuffer::StorageBuffer | QRhiBuffer::IndirectBuffer,
                                     cmdBufSize);
        if (!d.gpuCmdBuf->create())
            qFatal("failed to create d.gpuCmdBuf");
        d.releasePool << d.gpuCmdBuf;

        static constexpr quint32 zero = 0;
        d.countBuf = m_r->newBuffer(QRhiBuffer::Static,
                                    QRhiBuffer::StorageBuffer | QRhiBuffer::IndirectBuffer,
                                    sizeof(quint32));
        if (!d.countBuf->create())
            qFatal("failed to create d.countBuf");
        d.releasePool << d.countBuf;
        d.initialUpdates->uploadStaticBuffer(d.countBuf, &zero);

        d.paramsBuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 3 * sizeof(quint32));
        if (!d.paramsBuf->create())
            qFatal("failed to create d.paramsBuf");
        d.releasePool << d.paramsBuf;

        d.computeSrb = m_r->newShaderResourceBindings();
        d.computeSrb->setBindings({
            QRhiShaderResourceBinding::bufferLoadStore(0, QRhiShaderResourceBinding::ComputeStage, d.srcCmdBuf),
            QRhiShaderResourceBinding::bufferLoadStore(1, QRhiShaderResourceBinding::ComputeStage, d.gpuCmdBuf),
            QRhiShaderResourceBinding::bufferLoadStore(2, QRhiShaderResourceBinding::ComputeStage, d.countBuf),
            QRhiShaderResourceBinding::uniformBuffer(3, QRhiShaderResourceBinding::ComputeStage, d.paramsBuf)
        });
        if (!d.computeSrb->create())
            qFatal("failed to create d.computeSrb");
        d.releasePool << d.computeSrb;

        d.computePs = m_r->newComputePipeline();
        d.computePs->setShaderResourceBindings(d.computeSrb);
        d.computePs->setShaderStage({ QRhiShaderStage::Compute, getShader(QLatin1String(":/buildcmds.comp.qsb")) });
        if (!d.computePs->create())
            qFatal("failed to create d.computePs");
        d.releasePool << d.computePs;
    } else {
        qWarning("Compute is not supported, the buildIndirect() path is unavailable");
    }
}

void Window::customRelease()
{
    qDeleteAll(d.releasePool);
    d.releasePool.clear();
    d.cmds.clear();
}

void Window::customRender()
{
    const QSize outputSize = m_sc->currentPixelSize();
    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();

    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch();
    if (d.initialUpdates) {
        u->merge(d.initialUpdates);
        d.initialUpdates->release();
        d.initialUpdates = nullptr;
    }

    QMatrix4x4 mvp = m_r->clipSpaceCorrMatrix();
    mvp.perspective(45.0f, float(outputSize.width()) / float(outputSize.height()), 1.0f, 100.0f);
    mvp.translate(0.0f, 0.0f, -22.0f);
    mvp.rotate(d.rotation, 0.3f, 1.0f, 0.0f);
    u->updateDynamicBuffer(d.ubuf, 0, 64, mvp.constData());

    const bool gpuBuild = d.mode == 1 && d.computeSupported;
    const bool compact = gpuBuild && d.useCountBuffer && d.countSupported;

    if (!compact)
        d.haveDeviceCount = false;

    if (!gpuBuild && d.icb->isGpuBuilt()) {
        // buildIndirect() cannot be undone: an indirect command buffer that
        // gets its commands from the GPU keeps doing so, and clear() does not
        // change that. Going back to CPU-recorded commands therefore means
        // recreating the object.
        if (!d.icb->create())
            qFatal("failed to recreate d.icb");
        d.recordedForDensity = -1;
    }

    if (gpuBuild) {
        const quint32 params[3] = { OBJECT_COUNT, quint32(d.density), compact ? 1u : 0u };
        u->updateDynamicBuffer(d.paramsBuf, 0, sizeof(params), params);
        if (compact) {
            quint32 zero = 0;
            u->uploadStaticBuffer(d.countBuf, &zero);
        }

        cb->beginComputePass(u);
        cb->setComputePipeline(d.computePs);
        cb->setShaderResources();
        cb->dispatch(int((OBJECT_COUNT + 63) / 64), 1, 1);
        u = nullptr;

        if (compact && d.readbackSupported && !d.countReadbackPending) {
            // Just for the GUI: get the number the device came up with, so that
            // the exact draw count can be shown, not only the upper bound that
            // commandCount() is in this case.
            QRhiResourceUpdateBatch *rub = m_r->nextResourceUpdateBatch();
            rub->readBackBuffer(d.countBuf, 0, sizeof(quint32), &d.countReadback);
            d.countReadbackPending = true;
            cb->endComputePass(rub);
        } else {
            cb->endComputePass();
        }

        QRhiIndirectCommandBufferBuildInfo info;
        info.topology = d.ps->topology();
        info.sourceBuffer = d.gpuCmdBuf;
        // An upper bound when there is a count buffer, the exact number of
        // draws otherwise. Either way the source buffer's size says nothing.
        info.commandCount = OBJECT_COUNT;
        info.countBuffer = compact ? d.countBuf : nullptr;
        info.indexBuffer = d.ibuf;
        info.indexFormat = QRhiCommandBuffer::IndexUInt16;
        cb->buildIndirect(d.icb, info);

        d.recordedForDensity = -1;
    } else if (d.recordedForDensity != d.density) {
        // Re-record only when the visible set actually changed; the recorded
        // contents are otherwise reused as they are, frame after frame.
        d.icb->clear();
        for (quint32 i = 0; i < OBJECT_COUNT; ++i) {
            if (!visible(i, quint32(d.density)))
                continue;
            const QRhiIndexedIndirectDrawCommand &c(d.cmds.at(int(i)));
            d.icb->drawIndexed(c.indexCount, 1, c.firstIndex, c.vertexOffset);
        }
        u->commitIndirectCommandBuffer(d.icb);
        d.recordedForDensity = d.density;
    }

    if (d.countedForDensity != d.density) {
        d.visibleOnCpu = 0;
        for (quint32 i = 0; i < OBJECT_COUNT; ++i) {
            if (visible(i, quint32(d.density)))
                ++d.visibleOnCpu;
        }
        d.countedForDensity = d.density;
    }

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(d.ps);
    cb->setViewport({ 0, 0, float(outputSize.width()), float(outputSize.height()) });
    const QRhiCommandBuffer::VertexInput vbinding(d.vbuf, 0);
    cb->setVertexInput(0, 1, &vbinding, d.ibuf, 0, QRhiCommandBuffer::IndexUInt16);

    cb->setShaderResources(d.srb);
    cb->executeIndirect(d.icb);

    m_imguiRenderer->render();

    cb->endPass();

    if (d.rotate)
        d.rotation += 0.2f;
    ++d.frame;
}

void Window::customGui()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(630, 420), ImGuiCond_FirstUseEver);
    ImGui::Begin("QRhiIndirectCommandBuffer");

    ImGui::Text("How the indirect command buffer is populated");
    ImGui::RadioButton("Recorded on the CPU", &d.mode, 0);
    if (d.computeSupported) {
        ImGui::RadioButton("Built on the GPU (buildIndirect)", &d.mode, 1);
        if (d.mode == 1) {
            if (d.countSupported) {
                ImGui::Checkbox("Device-side count buffer", &d.useCountBuffer);
            } else {
                d.useCountBuffer = false;
                ImGui::Text("DrawIndirectCount is not supported");
            }
        }
    } else {
        d.mode = 0;
        ImGui::Text("Compute is not supported, buildIndirect() unavailable");
    }
    ImGui::Text("The image must not change when switching.");

    ImGui::Separator();
    ImGui::SliderInt("Density", &d.density, 0, 1000);
    ImGui::Checkbox("Rotate", &d.rotate);

    ImGui::Separator();
    ImGui::Text("maxCommandCount      %u", d.icb->maxCommandCount());
    ImGui::Text("isGpuBuilt           %s", d.icb->isGpuBuilt() ? "true" : "false");
    if (d.icb->isGpuBuilt()) {
        if (d.useCountBuffer && d.countSupported) {
            ImGui::Text("commandCount         %u (an upper bound, the count buffer decides)",
                        d.icb->commandCount());
            if (d.haveDeviceCount) {
                ImGui::Text("draws issued         %u (read back from the count buffer)",
                            d.deviceCount);
            } else if (d.readbackSupported) {
                ImGui::Text("draws issued         (waiting for the readback)");
            } else {
                ImGui::Text("draws issued         (unknown, buffer readback is not supported)");
            }
        } else {
            ImGui::Text("commandCount         %u (exact, the culled objects draw 0 indices)",
                        d.icb->commandCount());
        }
    } else {
        ImGui::Text("recordedCommandCount %u", d.icb->recordedCommandCount());
        ImGui::Text("commandCount         %u (exact, same as recordedCommandCount)",
                    d.icb->commandCount());
    }
    ImGui::Text("objects passing the density test on the CPU: %u", d.visibleOnCpu);

    ImGui::End();
}
