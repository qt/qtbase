// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#define EXAMPLEFW_PREINIT
#define EXAMPLEFW_IMGUI
#include "../shared/examplefw.h"
#include "../shared/cube.h"
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>

// Uniform buffers vs push constants.
//
// The same scene is rendered in four ways that differ only in how two pieces
// of frequently changing data reach the vertex shader: the camera matrix,
// which changes every frame, and the per-object translation, rotation and
// color, which change every frame for every object.
//
//   1. camera in a uniform buffer, per-object data in a uniform buffer
//      (one Dynamic QRhiBuffer and one QRhiShaderResourceBindings per object)
//   2. camera in push constants, per-object data in uniform buffers
//   3. camera in a uniform buffer, per-object data in push constants
//   4. camera and per-object data in push constants
//
// The image must not change when switching, the animation state is advanced
// independently of the mode. What changes is the CPU time spent updating and
// recording, and the GPU time.
//
// Note that with OpenGL and D3D11 push constants are emulated (plain uniforms
// and a small dynamic constant buffer, respectively), so there the push
// constant modes are expected to be roughly on par with the uniform buffer
// ones. The difference is expected to show with Vulkan, D3D12 and Metal.
//
// The CPU time reported for customRender() covers the buffer updates and
// recording into the QRhi command buffer. With OpenGL, Vulkan and D3D11 the
// actual API calls are made later, in endFrame(), which is only covered by the
// frame interval. D3D12 records into the native command list directly, so
// there customRender() includes that work.
//
// VSync stays on, like in the other manual tests. The frame interval and fps
// are therefore capped by the refresh rate; the comparison rests on the
// customRender() CPU time and the GPU time. For an unthrottled run set
// scFlags |= QRhiSwapChain::NoVSync in preInit(), as triquadcube does with
// its NO_VSYNC define.

const int MAX_OBJECTS = 25000;
const int HISTORY = 120;

// The per-object data: the same 32 bytes go into the uniform buffer at
// binding 1 in modes 1 and 2, into the push constant block in mode 3, and to
// offset 64 of the push constant block in mode 4. Only vec4s, so std140 and
// std430 agree and the block is expressible in HLSL as well.
struct ObjectData {
    float translation[4]; // xyz = position, w = rotation angle around Y
    float color[4];
};
static_assert(sizeof(ObjectData) == 32);

// Mode 4's push constant block.
struct FullPushConstants {
    float camera[16];
    ObjectData object;
};
static_assert(sizeof(FullPushConstants) == 96);

// The plot is live, but the numbers would be unreadable if they changed on
// every frame, so those are accumulated and refreshed every DISPLAY_INTERVAL.
const qint64 DISPLAY_INTERVAL_NS = 500 * 1000 * 1000;

struct Series {
    float v[HISTORY] = {};
    int head = 0;
    int count = 0;
    float accSum = 0.0f;
    float accMax = 0.0f;
    int accCount = 0;
    float shownAvg = 0.0f;
    float shownMax = 0.0f;
    void push(float x) {
        v[head] = x;
        head = (head + 1) % HISTORY;
        count = qMin(count + 1, HISTORY);
        accSum += x;
        accMax = qMax(accMax, x);
        accCount += 1;
    }
    void refreshShown() {
        if (accCount) {
            shownAvg = accSum / accCount;
            shownMax = accMax;
        }
        accSum = 0.0f;
        accMax = 0.0f;
        accCount = 0;
    }
    float plotMax() const { return *std::max_element(v, v + HISTORY); }
};

struct {
    QList<QRhiResource *> releasePool;

    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *camUbuf = nullptr;
    QRhiShaderResourceBindings *camSrb = nullptr;
    QRhiShaderResourceBindings *emptySrb = nullptr;
    QRhiGraphicsPipeline *psUbo = nullptr;
    QRhiGraphicsPipeline *psCamPc = nullptr;
    QRhiGraphicsPipeline *psObjPc = nullptr;
    QRhiGraphicsPipeline *psPc = nullptr;

    // Modes 1 and 2: one uniform buffer and one srb per object. Grows on
    // demand, never shrinks.
    QList<QRhiBuffer *> objUbufs;
    QList<QRhiShaderResourceBindings *> objSrbs;

