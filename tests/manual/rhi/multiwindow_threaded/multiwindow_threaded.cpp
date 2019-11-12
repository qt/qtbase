/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QEvent>
#include <QCommandLineParser>
#include <QElapsedTimer>

#include <QtGui/private/qshader_p.h>
#include <QFile>
#include <QtGui/private/qrhiprofiler_p.h>

#ifndef QT_NO_OPENGL
#include <QtGui/private/qrhigles2_p.h>
#include <QOffscreenSurface>
#endif

#if QT_CONFIG(vulkan)
#include <QLoggingCategory>
#include <QtGui/private/qrhivulkan_p.h>
#endif

#ifdef Q_OS_WIN
#include <QtGui/private/qrhid3d11_p.h>
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
#include <QtGui/private/qrhimetal_p.h>
#endif

#include "window.h"

#include "../shared/cube.h"

static QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

static GraphicsApi graphicsApi;

static QString graphicsApiName()
{
    switch (graphicsApi) {
    case OpenGL:
        return QLatin1String("OpenGL 2.x");
    case Vulkan:
        return QLatin1String("Vulkan");
    case D3D11:
        return QLatin1String("Direct3D 11");
    case Metal:
        return QLatin1String("Metal");
    default:
        break;
    }
    return QString();
}

#if QT_CONFIG(vulkan)
QVulkanInstance *instance = nullptr;
#endif

// Window (main thread) emit signals -> Renderer::send* (main thread) -> event queue (add on main, process on render thread) -> Renderer::renderEvent (render thread)

// event queue is taken from the Qt Quick scenegraph as-is
// all this below is conceptually the same as the QSG threaded render loop
class RenderThreadEventQueue : public QQueue<QEvent *>
{
public:
    RenderThreadEventQueue()
        : waiting(false)
    {
    }

    void addEvent(QEvent *e) {
        mutex.lock();
        enqueue(e);
        if (waiting)
            condition.wakeOne();
        mutex.unlock();
    }

    QEvent *takeEvent(bool wait) {
        mutex.lock();
        if (isEmpty() && wait) {
            waiting = true;
            condition.wait(&mutex);
            waiting = false;
        }
        QEvent *e = dequeue();
        mutex.unlock();
        return e;
    }

    bool hasMoreEvents() {
        mutex.lock();
        bool has = !isEmpty();
        mutex.unlock();
        return has;
    }

private:
    QMutex mutex;
    QWaitCondition condition;
    bool waiting;
};

struct Renderer;

struct Thread : public QThread
{
    Thread(Renderer *renderer_)
        : renderer(renderer_)
    {
        active = true;
        start();
    }
    void run() override;

    Renderer *renderer;
    bool active;
    RenderThreadEventQueue eventQueue;
    bool sleeping = false;
    bool stopEventProcessing = false;
    bool pendingRender = false;
    bool pendingRenderIsNewExpose = false;
    // mutex and cond used to allow the main thread waiting until something completes on the render thread
    QMutex mutex;
    QWaitCondition cond;
};

class RenderThreadEvent : public QEvent
{
public:
    RenderThreadEvent(QEvent::Type type) : QEvent(type) { }
};

class InitEvent : public RenderThreadEvent
{
public:
    static const QEvent::Type TYPE = QEvent::Type(QEvent::User + 1);
    InitEvent() : RenderThreadEvent(TYPE)
    { }
};

class RequestRenderEvent : public RenderThreadEvent
{
public:
    static const QEvent::Type TYPE = QEvent::Type(QEvent::User + 2);
    RequestRenderEvent(bool newlyExposed_) : RenderThreadEvent(TYPE), newlyExposed(newlyExposed_)
    { }
    bool newlyExposed;
};

class SurfaceCleanupEvent : public RenderThreadEvent
{
public:
    static const QEvent::Type TYPE = QEvent::Type(QEvent::User + 3);
    SurfaceCleanupEvent() : RenderThreadEvent(TYPE)
    { }
};

