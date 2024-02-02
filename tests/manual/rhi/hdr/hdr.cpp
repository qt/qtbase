// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Test application for HDR with scRGB.
// Launch with the argument "scrgb" or "sdr", perhaps side-by-side even.

#define EXAMPLEFW_PREINIT
#define EXAMPLEFW_IMGUI
#include "../shared/examplefw.h"

#include "../shared/cube.h"

QByteArray loadHdr(const QString &fn, QSize *size)
{
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("Failed to open %s", qPrintable(fn));
        return QByteArray();
    }

    char sig[256];
    f.read(sig, 11);
    if (strncmp(sig, "#?RADIANCE\n", 11))
        return QByteArray();

    QByteArray buf = f.readAll();
    const char *p = buf.constData();
    const char *pEnd = p + buf.size();

    // Process lines until the empty one.
    QByteArray line;
    while (p < pEnd) {
        char c = *p++;
        if (c == '\n') {
            if (line.isEmpty())
                break;
            if (line.startsWith(QByteArrayLiteral("FORMAT="))) {
                const QByteArray format = line.mid(7).trimmed();
                if (format != QByteArrayLiteral("32-bit_rle_rgbe")) {
                    qWarning("HDR format '%s' is not supported", format.constData());
                    return QByteArray();
                }
            }
            line.clear();
        } else {
            line.append(c);
        }
    }
    if (p == pEnd) {
        qWarning("Malformed HDR image data at property strings");
        return QByteArray();
    }

    // Get the resolution string.
    while (p < pEnd) {
        char c = *p++;
        if (c == '\n')
            break;
        line.append(c);
    }
    if (p == pEnd) {
        qWarning("Malformed HDR image data at resolution string");
        return QByteArray();
    }

    int w = 0, h = 0;
    // We only care about the standard orientation.
    if (!sscanf(line.constData(), "-Y %d +X %d", &h, &w)) {
        qWarning("Unsupported HDR resolution string '%s'", line.constData());
        return QByteArray();
    }
    if (w <= 0 || h <= 0) {
        qWarning("Invalid HDR resolution");
        return QByteArray();
    }

    // output is RGBA32F
    const int blockSize = 4 * sizeof(float);
    QByteArray data;
    data.resize(w * h * blockSize);

    typedef unsigned char RGBE[4];
    RGBE *scanline = new RGBE[w];

    for (int y = 0; y < h; ++y) {
        if (pEnd - p < 4) {
            qWarning("Unexpected end of HDR data");
            delete[] scanline;
            return QByteArray();
        }

        scanline[0][0] = *p++;
        scanline[0][1] = *p++;
        scanline[0][2] = *p++;
        scanline[0][3] = *p++;

        if (scanline[0][0] == 2 && scanline[0][1] == 2 && scanline[0][2] < 128) {
            // new rle, the first pixel was a dummy
            for (int channel = 0; channel < 4; ++channel) {
                for (int x = 0; x < w && p < pEnd; ) {
                    unsigned char c = *p++;
                    if (c > 128) { // run
                        if (p < pEnd) {
                            int repCount = c & 127;
                            c = *p++;
                            while (repCount--)
                                scanline[x++][channel] = c;
                        }
                    } else { // not a run
                        while (c-- && p < pEnd)
                            scanline[x++][channel] = *p++;
                    }
                }
            }
        } else {
            // old rle
            scanline[0][0] = 2;
            int bitshift = 0;
            int x = 1;
            while (x < w && pEnd - p >= 4) {
                scanline[x][0] = *p++;
                scanline[x][1] = *p++;
                scanline[x][2] = *p++;
                scanline[x][3] = *p++;

                if (scanline[x][0] == 1 && scanline[x][1] == 1 && scanline[x][2] == 1) { // run
                    int repCount = scanline[x][3] << bitshift;
                    while (repCount--) {
                        memcpy(scanline[x], scanline[x - 1], 4);
                        ++x;
                    }
                    bitshift += 8;
                } else { // not a run
                    ++x;
                    bitshift = 0;
                }
            }
        }

        // adjust for -Y orientation
        float *fp = reinterpret_cast<float *>(data.data() + (h - 1 - y) * blockSize * w);
        for (int x = 0; x < w; ++x) {
            float d = qPow(2.0f, float(scanline[x][3]) - 128.0f);
            float r = scanline[x][0] / 256.0f * d;
            float g = scanline[x][1] / 256.0f * d;
            float b = scanline[x][2] / 256.0f * d;
            float a = 1.0f;
            *fp++ = r;
            *fp++ = g;
            *fp++ = b;
            *fp++ = a;
        }
    }

    delete[] scanline;

    *size = QSize(w, h);

    return data;
}