    QRhiResourceUpdateBatch *initialUpdates = nullptr;

    ObjectData objData[MAX_OBJECTS];
    float rotSpeed[MAX_OBJECTS];
    float rotPhase[MAX_OBJECTS];
    float camRot = 0.0f;
    float t = 0.0f;
    QMatrix4x4 camera;

    int mode = 0;
    int objectCount = 10000;
    bool pcSupported = false;
    bool fullPcSupported = false;
    int maxPcSize = 0;
    bool timestampsSupported = false;

    QElapsedTimer clock;
    qint64 lastRenderStartNs = -1;
    qint64 lastDisplayRefreshNs = 0;
    Series recordMs;
    Series frameMs;
    Series gpuMs;
} d;

void preInit()
{
    debugLayer = false;
}

static void ensureObjectResources(QRhi *rhi, int count)
{
    while (d.objUbufs.count() < count) {
        QRhiBuffer *ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(ObjectData));
        ubuf->create();
        QRhiShaderResourceBindings *srb = rhi->newShaderResourceBindings();
        srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, d.camUbuf),
            QRhiShaderResourceBinding::uniformBuffer(1, QRhiShaderResourceBinding::VertexStage, ubuf)
        });
        srb->create();
        d.objUbufs.append(ubuf);
        d.objSrbs.append(srb);
    }
}

void Window::customInit()
{
    d.pcSupported = m_r->isFeatureSupported(QRhi::PushConstants);
    d.maxPcSize = m_r->resourceLimit(QRhi::MaxPushConstantsSize);
    d.fullPcSupported = d.pcSupported && d.maxPcSize >= int(sizeof(FullPushConstants));
    d.timestampsSupported = m_r->isFeatureSupported(QRhi::Timestamps);
    qDebug("PushConstants %s, MaxPushConstantsSize is %d bytes",
           d.pcSupported ? "supported" : "not supported", d.maxPcSize);

    d.initialUpdates = m_r->nextResourceUpdateBatch();

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube));
    d.vbuf->create();
    d.releasePool << d.vbuf;
    d.initialUpdates->uploadStaticBuffer(d.vbuf, cube);

    d.camUbuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64);
    d.camUbuf->create();
    d.releasePool << d.camUbuf;

    d.camSrb = m_r->newShaderResourceBindings();
    d.camSrb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, d.camUbuf)
    });
    d.camSrb->create();
    d.releasePool << d.camSrb;

    d.emptySrb = m_r->newShaderResourceBindings();
    d.emptySrb->create();
    d.releasePool << d.emptySrb;

    ensureObjectResources(m_r, d.objectCount);

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 3 * sizeof(float) },
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
    });

    const QShader fs = getShader(QLatin1String(":/material.frag.qsb"));
    auto createPipeline = [&](const char *vertShader, QRhiShaderResourceBindings *srb) {
        QRhiGraphicsPipeline *ps = m_r->newGraphicsPipeline();
        d.releasePool << ps;
        ps->setDepthTest(true);
        ps->setDepthWrite(true);
        ps->setShaderStages({
            { QRhiShaderStage::Vertex, getShader(QLatin1String(vertShader)) },
            { QRhiShaderStage::Fragment, fs }
        });
        ps->setVertexInputLayout(inputLayout);
        ps->setShaderResourceBindings(srb);
        ps->setRenderPassDescriptor(m_rp);
        ps->create();
        return ps;
    };

    d.psUbo = createPipeline(":/ubo.vert.qsb", d.objSrbs[0]);
    if (d.pcSupported) {
        d.psCamPc = createPipeline(":/campc.vert.qsb", d.objSrbs[0]);
        d.psObjPc = createPipeline(":/objpc.vert.qsb", d.camSrb);
        if (d.fullPcSupported)
            d.psPc = createPipeline(":/pc.vert.qsb", d.emptySrb);
    }

    // Fixed seed, so that runs are comparable.
    QRandomGenerator rgen(1234u);
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        d.objData[i].translation[0] = rgen.bounded(8000) / 100.0f - 40.0f;
        d.objData[i].translation[1] = rgen.bounded(8000) / 100.0f - 40.0f;
        d.objData[i].translation[2] = rgen.bounded(100) / -4.0f;
        d.objData[i].translation[3] = 0.0f;
        d.objData[i].color[0] = i / float(MAX_OBJECTS);
        d.objData[i].color[1] = 0.3f + 0.7f * rgen.bounded(1000) / 1000.0f;
        d.objData[i].color[2] = rgen.bounded(1000) / 1000.0f;
        d.objData[i].color[3] = 1.0f;
        d.rotSpeed[i] = 0.5f + 2.0f * rgen.bounded(1000) / 1000.0f;
        d.rotPhase[i] = 6.2831853f * rgen.bounded(1000) / 1000.0f;
    }

    d.clock.start();
}