class CloseEvent : public RenderThreadEvent
{
public:
    static const QEvent::Type TYPE = QEvent::Type(QEvent::User + 4);
    CloseEvent() : RenderThreadEvent(TYPE)
    { }
};

class SyncSurfaceSizeEvent : public RenderThreadEvent
{
public:
    static const QEvent::Type TYPE = QEvent::Type(QEvent::User + 5);
    SyncSurfaceSizeEvent() : RenderThreadEvent(TYPE)
    { }
};

struct Renderer
{
    // ctor and dtor and send* are called main thread, rest on the render thread

    Renderer(QWindow *w, const QColor &bgColor, int rotationAxis);
    ~Renderer();

    void sendInit();
    void sendRender(bool newlyExposed);
    void sendSurfaceGoingAway();
    void sendSyncSurfaceSize();

    QWindow *window;
    Thread *thread;
    QRhi *r = nullptr;
#ifndef QT_NO_OPENGL
    QOffscreenSurface *fallbackSurface = nullptr;
#endif

    void createRhi();
    void destroyRhi();
    void renderEvent(QEvent *e);
    void init();
    void releaseSwapChain();
    void releaseResources();
    void render(bool newlyExposed, bool wakeBeforePresent);

    QColor m_bgColor;
    int m_rotationAxis;

    QVector<QRhiResource *> m_releasePool;
    bool m_hasSwapChain = false;
    QRhiSwapChain *m_sc = nullptr;
    QRhiRenderBuffer *m_ds = nullptr;
    QRhiRenderPassDescriptor *m_rp = nullptr;

    QRhiBuffer *m_vbuf = nullptr;
    QRhiBuffer *m_ubuf = nullptr;
    QRhiTexture *m_tex = nullptr;
    QRhiSampler *m_sampler = nullptr;
    QRhiShaderResourceBindings *m_srb = nullptr;
    QRhiGraphicsPipeline *m_ps = nullptr;
    QRhiResourceUpdateBatch *m_initialUpdates = nullptr;

    QMatrix4x4 m_proj;
    float m_rotation = 0;
    int m_frameCount = 0;
};

void Thread::run()
{
    while (active) {
#ifdef Q_OS_DARWIN
        QMacAutoReleasePool autoReleasePool;
#endif

        if (pendingRender) {
            pendingRender = false;
            renderer->render(pendingRenderIsNewExpose, false);
        }

        while (eventQueue.hasMoreEvents()) {
            QEvent *e = eventQueue.takeEvent(false);
            renderer->renderEvent(e);
            delete e;
        }

        if (active && !pendingRender) {
            sleeping = true;
            stopEventProcessing = false;
            while (!stopEventProcessing) {
                QEvent *e = eventQueue.takeEvent(true);
                renderer->renderEvent(e);
                delete e;
            }
            sleeping = false;
        }
    }
}

Renderer::Renderer(QWindow *w, const QColor &bgColor, int rotationAxis)
    : window(w),
      m_bgColor(bgColor),
      m_rotationAxis(rotationAxis)
{ // main thread
    thread = new Thread(this);

#ifndef QT_NO_OPENGL
    if (graphicsApi == OpenGL)
        fallbackSurface = QRhiGles2InitParams::newFallbackSurface();
#endif
}

Renderer::~Renderer()
{ // main thread
    thread->eventQueue.addEvent(new CloseEvent);
    thread->wait();
    delete thread;

#ifndef QT_NO_OPENGL
    delete fallbackSurface;
#endif
}

void Renderer::createRhi()
{
    if (r)
        return;

    qDebug() << "renderer" << this << "creating rhi";
    QRhi::Flags rhiFlags = QRhi::EnableProfiling;

#ifndef QT_NO_OPENGL
    if (graphicsApi == OpenGL) {
        QRhiGles2InitParams params;
        params.fallbackSurface = fallbackSurface;
        params.window = window;
        r = QRhi::create(QRhi::OpenGLES2, &params, rhiFlags);
    }
#endif

#if QT_CONFIG(vulkan)
    if (graphicsApi == Vulkan) {
        QRhiVulkanInitParams params;
        params.inst = instance;
        params.window = window;
        r = QRhi::create(QRhi::Vulkan, &params, rhiFlags);
    }
#endif

#ifdef Q_OS_WIN
    if (graphicsApi == D3D11) {
        QRhiD3D11InitParams params;
        params.enableDebugLayer = true;
        r = QRhi::create(QRhi::D3D11, &params, rhiFlags);
    }
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    if (graphicsApi == Metal) {
        QRhiMetalInitParams params;
        r = QRhi::create(QRhi::Metal, &params, rhiFlags);
    }
#endif

    if (!r)
        qFatal("Failed to create RHI backend");
}

