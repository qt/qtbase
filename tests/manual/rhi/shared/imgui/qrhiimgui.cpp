// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "qrhiimgui_p.h"
#include <QtCore/qfile.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qevent.h>
#include <QtGui/qclipboard.h>
#include <QtGui/qimage.h>

#include "imgui.h"

// the imgui default
static_assert(sizeof(ImDrawVert) == 20);
// switched to uint in imconfig.h to avoid trouble with 4 byte offset alignment reqs
static_assert(sizeof(ImDrawIdx) == 4);

QT_BEGIN_NAMESPACE

static QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

QRhiImguiRenderer::~QRhiImguiRenderer()
{
    releaseResources();
}

void QRhiImguiRenderer::releaseResources()
{
    for (Texture &t : m_textures) {
        delete t.tex;
        delete t.srb;
    }
    m_textures.clear();

    m_vbuf.reset();
    m_ibuf.reset();
    m_ubuf.reset();
    m_ps.reset();
    m_sampler.reset();

    m_rhi = nullptr;
}

void QRhiImguiRenderer::prepare(QRhi *rhi, QRhiRenderTarget *rt, QRhiCommandBuffer *cb, const QMatrix4x4 &mvp, float opacity)
{
    if (!m_rhi) {
        m_rhi = rhi;
    } else if (m_rhi != rhi) {
        releaseResources();
        m_rhi = rhi;
    }

    if (!m_rhi || f.draw.isEmpty())
        return;

    m_rt = rt;
    m_cb = cb;

    if (!m_vbuf) {
        m_vbuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, f.totalVbufSize));
        m_vbuf->setName(QByteArrayLiteral("imgui vertex buffer"));
        if (!m_vbuf->create())
            return;
    } else {
        if (f.totalVbufSize > m_vbuf->size()) {
            m_vbuf->setSize(f.totalVbufSize);
            if (!m_vbuf->create())
                return;
        }
    }
    if (!m_ibuf) {
        m_ibuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::IndexBuffer, f.totalIbufSize));
        m_ibuf->setName(QByteArrayLiteral("imgui index buffer"));
        if (!m_ibuf->create())
            return;
    } else {
        if (f.totalIbufSize > m_ibuf->size()) {
            m_ibuf->setSize(f.totalIbufSize);
            if (!m_ibuf->create())
                return;
        }
    }

    if (!m_ubuf) {
        m_ubuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 4));
        m_ubuf->setName(QByteArrayLiteral("imgui uniform buffer"));
        if (!m_ubuf->create())
            return;
    }

    if (!m_sampler) {
        m_sampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                          QRhiSampler::Repeat, QRhiSampler::Repeat));
        m_sampler->setName(QByteArrayLiteral("imgui sampler"));
        if (!m_sampler->create())
            return;
    }

    if (m_textures.isEmpty()) {
        Texture fontTex;
        fontTex.image = sf.fontTextureData;
        m_textures.append(fontTex);
    } else if (!sf.fontTextureData.isNull()) {
        Texture fontTex;
        fontTex.image = sf.fontTextureData;
        delete m_textures[0].tex;
        delete m_textures[0].srb;
        m_textures[0] = fontTex;
    }

    QVarLengthArray<int, 8> texturesNeedUpdate;
    for (int i = 0; i < m_textures.count(); ++i) {
        Texture &t(m_textures[i]);
        if (!t.tex) {
            t.tex = m_rhi->newTexture(QRhiTexture::RGBA8, t.image.size());
            t.tex->setName(QByteArrayLiteral("imgui texture ") + QByteArray::number(i));
            if (!t.tex->create())
                return;
            texturesNeedUpdate.append(i);
        }
        if (!t.srb) {
            t.srb = m_rhi->newShaderResourceBindings();
            t.srb->setBindings({
                QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_ubuf.get()),
                QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, t.tex, m_sampler.get())
            });
            if (!t.srb->create())
                return;
        }
    }

    // If layer.enabled is toggled on the item or an ancestor, the render
    // target is then suddenly different and may not be compatible.
    if (m_ps && m_rt->renderPassDescriptor()->serializedFormat() != m_renderPassFormat)
        m_ps.reset();

    if (!m_ps) {
        QShader vs = getShader(QLatin1String(":/imgui.vert.qsb"));
        QShader fs = getShader(QLatin1String(":/imgui.frag.qsb"));
        if (!vs.isValid() || !fs.isValid()) {
            qWarning("Failed to load imgui shaders");
            return;
        }

        m_ps.reset(m_rhi->newGraphicsPipeline());
        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        // Premultiplied alpha (matches imgui.frag). Would not be needed if we
        // only cared about outputting to the window (the common case), but
        // once going through a texture (Item layer, ShaderEffect) which is
        // then sampled by Quick, the result wouldn't be correct otherwise.
        blend.srcColor = QRhiGraphicsPipeline::One;
        blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        blend.srcAlpha = QRhiGraphicsPipeline::One;
        blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        m_ps->setTargetBlends({ blend });
        m_ps->setCullMode(QRhiGraphicsPipeline::None);
        m_ps->setDepthTest(true);
        m_ps->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);
        m_ps->setDepthWrite(false);
        m_ps->setFlags(QRhiGraphicsPipeline::UsesScissor);

        m_ps->setShaderStages({
            { QRhiShaderStage::Vertex, vs },
            { QRhiShaderStage::Fragment, fs }
        });

        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({
            { 4 * sizeof(float) + sizeof(quint32) }
        });
        inputLayout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
            { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) },
            { 0, 2, QRhiVertexInputAttribute::UNormByte4, 4 * sizeof(float) }
        });

        m_ps->setVertexInputLayout(inputLayout);
        m_ps->setShaderResourceBindings(m_textures[0].srb);
        m_ps->setRenderPassDescriptor(m_rt->renderPassDescriptor());
        m_renderPassFormat = m_rt->renderPassDescriptor()->serializedFormat();

        if (!m_ps->create())
            return;
    }

    QRhiResourceUpdateBatch *u = m_rhi->nextResourceUpdateBatch();

    for (const CmdListBuffer &b : f.vbuf)
        u->updateDynamicBuffer(m_vbuf.get(), b.offset, b.data.size(), b.data.constData());

    for (const CmdListBuffer &b : f.ibuf)
        u->updateDynamicBuffer(m_ibuf.get(), b.offset, b.data.size(), b.data.constData());

    u->updateDynamicBuffer(m_ubuf.get(), 0, 64, mvp.constData());
    u->updateDynamicBuffer(m_ubuf.get(), 64, 4, &opacity);

    for (int i = 0; i < texturesNeedUpdate.count(); ++i) {
        Texture &t(m_textures[texturesNeedUpdate[i]]);
        u->uploadTexture(t.tex, t.image);
        t.image = QImage();
    }

    m_cb->resourceUpdate(u);
}