void Window::customRelease()
{
    qDeleteAll(d.objSrbs);
    d.objSrbs.clear();
    qDeleteAll(d.objUbufs);
    d.objUbufs.clear();
    qDeleteAll(d.releasePool);
    d.releasePool.clear();
}

void Window::customRender()
{
    const QSize outputSizeInPixels = m_sc->currentPixelSize();
    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();

    const qint64 nowNs = d.clock.nsecsElapsed();
    if (d.lastRenderStartNs >= 0)
        d.frameMs.push((nowNs - d.lastRenderStartNs) / 1.0e6f);
    d.lastRenderStartNs = nowNs;
    if (d.timestampsSupported) {
        // 0 for the first frames and after a resize, may refer to a frame
        // from a few frames ago.
        const double gpuSec = cb->lastCompletedGpuTime();
        if (gpuSec > 0.0)
            d.gpuMs.push(float(gpuSec * 1000.0));
    }

    if (!d.pcSupported)
        d.mode = 0;
    else if (d.mode == 3 && !d.fullPcSupported)
        d.mode = 2;

    const bool cameraInUbuf = d.mode == 0 || d.mode == 2;
    const bool objectsInUbuf = d.mode == 0 || d.mode == 1;

    // Growing the pool is a one-off cost, keep it out of the measurement.
    if (objectsInUbuf)
        ensureObjectResources(m_r, d.objectCount);

    const qint64 recordStartNs = d.clock.nsecsElapsed();

    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch();
    if (d.initialUpdates) {
        u->merge(d.initialUpdates);
        d.initialUpdates->release();
        d.initialUpdates = nullptr;
    }

    // The animation is the same regardless of the mode.
    d.camera = m_proj;
    d.camera.rotate(d.camRot, 0, 1, 0);
    d.camera.scale(0.05f);
    for (int i = 0; i < d.objectCount; ++i)
        d.objData[i].translation[3] = std::fmod(d.t * d.rotSpeed[i] + d.rotPhase[i], 6.2831853f);

    if (cameraInUbuf)
        u->updateDynamicBuffer(d.camUbuf, 0, 64, d.camera.constData());

    if (objectsInUbuf) {
        for (int i = 0; i < d.objectCount; ++i) {
            char *p = d.objUbufs[i]->beginFullDynamicBufferUpdateForCurrentFrame();
            memcpy(p, &d.objData[i], sizeof(ObjectData));
            d.objUbufs[i]->endFullDynamicBufferUpdateForCurrentFrame();
        }
    }

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u);
    const QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, 0);
    const QRhiViewport viewport(0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()));

    switch (d.mode) {
    case 0:
        cb->setGraphicsPipeline(d.psUbo);
        cb->setViewport(viewport);
        cb->setVertexInput(0, 1, &vbufBinding);
        for (int i = 0; i < d.objectCount; ++i) {
            cb->setShaderResources(d.objSrbs[i]);
            cb->draw(36);
        }
        break;
    case 1:
        cb->setGraphicsPipeline(d.psCamPc);
        cb->setViewport(viewport);
        cb->setVertexInput(0, 1, &vbufBinding);
        // Once per frame; changing the srb below does not disturb it, only a
        // pipeline change would.
        cb->setPushConstants(0, 64, d.camera.constData());
        for (int i = 0; i < d.objectCount; ++i) {
            cb->setShaderResources(d.objSrbs[i]);
            cb->draw(36);
        }
        break;
    case 2:
        cb->setGraphicsPipeline(d.psObjPc);
        cb->setViewport(viewport);
        cb->setShaderResources();
        cb->setVertexInput(0, 1, &vbufBinding);
        for (int i = 0; i < d.objectCount; ++i) {
            cb->setPushConstants(0, sizeof(ObjectData), &d.objData[i]);
            cb->draw(36);
        }
        break;
    case 3:
        cb->setGraphicsPipeline(d.psPc);
        cb->setViewport(viewport);
        cb->setShaderResources();
        cb->setVertexInput(0, 1, &vbufBinding);
        // The camera once, then only the per-object part of the block.
        cb->setPushConstants(0, 64, d.camera.constData());
        for (int i = 0; i < d.objectCount; ++i) {
            cb->setPushConstants(64, sizeof(ObjectData), &d.objData[i]);
            cb->draw(36);
        }
        break;
    }

    d.recordMs.push((d.clock.nsecsElapsed() - recordStartNs) / 1.0e6f);

    m_imguiRenderer->render();
    cb->endPass();

    d.camRot += 0.1f;
    d.t += 0.01f;
}