struct {
    QMatrix4x4 winProj;
    QList<QRhiResource *> releasePool;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiTexture *tex = nullptr;
    QRhiSampler *sampler = nullptr;
    QRhiShaderResourceBindings *srb = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;
    bool showDemoWindow = true;
    QVector3D rotation;

    bool usingHDRWindow;
    bool adjustSDR = false;
    float SDRWhiteLevelInNits = 200.0f;
    bool tonemapHDR = false;
    float tonemapInMax = 2.5f;
    float tonemapOutMax = 0.0f;

    QString imageFile;
} d;

void preInit()
{
    QStringList args = QCoreApplication::arguments();
    if (args.contains("scrgb")) {
        d.usingHDRWindow = true;
        swapchainFormat = QRhiSwapChain::HDRExtendedSrgbLinear;
    } else if (args.contains("p3")) {
        d.usingHDRWindow = true;
        swapchainFormat = QRhiSwapChain::HDRExtendedDisplayP3Linear;
    } else if (args.contains("sdr")) {
        d.usingHDRWindow = false;
        swapchainFormat = QRhiSwapChain::SDR;
    } else {
        qFatal("Missing command line argument, specify scrgb or sdr");
    }

    if (args.contains("file")) {
        d.imageFile = args[args.indexOf("file") + 1];
        qDebug("Using HDR image file %s", qPrintable(d.imageFile));
    } else {
        qFatal("Missing command line argument, specify 'file' followed by a .hdr file. "
               "Download for example the original .exr from https://viewer.openhdr.org/i/5fcb9a595812624a99d24c62/linear "
               "and use ImageMagick's 'convert' to convert from .exr to .hdr");
    }
}

void Window::customInit()
{
    if (!m_r->isTextureFormatSupported(QRhiTexture::RGBA32F))
        qWarning("RGBA32F texture format is not supported");

    d.initialUpdates = m_r->nextResourceUpdateBatch();

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube));
    d.vbuf->create();
    d.releasePool << d.vbuf;

    d.initialUpdates->uploadStaticBuffer(d.vbuf, cube);

    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 4 * 4);
    d.ubuf->create();
    d.releasePool << d.ubuf;

    qint32 flip = 1;
    d.initialUpdates->updateDynamicBuffer(d.ubuf, 64, 4, &flip);

    qint32 doLinearToSRGBInShader = !d.usingHDRWindow;
    d.initialUpdates->updateDynamicBuffer(d.ubuf, 68, 4, &doLinearToSRGBInShader);

    QSize size;
    QByteArray floatData = loadHdr(d.imageFile, &size);

    d.tex = m_r->newTexture(QRhiTexture::RGBA32F, size);
    d.releasePool << d.tex;
    d.tex->create();
    QRhiTextureUploadDescription desc({ 0, 0, { floatData.constData(), quint32(floatData.size()) } });
    d.initialUpdates->uploadTexture(d.tex, desc);

    d.sampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
    d.releasePool << d.sampler;
    d.sampler->create();

    d.srb = m_r->newShaderResourceBindings();
    d.releasePool << d.srb;
    d.srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.tex, d.sampler)
    });
    d.srb->create();

    d.ps = m_r->newGraphicsPipeline();
    d.releasePool << d.ps;
    d.ps->setCullMode(QRhiGraphicsPipeline::Back);
    const QRhiShaderStage stages[] = {
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/hdrtexture.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/hdrtexture.frag.qsb")) }
    };
    d.ps->setShaderStages(stages, stages + 2);
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 3 * sizeof(float) },
        { 2 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
        { 1, 1, QRhiVertexInputAttribute::Float2, 0 }
    });
    d.ps->setVertexInputLayout(inputLayout);
    d.ps->setShaderResourceBindings(d.srb);
    d.ps->setRenderPassDescriptor(m_rp);
    d.ps->create();
}