void Renderer::destroyRhi()
{
    qDebug() << "renderer" << this << "destroying rhi";
    delete r;
    r = nullptr;
}

void Renderer::renderEvent(QEvent *e)
{
    Q_ASSERT(QThread::currentThread() == thread);

    if (thread->sleeping)
        thread->stopEventProcessing = true;

    switch (int(e->type())) {
    case InitEvent::TYPE:
        qDebug() << "renderer" << this << "for window" << window << "is initializing";
        createRhi();
        init();
        break;
    case RequestRenderEvent::TYPE:
        thread->pendingRender = true;
        thread->pendingRenderIsNewExpose = static_cast<RequestRenderEvent *>(e)->newlyExposed;
        break;
    case SurfaceCleanupEvent::TYPE: // when the QWindow is closed, before QPlatformWindow goes away
        thread->mutex.lock();
        qDebug() << "renderer" << this << "for window" << window << "is destroying swapchain";
        thread->pendingRender = false;
        releaseSwapChain();
        thread->cond.wakeOne();
        thread->mutex.unlock();
        break;
    case CloseEvent::TYPE: // when destroying the window+renderer (NB not the same as hitting X on the window, that's just QWindow close)
        qDebug() << "renderer" << this << "for window" << window << "is shutting down";
        thread->pendingRender = false;
        thread->active = false;
        thread->stopEventProcessing = true;
        releaseResources();
        destroyRhi();
        break;
    case SyncSurfaceSizeEvent::TYPE:
        thread->mutex.lock();
        thread->pendingRender = false;
        render(false, true);
        break;
    default:
        break;
    }
}

void Renderer::init()
{
    m_sc = r->newSwapChain();
    m_ds = r->newRenderBuffer(QRhiRenderBuffer::DepthStencil,
                              QSize(),
                              1,
                              QRhiRenderBuffer::UsedWithSwapChainOnly);
    m_releasePool << m_ds;
    m_sc->setWindow(window);
    m_sc->setDepthStencil(m_ds);
    m_rp = m_sc->newCompatibleRenderPassDescriptor();
    m_releasePool << m_rp;
    m_sc->setRenderPassDescriptor(m_rp);

    m_vbuf = r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube));
    m_releasePool << m_vbuf;
    m_vbuf->build();

    m_ubuf = r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 4);
    m_releasePool << m_ubuf;
    m_ubuf->build();

    QImage image = QImage(QLatin1String(":/qt256.png")).convertToFormat(QImage::Format_RGBA8888);
    m_tex = r->newTexture(QRhiTexture::RGBA8, image.size());
    m_releasePool << m_tex;
    m_tex->build();

    m_sampler = r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
    m_releasePool << m_sampler;
    m_sampler->build();

    m_srb = r->newShaderResourceBindings();
    m_releasePool << m_srb;
    m_srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_ubuf),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_tex, m_sampler)
    });
    m_srb->build();

    m_ps = r->newGraphicsPipeline();
    m_releasePool << m_ps;

    m_ps->setDepthTest(true);
    m_ps->setDepthWrite(true);
    m_ps->setDepthOp(QRhiGraphicsPipeline::Less);

    m_ps->setCullMode(QRhiGraphicsPipeline::Back);
    m_ps->setFrontFace(QRhiGraphicsPipeline::CCW);

    m_ps->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/texture.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/texture.frag.qsb")) }
    });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 3 * sizeof(float) },
        { 2 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
        { 1, 1, QRhiVertexInputAttribute::Float2, 0 }
    });

    m_ps->setVertexInputLayout(inputLayout);
    m_ps->setShaderResourceBindings(m_srb);
    m_ps->setRenderPassDescriptor(m_rp);

    m_ps->build();

    m_initialUpdates = r->nextResourceUpdateBatch();

    m_initialUpdates->uploadStaticBuffer(m_vbuf, cube);

    qint32 flip = 0;
    m_initialUpdates->updateDynamicBuffer(m_ubuf, 64, 4, &flip);

    m_initialUpdates->uploadTexture(m_tex, image);
}