void QRhiImguiRenderer::render()
{
    if (!m_rhi || f.draw.isEmpty() || !m_ps)
        return;

    m_cb->setGraphicsPipeline(m_ps.get());

    const QSize viewportSize = m_rt->pixelSize();
    bool needsViewport = true;

    for (const DrawCmd &c : f.draw) {
        QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf.get(), f.vbuf[c.cmdListBufferIdx].offset);
        if (needsViewport) {
            needsViewport = false;
            m_cb->setViewport({ 0, 0, float(viewportSize.width()), float(viewportSize.height()) });
        }
        const float sx1 = c.clipRect.x() + c.itemPixelOffset.x();
        const float sy1 = c.clipRect.y() + c.itemPixelOffset.y();
        const float sx2 = c.clipRect.z() + c.itemPixelOffset.x();
        const float sy2 = c.clipRect.w() + c.itemPixelOffset.y();
        QPoint scissorPos = QPointF(sx1, viewportSize.height() - sy2).toPoint();
        QSize scissorSize = QSizeF(sx2 - sx1, sy2 - sy1).toSize();
        scissorPos.setX(qMax(0, scissorPos.x()));
        scissorPos.setY(qMax(0, scissorPos.y()));
        scissorSize.setWidth(qMin(viewportSize.width(), scissorSize.width()));
        scissorSize.setHeight(qMin(viewportSize.height(), scissorSize.height()));
        m_cb->setScissor({ scissorPos.x(), scissorPos.y(), scissorSize.width(), scissorSize.height() });
        m_cb->setShaderResources(m_textures[c.textureIndex].srb);
        m_cb->setVertexInput(0, 1, &vbufBinding, m_ibuf.get(), c.indexOffset, QRhiCommandBuffer::IndexUInt32);
        m_cb->drawIndexed(c.elemCount);
    }
}