static void plotSeries(const char *label, const Series &s, const char *suffix = "")
{
    ImGui::Text("%s", label);
    char overlay[96];
    snprintf(overlay, sizeof(overlay), "avg %.3f ms, max %.3f ms%s", s.shownAvg, s.shownMax, suffix);
    ImGui::PushID(label);
    // PlotLines does not accept a negative width the way most widgets do, so
    // the available width has to be passed explicitly.
    ImGui::PlotLines("", s.v, HISTORY, s.head, overlay, 0.0f, qMax(s.plotMax(), 0.001f),
                     ImVec2(ImGui::GetContentRegionAvail().x, 40));
    ImGui::PopID();
}

void Window::customGui()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    // Fixed width, height follows the content.
    ImGui::SetNextWindowSizeConstraints(ImVec2(560, 0), ImVec2(560, FLT_MAX));
    ImGui::Begin("Push constants vs uniform buffers", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    const qint64 nowNs = d.clock.nsecsElapsed();
    if (nowNs - d.lastDisplayRefreshNs >= DISPLAY_INTERVAL_NS) {
        d.lastDisplayRefreshNs = nowNs;
        d.recordMs.refreshShown();
        d.frameMs.refreshShown();
        d.gpuMs.refreshShown();
    }

    if (d.pcSupported)
        ImGui::Text("%s, push constants: max %d bytes", qPrintable(graphicsApiName()), d.maxPcSize);
    else
        ImGui::Text("%s, push constants: not supported", qPrintable(graphicsApiName()));
    ImGui::Text("Debug/validation layer: disabled by this test");
    ImGui::Separator();

    ImGui::Text("UBO = uniform buffer, PC = push constants");
    ImGui::RadioButton("Camera: UBO, objects: UBO", &d.mode, 0);
    ImGui::BeginDisabled(!d.pcSupported);
    ImGui::RadioButton("Camera: PC, objects: UBO", &d.mode, 1);
    ImGui::RadioButton("Camera: UBO, objects: PC", &d.mode, 2);
    ImGui::EndDisabled();
    ImGui::BeginDisabled(!d.fullPcSupported);
    ImGui::RadioButton("Camera: PC, objects: PC", &d.mode, 3);
    ImGui::EndDisabled();
    ImGui::Text("The image must not change when switching.");
    ImGui::Separator();

    ImGui::SliderInt("Objects", &d.objectCount, 100, MAX_OBJECTS);
    ImGui::Text("Draws: %d, buffers: %d", d.objectCount, int(d.objUbufs.count()));
    ImGui::SameLine();
    if (ImGui::Button("Reset history")) {
        d.recordMs = Series();
        d.frameMs = Series();
        d.gpuMs = Series();
    }
    ImGui::Separator();

    plotSeries("customRender CPU (updates + recording)", d.recordMs);
    char fps[32];
    snprintf(fps, sizeof(fps), ", %.0f fps", d.frameMs.shownAvg > 0.0f ? 1000.0f / d.frameMs.shownAvg : 0.0f);
    plotSeries("Frame interval (vsync-capped, incl. endFrame)", d.frameMs, fps);
    if (d.timestampsSupported)
        plotSeries("GPU frame time (lastCompletedGpuTime)", d.gpuMs);
    else
        ImGui::Text("GPU frame time: QRhi::Timestamps is not supported");

    ImGui::End();
}