void Renderer::releaseSwapChain()
{
    if (m_hasSwapChain) {
        m_hasSwapChain = false;
        m_sc->release();
    }
}

void Renderer::releaseResources()
{
    qDeleteAll(m_releasePool);
    m_releasePool.clear();

    delete m_sc;
    m_sc = nullptr;
}

void Renderer::render(bool newlyExposed, bool wakeBeforePresent)
{
    // This function handles both resizing and rendering. Resizes have some
    // complications due to the threaded model (check exposeEvent and
    // sendSyncSurfaceSize) but don't have to worry about that in here.

    auto buildOrResizeSwapChain = [this] {
        qDebug() << "renderer" << this << "build or resize swapchain for window" << window;
        m_hasSwapChain = m_sc->buildOrResize();
        const QSize outputSize = m_sc->currentPixelSize();
        qDebug() << "  size is" << outputSize;
        m_proj = r->clipSpaceCorrMatrix();
        m_proj.perspective(45.0f, outputSize.width() / (float) outputSize.height(), 0.01f, 100.0f);
        m_proj.translate(0, 0, -4);
    };

    auto wakeUpIfNeeded = [wakeBeforePresent, this] {
        // make sure the main/gui thread is not blocked when issuing the Present (or equivalent)
        if (wakeBeforePresent) {
            thread->cond.wakeOne();
            thread->mutex.unlock();
        }
    };

    const QSize surfaceSize = m_sc->surfacePixelSize();
    if (surfaceSize.isEmpty()) {
        wakeUpIfNeeded();
        return;
    }

    if (newlyExposed || m_sc->currentPixelSize() != surfaceSize)
        buildOrResizeSwapChain();

    if (!m_hasSwapChain) {
        wakeUpIfNeeded();
        return;
    }

    QRhi::FrameOpResult result = r->beginFrame(m_sc);
    if (result == QRhi::FrameOpSwapChainOutOfDate) {
        buildOrResizeSwapChain();
        if (!m_hasSwapChain) {
            wakeUpIfNeeded();
            return;
        }
        result = r->beginFrame(m_sc);
    }
    if (result != QRhi::FrameOpSuccess) {
        wakeUpIfNeeded();
        return;
    }

    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    const QSize outputSize = m_sc->currentPixelSize();

    QRhiResourceUpdateBatch *u = r->nextResourceUpdateBatch();
    if (m_initialUpdates) {
        u->merge(m_initialUpdates);
        m_initialUpdates->release();
        m_initialUpdates = nullptr;
    }

    m_rotation += 1.0f;
    QMatrix4x4 mvp = m_proj;
    mvp.scale(0.5f);
    mvp.rotate(m_rotation, m_rotationAxis == 0 ? 1 : 0, m_rotationAxis == 1 ? 1 : 0, m_rotationAxis == 2 ? 1 : 0);
    u->updateDynamicBuffer(m_ubuf, 0, 64, mvp.constData());

    cb->beginPass(m_sc->currentFrameRenderTarget(),
                  QColor::fromRgbF(float(m_bgColor.redF()), float(m_bgColor.greenF()), float(m_bgColor.blueF()), 1.0f),
                  { 1.0f, 0 },
                  u);

    cb->setGraphicsPipeline(m_ps);
    cb->setViewport(QRhiViewport(0, 0, outputSize.width(), outputSize.height()));
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBindings[] = {
        { m_vbuf, 0 },
        { m_vbuf, 36 * 3 * sizeof(float) }
    };
    cb->setVertexInput(0, 2, vbufBindings);
    cb->draw(36);

    cb->endPass();

    wakeUpIfNeeded();

    r->endFrame(m_sc);

    m_frameCount += 1;
    if ((m_frameCount % 300) == 0) {
        const QRhiProfiler::CpuTime ff = r->profiler()->frameToFrameTimes(m_sc);
        const QRhiProfiler::CpuTime be = r->profiler()->frameBuildTimes(m_sc);
        const QRhiProfiler::GpuTime gp = r->profiler()->gpuFrameTimes(m_sc);
        if (r->isFeatureSupported(QRhi::Timestamps)) {
            qDebug("[renderer %p] frame-to-frame: min %lld max %lld avg %f. "
                   "frame build: min %lld max %lld avg %f. "
                   "gpu frame time: min %f max %f avg %f",
                   this,
                   ff.minTime, ff.maxTime, ff.avgTime,
                   be.minTime, be.maxTime, be.avgTime,
                   gp.minTime, gp.maxTime, gp.avgTime);
        } else {
            qDebug("[renderer %p] frame-to-frame: min %lld max %lld avg %f. "
                   "frame build: min %lld max %lld avg %f. ",
                   this,
                   ff.minTime, ff.maxTime, ff.avgTime,
                   be.minTime, be.maxTime, be.avgTime);
        }
    }
}

