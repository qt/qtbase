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

// Adapted from hellominimalcrossgfxtriangle with the frame rendering stripped out.
// Include this file and implement Window::customInit, release and render.
// Debug/validation layer is enabled for D3D and Vulkan.

#include <QGuiApplication>
#include <QCommandLineParser>
#include <QWindow>
#include <QPlatformSurfaceEvent>
#include <QElapsedTimer>
#include <QTimer>
#include <QLoggingCategory>

#include <QtGui/private/qshader_p.h>
#include <QFile>
#include <QtGui/private/qrhiprofiler_p.h>
#include <QtGui/private/qrhinull_p.h>

#ifndef QT_NO_OPENGL
#include <QtGui/private/qrhigles2_p.h>
#include <QOffscreenSurface>
#endif

#if QT_CONFIG(vulkan)
#include <QtGui/private/qrhivulkan_p.h>
#endif

#ifdef Q_OS_WIN
#include <QtGui/private/qrhid3d11_p.h>
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
#include <QtGui/private/qrhimetal_p.h>
#endif

QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

enum GraphicsApi
{
    Null,
    OpenGL,
    Vulkan,
    D3D11,
    Metal
};

GraphicsApi graphicsApi;

QString graphicsApiName()
{
    switch (graphicsApi) {
    case Null:
        return QLatin1String("Null (no output)");
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

QRhi::Flags rhiFlags = QRhi::EnableDebugMarkers;
int sampleCount = 1;
QRhiSwapChain::Flags scFlags;
QRhi::BeginFrameFlags beginFrameFlags;
QRhi::EndFrameFlags endFrameFlags;
int framesUntilTdr = -1;
bool transparentBackground = false;

class Window : public QWindow
{
public:
    Window();
    ~Window();

protected:
    void init();
    void releaseResources();
    void resizeSwapChain();
    void releaseSwapChain();
    void render();

    void customInit();
    void customRelease();
    void customRender();

    void exposeEvent(QExposeEvent *) override;
    bool event(QEvent *) override;
#ifdef EXAMPLEFW_KEYPRESS_EVENTS
    void keyPressEvent(QKeyEvent *e) override;
#endif

    bool m_running = false;
    bool m_notExposed = false;
    bool m_newlyExposed = false;

    QRhi *m_r = nullptr;
    bool m_hasSwapChain = false;
    QRhiSwapChain *m_sc = nullptr;
    QRhiRenderBuffer *m_ds = nullptr;
    QRhiRenderPassDescriptor *m_rp = nullptr;

    QMatrix4x4 m_proj;

    QElapsedTimer m_timer;
    int m_frameCount;

#ifndef QT_NO_OPENGL
    QOffscreenSurface *m_fallbackSurface = nullptr;
#endif

    QColor m_clearColor;

    friend int main(int, char**);
};

Window::Window()
{
    // Tell the platform plugin what we want.
    switch (graphicsApi) {
    case OpenGL:
#if QT_CONFIG(opengl)
        setSurfaceType(OpenGLSurface);
        setFormat(QRhiGles2InitParams::adjustedFormat());
#endif
        break;
    case Vulkan:
        setSurfaceType(VulkanSurface);
        break;
    case D3D11:
        setSurfaceType(OpenGLSurface); // not a typo
        break;
    case Metal:
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
        setSurfaceType(MetalSurface);
#endif
        break;
    default:
        break;
    }

    m_clearColor = transparentBackground ? Qt::transparent : QColor::fromRgbF(0.4f, 0.7f, 0.0f, 1.0f);
}

Window::~Window()
{
    releaseResources();
}

void Window::exposeEvent(QExposeEvent *)
{
    // initialize and start rendering when the window becomes usable for graphics purposes
    if (isExposed() && !m_running) {
        m_running = true;
        init();
        resizeSwapChain();
    }

    const QSize surfaceSize = m_hasSwapChain ? m_sc->surfacePixelSize() : QSize();

    // stop pushing frames when not exposed (or size is 0)
    if ((!isExposed() || (m_hasSwapChain && surfaceSize.isEmpty())) && m_running)
        m_notExposed = true;

    // continue when exposed again and the surface has a valid size.
    // note that the surface size can be (0, 0) even though size() reports a valid one...
    if (isExposed() && m_running && m_notExposed && !surfaceSize.isEmpty()) {
        m_notExposed = false;
        m_newlyExposed = true;
    }

    // always render a frame on exposeEvent() (when exposed) in order to update
    // immediately on window resize.
    if (isExposed() && !surfaceSize.isEmpty())
        render();
}

bool Window::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::UpdateRequest:
        render();
        break;

    case QEvent::PlatformSurface:
        // this is the proper time to tear down the swapchain (while the native window and surface are still around)
        if (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
            releaseSwapChain();
        break;

    default:
        break;
    }

    return QWindow::event(e);
}

void Window::init()
{
    if (graphicsApi == Null) {
        QRhiNullInitParams params;
        m_r = QRhi::create(QRhi::Null, &params, rhiFlags);
    }

#ifndef QT_NO_OPENGL
    if (graphicsApi == OpenGL) {
        m_fallbackSurface = QRhiGles2InitParams::newFallbackSurface();
        QRhiGles2InitParams params;
        params.fallbackSurface = m_fallbackSurface;
        params.window = this;
        m_r = QRhi::create(QRhi::OpenGLES2, &params, rhiFlags);
    }
#endif

#if QT_CONFIG(vulkan)
    if (graphicsApi == Vulkan) {
        QRhiVulkanInitParams params;
        params.inst = vulkanInstance();
        params.window = this;
        m_r = QRhi::create(QRhi::Vulkan, &params, rhiFlags);
    }
#endif

#ifdef Q_OS_WIN
    if (graphicsApi == D3D11) {
        QRhiD3D11InitParams params;
        params.enableDebugLayer = true;
        if (framesUntilTdr > 0) {
            params.framesUntilKillingDeviceViaTdr = framesUntilTdr;
            params.repeatDeviceKill = true;
        }
        m_r = QRhi::create(QRhi::D3D11, &params, rhiFlags);
    }
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    if (graphicsApi == Metal) {
        QRhiMetalInitParams params;
        m_r = QRhi::create(QRhi::Metal, &params, rhiFlags);
    }
#endif

    if (!m_r)
        qFatal("Failed to create RHI backend");

    // now onto the backend-independent init

    m_sc = m_r->newSwapChain();
    // allow depth-stencil, although we do not actually enable depth test/write for the triangle
    m_ds = m_r->newRenderBuffer(QRhiRenderBuffer::DepthStencil,
                                QSize(), // no need to set the size here, due to UsedWithSwapChainOnly
                                sampleCount,
                                QRhiRenderBuffer::UsedWithSwapChainOnly);
    m_sc->setWindow(this);
    m_sc->setDepthStencil(m_ds);
    m_sc->setSampleCount(sampleCount);
    m_sc->setFlags(scFlags);
    m_rp = m_sc->newCompatibleRenderPassDescriptor();
    m_sc->setRenderPassDescriptor(m_rp);

    customInit();
}

void Window::releaseResources()
{
    customRelease();

    delete m_rp;
    m_rp = nullptr;

    delete m_ds;
    m_ds = nullptr;

    delete m_sc;
    m_sc = nullptr;

    delete m_r;
    m_r = nullptr;

#ifndef QT_NO_OPENGL
    delete m_fallbackSurface;
    m_fallbackSurface = nullptr;
#endif
}

void Window::resizeSwapChain()
{
    m_hasSwapChain = m_sc->buildOrResize(); // also handles m_ds

    m_frameCount = 0;
    m_timer.restart();

    const QSize outputSize = m_sc->currentPixelSize();
    m_proj = m_r->clipSpaceCorrMatrix();
    m_proj.perspective(45.0f, outputSize.width() / (float) outputSize.height(), 0.01f, 1000.0f);
    m_proj.translate(0, 0, -4);
}

void Window::releaseSwapChain()
{
    if (m_hasSwapChain) {
        m_hasSwapChain = false;
        m_sc->release();
    }
}

void Window::render()
{
    if (!m_hasSwapChain || m_notExposed)
        return;

    // If the window got resized or got newly exposed, resize the swapchain.
    // (the newly-exposed case is not actually required by some
    // platforms/backends, but f.ex. Vulkan on Windows seems to need it)
    if (m_sc->currentPixelSize() != m_sc->surfacePixelSize() || m_newlyExposed) {
        resizeSwapChain();
        if (!m_hasSwapChain)
            return;
        m_newlyExposed = false;
    }

    // Start a new frame. This is where we block when too far ahead of
    // GPU/present, and that's what throttles the thread to the refresh rate.
    // (except for OpenGL where it happens either in endFrame or somewhere else
    // depending on the GL implementation)
    QRhi::FrameOpResult r = m_r->beginFrame(m_sc, beginFrameFlags);
    if (r == QRhi::FrameOpSwapChainOutOfDate) {
        resizeSwapChain();
        if (!m_hasSwapChain)
            return;
        r = m_r->beginFrame(m_sc);
    }
    if (r != QRhi::FrameOpSuccess) {
        requestUpdate();
        return;
    }

    m_frameCount += 1;
    if (m_timer.elapsed() > 1000) {
        if (rhiFlags.testFlag(QRhi::EnableProfiling)) {
            const QRhiProfiler::CpuTime ff = m_r->profiler()->frameToFrameTimes(m_sc);
            const QRhiProfiler::CpuTime be = m_r->profiler()->frameBuildTimes(m_sc);
            const QRhiProfiler::GpuTime gp = m_r->profiler()->gpuFrameTimes(m_sc);
            if (m_r->isFeatureSupported(QRhi::Timestamps)) {
                qDebug("ca. %d fps. "
                       "frame-to-frame: min %lld max %lld avg %f. "
                       "frame build: min %lld max %lld avg %f. "
                       "gpu frame time: min %f max %f avg %f",
                       m_frameCount,
                       ff.minTime, ff.maxTime, ff.avgTime,
                       be.minTime, be.maxTime, be.avgTime,
                       gp.minTime, gp.maxTime, gp.avgTime);
            } else {
                qDebug("ca. %d fps. "
                       "frame-to-frame: min %lld max %lld avg %f. "
                       "frame build: min %lld max %lld avg %f. ",
                       m_frameCount,
                       ff.minTime, ff.maxTime, ff.avgTime,
                       be.minTime, be.maxTime, be.avgTime);
            }
        } else {
            qDebug("ca. %d fps", m_frameCount);
        }

        m_timer.restart();
        m_frameCount = 0;
    }

    customRender();

    m_r->endFrame(m_sc, endFrameFlags);

    if (!scFlags.testFlag(QRhiSwapChain::NoVSync))
        requestUpdate();
    else // try prevent all delays when NoVSync
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
}

int main(int argc, char **argv)
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    QLoggingCategory::setFilterRules(QLatin1String("qt.rhi.*=true"));

    // Defaults.
#if defined(Q_OS_WIN)
    graphicsApi = D3D11;
#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    graphicsApi = Metal;
#elif QT_CONFIG(vulkan)
    graphicsApi = Vulkan;
#else
    graphicsApi = OpenGL;
#endif

    // Allow overriding via the command line.
    QCommandLineParser cmdLineParser;
    cmdLineParser.addHelpOption();
    QCommandLineOption nullOption({ "n", "null" }, QLatin1String("Null"));
    cmdLineParser.addOption(nullOption);
    QCommandLineOption glOption({ "g", "opengl" }, QLatin1String("OpenGL (2.x)"));
    cmdLineParser.addOption(glOption);
    QCommandLineOption vkOption({ "v", "vulkan" }, QLatin1String("Vulkan"));
    cmdLineParser.addOption(vkOption);
    QCommandLineOption d3dOption({ "d", "d3d11" }, QLatin1String("Direct3D 11"));
    cmdLineParser.addOption(d3dOption);
    QCommandLineOption mtlOption({ "m", "metal" }, QLatin1String("Metal"));
    cmdLineParser.addOption(mtlOption);
    // Testing cleanup both with QWindow::close() (hitting X or Alt-F4) and
    // QCoreApplication::quit() (e.g. what a menu widget would do) is important.
    // Use this parameter for the latter.
    QCommandLineOption sdOption({ "s", "self-destruct" }, QLatin1String("Self-destruct after 5 seconds."));
    cmdLineParser.addOption(sdOption);
    // Attempt testing device lost situations on D3D at least.
    QCommandLineOption tdrOption(QLatin1String("curse"), QLatin1String("Curse the graphics device. "
                                                        "(generate a device reset every <count> frames when on D3D11)"),
                                 QLatin1String("count"));
    cmdLineParser.addOption(tdrOption);
    // Allow testing preferring the software adapter (D3D).
    QCommandLineOption swOption(QLatin1String("software"), QLatin1String("Prefer a software renderer when choosing the adapter. "
                                                                         "Only applicable with some APIs and platforms."));
    cmdLineParser.addOption(swOption);
    // Allow testing having a semi-transparent window.
    QCommandLineOption transparentOption(QLatin1String("transparent"), QLatin1String("Make background transparent"));
    cmdLineParser.addOption(transparentOption);

    cmdLineParser.process(app);
    if (cmdLineParser.isSet(nullOption))
        graphicsApi = Null;
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

    if (cmdLineParser.isSet(transparentOption)) {
        transparentBackground = true;
        scFlags |= QRhiSwapChain::SurfaceHasPreMulAlpha;
    }

#ifdef EXAMPLEFW_PREINIT
    void preInit();
    preInit();
#endif

    // OpenGL specifics.
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    if (sampleCount > 1)
        fmt.setSamples(sampleCount);
    if (scFlags.testFlag(QRhiSwapChain::NoVSync))
        fmt.setSwapInterval(0);
    if (scFlags.testFlag(QRhiSwapChain::sRGB))
        fmt.setColorSpace(QSurfaceFormat::sRGBColorSpace);
    // Exception: The alpha size is not necessarily OpenGL specific.
    if (transparentBackground)
        fmt.setAlphaBufferSize(8);
    QSurfaceFormat::setDefaultFormat(fmt);

    // Vulkan setup.
#if QT_CONFIG(vulkan)
    QVulkanInstance inst;
    if (graphicsApi == Vulkan) {
#ifndef Q_OS_ANDROID
        inst.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");
#else
        inst.setLayers(QByteArrayList()
                       << "VK_LAYER_GOOGLE_threading"
                       << "VK_LAYER_LUNARG_parameter_validation"
                       << "VK_LAYER_LUNARG_object_tracker"
                       << "VK_LAYER_LUNARG_core_validation"
                       << "VK_LAYER_LUNARG_image"
                       << "VK_LAYER_LUNARG_swapchain"
                       << "VK_LAYER_GOOGLE_unique_objects");
#endif
        inst.setExtensions(QByteArrayList()
                           << "VK_KHR_get_physical_device_properties2");
        if (!inst.create()) {
            qWarning("Failed to create Vulkan instance, switching to OpenGL");
            graphicsApi = OpenGL;
        }
    }
#endif

    if (cmdLineParser.isSet(tdrOption))
        framesUntilTdr = cmdLineParser.value(tdrOption).toInt();

    if (cmdLineParser.isSet(swOption))
        rhiFlags |= QRhi::PreferSoftwareRenderer;

    // Create and show the window.
    Window w;
#if QT_CONFIG(vulkan)
    if (graphicsApi == Vulkan)
        w.setVulkanInstance(&inst);
#endif
    w.resize(1280, 720);
    w.setTitle(QCoreApplication::applicationName() + QLatin1String(" - ") + graphicsApiName());
    w.show();

    if (cmdLineParser.isSet(sdOption))
        QTimer::singleShot(5000, qGuiApp, SLOT(quit()));

    int ret = app.exec();

    // Window::event() will not get invoked when the
    // PlatformSurfaceAboutToBeDestroyed event is sent during the QWindow
    // destruction. That happens only when exiting via app::quit() instead of
    // the more common QWindow::close(). Take care of it: if the
    // QPlatformWindow is still around (there was no close() yet), get rid of
    // the swapchain while it's not too late.
    if (w.handle())
        w.releaseSwapChain();

    return ret;
}