static const char *getClipboardText(void *)
{
    static QByteArray contents;
    contents = QGuiApplication::clipboard()->text().toUtf8();
    return contents.constData();
}

static void setClipboardText(void *, const char *text)
{
    QGuiApplication::clipboard()->setText(QString::fromUtf8(text));
}

QRhiImgui::QRhiImgui()
{
    ImGui::CreateContext();
    rebuildFontAtlas();
    ImGuiIO &io(ImGui::GetIO());
    io.GetClipboardTextFn = getClipboardText;
    io.SetClipboardTextFn = setClipboardText;
}

QRhiImgui::~QRhiImgui()
{
    ImGui::DestroyContext();
}

void QRhiImgui::rebuildFontAtlas()
{
    unsigned char *pixels;
    int w, h;
    ImGuiIO &io(ImGui::GetIO());
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    const QImage wrapperImg(const_cast<const uchar *>(pixels), w, h, QImage::Format_RGBA8888);
    sf.fontTextureData = wrapperImg.copy();
    io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(quintptr(0)));
}

void QRhiImgui::nextFrame(const QSizeF &logicalOutputSize, float dpr, const QPointF &logicalOffset, FrameFunc frameFunc)
{
    ImGuiIO &io(ImGui::GetIO());

    const QPointF itemPixelOffset = logicalOffset * dpr;
    f.outputPixelSize = (logicalOutputSize * dpr).toSize();
    io.DisplaySize.x = logicalOutputSize.width();
    io.DisplaySize.y = logicalOutputSize.height();
    io.DisplayFramebufferScale = ImVec2(dpr, dpr);

    ImGui::NewFrame();
    if (frameFunc)
        frameFunc();
    ImGui::Render();

    ImDrawData *draw = ImGui::GetDrawData();
    draw->ScaleClipRects(ImVec2(dpr, dpr));

    f.vbuf.resize(draw->CmdListsCount);
    f.ibuf.resize(draw->CmdListsCount);
    f.totalVbufSize = 0;
    f.totalIbufSize = 0;
    for (int n = 0; n < draw->CmdListsCount; ++n) {
        const ImDrawList *cmdList = draw->CmdLists[n];
        const int vbufSize = cmdList->VtxBuffer.Size * sizeof(ImDrawVert);
        f.vbuf[n].offset = f.totalVbufSize;
        f.totalVbufSize += vbufSize;
        const int ibufSize = cmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
        f.ibuf[n].offset = f.totalIbufSize;
        f.totalIbufSize += ibufSize;
    }
    f.draw.clear();
    for (int n = 0; n < draw->CmdListsCount; ++n) {
        const ImDrawList *cmdList = draw->CmdLists[n];
        f.vbuf[n].data = QByteArray(reinterpret_cast<const char *>(cmdList->VtxBuffer.Data),
                                    cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        f.ibuf[n].data = QByteArray(reinterpret_cast<const char *>(cmdList->IdxBuffer.Data),
                                    cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
        const ImDrawIdx *indexBufOffset = nullptr;
        for (int i = 0; i < cmdList->CmdBuffer.Size; ++i) {
            const ImDrawCmd *cmd = &cmdList->CmdBuffer[i];
            const quint32 indexOffset = f.ibuf[n].offset + quintptr(indexBufOffset);
            if (!cmd->UserCallback) {
                QRhiImguiRenderer::DrawCmd dc;
                dc.cmdListBufferIdx = n;
                dc.textureIndex = int(reinterpret_cast<qintptr>(cmd->TextureId));
                dc.indexOffset = indexOffset;
                dc.elemCount = cmd->ElemCount;
                dc.itemPixelOffset = itemPixelOffset;
                dc.clipRect = QVector4D(cmd->ClipRect.x, cmd->ClipRect.y, cmd->ClipRect.z, cmd->ClipRect.w);
                f.draw.append(dc);
            } else {
                cmd->UserCallback(cmdList, cmd);
            }
            indexBufOffset += cmd->ElemCount;
        }
    }
}

void QRhiImgui::syncRenderer(QRhiImguiRenderer *renderer)
{
    renderer->sf = sf;
    sf.fontTextureData = QImage();
    renderer->f = std::move(f);
}

static void updateKeyboardModifiers(Qt::KeyboardModifiers modifiers)
{
    ImGuiIO &io(ImGui::GetIO());
    io.AddKeyEvent(ImGuiKey_ModCtrl, modifiers.testFlag(Qt::ControlModifier));
    io.AddKeyEvent(ImGuiKey_ModShift, modifiers.testFlag(Qt::ShiftModifier));
    io.AddKeyEvent(ImGuiKey_ModAlt, modifiers.testFlag(Qt::AltModifier));
    io.AddKeyEvent(ImGuiKey_ModSuper, modifiers.testFlag(Qt::MetaModifier));
}

static ImGuiKey mapKey(int k)
{
    switch (k) {
    case Qt::Key_Space:
        return ImGuiKey_Space;
    case Qt::Key_Apostrophe:
        return ImGuiKey_Apostrophe;
    case Qt::Key_Comma:
        return ImGuiKey_Comma;
    case Qt::Key_Minus:
        return ImGuiKey_Minus;
    case Qt::Key_Period:
        return ImGuiKey_Period;
    case Qt::Key_Slash:
        return ImGuiKey_Slash;
    case Qt::Key_0:
        return ImGuiKey_0;
    case Qt::Key_1:
        return ImGuiKey_1;
    case Qt::Key_2:
        return ImGuiKey_2;
    case Qt::Key_3:
        return ImGuiKey_3;
    case Qt::Key_4:
        return ImGuiKey_4;
    case Qt::Key_5:
        return ImGuiKey_5;
    case Qt::Key_6:
        return ImGuiKey_6;
    case Qt::Key_7:
        return ImGuiKey_8;
    case Qt::Key_8:
        return ImGuiKey_8;
    case Qt::Key_9:
        return ImGuiKey_9;
    case Qt::Key_Semicolon:
        return ImGuiKey_Semicolon;
    case Qt::Key_Equal:
        return ImGuiKey_Equal;
    case Qt::Key_A:
        return ImGuiKey_A;
    case Qt::Key_B:
        return ImGuiKey_B;
    case Qt::Key_C:
        return ImGuiKey_C;
    case Qt::Key_D:
        return ImGuiKey_D;
    case Qt::Key_E:
        return ImGuiKey_E;
    case Qt::Key_F:
        return ImGuiKey_F;
    case Qt::Key_G:
        return ImGuiKey_G;
    case Qt::Key_H:
        return ImGuiKey_H;
    case Qt::Key_I:
        return ImGuiKey_I;
    case Qt::Key_J:
        return ImGuiKey_J;
    case Qt::Key_K:
        return ImGuiKey_K;
    case Qt::Key_L:
        return ImGuiKey_L;
    case Qt::Key_M:
        return ImGuiKey_M;
    case Qt::Key_N:
        return ImGuiKey_N;
    case Qt::Key_O:
        return ImGuiKey_O;
    case Qt::Key_P:
        return ImGuiKey_P;
    case Qt::Key_Q:
        return ImGuiKey_Q;
    case Qt::Key_R:
        return ImGuiKey_R;
    case Qt::Key_S:
        return ImGuiKey_S;
    case Qt::Key_T:
        return ImGuiKey_T;
    case Qt::Key_U:
        return ImGuiKey_U;
    case Qt::Key_V:
        return ImGuiKey_V;
    case Qt::Key_W:
        return ImGuiKey_W;
    case Qt::Key_X:
        return ImGuiKey_X;
    case Qt::Key_Y:
        return ImGuiKey_Y;
    case Qt::Key_Z:
        return ImGuiKey_Z;
    case Qt::Key_BracketLeft:
        return ImGuiKey_LeftBracket;
    case Qt::Key_Backslash:
        return ImGuiKey_Backslash;
    case Qt::Key_BracketRight:
        return ImGuiKey_RightBracket;
    case Qt::Key_QuoteLeft:
        return ImGuiKey_GraveAccent;
    case Qt::Key_Escape:
        return ImGuiKey_Escape;
    case Qt::Key_Tab:
        return ImGuiKey_Tab;
    case Qt::Key_Backspace:
        return ImGuiKey_Backspace;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return ImGuiKey_Enter;
    case Qt::Key_Insert:
        return ImGuiKey_Insert;
    case Qt::Key_Delete:
        return ImGuiKey_Delete;
    case Qt::Key_Pause:
        return ImGuiKey_Pause;
    case Qt::Key_Print:
        return ImGuiKey_PrintScreen;
    case Qt::Key_Home:
        return ImGuiKey_Home;
    case Qt::Key_End:
        return ImGuiKey_End;
    case Qt::Key_Left:
        return ImGuiKey_LeftArrow;
    case Qt::Key_Up:
        return ImGuiKey_UpArrow;
    case Qt::Key_Right:
        return ImGuiKey_RightArrow;
    case Qt::Key_Down:
        return ImGuiKey_DownArrow;
    case Qt::Key_PageUp:
        return ImGuiKey_PageUp;
    case Qt::Key_PageDown:
        return ImGuiKey_PageDown;
    case Qt::Key_Shift:
        return ImGuiKey_LeftShift;
    case Qt::Key_Control:
        return ImGuiKey_LeftCtrl;
    case Qt::Key_Meta:
        return ImGuiKey_LeftSuper;
    case Qt::Key_Alt:
        return ImGuiKey_LeftAlt;
    case Qt::Key_CapsLock:
        return ImGuiKey_CapsLock;
    case Qt::Key_NumLock:
        return ImGuiKey_NumLock;
    case Qt::Key_ScrollLock:
        return ImGuiKey_ScrollLock;
    case Qt::Key_F1:
        return ImGuiKey_F1;
    case Qt::Key_F2:
        return ImGuiKey_F2;
    case Qt::Key_F3:
        return ImGuiKey_F3;
    case Qt::Key_F4:
        return ImGuiKey_F4;
    case Qt::Key_F5:
        return ImGuiKey_F5;
    case Qt::Key_F6:
        return ImGuiKey_F6;
    case Qt::Key_F7:
        return ImGuiKey_F7;
    case Qt::Key_F8:
        return ImGuiKey_F8;
    case Qt::Key_F9:
        return ImGuiKey_F9;
    case Qt::Key_F10:
        return ImGuiKey_F10;
    case Qt::Key_F11:
        return ImGuiKey_F11;
    case Qt::Key_F12:
        return ImGuiKey_F12;
    default:
        break;
    }
    return ImGuiKey_None;
}

bool QRhiImgui::processEvent(QEvent *event)
{
    ImGuiIO &io(ImGui::GetIO());

    switch (event->type()) {
    case QEvent::MouseButtonPress:
    {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        updateKeyboardModifiers(me->modifiers());
        Qt::MouseButtons buttons = me->buttons();
        if (buttons.testFlag(Qt::LeftButton) && !pressedMouseButtons.testFlag(Qt::LeftButton))
            io.AddMouseButtonEvent(0, true);
        if (buttons.testFlag(Qt::RightButton) && !pressedMouseButtons.testFlag(Qt::RightButton))
            io.AddMouseButtonEvent(1, true);
        if (buttons.testFlag(Qt::MiddleButton) && !pressedMouseButtons.testFlag(Qt::MiddleButton))
            io.AddMouseButtonEvent(2, true);
        pressedMouseButtons = buttons;
   }
        return true;

    case QEvent::MouseButtonRelease:
    {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        Qt::MouseButtons buttons = me->buttons();
        if (!buttons.testFlag(Qt::LeftButton) && pressedMouseButtons.testFlag(Qt::LeftButton))
            io.AddMouseButtonEvent(0, false);
        if (!buttons.testFlag(Qt::RightButton) && pressedMouseButtons.testFlag(Qt::RightButton))
            io.AddMouseButtonEvent(1, false);
        if (!buttons.testFlag(Qt::MiddleButton) && pressedMouseButtons.testFlag(Qt::MiddleButton))
            io.AddMouseButtonEvent(2, false);
        pressedMouseButtons = buttons;
    }
        return true;

    case QEvent::MouseMove:
    {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        const QPointF pos = me->position();
        io.AddMousePosEvent(pos.x(), pos.y());
    }
        return true;

    case QEvent::Wheel:
    {
        QWheelEvent *we = static_cast<QWheelEvent *>(event);
        QPointF wheel(we->angleDelta().x() / 120.0f, we->angleDelta().y() / 120.0f);
        io.AddMouseWheelEvent(wheel.x(), wheel.y());
    }
        return true;

    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        const bool down = event->type() == QEvent::KeyPress;
        updateKeyboardModifiers(ke->modifiers());
        io.AddKeyEvent(mapKey(ke->key()), down);
        if (down && !ke->text().isEmpty()) {
            const QByteArray text = ke->text().toUtf8();
            io.AddInputCharactersUTF8(text.constData());
        }
    }
        return true;

    default:
        break;
    }

    return false;
}

QT_END_NAMESPACE