void Renderer::sendInit()
{ // main thread
    InitEvent *e = new InitEvent;
    thread->eventQueue.addEvent(e);
}

void Renderer::sendRender(bool newlyExposed)
{ // main thread
    RequestRenderEvent *e = new RequestRenderEvent(newlyExposed);
    thread->eventQueue.addEvent(e);
}

void Renderer::sendSurfaceGoingAway()
{ // main thread
    SurfaceCleanupEvent *e = new SurfaceCleanupEvent;

    // cannot let this thread to proceed with tearing down the native window
    // before the render thread completes the swapchain release
    thread->mutex.lock();

    thread->eventQueue.addEvent(e);

    thread->cond.wait(&thread->mutex);
    thread->mutex.unlock();
}

void Renderer::sendSyncSurfaceSize()
{ // main thread
    SyncSurfaceSizeEvent *e = new SyncSurfaceSizeEvent;
    // must block to prevent surface size mess. the render thread will do a
    // full rendering round before it unlocks which is good since it can thus
    // pick up and the surface (window) size atomically.
    thread->mutex.lock();
    thread->eventQueue.addEvent(e);
    thread->cond.wait(&thread->mutex);
    thread->mutex.unlock();
}

struct WindowAndRenderer
{
    QWindow *window;
    Renderer *renderer;
};

QVector<WindowAndRenderer> windows;

void createWindow()
{
    static QColor colors[] = { Qt::red, Qt::green, Qt::blue, Qt::yellow, Qt::cyan, Qt::gray };
    const int n = windows.count();
    Window *w = new Window(QString::asprintf("Window+Thread #%d (%s)", n, qPrintable(graphicsApiName())), graphicsApi);
    Renderer *renderer = new Renderer(w, colors[n % 6], n % 3);;
    QObject::connect(w, &Window::initRequested, w, [renderer] {
        renderer->sendInit();
    });
    QObject::connect(w, &Window::renderRequested, w, [w, renderer](bool newlyExposed) {
        renderer->sendRender(newlyExposed);
        w->requestUpdate();
    });
    QObject::connect(w, &Window::surfaceGoingAway, w, [renderer] {
        renderer->sendSurfaceGoingAway();
    });
    QObject::connect(w, &Window::syncSurfaceSizeRequested, w, [renderer] {
        renderer->sendSyncSurfaceSize();
    });
    windows.append({ w, renderer });
    w->show();
}

void closeWindow()
{
    WindowAndRenderer wr = windows.takeLast();
    delete wr.renderer;
    delete wr.window;
}

int main(int argc, char **argv)
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);

#if defined(Q_OS_WIN)
    graphicsApi = D3D11;