void Window::customRelease()
{
    qDeleteAll(d.releasePool);
    d.releasePool.clear();
}

void Window::customRender()
{
    if (d.tonemapOutMax == 0.0f) {
        QRhiSwapChainHdrInfo info = m_sc->hdrInfo();
        switch (info.limitsType) {
        case QRhiSwapChainHdrInfo::LuminanceInNits:
            d.tonemapOutMax = info.limits.luminanceInNits.maxLuminance / 80.0f;
            break;
        case QRhiSwapChainHdrInfo::ColorComponentValue:
            // because on macOS it changes dynamically when starting up, so retry in next frame if it's still just 1.0
            if (info.limits.colorComponentValue.maxColorComponentValue > 1.0f)
                d.tonemapOutMax = info.limits.colorComponentValue.maxColorComponentValue;
            break;
        }
    }

    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch();
    if (d.initialUpdates) {
        u->merge(d.initialUpdates);
        d.initialUpdates->release();
        d.initialUpdates = nullptr;
    }

    QMatrix4x4 mvp = m_proj;
    mvp.rotate(d.rotation.x(), 1, 0, 0);
    mvp.rotate(d.rotation.y(), 0, 1, 0);
    mvp.rotate(d.rotation.z(), 0, 0, 1);
    u->updateDynamicBuffer(d.ubuf, 0, 64, mvp.constData());

    if (d.usingHDRWindow && d.tonemapHDR) {
        u->updateDynamicBuffer(d.ubuf, 72, 4, &d.tonemapInMax);
        u->updateDynamicBuffer(d.ubuf, 76, 4, &d.tonemapOutMax);
    } else {
        float zero[2] = {};
        u->updateDynamicBuffer(d.ubuf, 72, 8, zero);
    }

    QColor clearColor = Qt::green; // sRGB
    if (d.usingHDRWindow && d.adjustSDR) {
        float sdrMultiplier = d.SDRWhiteLevelInNits / 80.0f; // scRGB 1.0 = 80 nits (and linear gamma)
        clearColor = QColor::fromRgbF(clearColor.redF() * sdrMultiplier,
                                      clearColor.greenF() * sdrMultiplier,
                                      clearColor.blueF() * sdrMultiplier,
                                      1.0f);
    }

    const QSize outputSizeInPixels = m_sc->currentPixelSize();
    cb->beginPass(m_sc->currentFrameRenderTarget(), clearColor, { 1.0f, 0 }, u);

    cb->setGraphicsPipeline(d.ps);
    cb->setViewport(QRhiViewport(0, 0, outputSizeInPixels.width(), outputSizeInPixels.height()));
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBindings[] = {
        { d.vbuf, 0 },
        { d.vbuf, quint32(36 * 3 * sizeof(float)) }
    };
    cb->setVertexInput(0, 2, vbufBindings);
    cb->draw(36);

    m_imguiRenderer->render();

    cb->endPass();
}