#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    graphicsApi = Metal;
#elif QT_CONFIG(vulkan)
    graphicsApi = Vulkan;
#else
    graphicsApi = OpenGL;
#endif

    QCommandLineParser cmdLineParser;
    cmdLineParser.addHelpOption();
    QCommandLineOption glOption({ "g", "opengl" }, QLatin1String("OpenGL (2.x)"));
    cmdLineParser.addOption(glOption);
    QCommandLineOption vkOption({ "v", "vulkan" }, QLatin1String("Vulkan"));
    cmdLineParser.addOption(vkOption);
    QCommandLineOption d3dOption({ "d", "d3d11" }, QLatin1String("Direct3D 11"));
    cmdLineParser.addOption(d3dOption);
    QCommandLineOption mtlOption({ "m", "metal" }, QLatin1String("Metal"));
    cmdLineParser.addOption(mtlOption);
    cmdLineParser.process(app);
    if (cmdLineParser.isSet(glOption))
        graphicsApi = OpenGL;
    if (cmdLineParser.isSet(vkOption))
        graphicsApi = Vulkan;
    if (cmdLineParser.isSet(d3dOption))
        graphicsApi = D3D11;
    if (cmdLineParser.isSet(mtlOption))
        graphicsApi = Metal;

    qDebug("Selected graphics API is %s", qPrintable(graphicsApiName()));
    qDebug("This is a multi-api example, use command line arguments to override:\n%s", qPrintable(cmdLineParser.helpText()));

#if QT_CONFIG(vulkan)
    instance = new QVulkanInstance;
    if (graphicsApi == Vulkan) {
#ifndef Q_OS_ANDROID
        instance->setLayers({ "VK_LAYER_LUNARG_standard_validation" });
#else
        instance->setLayers(QByteArrayList()
                       << "VK_LAYER_GOOGLE_threading"
                       << "VK_LAYER_LUNARG_parameter_validation"
                       << "VK_LAYER_LUNARG_object_tracker"
                       << "VK_LAYER_LUNARG_core_validation"
                       << "VK_LAYER_LUNARG_image"
                       << "VK_LAYER_LUNARG_swapchain"
                       << "VK_LAYER_GOOGLE_unique_objects");
#endif
        if (!instance->create()) {
            qWarning("Failed to create Vulkan instance, switching to OpenGL");
            graphicsApi = OpenGL;
        }
    }
#endif

    int winCount = 0;
    QWidget w;
    w.resize(800, 600);
    w.setWindowTitle(QCoreApplication::applicationName() + QLatin1String(" - ") + graphicsApiName());
    QVBoxLayout *layout = new QVBoxLayout(&w);

    QPlainTextEdit *info = new QPlainTextEdit(
                QLatin1String("This application tests rendering on a separate thread per window, with dedicated QRhi instances and resources. "
                              "\n\nThis is the same concept as the Qt Quick Scenegraph's threaded render loop. This should allow rendering to the different windows "
                              "without unintentionally throttling each other's threads."
                              "\n\nUsing API: ") + graphicsApiName());
    info->setReadOnly(true);
    layout->addWidget(info);
    QLabel *label = new QLabel(QLatin1String("Window and thread count: 0"));
    layout->addWidget(label);
    QPushButton *btn = new QPushButton(QLatin1String("New window"));
    QObject::connect(btn, &QPushButton::clicked, btn, [label, &winCount] {
        winCount += 1;
        label->setText(QString::asprintf("Window count: %d", winCount));
        createWindow();
    });
    layout->addWidget(btn);
    btn = new QPushButton(QLatin1String("Close window"));
    QObject::connect(btn, &QPushButton::clicked, btn, [label, &winCount] {
        if (winCount > 0) {
            winCount -= 1;
            label->setText(QString::asprintf("Window count: %d", winCount));
            closeWindow();
        }
    });
    layout->addWidget(btn);
    w.show();

    int result = app.exec();

    for (const WindowAndRenderer &wr : windows) {
        delete wr.renderer;
        delete wr.window;
    }

#if QT_CONFIG(vulkan)
    delete instance;
#endif

    return result;
}