static void addTip(const char *s)
{
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(300);
        ImGui::TextUnformatted(s);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void Window::customGui()
{
    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::SetNextWindowPos(ImVec2(10, 420), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("HDR test");

    if (d.usingHDRWindow) {
        if (swapchainFormat == QRhiSwapChain::HDRExtendedDisplayP3Linear) {
            ImGui::Text("The window is now Extended Linear Display P3 + FP16 color buffer,\n"
                        "the ImGui UI and the green background are considered SDR content,\n"
                        "the cube is using a HDR texture.");
        } else {
            ImGui::Text("The window is now scRGB (Extended Linear sRGB) + FP16 color buffer,\n"
                        "the ImGui UI and the green background are considered SDR content,\n"
                        "the cube is using a HDR texture.");
        }
        ImGui::Checkbox("Adjust SDR content", &d.adjustSDR);
        addTip("Multiplies fragment colors for non-HDR content with sdr_white_level / 80. "
               "Not relevant with macOS (due to EDR being display-referred).");
        if (d.adjustSDR) {
            ImGui::SliderFloat("SDR white level (nits)", &d.SDRWhiteLevelInNits, 0.0f, 1000.0f);
            imguiHDRMultiplier = d.SDRWhiteLevelInNits / 80.0f; // scRGB 1.0 = 80 nits (and linear gamma)
        } else {
            imguiHDRMultiplier = 1.0f; // 0 would mean linear to sRGB; don't want that with HDR
        }
        ImGui::Separator();
        ImGui::Checkbox("Tonemap HDR content", &d.tonemapHDR);
        addTip("Perform some most basic Reinhard tonemapping (changed to suit HDR) on the 3D content (the cube). "
               "Display max luminance is set to the max color component value (macOS) or max luminance in nits / 80 (Windows) by default.");
        if (d.tonemapHDR) {
            ImGui::SliderFloat("Content max luminance\n(color component value)", &d.tonemapInMax, 0.0f, 20.0f);
            ImGui::SliderFloat("Display max luminance\n(color component value)", &d.tonemapOutMax, 0.0f, 20.0f);
        }
    } else {
        ImGui::Text("The window is standard dynamic range (no HDR, so non-linear sRGB effectively).\n"
                    "Here we just do linear -> sRGB for everything (UI, textured cube)\n"
                    "at the end of the pipeline, while the Qt::green background is already sRGB.");
    }

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(850, 560), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420, 140), ImGuiCond_FirstUseEver);
    ImGui::Begin("Misc");
    float *rot = reinterpret_cast<float *>(&d.rotation);
    ImGui::SliderFloat("Rotation X", &rot[0], 0.0f, 360.0f);
    ImGui::SliderFloat("Rotation Y", &rot[1], 0.0f, 360.0f);
    ImGui::SliderFloat("Rotation Z", &rot[2], 0.0f, 360.0f);
    ImGui::End();

    if (d.usingHDRWindow) {
        ImGui::SetNextWindowPos(ImVec2(850, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(420, 180), ImGuiCond_FirstUseEver);
        ImGui::Begin("Actual platform info");
        QRhiSwapChainHdrInfo info = m_sc->hdrInfo();
        switch (info.limitsType) {
        case QRhiSwapChainHdrInfo::LuminanceInNits:
            ImGui::Text("Platform provides luminance in nits");
            addTip("Windows/D3D: On laptops this will be screen brightness dependent. Increasing brightness implies the max luminance decreases. "
                   "It also seems to be affected by HDR Content Brightness in the Settings, if there is one (e.g. on laptops; not to be confused with SDR Content Brightess). "
                   "(note that the DXGI query does not seem to return changed values if there are runtime changes unless restarting the app).");
            ImGui::Text("Min luminance: %.4f\nMax luminance: %.4f",
                        info.limits.luminanceInNits.minLuminance,
                        info.limits.luminanceInNits.maxLuminance);
            break;
        case QRhiSwapChainHdrInfo::ColorComponentValue:
            ImGui::Text("Platform provides color component values");
            addTip("macOS/Metal: On laptops this will be screen brightness dependent. Increasing brightness decreases the max color value. "
                   "Max brightness may bring it down to 1.0.");
            ImGui::Text("maxColorComponentValue: %.4f\nmaxPotentialColorComponentValue: %.4f",
                        info.limits.colorComponentValue.maxColorComponentValue,
                        info.limits.colorComponentValue.maxPotentialColorComponentValue);
            break;
        }
        ImGui::Separator();
        switch (info.luminanceBehavior) {
        case QRhiSwapChainHdrInfo::SceneReferred:
            ImGui::Text("Luminance behavior is scene-referred");
            break;
        case QRhiSwapChainHdrInfo::DisplayReferred:
            ImGui::Text("Luminance behavior is display-referred");
            break;
        }
        addTip("Windows (DWM) HDR is scene-referred: 1.0 = 80 nits.\n\n"
               "Apple EDR is display-referred: the value of 1.0 refers to whatever the system's current SDR white level is (100, 200, ... nits depending on the brightness).");
        if (info.luminanceBehavior == QRhiSwapChainHdrInfo::SceneReferred) {
            ImGui::Text("SDR white level: %.4f nits", info.sdrWhiteLevel);
            addTip("On Windows this is queried from DISPLAYCONFIG_SDR_WHITE_LEVEL. "
                   "Affected by the slider in the Windows Settings (System/Display/HDR/[S|H]DR Content Brightness). "
                   "With max screen brightness (laptops) the value will likely be the same as the max luminance.");
        }
        ImGui::End();
    }
}
