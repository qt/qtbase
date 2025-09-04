// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qrhi_p.h"
#include <qmath.h>
#include <QLoggingCategory>
#include "private/qloggingregistry_p.h"

#include "qrhinull_p.h"
#ifndef QT_NO_OPENGL
#include "qrhigles2_p.h"
#endif
#if QT_CONFIG(vulkan)
#include "qrhivulkan_p.h"
#endif
#ifdef Q_OS_WIN
#include "qrhid3d11_p.h"
#include "qrhid3d12_p.h"
#endif
#if QT_CONFIG(metal)
#include "qrhimetal_p.h"
#endif

#include <memory>

QT_BEGIN_NAMESPACE

// Play nice with QSG_INFO since that is still the most commonly used
// way to get graphics info printed from Qt Quick apps, and the Quick
// scenegraph is our primary user.
Q_LOGGING_CATEGORY_WITH_ENV_OVERRIDE(QRHI_LOG_INFO, "QSG_INFO", "qt.rhi.general")

Q_LOGGING_CATEGORY(QRHI_LOG_RUB, "qt.rhi.rub")

/*!
    \class QRhi
    \ingroup painting-3D
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6

    \brief Accelerated 2D/3D graphics API abstraction.

    The Qt Rendering Hardware Interface is an abstraction for hardware accelerated
    graphics APIs, such as, \l{https://www.khronos.org/opengl/}{OpenGL},
    \l{https://www.khronos.org/opengles/}{OpenGL ES},
    \l{https://docs.microsoft.com/en-us/windows/desktop/direct3d}{Direct3D},
    \l{https://developer.apple.com/metal/}{Metal}, and
    \l{https://www.khronos.org/vulkan/}{Vulkan}.

    \warning The QRhi family of classes in the Qt Gui module, including QShader
    and QShaderDescription, offer limited compatibility guarantees. There are
    no source or binary compatibility guarantees for these classes, meaning the
    API is only guaranteed to work with the Qt version the application was
    developed against. Source incompatible changes are however aimed to be kept
    at a minimum and will only be made in minor releases (6.7, 6.8, and so on).
    To use these classes in an application, link to
    \c{Qt::GuiPrivate} (if using CMake), and include the headers with the \c
    rhi prefix, for example \c{#include <rhi/qrhi.h>}.

    Each QRhi instance is backed by a backend for a specific graphics API. The
    selection of the backend is a run time choice and is up to the application
    or library that creates the QRhi instance. Some backends are available on
    multiple platforms (OpenGL, Vulkan, Null), while APIs specific to a given
    platform are only available when running on the platform in question (Metal
    on macOS/iOS, Direct3D on Windows).

    The available backends currently are:

    \list

    \li OpenGL 2.1 / OpenGL ES 2.0 or newer. Some extensions and newer core
    specification features are utilized when present, for example to enable
    multisample framebuffers or compute shaders. Operating in core profile
    contexts is supported as well. If necessary, applications can query the
    \l{QRhi::Feature}{feature flags} at runtime to check for features that are
    not supported in the OpenGL context backing the QRhi. The OpenGL backend
    builds on QOpenGLContext, QOpenGLFunctions, and the related cross-platform
    infrastructure of the Qt GUI module.

    \li Direct3D 11.2 and newer (with DXGI 1.3 and newer), using Shader Model
    5.0 or newer. When the D3D runtime has no support for 11.2 features or
    Shader Model 5.0, initialization using an accelerated graphics device will
    fail, but using the
    \l{https://learn.microsoft.com/en-us/windows/win32/direct3darticles/directx-warp}{software
    adapter} is still an option.

    \li Direct3D 12 on Windows 10 version 1703 and newer, with Shader Model 5.0
    or newer. Qt requires ID3D12Device2 to be present, hence the requirement
    for at least version 1703 of Windows 10. The D3D12 device is by default
    created with specifying a minimum feature level of
    \c{D3D_FEATURE_LEVEL_11_0}.

    \li Metal 1.2 or newer.

    \li Vulkan 1.0 or newer, optionally utilizing some Vulkan 1.1 level
    features.

    \li Null, a "dummy" backend that issues no graphics calls at all.

    \endlist

    In order to allow shader code to be written once in Qt applications and
    libraries, all shaders are expected to be written in a single language
    which is then compiled into SPIR-V. Versions for various shading language
    are then generated from that, together with reflection information (inputs,
    outputs, shader resources). This is then packed into easily and efficiently
    serializable QShader instances. The compilers and tools to generate such
    shaders are not part of QRhi and the Qt GUI module, but the core classes
    for using such shaders, QShader and QShaderDescription, are. The APIs and
    tools for performing compilation and translation are part of the Qt Shader
    Tools module.

    See the \l{RHI Window Example} for an introductory example of creating a
    portable, cross-platform application that performs accelerated 3D rendering
    onto a QWindow using QRhi.

    \section1 An Impression of the API

    To provide a quick look at the API with a short yet complete example that
    does not involve window-related setup, the following is a complete,
    runnable cross-platform application that renders 20 frames off-screen, and
    then saves the generated images to files after reading back the texture
    contents from the GPU. For an example that renders on-screen, which then
    involves setting up a QWindow and a swapchain, refer to the
    \l{RHI Window Example}.

    For brevity, the initialization of the QRhi is done based on the platform:
    the sample code here chooses Direct 3D 12 on Windows, Metal on macOS and
    iOS, and Vulkan otherwise. OpenGL and Direct 3D 11 are never used by this
    application, but support for those could be introduced with a few
    additional lines.

    \snippet rhioffscreen/main.cpp 0

    The result of the application is 20 \c PNG images (frame0.png -
    frame19.png). These contain a rotating triangle with varying opacity over a
    green background.

    The vertex and fragment shaders are expected to be processed and packaged
    into \c{.qsb} files. The Vulkan-compatible GLSL source code is the
    following:

    \e color.vert
    \snippet rhioffscreen/color.vert 0

    \e color.frag
    \snippet rhioffscreen/color.frag 0

    To manually compile and transpile these shaders to a number of targets
    (SPIR-V, HLSL, MSL, GLSL) and generate the \c{.qsb} files the application
    loads at run time, run \c{qsb --qt6 color.vert -o color.vert.qsb} and
    \c{qsb --qt6 color.frag -o color.frag.qsb}. Alternatively, the Qt Shader
    Tools module offers build system integration for CMake, the
    \c qt_add_shaders() CMake function, that can achieve the same at build time.

    \section1 Security Considerations

    All data consumed by QRhi and related classes such as QShader are considered
    trusted content.

    \warning Application developers are advised to carefully consider the
    potential implications before allowing the feeding of user-provided content
    that is not part of the application and is not under the developers'
    control. (this includes all vertex/index data, shaders, pipeline and draw
    call parameters, etc.)

    \section1 Design Fundamentals

    A QRhi cannot be instantiated directly. Instead, use the create()
    function. Delete the QRhi instance normally to release the graphics device.

    \section2 Resources

    Instances of classes deriving from QRhiResource, such as, QRhiBuffer,
    QRhiTexture, etc., encapsulate zero, one, or more native graphics
    resources. Instances of such classes are always created via the \c new
    functions of the QRhi, such as, newBuffer(), newTexture(),
    newTextureRenderTarget(), newSwapChain().

    \code
        QRhiBuffer *vbuf = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData));
        if (!vbuf->create()) { error(); }
        // ...
        delete vbuf;
    \endcode

    \list

    \li The returned value from functions like newBuffer() is always owned by
    the caller.

    \li Just creating an instance of a QRhiResource subclass never allocates or
    initializes any native resources. That is only done when calling the
    \c create() function of a subclass, for example, QRhiBuffer::create() or
    QRhiTexture::create().

    \li The exceptions are
    QRhiTextureRenderTarget::newCompatibleRenderPassDescriptor(),
    QRhiSwapChain::newCompatibleRenderPassDescriptor(), and
    QRhiRenderPassDescriptor::newCompatibleRenderPassDescriptor(). There is no
    \c create() operation for these and the returned object is immediately
    active.

    \li The resource objects themselves are treated as immutable: once a
    resource has create() called, changing any parameters via the setters, such as,
    QRhiTexture::setPixelSize(), has no effect, unless the underlying native
    resource is released and \c create() is called again. See more about resource
    reuse in the sections below.

    \li The underlying native resources are scheduled for releasing by the
    QRhiResource destructor, or by calling QRhiResource::destroy(). Backends
    often queue release requests and defer executing them to an unspecified
    time, this is hidden from the applications. This way applications do not
    have to worry about releasing native resources that may still be in use by
    an in-flight frame.

    \li Note that this does not mean that a QRhiResource can freely be
    destroy()'ed or deleted within a frame (that is, in a
    \l{QRhi::beginFrame()}{beginFrame()} - \l{QRhi::endFrame()}{endFrame()}
    section). As a general rule, all referenced QRhiResource objects must stay
    unchanged until the frame is submitted by calling
    \l{QRhi::endFrame()}{endFrame()}. To ease this,
    QRhiResource::deleteLater() is provided as a convenience.

    \endlist

    \section2 Command buffers and deferred command execution

    Regardless of the design and capabilities of the underlying graphics API,
    all QRhi backends implement some level of command buffers. No
    QRhiCommandBuffer function issues any native bind or draw command (such as,
    \c glDrawElements) directly. Commands are always recorded in a queue,
    either native or provided by the QRhi backend. The command buffer is
    submitted, and so execution starts only upon QRhi::endFrame() or
    QRhi::finish().

    The deferred nature has consequences for some types of objects. For example,
    writing to a dynamic buffer multiple times within a frame, in case such
    buffers are backed by host-visible memory, will result in making the
    results of all writes are visible to all draw calls in the command buffer
    of the frame, regardless of when the dynamic buffer update was recorded
    relative to a draw call.

    Furthermore, instances of QRhiResource subclasses must be treated immutable
    within a frame in which they are referenced in any way. Create
    all resources upfront, before starting to record commands for the next
    frame. Reusing a QRhiResource instance within a frame (by calling \c create()
    then referencing it again in the same \c{beginFrame - endFrame} section)
    should be avoided as it may lead to unexpected results, depending on the
    backend.

    As a general rule, all referenced QRhiResource objects must stay valid and
    unmodified until the frame is submitted by calling
    \l{QRhi::endFrame()}{endFrame()}. On the other hand, calling
    \l{QRhiResource::destroy()}{destroy()} or deleting the QRhiResource are
    always safe once the frame is submitted, regardless of the status of the
    underlying native resources (which may still be in use by the GPU - but
    that is taken care of internally).

    Unlike APIs like OpenGL, upload and copy type of commands cannot be mixed
    with draw commands. The typical renderer will involve a sequence similar to
    the following:

    \list
    \li (re)create resources
    \li begin frame
    \li record/issue uploads and copies
    \li start recording a render pass
    \li record draw calls
    \li end render pass
    \li end frame
    \endlist

    Recording copy type of operations happens via QRhiResourceUpdateBatch. Such
    operations are committed typically on
    \l{QRhiCommandBuffer::beginPass()}{beginPass()}.

    When working with legacy rendering engines designed for OpenGL, the
    migration to QRhi often involves redesigning from having a single \c render
    step (that performs copies and uploads, clears buffers, and issues draw
    calls, all mixed together) to a clearly separated, two phase \c prepare -
    \c render setup where the \c render step only starts a renderpass and
    records draw calls, while all resource creation and queuing of updates,
    uploads and copies happens beforehand, in the \c prepare step.

    QRhi does not at the moment allow freely creating and submitting command
    buffers. This may be lifted in the future to some extent, in particular if
    compute support is introduced, but the model of well defined
    \c{frame-start} and \c{frame-end} points, combined with a dedicated,
    "frame" command buffer, where \c{frame-end} implies presenting, is going to
    remain the primary way of operating since this is what fits Qt's various UI
    technologies best.

    \section2 Threading

    A QRhi instance and the associated resources can be created and used on any
    thread but all usage must be limited to that one single thread. When
    rendering to multiple QWindows in an application, having a dedicated thread
    and QRhi instance for each window is often advisable, as this can eliminate
    issues with unexpected throttling caused by presenting to multiple windows.
    Conceptually that is then the same as how Qt Quick scene graph's threaded
    render loop operates when working directly with OpenGL: one thread for each
    window, one QOpenGLContext for each thread. When moving onto QRhi,
    QOpenGLContext is replaced by QRhi, making the migration straightforward.

    When it comes to externally created native objects, such as OpenGL contexts
    passed in via QRhiGles2NativeHandles, it is up to the application to ensure
    they are not misused by other threads.

    Resources are not shareable between QRhi instances. This is an intentional
    choice since QRhi hides most queue, command buffer, and resource
    synchronization related tasks, and provides no API for them. Safe and
    efficient concurrent use of graphics resources from multiple threads is
    tied to those concepts, however, and is thus a topic that is currently out
    of scope, but may be introduced in the future.

    \note The Metal backend requires that an autorelease pool is available on
    the rendering thread, ideally wrapping each iteration of the render loop.
    This needs no action from the users of QRhi when rendering on the main
    (gui) thread, but becomes important when a separate, dedicated render
    thread is used.

    \section2 Resource synchronization

    QRhi does not expose APIs for resource barriers or image layout
    transitions. Such synchronization is done implicitly by the backends, where
    applicable (for example, Vulkan), by tracking resource usage as necessary.
    Buffer and image barriers are inserted before render or compute passes
    transparently to the application.

    \note Resources within a render or compute pass are expected to be bound to
    a single usage during that pass. For example, a buffer can be used as
    vertex, index, uniform, or storage buffer, but not a combination of them
    within a single pass. However, it is perfectly fine to use a buffer as a
    storage buffer in a compute pass, and then as a vertex buffer in a render
    pass, for example, assuming the buffer declared both usages upon creation.

    \note Textures have this rule relaxed in certain cases, because using two
    subresources (typically two different mip levels) of the same texture for
    different access (one for load, one for store) is supported even within the
    same pass.

    \section2 Resource reuse

    From the user's point of view a QRhiResource is reusable immediately after
    calling QRhiResource::destroy(). With the exception of swapchains, calling
    \c create() on an already created object does an implicit \c destroy(). This
    provides a handy shortcut to reuse a QRhiResource instance with different
    parameters, with a new native graphics object underneath.

    The importance of reusing the same object lies in the fact that some
    objects reference other objects: for example, a QRhiShaderResourceBindings
    can reference QRhiBuffer, QRhiTexture, and QRhiSampler instances. If in a
    later frame one of these buffers need to be resized or a sampler parameter
    needs changing, destroying and creating a whole new QRhiBuffer or
    QRhiSampler would invalidate all references to the old instance. By just
    changing the appropriate parameters via QRhiBuffer::setSize() or similar
    and then calling QRhiBuffer::create(), everything works as expected and
    there is no need to touch the QRhiShaderResourceBindings at all, even
    though there is a good chance that under the hood the QRhiBuffer is now
    backed by a whole new native buffer.

    \code
        QRhiBuffer *ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 256);
        ubuf->create();

        QRhiShaderResourceBindings *srb = rhi->newShaderResourceBindings()
        srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, ubuf)
        });
        srb->create();

        // ...

        // now in a later frame we need to grow the buffer to a larger size
        ubuf->setSize(512);
        ubuf->create(); // same as ubuf->destroy(); ubuf->create();

        // srb needs no changes whatsoever, any references in it to ubuf
        // stay valid. When it comes to internal details, such as that
        // ubuf may now be backed by a completely different native buffer
        // resource, that is is recognized and handled automatically by the
        // next setShaderResources().
    \endcode

    QRhiTextureRenderTarget offers the same contract: calling
    QRhiCommandBuffer::beginPass() is safe even when one of the render target's
    associated textures or renderbuffers has been rebuilt (by calling \c
    create() on it) since the creation of the render target object. This allows
    the application to resize a texture by setting a new pixel size on the
    QRhiTexture and calling create(), thus creating a whole new native texture
    resource underneath, without having to update the QRhiTextureRenderTarget
    as that will be done implicitly in beginPass().

    \section2 Pooled objects

    In addition to resources, there are pooled objects as well, such as,
    QRhiResourceUpdateBatch. An instance is retrieved via a \c next function,
    such as, nextResourceUpdateBatch(). The caller does not own the returned
    instance in this case. The only valid way of operating here is calling
    functions on the QRhiResourceUpdateBatch and then passing it to
    QRhiCommandBuffer::beginPass() or QRhiCommandBuffer::endPass(). These
    functions take care of returning the batch to the pool. Alternatively, a
    batch can be "canceled" and returned to the pool without processing by
    calling QRhiResourceUpdateBatch::release().

    A typical pattern is thus:

    \code
        QRhiResourceUpdateBatch *resUpdates = rhi->nextResourceUpdateBatch();
        // ...
        resUpdates->updateDynamicBuffer(ubuf, 0, 64, mvp.constData());
        if (!image.isNull()) {
            resUpdates->uploadTexture(texture, image);
            image = QImage();
        }
        // ...
        QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
        // note the last argument
        cb->beginPass(swapchain->currentFrameRenderTarget(), clearCol, clearDs, resUpdates);
    \endcode

    \section2 Swapchain specifics

    QRhiSwapChain features some special semantics due to the peculiar nature of
    swapchains.

    \list

    \li It has no \c create() but rather a QRhiSwapChain::createOrResize().
    Repeatedly calling this function is \b not the same as calling
    QRhiSwapChain::destroy() followed by QRhiSwapChain::createOrResize(). This
    is because swapchains often have ways to handle the case where buffers need
    to be resized in a manner that is more efficient than a brute force
    destroying and recreating from scratch.

    \li An active QRhiSwapChain must be released by calling
    \l{QRhiSwapChain::destroy()}{destroy()}, or by destroying the object, before
    the QWindow's underlying QPlatformWindow, and so the associated native
    window object, is destroyed. It should not be postponed because releasing
    the swapchain may become problematic (and with some APIs, like Vulkan, is
    explicitly disallowed) when the native window is not around anymore, for
    example because the QPlatformWindow got destroyed upon getting a
    QWindow::close(). Therefore, releasing the swapchain must happen whenever
    the targeted QWindow sends the
    QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed event. If the event does
    not arrive before the destruction of the QWindow - this can happen when
    using QCoreApplication::quit() -, then check QWindow::handle() after the
    event loop exits and invoke the swapchain release when non-null (meaning
    the underlying native window is still around).

    \endlist

    \section2 Ownership

    The general rule is no ownership transfer. Creating a QRhi with an already
    existing graphics device does not mean the QRhi takes ownership of the
    device object. Similarly, ownership is not given away when a device or
    texture object is "exported" via QRhi::nativeHandles() or
    QRhiTexture::nativeTexture(). Most importantly, passing pointers in structs
    and via setters does not transfer ownership.

    \section1 Troubleshooting and Profiling

    \section2 Error reporting

    Functions such as \l QRhi::create() and the resource classes' \c create()
    member functions (e.g., \l QRhiBuffer::create()) indicate failure with the
    return value (\nullptr or
    \c false, respectively). When working with QShader, \l QShader::fromSerialized()
    returns an invalid QShader (for which \l{QShader::isValid()}{isValid()} returns
    \c false) when the data passed to the function cannot be successfully deserialized.
    Some functions, beginFrame() in particular, may also sometimes report "soft failures",
    such as \l FrameOpSwapChainOutOfDate, which do not indicate an unrecoverable error,
    but rather should be seen as a "try again later" response.

    Warnings and errors may get printed at any time to the debug output via
    qWarning(). It is therefore always advisable to inspect the output of the
    application.

    Additional debug messages can be enabled via the following logging
    categories. Messages from these categories are not printed by default
    unless explicitly enabled via QLoggingCategory or the \c QT_LOGGING_RULES
    environment variable. For better interoperation with Qt Quick, the
    environment variable \c{QSG_INFO} also enables these debug prints.

    \list
    \li \c{qt.rhi.general}
    \endlist

    Additionally, applications can query the \l{QRhi::backendName()}{QRhi
    backend name} and
    \l{QRhi::driverInfo()}{graphics device information} from a successfully
    initialized QRhi. This can then be printed to the user or stored in the
    application logs even in production builds, if desired.

    \section2 Investigating rendering problems

    When the rendering results are not as expected, or the application is
    experiencing problems, always consider checking with the the native 3D
    APIs' debug and validation facilities. QRhi itself features limited error
    checking since replicating the already existing, vast amount of
    functionality in the underlying layers is not reasonable.

    \list

    \li For Vulkan, controlling the
    \l{https://github.com/KhronosGroup/Vulkan-ValidationLayers}{Vulkan
    Validation Layers} is not in the scope of the QRhi, but rather can be
    achieved by configuring the \l QVulkanInstance with the appropriate layers.
    For example, call \c{instance.setLayers({ "VK_LAYER_KHRONOS_validation" });}
    before invoking \l{QVulkanInstance::create()}{create()} on the QVulkanInstance.
    (note that this assumes that the validation layers are actually installed
    and available, e.g. from the Vulkan SDK) By default, QVulkanInstance conveniently
    redirects the Vulkan debug messages to qDebug, meaning the validation messages get
    printed just like other Qt warnings.

    \li With Direct 3D 11 and 12, a graphics device with the debug layer
    enabled can be requested by toggling the \c enableDebugLayer flag in the
    appropriate \l{QRhiD3D11InitParams}{init params struct}. The messages appear on the
    debug output, which is visible in Qt Creator's messages panel or via a tool
    such as \l{https://learn.microsoft.com/en-us/sysinternals/downloads/debugview}{DebugView}.

    \li For Metal, controlling Metal Validation is outside of QRhi's scope.
    Rather, to enable validation, run the application with the environment
    variable \c{METAL_DEVICE_WRAPPER_TYPE=1} set, or run the application within
    XCode. There may also be further settings and environment variable in modern
    XCode and macOS versions. See for instance
    \l{https://developer.apple.com/documentation/metal/diagnosing_metal_programming_issues_early}{this
    page}.

    \endlist

    \section2 Frame captures and performance profiling

    A Qt application rendering with QRhi to a window while relying on a 3D API
    under the hood, is, from the windowing and graphics pipeline perspective at
    least, no different from any other (non-Qt) applications using the same 3D
    API. This means that tools and practices for debugging and profiling
    applications involving 3D graphics, such as games, all apply to such a Qt
    application as well.

    A few examples of tools that can provide insights into the rendering
    internals of Qt applications that use QRhi, which includes Qt Quick and Qt
    Quick 3D based projects as well:

    \list

    \li \l{https://renderdoc.org/}{RenderDoc} allows taking frame captures and
    introspecting the recorded commands and pipeline state on Windows and Linux
    for applications using OpenGL, Vulkan, D3D11, or D3D12. When trying to
    figure out why some parts of the 3D scene do not show up as expected,
    RenderDoc is often a fast and efficient way to check the pipeline stages
    and the related state and discover the missing or incorrect value. It is
    also a tool that is actively used when developing Qt itself.

    \li For NVIDIA-based systems,
    \l{https://developer.nvidia.com/nsight-graphics}{Nsight Graphics} provides
    a graphics debugger tool on Windows and Linux. In addition to investigating the commands
    in the frame and the pipeline, the vendor-specific tools allow looking at timings and
    hardware performance information, which is not something simple frame captures can provide.

    \li For AMD-based systems, the \l{https://gpuopen.com/rgp/}{Radeon GPU
    Profiler} can be used to gain deeper insights into the application's
    rendering and its performance.

    \li As QRhi supports Direct 3D 12, using
    \l{https://devblogs.microsoft.com/pix/download/}{PIX}, a performance tuning
    and debugging tool for DirectX 12 games on Windows is an option as well.

    \li On macOS,
    \l{https://developer.apple.com/documentation/metal/debugging_tools/viewing_your_gpu_workload_with_the_metal_debugger}{the
    XCode Metal debugger} can be used to take and introspect frame
    captures, to investigate performance details, and debug shaders. In macOS 13 it is also possible
    to enable an overlay that displays frame rate and other information for any Metal-based window by
    setting the environment variable \c{MTL_HUD_ENABLED=1}.

    \endlist

    On mobile and embedded platforms, there may be vendor and platform-specific
    tools, provided by the GPU or SoC vendor, available to perform performance
    profiling of application using OpenGL ES or Vulkan.

    When capturing frames, remember that objects and groups of commands can be
    named via debug markers, as long as \l{QRhi::EnableDebugMarkers}{debug
    markers were enabled} for the QRhi, and the graphics API in use supports
    this. To annotate the command stream, call
    \l{QRhiCommandBuffer::debugMarkBegin()}{debugMarkBegin()},
    \l{QRhiCommandBuffer::debugMarkEnd()}{debugMarkEnd()} and/or
    \l{QRhiCommandBuffer::debugMarkMsg()}{debugMarkMsg()}.
    This can be particularly useful in larger frames with multiple render passes.
    Resources are named by calling \l{QRhiResource::setName()}{setName()} before create().

    To perform basic timing measurements on the CPU and GPU side within the
    application, \l QElapsedTimer and
    \l QRhiCommandBuffer::lastCompletedGpuTime() can be used. The latter is
    only available with select graphics APIs at the moment and requires opting
    in via the \l QRhi::EnableTimestamps flag.

    \section2 Resource leak checking

    When destroying a QRhi object without properly destroying all buffers,
    textures, and other resources created from it, warnings about this are
    printed to the debug output whenever the application is a debug build, or
    when the \c QT_RHI_LEAK_CHECK environment variable is set to a non-zero
    value. This is a simple way to discover design issues around resource
    handling within the application rendering logic. Note however that some
    platforms and underlying graphics APIs may perform their own allocation and
    resource leak detection as well, over which Qt will have no direct control.
    For example, when using Vulkan, the memory allocator may raise failing
    assertions in debug builds when resources that own graphics memory
    allocations are not destroyed before the QRhi. In addition, the Vulkan
    validation layer, when enabled, will issue warnings about native graphics
    resources that were not released. Similarly, with Direct 3D warnings may
    get printed about unreleased COM objects when the application does not
    destroy the QRhi and its resources in the correct order.

    \sa {RHI Window Example}, QRhiCommandBuffer, QRhiResourceUpdateBatch,
    QRhiShaderResourceBindings, QShader, QRhiBuffer, QRhiTexture,
    QRhiRenderBuffer, QRhiSampler, QRhiTextureRenderTarget,
    QRhiGraphicsPipeline, QRhiComputePipeline, QRhiSwapChain
 */

/*!
    \enum QRhi::Implementation
    Describes which graphics API-specific backend gets used by a QRhi instance.

    \value Null
    \value Vulkan
    \value OpenGLES2
    \value D3D11
    \value D3D12
    \value Metal
 */

/*!
    \enum QRhi::Flag
    Describes what special features to enable.

    \value EnableDebugMarkers Enables debug marker groups. Without this frame
    debugging features like making debug groups and custom resource name
    visible in external GPU debugging tools will not be available and functions
    like QRhiCommandBuffer::debugMarkBegin() will become no-ops. Avoid enabling
    in production builds as it may involve a small performance impact. Has no
    effect when the QRhi::DebugMarkers feature is not reported as supported.

    \value EnableTimestamps Enables GPU timestamp collection. When not set,
    QRhiCommandBuffer::lastCompletedGpuTime() always returns 0. Enable this
    only when needed since there may be a small amount of extra work involved
    (e.g. timestamp queries), depending on the underlying graphics API. Has no
    effect when the QRhi::Timestamps feature is not reported as supported.

    \value PreferSoftwareRenderer Indicates that backends should prefer
    choosing an adapter or physical device that renders in software on the CPU.
    For example, with Direct3D there is typically a "Basic Render Driver"
    adapter available with \c{DXGI_ADAPTER_FLAG_SOFTWARE}. Setting this flag
    requests the backend to choose that adapter over any other, as long as no
    specific adapter was forced by other backend-specific means. With Vulkan
    this maps to preferring physical devices with
    \c{VK_PHYSICAL_DEVICE_TYPE_CPU}. When not available, or when it is not
    possible to decide if an adapter/device is software-based, this flag is
    ignored. It may also be ignored with graphics APIs that have no concept and
    means of enumerating adapters/devices.

    \value EnablePipelineCacheDataSave Enables retrieving the pipeline cache
    contents, where applicable. When not set, pipelineCacheData() will return
    an empty blob always. With backends where retrieving and restoring the
    pipeline cache contents is not supported, the flag has no effect and the
    serialized cache data is always empty. The flag provides an opt-in
    mechanism because the cost of maintaining the related data structures is
    not insignificant with some backends. With Vulkan this feature maps
    directly to VkPipelineCache, vkGetPipelineCacheData and
    VkPipelineCacheCreateInfo::pInitialData. With Direct3D 11 there is no real
    pipline cache, but the results of HLSL->DXBC compilations are stored and
    can be serialized/deserialized via this mechanism. This allows skipping the
    time consuming D3DCompile() in future runs of the applications for shaders
    that come with HLSL source instead of offline pre-compiled bytecode. This
    can provide a huge boost in startup and load times, if there is a lot of
    HLSL source compilation happening. With OpenGL the "pipeline cache" is
    simulated by retrieving and loading shader program binaries (if supported
    by the driver). With OpenGL there are additional, disk-based caching
    mechanisms for shader/program binaries provided by Qt. Writing to those may
    get disabled whenever this flag is set since storing program binaries to
    multiple caches is not sensible.

    \value SuppressSmokeTestWarnings Indicates that, with backends where this
    is relevant, certain, non-fatal QRhi::create() failures should not
    produce qWarning() calls. For example, with D3D11, passing this flag
    makes a number of warning messages (that appear due to QRhi::create()
    failing) to become categorized debug prints instead under the commonly used
    \c{qt.rhi.general} logging category. This can be used by engines, such as
    Qt Quick, that feature fallback logic, i.e. they retry calling create()
    with a different set of flags (such as, \l PreferSoftwareRenderer), in order
    to hide the unconditional warnings from the output that would be printed
    when the first create() attempt had failed.
 */

/*!
    \enum QRhi::FrameOpResult
    Describes the result of operations that can have a soft failure.

    \value FrameOpSuccess Success

    \value FrameOpError Unspecified error

    \value FrameOpSwapChainOutOfDate The swapchain is in an inconsistent state
    internally. This can be recoverable by attempting to repeat the operation
    (such as, beginFrame()) later.

    \value FrameOpDeviceLost The graphics device was lost. This can be
    recoverable by attempting to repeat the operation (such as, beginFrame())
    after releasing and reinitializing all objects backed by native graphics
    resources. See isDeviceLost().
 */

/*!
    \enum QRhi::Feature
    Flag values to indicate what features are supported by the backend currently in use.

    \value MultisampleTexture Indicates that textures with a sample count larger
    than 1 are supported. In practice this feature will be unsupported with
    OpenGL ES versions older than 3.1, and OpenGL older than 3.0.

    \value MultisampleRenderBuffer Indicates that renderbuffers with a sample
    count larger than 1 are supported. In practice this feature will be
    unsupported with OpenGL ES 2.0, and may also be unsupported with OpenGL 2.x
    unless the relevant extensions are present.

    \value DebugMarkers Indicates that debug marker groups (and so
    QRhiCommandBuffer::debugMarkBegin()) are supported.

    \value Timestamps Indicates that command buffer timestamps are supported.
    Relevant for QRhiCommandBuffer::lastCompletedGpuTime(). This can be
    expected to be supported on Metal, Vulkan, Direct 3D 11 and 12, and OpenGL
    contexts of version 3.3 or newer. However, with some of these APIs support
    for timestamp queries is technically optional, and therefore it cannot be
    guaranteed that this feature is always supported with every implementation
    of them.

    \value Instancing Indicates that instanced drawing is supported. In
    practice this feature will be unsupported with OpenGL ES 2.0 and OpenGL
    3.2 or older.

    \value CustomInstanceStepRate Indicates that instance step rates other
    than 1 are supported. In practice this feature will always be unsupported
    with OpenGL. In addition, running with Vulkan 1.0 without
    VK_EXT_vertex_attribute_divisor will also lead to reporting false for this
    feature.

    \value PrimitiveRestart Indicates that restarting the assembly of
    primitives when encountering an index value of 0xFFFF
    (\l{QRhiCommandBuffer::IndexUInt16}{IndexUInt16}) or 0xFFFFFFFF
    (\l{QRhiCommandBuffer::IndexUInt32}{IndexUInt32}) is enabled, for certain
    primitive topologies at least. QRhi will try to enable this with all
    backends, but in some cases it will not be supported. Dynamically
    controlling primitive restart is not possible since with some APIs
    primitive restart with a fixed index is always on. Applications must assume
    that whenever this feature is reported as supported, the above mentioned
    index values \c may be treated specially, depending on the topology. The
    only two topologies where primitive restart is guaranteed to behave
    identically across backends, as long as this feature is reported as
    supported, are \l{QRhiGraphicsPipeline::LineStrip}{LineStrip} and
    \l{QRhiGraphicsPipeline::TriangleStrip}{TriangleStrip}.

    \value NonDynamicUniformBuffers Indicates that creating buffers with the
    usage \l{QRhiBuffer::UniformBuffer}{UniformBuffer} and the types
    \l{QRhiBuffer::Immutable}{Immutable} or \l{QRhiBuffer::Static}{Static} is
    supported. When reported as unsupported, uniform (constant) buffers must be
    created as \l{QRhiBuffer::Dynamic}{Dynamic}. (which is recommended
    regardless)

    \value NonFourAlignedEffectiveIndexBufferOffset Indicates that effective
    index buffer offsets (\c{indexOffset + firstIndex * indexComponentSize})
    that are not 4 byte aligned are supported. When not supported, attempting
    to issue a \l{QRhiCommandBuffer::drawIndexed()}{drawIndexed()} with a
    non-aligned effective offset may lead to unspecified behavior. Relevant in
    particular for Metal, where this will be reported as unsupported.

    \value NPOTTextureRepeat Indicates that the
    \l{QRhiSampler::Repeat}{Repeat} wrap mode and mipmap filtering modes are
    supported for textures with a non-power-of-two size. In practice this can
    only be false with OpenGL ES 2.0 implementations without
    \c{GL_OES_texture_npot}.

    \value RedOrAlpha8IsRed Indicates that the
    \l{QRhiTexture::RED_OR_ALPHA8}{RED_OR_ALPHA8} format maps to a one
    component 8-bit \c red format. This is the case for all backends except
    OpenGL when using either OpenGL ES or a non-core profile context. There
    \c{GL_ALPHA}, a one component 8-bit \c alpha format, is used
    instead. Using the special texture format allows having a single code
    path for creating textures, leaving it up to the backend to decide the
    actual format, while the feature flag can be used to pick the
    appropriate shader variant for sampling the texture.

    \value ElementIndexUint Indicates that 32-bit unsigned integer elements are
    supported in the index buffer. In practice this is true everywhere except
    when running on plain OpenGL ES 2.0 implementations without the necessary
    extension. When false, only 16-bit unsigned elements are supported in the
    index buffer.

    \value Compute Indicates that compute shaders, image load/store, and
    storage buffers are supported. OpenGL older than 4.3 and OpenGL ES older
    than 3.1 have no compute support.

    \value WideLines Indicates that lines with a width other than 1 are
    supported. When reported as not supported, the line width set on the
    graphics pipeline state is ignored. This can always be false with some
    backends (D3D11, D3D12, Metal). With Vulkan, the value depends on the
    implementation. With OpenGL, wide lines are not supported in core profile
    contexts.

    \value VertexShaderPointSize Indicates that the size of rasterized points
    set via \c{gl_PointSize} in the vertex shader is taken into account. When
    reported as not supported, drawing points with a size other than 1 is not
    supported. Setting \c{gl_PointSize} in the shader is still valid then, but
    is ignored. (for example, when generating HLSL, the assignment is silently
    dropped from the generated code) Note that some APIs (Metal, Vulkan)
    require the point size to be set in the shader explicitly whenever drawing
    points, even when the size is 1, as they do not automatically default to 1.

    \value BaseVertex Indicates that
    \l{QRhiCommandBuffer::drawIndexed()}{drawIndexed()} supports the \c
    vertexOffset argument. When reported as not supported, the vertexOffset
    value in an indexed draw is ignored. In practice this feature will be
    unsupported with OpenGL and OpenGL ES versions lower than 3.2, and with
    Metal on older iOS devices, including the iOS Simulator.

    \value BaseInstance Indicates that instanced draw commands support the \c
    firstInstance argument. When reported as not supported, the firstInstance
    value is ignored and the instance ID starts from 0. In practice this feature
    will be unsupported with with Metal on older iOS devices, including the iOS
    Simulator, and all versions of OpenGL. The latter is due to OpenGL ES not
    supporting draw calls with a base instance at all. Currently QRhi's OpenGL
    backend does not implement the functionality for OpenGL (non-ES) either,
    because portable applications cannot rely on a non-zero base instance in
    practice due to GLES.

    \value TriangleFanTopology Indicates that QRhiGraphicsPipeline::setTopology()
    supports QRhiGraphicsPipeline::TriangleFan. In practice this feature will be
    unsupported with Metal and Direct 3D 11/12.

    \value ReadBackNonUniformBuffer Indicates that
    \l{QRhiResourceUpdateBatch::readBackBuffer()}{reading buffer contents} is
    supported for QRhiBuffer instances with a usage different than
    UniformBuffer. In practice this feature will be unsupported with OpenGL ES
    2.0.

    \value ReadBackNonBaseMipLevel Indicates that specifying a mip level other
    than 0 is supported when reading back texture contents. When not supported,
    specifying a non-zero level in QRhiReadbackDescription leads to returning
    an all-zero image. In practice this feature will be unsupported with OpenGL
    ES 2.0.

    \value TexelFetch Indicates that texelFetch() and textureLod() are available
    in shaders. In practice this will be reported as unsupported with OpenGL ES
    2.0 and OpenGL 2.x contexts, because GLSL 100 es and versions before 130 do
    not support these functions.

    \value RenderToNonBaseMipLevel Indicates that specifying a mip level other
    than 0 is supported when creating a QRhiTextureRenderTarget with a
    QRhiTexture as its color attachment. When not supported, create() will fail
    whenever the target mip level is not zero. In practice this feature will be
    unsupported with OpenGL ES 2.0.

    \value IntAttributes Indicates that specifying input attributes with
    signed and unsigned integer types for a shader pipeline is supported. When
    not supported, build() will succeed but just show a warning message and the
    values of the target attributes will be broken. In practice this feature
    will be unsupported with OpenGL ES 2.0 and OpenGL 2.x.

    \value ScreenSpaceDerivatives Indicates that functions such as dFdx(),
    dFdy(), and fwidth() are supported in shaders. In practice this feature will
    be unsupported with OpenGL ES 2.0 without the GL_OES_standard_derivatives
    extension.

    \value ReadBackAnyTextureFormat Indicates that reading back texture
    contents can be expected to work for any QRhiTexture::Format. Backends
    other than OpenGL can be expected to return true for this feature. When
    reported as false, which will typically happen with OpenGL, only the
    formats QRhiTexture::RGBA8 and QRhiTexture::BGRA8 are guaranteed to be
    supported for readbacks. In addition, with OpenGL, but not OpenGL ES,
    reading back the 1 byte per component formats QRhiTexture::R8 and
    QRhiTexture::RED_OR_ALPHA8 are supported as well. Reading back floating
    point formats QRhiTexture::RGBA16F and RGBA32F may work too with OpenGL, as
    long as the implementation provides support for these, but QRhi can give no
    guarantees, as indicated by this flag.

    \value PipelineCacheDataLoadSave Indicates that the pipelineCacheData() and
    setPipelineCacheData() functions are functional. When not supported, the
    functions will not perform any action, the retrieved blob is always empty,
    and thus no benefits can be expected from retrieving and, during a
    subsequent run of the application, reloading the pipeline cache content.

    \value ImageDataStride Indicates that specifying a custom stride (row
    length) for raw image data in texture uploads is supported. When not
    supported (which can happen when the underlying API is OpenGL ES 2.0 without
    support for GL_UNPACK_ROW_LENGTH),
    QRhiTextureSubresourceUploadDescription::setDataStride() must not be used.

    \value RenderBufferImport Indicates that QRhiRenderBuffer::createFrom() is
    supported. For most graphics APIs this is not sensible because
    QRhiRenderBuffer encapsulates texture objects internally, just like
    QRhiTexture. With OpenGL however, renderbuffer object exist as a separate
    object type in the API, and in certain environments (for example, where one
    may want to associated a renderbuffer object with an EGLImage object) it is
    important to allow wrapping an existing OpenGL renderbuffer object with a
    QRhiRenderBuffer.

    \value ThreeDimensionalTextures Indicates that 3D textures are supported.
    In practice this feature will be unsupported with OpenGL and OpenGL ES
    versions lower than 3.0.

    \value RenderTo3DTextureSlice Indicates that rendering to a slice in a 3D
    texture is supported. This can be unsupported with Vulkan 1.0 due to
    relying on VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT which is a Vulkan 1.1
    feature.

    \value TextureArrays Indicates that texture arrays are supported and
    QRhi::newTextureArray() is functional. Note that even when texture arrays
    are not supported, arrays of textures are still available as those are two
    independent features.

    \value Tessellation Indicates that the tessellation control and evaluation
    stages are supported. When reported as supported, the topology of a
    QRhiGraphicsPipeline can be set to
    \l{QRhiGraphicsPipeline::Patches}{Patches}, the number of control points
    can be set via
    \l{QRhiGraphicsPipeline::setPatchControlPointCount()}{setPatchControlPointCount()},
    and shaders for tessellation control and evaluation can be specified in the
    QRhiShaderStage list. Tessellation shaders have portability issues between
    APIs (for example, translating GLSL/SPIR-V to HLSL is problematic due to
    the way hull shaders are structured, whereas Metal uses a somewhat
    different tessellation pipeline than others), and therefore unexpected
    issues may still arise, even though basic functionality is implemented
    across all the underlying APIs. For Direct 3D in particular, handwritten
    HLSL hull and domain shaders must be injected into each QShader for the
    tessellation control and evaluation stages, respectively, since qsb cannot
    generate these from SPIR-V. Note that isoline tessellation should be
    avoided as it will not be supported by all backends. The maximum patch
    control point count portable between backends is 32.

    \value GeometryShader Indicates that the geometry shader stage is
    supported. When supported, a geometry shader can be specified in the
    QRhiShaderStage list. Geometry Shaders are considered an experimental
    feature in QRhi and can only be expected to be supported with Vulkan,
    Direct 3D, OpenGL (3.2+) and OpenGL ES (3.2+), assuming the implementation
    reports it as supported at run time. Geometry shaders have portability
    issues between APIs, and therefore no guarantees can be given for a
    universal solution. They will never be supported with Metal. Whereas with
    Direct 3D a handwritten HLSL geometry shader must be injected into each
    QShader for the geometry stage since qsb cannot generate this from SPIR-V.

    \value TextureArrayRange Indicates that for
    \l{QRhi::newTextureArray()}{texture arrays} it is possible to specify a
    range that is exposed to the shaders. Normally all array layers are exposed
    and it is up to the shader to select the layer (via the third coordinate
    passed to texture() when sampling the \c sampler2DArray). When supported,
    calling QRhiTexture::setArrayRangeStart() and
    QRhiTexture::setArrayRangeLength() before
    \l{QRhiTexture::create()}{building} or
    \l{QRhiTexture::createFrom()}{importing} the native texture has an effect,
    and leads to selecting only the specified range from the array. This will
    be necessary in special cases, such as when working with accelerated video
    decoding and Direct 3D 11, because a texture array with both
    \c{D3D11_BIND_DECODER} and \c{D3D11_BIND_SHADER_RESOURCE} on it is only
    usable as a shader resource if a single array layer is selected. Note that
    all this is applicable only when the texture is used as a
    QRhiShaderResourceBinding::SampledTexture or
    QRhiShaderResourceBinding::Texture shader resource, and is not compatible
    with image load/store. This feature is only available with some backends as
    it does not map well to all graphics APIs, and it is only meant to provide
    support for special cases anyhow. In practice the feature can be expected to
    be supported with Direct3D 11/12 and Vulkan.

    \value NonFillPolygonMode Indicates that setting a PolygonMode other than
    the default Fill is supported for QRhiGraphicsPipeline. A common use case
    for changing the mode to Line is to get wireframe rendering. This however
    is not available as a core OpenGL ES feature, and is optional with Vulkan
    as well as some mobile GPUs may not offer the feature.

    \value OneDimensionalTextures Indicates that 1D textures are supported.
    In practice this feature will be unsupported on OpenGL ES.

    \value OneDimensionalTextureMipmaps Indicates that generating 1D texture
    mipmaps are supported. In practice this feature will be unsupported on
    backends that do not report support for
    \l{OneDimensionalTextures}, Metal, and Direct 3D 12.

    \value HalfAttributes Indicates that specifying input attributes with half
    precision (16bit) floating point types for a shader pipeline is supported.
    When not supported, build() will succeed but just show a warning message
    and the values of the target attributes will be broken. In practice this
    feature will be unsupported in some OpenGL ES 2.0 and OpenGL 2.x
    implementations. Note that while Direct3D 11/12 does support half precision
    input attributes, it does not support the half3 type. The D3D backends pass
    half3 attributes as half4. To ensure cross platform compatibility, half3
    inputs should be padded to 8 bytes.

    \value RenderToOneDimensionalTexture Indicates that 1D texture render
    targets are supported. In practice this feature will be unsupported on
    backends that do not report support for
    \l{OneDimensionalTextures}, and Metal.

    \value ThreeDimensionalTextureMipmaps Indicates that generating 3D texture
    mipmaps are supported. This is typically supported with all backends starting
    with Qt 6.10.

    \value MultiView Indicates that multiview, see e.g.
    \l{https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_multiview.html}{VK_KHR_multiview}
    is supported. With OpenGL ES 2.0, Direct 3D 11, and OpenGL (ES)
    implementations without \c{GL_OVR_multiview2} this feature will not be
    supported. With Vulkan 1.1 and newer, and Direct 3D 12 multiview is
    typically supported. When reported as supported, creating a
    QRhiTextureRenderTarget with a QRhiColorAttachment that references a texture
    array and has \l{QRhiColorAttachment::setMultiViewCount()}{multiViewCount}
    set enables recording a render pass that uses multiview rendering. In addition,
    any QRhiGraphicsPipeline used in that render pass must have
    \l{QRhiGraphicsPipeline::setMultiViewCount()}{the same view count set}. Note that
    multiview is only available in combination with 2D texture arrays. It cannot
    be used to optimize the rendering into individual textures (e.g. two, for
    the left and right eyes). Rather, the target of a multiview render pass is
    always a texture array, automatically rendering to the layer (array element)
    corresponding to each view. Therefore this feature implies \l TextureArrays
    as well. Multiview rendering is not supported in combination with
    tessellation or geometry shaders. See QRhiColorAttachment::setMultiViewCount()
    for further details on multiview rendering. This enum value has been introduced in Qt 6.7.

    \value TextureViewFormat Indicates that setting a
    \l{QRhiTexture::setWriteViewFormat()}{view format} on a QRhiTexture is
    effective. When reported as supported, setting the read (sampling) or write
    (render target / image load-store) view mode changes the texture's viewing
    format. When unsupported, setting a view format has no effect. Note that Qt
    has no knowledge or control over format compatibility or resource view rules
    in the underlying 3D API and its implementation. Passing in unsuitable,
    incompatible formats may lead to errors and unspecified behavior. This is
    provided mainly to allow "casting" rendering into a texture created with an
    sRGB format to non-sRGB to avoid the unwanted linear->sRGB conversion on
    shader writes. Other types of casting may or may not be functional,
    depending on the underlying API. Currently implemented for Vulkan and Direct
    3D 12. With D3D12 the feature is available only if
    \c CastingFullyTypedFormatSupported is supported, see
    \l{https://microsoft.github.io/DirectX-Specs/d3d/RelaxedCasting.html} (and
    note that QRhi always uses fully typed formats for textures.) This enum
    value has been introduced in Qt 6.8.

    \value ResolveDepthStencil Indicates that resolving a multisample depth or
    depth-stencil texture is supported. Otherwise,
    \l{QRhiTextureRenderTargetDescription::setDepthResolveTexture()}{setting a
    depth resolve texture} is not functional and must be avoided. Direct 3D 11
    and 12 have no support for resolving depth/depth-stencil formats, and
    therefore this feature will never be supported with those. Vulkan 1.0 has no
    API to request resolving a depth-stencil attachment. Therefore, with Vulkan
    this feature will only be supported with Vulkan 1.2 and up, and on 1.1
    implementations with the appropriate extensions present. This feature is
    provided for the rare case when resolving into a non-multisample depth
    texture becomes necessary, for example when rendering into an
    OpenXR-provided depth texture (XR_KHR_composition_layer_depth). This enum
    value has been introduced in Qt 6.8.

    \value VariableRateShading Indicates that per-draw (per-pipeline) variable
    rate shading is supported. When reported as supported, \l
    QRhiCommandBuffer::setShadingRate() is functional and has an effect for
    QRhiGraphicsPipeline objects that declared \l
    QRhiGraphicsPipeline::UsesShadingRate in their flags. Call \l
    QRhi::supportedShadingRates() to check which rates are supported. (1x1 is
    always supported, other typical values are 2x2, 1x2, 2x1, 2x4, 4x2, 4x4).
    This feature can be expected to be supported with Direct 3D 12 and Vulkan,
    assuming the implementation and GPU used at run time supports VRS. This enum
    value has been introduced in Qt 6.9.

    \value VariableRateShadingMap Indicates that image-based specification of
    the shading rate is possible. The "image" is not necessarily a texture, it
    may be a native 3D API object, depending on the underlying backend and
    graphics API at run time. In practice this feature can be expected to be
    supported with Direct 3D 12, Vulkan, and Metal, assuming the GPU is modern
    enough to support VRS. To check if D3D12/Vulkan-style image-based VRS is
    suspported, use VariableRateShadingMapWithTexture instead. When this feature
    is reported as supported, there are two possibilities: when
    VariableRateShadingMapWithTexture is also true, then QRhiShadingRateMap
    consumes QRhiTexture objects via the createFrom() overload taking a
    QRhiTexture argument. When VariableRateShadingMapWithTexture is false, then
    QRhiShadingRateMap consumes some other type of native objects, for example
    an MTLRasterizationRateMap in case of Metal. Use the createFrom() overload
    taking a NativeShadingRateMap in this case. This enum value has been
    introduced in Qt 6.9.

    \value VariableRateShadingMapWithTexture Indicates that image-based
    specification of the shading rate is supported via regular textures. In
    practice this may be supported with Direct 3D 12 and Vulkan. This enum value
    has been introduced in Qt 6.9.

    \value PerRenderTargetBlending Indicates that per rendertarget blending is
    supported i.e. different render targets in MRT framebuffer can have different
    blending modes. In practice this can be expected to be supported everywhere
    except OpenGL ES, where it is only available with GLES 3.2 implementations.
    This enum value has been introduced in Qt 6.9.

    \value SampleVariables Indicates that gl_SampleID, gl_SamplePosition,
    gl_SampleMaskIn and gl_SampleMask variables are available in fragment shaders.
    In practice this can be expected to be supported everywhere except OpenGL ES,
    where it is only available with GLES 3.2 implementations.
    This enum value has been introduced in Qt 6.9.
 */

/*!
    \enum QRhi::BeginFrameFlag
    Flag values for QRhi::beginFrame()
 */

/*!
    \enum QRhi::EndFrameFlag
    Flag values for QRhi::endFrame()

    \value SkipPresent Specifies that no present command is to be queued or no
    swapBuffers call is to be made. This way no image is presented. Generating
    multiple frames with all having this flag set is not recommended (except,
    for example, for benchmarking purposes - but keep in mind that backends may
    behave differently when it comes to waiting for command completion without
    presenting so the results are not comparable between them)
 */

/*!
    \enum QRhi::ResourceLimit
    Describes the resource limit to query.

    \value TextureSizeMin Minimum texture width and height. This is typically
    1. The minimum texture size is handled gracefully, meaning attempting to
    create a texture with an empty size will instead create a texture with the
    minimum size.

    \value TextureSizeMax Maximum texture width and height. This depends on the
    graphics API and sometimes the platform or implementation as well.
    Typically the value is in the range 4096 - 16384. Attempting to create
    textures larger than this is expected to fail.

    \value MaxColorAttachments The maximum number of color attachments for a
    QRhiTextureRenderTarget, in case multiple render targets are supported. When
    MRT is not supported, the value is 1. Otherwise this is typically 8, but
    watch out for the fact that OpenGL only mandates 4 as the minimum, and that
    is what some OpenGL ES implementations provide.

    \value FramesInFlight The number of frames the backend may keep "in
    flight": with backends like Vulkan or Metal, it is the responsibility of
    QRhi to block whenever starting a new frame and finding the CPU is already
    \c{N - 1} frames ahead of the GPU (because the command buffer submitted in
    frame no. \c{current} - \c{N} has not yet completed). The value N is what
    is returned from here, and is typically 2. This can be relevant to
    applications that integrate rendering done directly with the graphics API,
    as such rendering code may want to perform double (if the value is 2)
    buffering for resources, such as, buffers, similarly to the QRhi backends
    themselves. The current frame slot index (a value running 0, 1, .., N-1,
    then wrapping around) is retrievable from QRhi::currentFrameSlot(). The
    value is 1 for backends where the graphics API offers no such low level
    control over the command submission process. Note that pipelining may still
    happen even when this value is 1 (some backends, such as D3D11, are
    designed to attempt to enable this, for instance, by using an update
    strategy for uniform buffers that does not stall the pipeline), but that is
    then not controlled by QRhi and so not reflected here in the API.

    \value MaxAsyncReadbackFrames The number of \l{QRhi::endFrame()}{submitted}
    frames (including the one that contains the readback) after which an
    asynchronous texture or buffer readback is guaranteed to complete upon
    \l{QRhi::beginFrame()}{starting a new frame}.

    \value MaxThreadGroupsPerDimension The maximum number of compute
    work/thread groups that can be dispatched. Effectively the maximum value
    for the arguments of QRhiCommandBuffer::dispatch(). Typically 65535.

    \value MaxThreadsPerThreadGroup The maximum number of invocations in a
    single local work group, or in other terminology, the maximum number of
    threads in a thread group. Effectively the maximum value for the product of
    \c local_size_x, \c local_size_y, and \c local_size_z in the compute
    shader. Typical values are 128, 256, 512, 1024, or 1536. Watch out that
    both OpenGL ES and Vulkan specify only 128 as the minimum required limit
    for implementations. While uncommon for Vulkan, some OpenGL ES 3.1
    implementations for mobile/embedded devices only support the spec-mandated
    minimum value.

    \value MaxThreadGroupX The maximum size of a work/thread group in the X
    dimension. Effectively the maximum value of \c local_size_x in the compute
    shader. Typically 256 or 1024.

    \value MaxThreadGroupY The maximum size of a work/thread group in the Y
    dimension. Effectively the maximum value of \c local_size_y in the compute
    shader. Typically 256 or 1024.

    \value MaxThreadGroupZ The maximum size of a work/thread group in the Z
    dimension. Effectively the maximum value of \c local_size_z in the compute
    shader. Typically 64 or 256.

    \value TextureArraySizeMax Maximum texture array size. Typically in range
    256 - 2048. Attempting to \l{QRhi::newTextureArray()}{create a texture
    array} with more elements will likely fail.

    \value MaxUniformBufferRange The number of bytes that can be exposed from a
    uniform buffer to the shaders at once. On OpenGL ES 2.0 and 3.0
    implementations this may be as low as 3584 bytes (224 four component, 32
    bits per component vectors). Elsewhere the value is typically 16384 (1024
    vec4s) or 65536 (4096 vec4s).

    \value MaxVertexInputs The number of input attributes to the vertex shader.
    The location in a QRhiVertexInputAttribute must be in range \c{[0,
    MaxVertexInputs-1]}. The value may be as low as 8 with OpenGL ES 2.0.
    Elsewhere, typical values are 16, 31, or 32.

    \value MaxVertexOutputs The maximum number of outputs (4 component vector
    \c out variables) from the vertex shader. The value may be as low as 8 with
    OpenGL ES 2.0, and 15 with OpenGL ES 3.0 and some Metal devices. Elsewhere,
    a typical value is 32.

    \value ShadingRateImageTileSize The tile size for shading rate textures. 0
    if the QRhi::VariableRateShadingMapWithTexture feature is not supported.
    Otherwise a value such as 16, indicating, for example, a tile size of 16x16.
    Each byte in the (R8UI) shading rate texture defines then the shading rate
    for a tile of 16x16 pixels. See \l QRhiShadingRateMap for details.
 */

/*!
    \class QRhiInitParams
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Base class for backend-specific initialization parameters.

    Contains fields that are relevant to all backends.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \class QRhiDepthStencilClearValue
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Specifies clear values for a depth or stencil buffer.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \fn QRhiDepthStencilClearValue::QRhiDepthStencilClearValue() = default

    Constructs a depth/stencil clear value with depth clear value 1.0f and
    stencil clear value 0.
 */

/*!
    Constructs a depth/stencil clear value with depth clear value \a d and
    stencil clear value \a s.
 */
QRhiDepthStencilClearValue::QRhiDepthStencilClearValue(float d, quint32 s)
    : m_d(d),
      m_s(s)
{
}

/*!
    \fn float QRhiDepthStencilClearValue::depthClearValue() const
    \return the depth clear value. In most cases this is 1.0f.
 */

/*!
    \fn void QRhiDepthStencilClearValue::setDepthClearValue(float d)
    Sets the depth clear value to \a d.
 */

/*!
    \fn quint32 QRhiDepthStencilClearValue::stencilClearValue() const
    \return the stencil clear value. In most cases this is 0.
 */

/*!
    \fn void QRhiDepthStencilClearValue::setStencilClearValue(quint32 s)
    Sets the stencil clear value to \a s.
 */

/*!
    \fn bool QRhiDepthStencilClearValue::operator==(const QRhiDepthStencilClearValue &a, const QRhiDepthStencilClearValue &b) noexcept

    \return \c true if the values in the two QRhiDepthStencilClearValue objects
    \a a and \a b are equal.
 */

/*!
    \fn bool QRhiDepthStencilClearValue::operator!=(const QRhiDepthStencilClearValue &a, const QRhiDepthStencilClearValue &b) noexcept

    \return \c false if the values in the two QRhiDepthStencilClearValue
    objects \a a and \a b are equal; otherwise returns \c true.

*/

/*!
    \fn size_t QRhiDepthStencilClearValue::qHash(const QRhiDepthStencilClearValue &key, size_t seed)
    \qhash{QRhiDepthStencilClearValue}
 */

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QRhiDepthStencilClearValue &v)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QRhiDepthStencilClearValue(depth-clear=" << v.depthClearValue()
                  << " stencil-clear=" << v.stencilClearValue()
                  << ')';
    return dbg;
}
#endif

/*!
    \class QRhiViewport
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Specifies a viewport rectangle.

    Used with QRhiCommandBuffer::setViewport().

    QRhi assumes OpenGL-style viewport coordinates, meaning x and y are
    bottom-left. Negative width or height are not allowed.

    Typical usage is like the following:

    \code
      const QSize outputSizeInPixels = swapchain->currentPixelSize();
      const QRhiViewport viewport(0, 0, outputSizeInPixels.width(), outputSizeInPixels.height());
      cb->beginPass(swapchain->currentFrameRenderTarget(), Qt::black, { 1.0f, 0 });
      cb->setGraphicsPipeline(ps);
      cb->setViewport(viewport);
      // ...
    \endcode

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    \sa QRhiCommandBuffer::setViewport(), QRhi::clipSpaceCorrMatrix(), QRhiScissor
 */

/*!
    \fn QRhiViewport::QRhiViewport() = default

    Constructs a viewport description with an empty rectangle and a depth range
    of 0.0f - 1.0f.

    \sa QRhi::clipSpaceCorrMatrix()
 */

/*!
    Constructs a viewport description with the rectangle specified by \a x, \a
    y, \a w, \a h and the depth range \a minDepth and \a maxDepth.

    \note \a x and \a y are assumed to be the bottom-left position. \a w and \a
    h should not be negative, the viewport will be ignored by
    QRhiCommandBuffer::setViewport() otherwise.

    \sa QRhi::clipSpaceCorrMatrix()
 */
QRhiViewport::QRhiViewport(float x, float y, float w, float h, float minDepth, float maxDepth)
    : m_rect { { x, y, w, h } },
      m_minDepth(minDepth),
      m_maxDepth(maxDepth)
{
}

/*!
    \fn std::array<float, 4> QRhiViewport::viewport() const
    \return the viewport x, y, width, and height.
 */

/*!
    \fn void QRhiViewport::setViewport(float x, float y, float w, float h)
    Sets the viewport's position and size to \a x, \a y, \a w, and \a h.

    \note Viewports are specified in a coordinate system that has its origin in
    the bottom-left.
 */

/*!
    \fn float QRhiViewport::minDepth() const
    \return the minDepth value of the depth range of the viewport.
 */

/*!
    \fn void QRhiViewport::setMinDepth(float minDepth)
    Sets the \a minDepth of the depth range of the viewport.
    By default this is set to 0.0f.
 */

/*!
    \fn float QRhiViewport::maxDepth() const
    \return the maxDepth value of the depth range of the viewport.
 */

/*!
    \fn void QRhiViewport::setMaxDepth(float maxDepth)
    Sets the \a maxDepth of the depth range of the viewport.
    By default this is set to 1.0f.
 */

/*!
    \fn bool QRhiViewport::operator==(const QRhiViewport &a, const QRhiViewport &b) noexcept

    \return \c true if the values in the two QRhiViewport objects
    \a a and \a b are equal.
 */

/*!
    \fn bool QRhiViewport::operator!=(const QRhiViewport &a, const QRhiViewport &b) noexcept

    \return \c false if the values in the two QRhiViewport
    objects \a a and \a b are equal; otherwise returns \c true.
*/

/*!
    \fn size_t QRhiViewport::qHash(const QRhiViewport &key, size_t seed)
    \qhash{QRhiViewport}
 */

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QRhiViewport &v)
{
    QDebugStateSaver saver(dbg);
    const std::array<float, 4> r = v.viewport();
    dbg.nospace() << "QRhiViewport(bottom-left-x=" << r[0]
                  << " bottom-left-y=" << r[1]
                  << " width=" << r[2]
                  << " height=" << r[3]
                  << " minDepth=" << v.minDepth()
                  << " maxDepth=" << v.maxDepth()
                  << ')';
    return dbg;
}
#endif

/*!
    \class QRhiScissor
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Specifies a scissor rectangle.

    Used with QRhiCommandBuffer::setScissor(). Setting a scissor rectangle is
    only possible with a QRhiGraphicsPipeline that has
    QRhiGraphicsPipeline::UsesScissor set.

    QRhi assumes OpenGL-style scissor coordinates, meaning x and y are
    bottom-left. Negative width or height are not allowed. However, apart from
    that, the flexible OpenGL semantics apply: negative x and y, partially out
    of bounds rectangles, etc. will be handled gracefully, clamping as
    appropriate. Therefore, any rendering logic targeting OpenGL can feed
    scissor rectangles into QRhiScissor as-is, without any adaptation.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    \sa QRhiCommandBuffer::setScissor(), QRhiViewport
 */

/*!
    \fn QRhiScissor::QRhiScissor() = default

    Constructs an empty scissor.
 */

/*!
    Constructs a scissor with the rectangle specified by \a x, \a y, \a w, and
    \a h.

    \note \a x and \a y are assumed to be the bottom-left position. Negative \a w
    or \a h are not allowed, such scissor rectangles will be ignored by
    QRhiCommandBuffer. Other than that, the flexible OpenGL semantics apply:
    negative x and y, partially out of bounds rectangles, etc. will be handled
    gracefully, clamping as appropriate.
 */
QRhiScissor::QRhiScissor(int x, int y, int w, int h)
    : m_rect { { x, y, w, h } }
{
}

/*!
    \fn std::array<int, 4> QRhiScissor::scissor() const
    \return the scissor position and size.
 */

/*!
    \fn void QRhiScissor::setScissor(int x, int y, int w, int h)
    Sets the scissor position and size to \a x, \a y, \a w, \a h.

    \note The position is always expected to be specified in a coordinate
    system that has its origin in the bottom-left corner, like OpenGL.
 */

/*!
    \fn bool QRhiScissor::operator==(const QRhiScissor &a, const QRhiScissor &b) noexcept

    \return \c true if the values in the two QRhiScissor objects
    \a a and \a b are equal.
 */

/*!
    \fn bool QRhiScissor::operator!=(const QRhiScissor &a, const QRhiScissor &b) noexcept

    \return \c false if the values in the two QRhiScissor
    objects \a a and \a b are equal; otherwise returns \c true.
*/

/*!
    \fn size_t QRhiScissor::qHash(const QRhiScissor &key, size_t seed)
    \qhash{QRhiScissor}
 */

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QRhiScissor &s)
{
    QDebugStateSaver saver(dbg);
    const std::array<int, 4> r = s.scissor();
    dbg.nospace() << "QRhiScissor(bottom-left-x=" << r[0]
                  << " bottom-left-y=" << r[1]
                  << " width=" << r[2]
                  << " height=" << r[3]
                  << ')';
    return dbg;
}
#endif

/*!
    \class QRhiVertexInputBinding
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Describes a vertex input binding.

    Specifies the stride (in bytes, must be a multiple of 4), the
    classification and optionally the instance step rate.

    As an example, assume a vertex shader with the following inputs:

    \badcode
        layout(location = 0) in vec4 position;
        layout(location = 1) in vec2 texcoord;
    \endcode

    Now let's assume also that 3 component vertex positions \c{(x, y, z)} and 2
    component texture coordinates \c{(u, v)} are provided in a non-interleaved
    format in a buffer (or separate buffers even). Defining two bindings
    could then be done like this:

    \code
        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({
            { 3 * sizeof(float) },
            { 2 * sizeof(float) }
        });
    \endcode

    Only the stride is interesting here since instancing is not used. The
    binding number is given by the index of the QRhiVertexInputBinding
    element in the bindings vector of the QRhiVertexInputLayout.

    Once a graphics pipeline with this vertex input layout is bound, the vertex
    inputs could be set up like the following for drawing a cube with 36
    vertices, assuming we have a single buffer with first the positions and
    then the texture coordinates:

    \code
        const QRhiCommandBuffer::VertexInput vbufBindings[] = {
            { cubeBuf, 0 },
            { cubeBuf, 36 * 3 * sizeof(float) }
        };
        cb->setVertexInput(0, 2, vbufBindings);
    \endcode

    Note how the index defined by \c {startBinding + i}, where \c i is the
    index in the second argument of
    \l{QRhiCommandBuffer::setVertexInput()}{setVertexInput()}, matches the
    index of the corresponding entry in the \c bindings vector of the
    QRhiVertexInputLayout.

    \note the stride must always be a multiple of 4.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    \sa QRhiCommandBuffer::setVertexInput()
 */

/*!
    \enum QRhiVertexInputBinding::Classification
    Describes the input data classification.

    \value PerVertex Data is per-vertex
    \value PerInstance Data is per-instance
 */

/*!
    \fn QRhiVertexInputBinding::QRhiVertexInputBinding() = default

    Constructs a default vertex input binding description.
 */

/*!
    Constructs a vertex input binding description with the specified \a stride,
    classification \a cls, and instance step rate \a stepRate.

    \note \a stepRate other than 1 is only supported when
    QRhi::CustomInstanceStepRate is reported to be supported.
 */
QRhiVertexInputBinding::QRhiVertexInputBinding(quint32 stride, Classification cls, quint32 stepRate)
    : m_stride(stride),
      m_classification(cls),
      m_instanceStepRate(stepRate)
{
}

/*!
    \fn quint32 QRhiVertexInputBinding::stride() const
    \return the stride in bytes.
 */

/*!
    \fn void QRhiVertexInputBinding::setStride(quint32 s)
    Sets the stride to \a s.
 */

/*!
    \fn QRhiVertexInputBinding::Classification QRhiVertexInputBinding::classification() const
    \return the input data classification.
 */

/*!
    \fn void QRhiVertexInputBinding::setClassification(Classification c)
    Sets the input data classification \a c. By default this is set to PerVertex.
 */

/*!
    \fn quint32 QRhiVertexInputBinding::instanceStepRate() const
    \return the instance step rate.
 */

/*!
    \fn void QRhiVertexInputBinding::setInstanceStepRate(quint32 rate)
    Sets the instance step \a rate. By default this is set to 1.
 */

/*!
    \fn bool QRhiVertexInputBinding::operator==(const QRhiVertexInputBinding &a, const QRhiVertexInputBinding &b) noexcept

    \return \c true if the values in the two QRhiVertexInputBinding objects
    \a a and \a b are equal.
 */

/*!
    \fn bool QRhiVertexInputBinding::operator!=(const QRhiVertexInputBinding &a, const QRhiVertexInputBinding &b) noexcept

    \return \c false if the values in the two QRhiVertexInputBinding
    objects \a a and \a b are equal; otherwise returns \c true.
*/

/*!
    \fn size_t QRhiVertexInputBinding::qHash(const QRhiVertexInputBinding &key, size_t seed)
    \qhash{QRhiVertexInputBinding}
 */

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QRhiVertexInputBinding &b)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QRhiVertexInputBinding(stride=" << b.stride()
                  << " cls=" << b.classification()
                  << " step-rate=" << b.instanceStepRate()
                  << ')';
    return dbg;
}
#endif

/*!
    \class QRhiVertexInputAttribute
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Describes a single vertex input element.

    The members specify the binding number, location, format, and offset for a
    single vertex input element.

    \note For HLSL it is assumed that the vertex shader translated from SPIR-V
    uses
    \c{TEXCOORD<location>} as the semantic for each input. Hence no separate
    semantic name and index.

    As an example, assume a vertex shader with the following inputs:

    \badcode
        layout(location = 0) in vec4 position;
        layout(location = 1) in vec2 texcoord;
    \endcode

    Now let's assume that we have 3 component vertex positions \c{(x, y, z)}
    and 2 component texture coordinates \c{(u, v)} are provided in a
    non-interleaved format in a buffer (or separate buffers even). Once two
    bindings are defined, the attributes could be specified as:

    \code
        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({
            { 3 * sizeof(float) },
            { 2 * sizeof(float) }
        });
        inputLayout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
            { 1, 1, QRhiVertexInputAttribute::Float2, 0 }
        });
    \endcode

    Once a graphics pipeline with this vertex input layout is bound, the vertex
    inputs could be set up like the following for drawing a cube with 36
    vertices, assuming we have a single buffer with first the positions and
    then the texture coordinates:

    \code
        const QRhiCommandBuffer::VertexInput vbufBindings[] = {
            { cubeBuf, 0 },
            { cubeBuf, 36 * 3 * sizeof(float) }
        };
        cb->setVertexInput(0, 2, vbufBindings);
    \endcode

    When working with interleaved data, there will typically be just one
    binding, with multiple attributes referring to that same buffer binding
    point:

    \code
        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({
            { 5 * sizeof(float) }
        });
        inputLayout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
            { 0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float) }
        });
    \endcode

    and then:

    \code
        const QRhiCommandBuffer::VertexInput vbufBinding(interleavedCubeBuf, 0);
        cb->setVertexInput(0, 1, &vbufBinding);
    \endcode

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    \sa QRhiCommandBuffer::setVertexInput()
 */

/*!
    \enum QRhiVertexInputAttribute::Format
    Specifies the type of the element data.

    \value Float4 Four component float vector
    \value Float3 Three component float vector
    \value Float2 Two component float vector
    \value Float Float
    \value UNormByte4 Four component normalized unsigned byte vector
    \value UNormByte2 Two component normalized unsigned byte vector
    \value UNormByte Normalized unsigned byte
    \value UInt4 Four component unsigned integer vector
    \value UInt3 Three component unsigned integer vector
    \value UInt2 Two component unsigned integer vector
    \value UInt Unsigned integer
    \value SInt4 Four component signed integer vector
    \value SInt3 Three component signed integer vector
    \value SInt2 Two component signed integer vector
    \value SInt Signed integer
    \value Half4 Four component half precision (16 bit) float vector
    \value Half3 Three component half precision (16 bit) float vector
    \value Half2 Two component half precision (16 bit) float vector
    \value Half Half precision (16 bit) float
    \value UShort4 Four component unsigned short (16 bit) integer vector
    \value UShort3 Three component unsigned short (16 bit) integer vector
    \value UShort2 Two component unsigned short (16 bit) integer vector
    \value UShort Unsigned short (16 bit) integer
    \value SShort4 Four component signed short (16 bit) integer vector
    \value SShort3 Three component signed short (16 bit) integer vector
    \value SShort2 Two component signed short (16 bit) integer vector
    \value SShort Signed short (16 bit) integer

    \note Support for half precision floating point attributes is indicated at
    run time by the QRhi::Feature::HalfAttributes feature flag.

    \note Direct3D 11/12 supports 16 bit input attributes, but does not support
    the Half3, UShort3 or SShort3 types. The D3D backends pass through Half3 as
    Half4, UShort3 as UShort4, and SShort3 as SShort4. To ensure cross platform
    compatibility, 16 bit inputs should be padded to 8 bytes.
 */

/*!
    \fn QRhiVertexInputAttribute::QRhiVertexInputAttribute() = default

    Constructs a default vertex input attribute description.
 */

/*!
    Constructs a vertex input attribute description with the specified \a
    binding number, \a location, \a format, and \a offset.

    \a matrixSlice should be -1 except when this attribute corresponds to a row
    or column of a matrix (for example, a 4x4 matrix becomes 4 vec4s, consuming
    4 consecutive vertex input locations), in which case it is the index of the
    row or column. \c{location - matrixSlice} must always be equal to the \c
    location for the first row or column of the unrolled matrix.
 */
QRhiVertexInputAttribute::QRhiVertexInputAttribute(int binding, int location, Format format, quint32 offset, int matrixSlice)
    : m_binding(binding),
      m_location(location),
      m_format(format),
      m_offset(offset),
      m_matrixSlice(matrixSlice)
{
}

/*!
    \fn int QRhiVertexInputAttribute::binding() const
    \return the binding point index.
 */

/*!
    \fn void QRhiVertexInputAttribute::setBinding(int b)
    Sets the binding point index to \a b.
    By default this is set to 0.
 */

/*!
    \fn int QRhiVertexInputAttribute::location() const
    \return the location of the vertex input element.
 */

/*!
    \fn void QRhiVertexInputAttribute::setLocation(int loc)
    Sets the location of the vertex input element to \a loc.
    By default this is set to 0.
 */

/*!
    \fn QRhiVertexInputAttribute::Format QRhiVertexInputAttribute::format() const
    \return the format of the vertex input element.
 */

/*!
    \fn void QRhiVertexInputAttribute::setFormat(Format f)
    Sets the format of the vertex input element to \a f.
    By default this is set to Float4.
 */

/*!
    \fn quint32 QRhiVertexInputAttribute::offset() const
    \return the byte offset for the input element.
 */

/*!
    \fn void QRhiVertexInputAttribute::setOffset(quint32 ofs)
    Sets the byte offset for the input element to \a ofs.
 */

/*!
    \fn int QRhiVertexInputAttribute::matrixSlice() const

    \return the matrix slice if the input element corresponds to a row or
    column of a matrix, or -1 if not relevant.
 */

/*!
    \fn void QRhiVertexInputAttribute::setMatrixSlice(int slice)

    Sets the matrix \a slice. By default this is set to -1, and should be set
    to a >= 0 value only when this attribute corresponds to a row or column of
    a matrix (for example, a 4x4 matrix becomes 4 vec4s, consuming 4
    consecutive vertex input locations), in which case it is the index of the
    row or column. \c{location - matrixSlice} must always be equal to the \c
    location for the first row or column of the unrolled matrix.
 */

/*!
    \fn bool QRhiVertexInputAttribute::operator==(const QRhiVertexInputAttribute &a, const QRhiVertexInputAttribute &b) noexcept

    \return \c true if the values in the two QRhiVertexInputAttribute objects
    \a a and \a b are equal.
 */

/*!
    \fn bool QRhiVertexInputAttribute::operator!=(const QRhiVertexInputAttribute &a, const QRhiVertexInputAttribute &b) noexcept

    \return \c false if the values in the two QRhiVertexInputAttribute
    objects \a a and \a b are equal; otherwise returns \c true.
*/

/*!
    \fn size_t QRhiVertexInputAttribute::qHash(const QRhiVertexInputAttribute &key, size_t seed)
    \qhash{QRhiVertexInputAttribute}
 */

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QRhiVertexInputAttribute &a)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QRhiVertexInputAttribute(binding=" << a.binding()
                  << " location=" << a.location()
                  << " format=" << a.format()
                  << " offset=" << a.offset()
                  << ')';
    return dbg;
}
#endif

QRhiVertexInputAttribute::Format QRhiImplementation::shaderDescVariableFormatToVertexInputFormat(QShaderDescription::VariableType type) const
{
    switch (type) {
    case QShaderDescription::Vec4:
        return QRhiVertexInputAttribute::Float4;
    case QShaderDescription::Vec3:
        return QRhiVertexInputAttribute::Float3;
    case QShaderDescription::Vec2:
        return QRhiVertexInputAttribute::Float2;
    case QShaderDescription::Float:
        return QRhiVertexInputAttribute::Float;

    case QShaderDescription::Int4:
        return QRhiVertexInputAttribute::SInt4;
    case QShaderDescription::Int3:
        return QRhiVertexInputAttribute::SInt3;
    case QShaderDescription::Int2:
        return QRhiVertexInputAttribute::SInt2;
    case QShaderDescription::Int:
        return QRhiVertexInputAttribute::SInt;

    case QShaderDescription::Uint4:
        return QRhiVertexInputAttribute::UInt4;
    case QShaderDescription::Uint3:
        return QRhiVertexInputAttribute::UInt3;
    case QShaderDescription::Uint2:
        return QRhiVertexInputAttribute::UInt2;
    case QShaderDescription::Uint:
        return QRhiVertexInputAttribute::UInt;

    case QShaderDescription::Half4:
        return QRhiVertexInputAttribute::Half4;
    case QShaderDescription::Half3:
        return QRhiVertexInputAttribute::Half3;
    case QShaderDescription::Half2:
        return QRhiVertexInputAttribute::Half2;
    case QShaderDescription::Half:
        return QRhiVertexInputAttribute::Half;

    default:
        Q_UNREACHABLE_RETURN(QRhiVertexInputAttribute::Float);
    }
}

quint32 QRhiImplementation::byteSizePerVertexForVertexInputFormat(QRhiVertexInputAttribute::Format format) const
{
    switch (format) {
    case QRhiVertexInputAttribute::Float4:
        return 4 * sizeof(float);
    case QRhiVertexInputAttribute::Float3:
        return 4 * sizeof(float); // vec3 still takes 16 bytes
    case QRhiVertexInputAttribute::Float2:
        return 2 * sizeof(float);
    case QRhiVertexInputAttribute::Float:
        return sizeof(float);

    case QRhiVertexInputAttribute::UNormByte4:
        return 4 * sizeof(quint8);
    case QRhiVertexInputAttribute::UNormByte2:
        return 2 * sizeof(quint8);
    case QRhiVertexInputAttribute::UNormByte:
        return sizeof(quint8);

    case QRhiVertexInputAttribute::UInt4:
        return 4 * sizeof(quint32);
    case QRhiVertexInputAttribute::UInt3:
        return 4 * sizeof(quint32); // ivec3 still takes 16 bytes
    case QRhiVertexInputAttribute::UInt2:
        return 2 * sizeof(quint32);
    case QRhiVertexInputAttribute::UInt:
        return sizeof(quint32);

    case QRhiVertexInputAttribute::SInt4:
        return 4 * sizeof(qint32);
    case QRhiVertexInputAttribute::SInt3:
        return 4 * sizeof(qint32); // uvec3 still takes 16 bytes
    case QRhiVertexInputAttribute::SInt2:
        return 2 * sizeof(qint32);
    case QRhiVertexInputAttribute::SInt:
        return sizeof(qint32);

    case QRhiVertexInputAttribute::Half4:
        return 4 * sizeof(qfloat16);
    case QRhiVertexInputAttribute::Half3:
        return 4 * sizeof(qfloat16); // half3 still takes 8 bytes
    case QRhiVertexInputAttribute::Half2:
        return 2 * sizeof(qfloat16);
    case QRhiVertexInputAttribute::Half:
        return sizeof(qfloat16);

    case QRhiVertexInputAttribute::UShort4:
        return 4 * sizeof(quint16);
    case QRhiVertexInputAttribute::UShort3:
        return 4 * sizeof(quint16); // ivec3 still takes 8 bytes
    case QRhiVertexInputAttribute::UShort2:
        return 2 * sizeof(quint16);
    case QRhiVertexInputAttribute::UShort:
        return sizeof(quint16);

    case QRhiVertexInputAttribute::SShort4:
        return 4 * sizeof(qint16);
    case QRhiVertexInputAttribute::SShort3:
        return 4 * sizeof(qint16); // uvec3 still takes 8 bytes
    case QRhiVertexInputAttribute::SShort2:
        return 2 * sizeof(qint16);
    case QRhiVertexInputAttribute::SShort:
        return sizeof(qint16);

    default:
        Q_UNREACHABLE_RETURN(1);
    }
}

/*!
    \class QRhiVertexInputLayout
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Describes the layout of vertex inputs consumed by a vertex shader.

    The vertex input layout is defined by the collections of
    QRhiVertexInputBinding and QRhiVertexInputAttribute.

    As an example, let's assume that we have a single buffer with 3 component
    vertex positions and 2 component UV coordinates interleaved (\c x, \c y, \c
    z, \c u, \c v), that the position and UV are expected at input locations 0
    and 1 by the vertex shader, and that the vertex buffer will be bound at
    binding point 0 using
    \l{QRhiCommandBuffer::setVertexInput()}{setVertexInput()} later on:

    \code
        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({
            { 5 * sizeof(float) }
        });
        inputLayout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
            { 0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float) }
        });
    \endcode

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \fn QRhiVertexInputLayout::QRhiVertexInputLayout() = default

    Constructs an empty vertex input layout description.
 */

/*!
    \fn void QRhiVertexInputLayout::setBindings(std::initializer_list<QRhiVertexInputBinding> list)
    Sets the bindings from the specified \a list.
 */

/*!
    \fn template<typename InputIterator> void QRhiVertexInputLayout::setBindings(InputIterator first, InputIterator last)
    Sets the bindings using the iterators \a first and \a last.
 */

/*!
    \fn const QRhiVertexInputBinding *QRhiVertexInputLayout::cbeginBindings() const
    \return a const iterator pointing to the first item in the binding list.
 */

/*!
    \fn const QRhiVertexInputBinding *QRhiVertexInputLayout::cendBindings() const
    \return a const iterator pointing just after the last item in the binding list.
 */

/*!
    \fn const QRhiVertexInputBinding *QRhiVertexInputLayout::bindingAt(qsizetype index) const
    \return the binding at the given \a index.
 */

/*!
    \fn qsizetype QRhiVertexInputLayout::bindingCount() const
    \return the number of bindings.
 */

/*!
    \fn void QRhiVertexInputLayout::setAttributes(std::initializer_list<QRhiVertexInputAttribute> list)
    Sets the attributes from the specified \a list.
 */

/*!
    \fn template<typename InputIterator> void QRhiVertexInputLayout::setAttributes(InputIterator first, InputIterator last)
    Sets the attributes using the iterators \a first and \a last.
 */

/*!
    \fn const QRhiVertexInputAttribute *QRhiVertexInputLayout::cbeginAttributes() const
    \return a const iterator pointing to the first item in the attribute list.
 */

/*!
    \fn const QRhiVertexInputAttribute *QRhiVertexInputLayout::cendAttributes() const
    \return a const iterator pointing just after the last item in the attribute list.
 */

/*!
    \fn const QRhiVertexInputAttribute *QRhiVertexInputLayout::attributeAt(qsizetype index) const
    \return the attribute at the given \a index.
 */

/*!
    \fn qsizetype QRhiVertexInputLayout::attributeCount() const
    \return the number of attributes.
 */

/*!
    \fn bool QRhiVertexInputLayout::operator==(const QRhiVertexInputLayout &a, const QRhiVertexInputLayout &b) noexcept

    \return \c true if the values in the two QRhiVertexInputLayout objects
    \a a and \a b are equal.
 */

/*!
    \fn bool QRhiVertexInputLayout::operator!=(const QRhiVertexInputLayout &a, const QRhiVertexInputLayout &b) noexcept

    \return \c false if the values in the two QRhiVertexInputLayout
    objects \a a and \a b are equal; otherwise returns \c true.
*/

/*!
    \fn size_t QRhiVertexInputLayout::qHash(const QRhiVertexInputLayout &key, size_t seed)
    \qhash{QRhiVertexInputLayout}
 */

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QRhiVertexInputLayout &v)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QRhiVertexInputLayout(bindings=" << v.m_bindings
                  << " attributes=" << v.m_attributes
                  << ')';
    return dbg;
}
#endif

/*!
    \class QRhiShaderStage
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Specifies the type and the shader code for a shader stage in the pipeline.

    When setting up a QRhiGraphicsPipeline, a collection of shader stages are
    specified. The QRhiShaderStage contains a QShader and some associated
    metadata, such as the graphics pipeline stage, and the
    \l{QShader::Variant}{shader variant} to select. There is no need to specify
    the shader language or version because the QRhi backend in use at runtime
    will take care of choosing the appropriate shader version from the
    collection within the QShader.

    The typical usage is in combination with
    QRhiGraphicsPipeline::setShaderStages(), shown here with a simple approach
    to load the QShader from \c{.qsb} files generated offline or at build time:

    \code
        QShader getShader(const QString &name)
        {
            QFile f(name);
            return f.open(QIODevice::ReadOnly) ? QShader::fromSerialized(f.readAll()) : QShader();
        }

        QShader vs = getShader("material.vert.qsb");
        QShader fs = getShader("material.frag.qsb");
        pipeline->setShaderStages({
            { QRhiShaderStage::Vertex, vs },
            { QRhiShaderStage::Fragment, fs }
        });
    \endcode

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \enum QRhiShaderStage::Type
    Specifies the type of the shader stage.

    \value Vertex Vertex stage

    \value TessellationControl Tessellation control (hull shader) stage. Must
    be used only when the QRhi::Tessellation feature is supported.

    \value TessellationEvaluation Tessellation evaluation (domain shader)
    stage. Must be used only when the QRhi::Tessellation feature is supported.

    \value Fragment Fragment (pixel shader) stage

    \value Compute Compute stage. Must be used only when the QRhi::Compute
    feature is supported.

    \value Geometry Geometry stage. Must be used only when the
    QRhi::GeometryShader feature is supported.
 */

/*!
    \fn QRhiShaderStage::QRhiShaderStage() = default

    Constructs a shader stage description for the vertex stage with an empty
    QShader.
 */

/*!
    \fn QRhiShaderStage::Type QRhiShaderStage::type() const
    \return the type of the stage.
 */

/*!
    \fn void QRhiShaderStage::setType(Type t)

    Sets the type of the stage to \a t. Setters should rarely be needed in
    pratice. Most applications will likely use the QRhiShaderStage constructor
    in most cases.
 */

/*!
    \fn QShader QRhiShaderStage::shader() const
    \return the QShader to be used for this stage in the graphics pipeline.
 */

/*!
    \fn void QRhiShaderStage::setShader(const QShader &s)
    Sets the shader collection \a s.
 */

/*!
    \fn QShader::Variant QRhiShaderStage::shaderVariant() const
    \return the requested shader variant.
 */

/*!
    \fn void QRhiShaderStage::setShaderVariant(QShader::Variant v)
    Sets the requested shader variant \a v.
 */

/*!
    Constructs a shader stage description with the \a type of the stage and the
    \a shader.

    The shader variant \a v defaults to QShader::StandardShader. A
    QShader contains multiple source and binary versions of a shader.
    In addition, it can also contain variants of the shader with slightly
    modified code. \a v can then be used to select the desired variant.
 */
QRhiShaderStage::QRhiShaderStage(Type type, const QShader &shader, QShader::Variant v)
    : m_type(type),
      m_shader(shader),
      m_shaderVariant(v)
{
}

/*!
    \fn bool QRhiShaderStage::operator==(const QRhiShaderStage &a, const QRhiShaderStage &b) noexcept

    \return \c true if the values in the two QRhiShaderStage objects
    \a a and \a b are equal.
 */

/*!
    \fn bool QRhiShaderStage::operator!=(const QRhiShaderStage &a, const QRhiShaderStage &b) noexcept

    \return \c false if the values in the two QRhiShaderStage
    objects \a a and \a b are equal; otherwise returns \c true.
*/

/*!
    \fn size_t QRhiShaderStage::qHash(const QRhiShaderStage &key, size_t seed)
    \qhash{QRhiShaderStage}
 */

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QRhiShaderStage &s)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QRhiShaderStage(type=" << s.type()
                  << " shader=" << s.shader()
                  << " variant=" << s.shaderVariant()
                  << ')';
    return dbg;
}
#endif

/*!
    \class QRhiColorAttachment
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Describes the a single color attachment of a render target.

    A color attachment is either a QRhiTexture or a QRhiRenderBuffer. The
    former, i.e. when texture() is set, is used in most cases.
    QRhiColorAttachment is commonly used in combination with
    QRhiTextureRenderTargetDescription.

    \note texture() and renderBuffer() cannot be both set (be non-null at the
    same time).

    Setting renderBuffer instead is recommended only when multisampling is
    needed. Relying on QRhi::MultisampleRenderBuffer is a better choice than
    QRhi::MultisampleTexture in practice since the former is available in more
    run time configurations (e.g. when running on OpenGL ES 3.0 which has no
    support for multisample textures, but does support multisample
    renderbuffers).

    When targeting a non-multisample texture, the layer() and level() indicate
    the targeted layer (face index \c{0-5} for cubemaps) and mip level. For 3D
    textures layer() specifies the slice (one 2D image within the 3D texture)
    to render to. For texture arrays layer() is the array index.

    When texture() or renderBuffer() is multisample, resolveTexture() can be
    set optionally. When set, samples are resolved automatically into that
    (non-multisample) texture at the end of the render pass. When rendering
    into a multisample renderbuffers, this is the only way to get resolved,
    non-multisample content out of them. Multisample textures allow sampling in
    shaders so for them this is just one option.

    \note when resolving is enabled, the multisample data may not be written
    out at all. This means that the multisample texture() must not be used
    afterwards with shaders for sampling when resolveTexture() is set.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    \sa QRhiTextureRenderTargetDescription
 */

/*!
    \fn QRhiColorAttachment::QRhiColorAttachment() = default

    Constructs an empty color attachment description.
 */

/*!
    Constructs a color attachment description that specifies \a texture as the
    associated color buffer.
 */
QRhiColorAttachment::QRhiColorAttachment(QRhiTexture *texture)
    : m_texture(texture)
{
}

/*!
    Constructs a color attachment description that specifies \a renderBuffer as
    the associated color buffer.
 */
QRhiColorAttachment::QRhiColorAttachment(QRhiRenderBuffer *renderBuffer)
    : m_renderBuffer(renderBuffer)
{
}

/*!
    \fn QRhiTexture *QRhiColorAttachment::texture() const

    \return the texture this attachment description references, or \nullptr if
    there is none.
 */

/*!
    \fn void QRhiColorAttachment::setTexture(QRhiTexture *tex)

    Sets the texture \a tex.

    \note texture() and renderBuffer() cannot be both set (be non-null at the
    same time).
 */

/*!
    \fn QRhiRenderBuffer *QRhiColorAttachment::renderBuffer() const

    \return the renderbuffer this attachment description references, or
    \nullptr if there is none.

    In practice associating a QRhiRenderBuffer with a QRhiColorAttachment makes
    the most sense when setting up multisample rendering via a multisample
    \l{QRhiRenderBuffer::Type}{color} renderbuffer that is then resolved into a
    non-multisample texture at the end of the render pass.
 */

/*!
    \fn void QRhiColorAttachment::setRenderBuffer(QRhiRenderBuffer *rb)

    Sets the renderbuffer \a rb.

    \note texture() and renderBuffer() cannot be both set (be non-null at the
    same time).
 */

/*!
    \fn int QRhiColorAttachment::layer() const
    \return the layer index (cubemap face or array layer). 0 by default.
 */

/*!
    \fn void QRhiColorAttachment::setLayer(int layer)
    Sets the \a layer index.
 */

/*!
    \fn int QRhiColorAttachment::level() const
    \return the mip level. 0 by default.
 */

/*!
    \fn void QRhiColorAttachment::setLevel(int level)
    Sets the mip \a level.
 */

/*!
    \fn QRhiTexture *QRhiColorAttachment::resolveTexture() const

    \return the resolve texture this attachment description references, or
    \nullptr if there is none.

    Setting a non-null resolve texture is applicable when the attachment
    references a multisample texture or renderbuffer. The QRhiTexture in the
    resolveTexture() is then a non-multisample 2D texture (or texture array)
    with the same size (but a sample count of 1). The multisample content is
    automatically resolved into this texture at the end of each render pass.
 */

/*!
    \fn void QRhiColorAttachment::setResolveTexture(QRhiTexture *tex)

    Sets the resolve texture \a tex.

    \a tex is expected to be a 2D texture or a 2D texture array. In either
    case, resolving targets a single mip level of a single layer (array
    element) of \a tex. The mip level and array layer are specified by
    resolveLevel() and resolveLayer().

    An exception is \l{setMultiViewCount()}{multiview}: when the color
    attachment is associated with a texture array and multiview is enabled, the
    resolve texture must also be a texture array with sufficient elements for
    all views. In this case all elements that correspond to views are resolved
    automatically; the behavior is similar to the following pseudo-code:
    \badcode
        for (i = 0; i < multiViewCount(); ++i)
            resolve texture's layer() + i into resolveTexture's resolveLayer() + i
    \endcode

    Setting a non-multisample texture to resolve a multisample texture or
    renderbuffer automatically at the end of the render pass is often
    preferable to working with multisample textures (and not setting a resolve
    texture), because it avoids the need for writing dedicated fragment shaders
    that work exclusively with multisample textures (\c sampler2DMS, \c
    texelFetch, etc.), and rather allows using the same shader as one would if
    the attachment's texture was not multisampled to begin with. This comes at
    the expense of an additional resource (the non-multisample \a tex).
 */

/*!
    \fn int QRhiColorAttachment::resolveLayer() const
    \return the currently set resolve texture layer. Defaults to 0.
 */

/*!
    \fn void QRhiColorAttachment::setResolveLayer(int layer)
    Sets the resolve texture \a layer to use.
 */

/*!
    \fn int QRhiColorAttachment::resolveLevel() const
    \return the currently set resolve texture mip level. Defaults to 0.
 */

/*!
    \fn void QRhiColorAttachment::setResolveLevel(int level)
    Sets the resolve texture mip \a level to use.
 */

/*!
    \fn int QRhiColorAttachment::multiViewCount() const

    \return the currently set number of views. Defaults to 0 which indicates
    the render target with this color attachment is not going to be used with
    multiview rendering.

    \since 6.7
 */

/*!
    \fn void QRhiColorAttachment::setMultiViewCount(int count)

    Sets the view \a count. Setting a value larger than 1 indicates that the
    render target with this color attachment is going to be used with multiview
    rendering. The default value is 0. Values smaller than 2 indicate no
    multiview rendering.

    When \a count is set to \c 2 or greater, the color attachment must be
    associated with a 2D texture array. layer() and multiViewCount() together
    define the range of texture array elements that are targeted during
    multiview rendering.

    For example, if \c layer is \c 0 and \c multiViewCount is \c 2, the texture
    array must have 2 (or more) elements, and the multiview rendering will
    target elements 0 and 1. The \c{gl_ViewIndex} variable in the shaders has a
    value of \c 0 or \c 1 then, where view \c 0 corresponds to the texture array
    element \c 0, and view \c 1 to the array element \c 1.

    \note Setting a \a count larger than 1, using a texture array as texture(),
    and calling \l{QRhiCommandBuffer::beginPass()}{beginPass()} on a
    QRhiTextureRenderTarget with this color attachment implies multiview
    rendering for the entire render pass. multiViewCount() should not be set
    unless multiview rendering is wanted. Multiview cannot be used with texture
    types other than 2D texture arrays. (although 3D textures may work,
    depending on the graphics API and backend; applications are nonetheless
    advised not to rely on that and only use 2D texture arrays as the render
    targets of multiview rendering)

    See
    \l{https://registry.khronos.org/OpenGL/extensions/OVR/OVR_multiview.txt}{GL_OVR_multiview}
    for more details regarding multiview rendering. Do note that Qt requires
    \l{https://registry.khronos.org/OpenGL/extensions/OVR/OVR_multiview2.txt}{GL_OVR_multiview2}
    as well, when running on OpenGL (ES).

    Multiview rendering is available only when the
    \l{QRhi::MultiView}{MultiView} feature is reported as supported from
    \l{QRhi::isFeatureSupported()}{isFeatureSupported()}.

    \note For portability, be aware of limitations that exist for multiview
    rendering with some of the graphics APIs. It is recommended that multiview
    render passes do not rely on any of the features that
    \l{https://registry.khronos.org/OpenGL/extensions/OVR/OVR_multiview.txt}{GL_OVR_multiview}
    declares as unsupported. The one exception is shader stage outputs other
    than \c{gl_Position} depending on \c{gl_ViewIndex}: that can be relied on
    (even with OpenGL) because QRhi never reports multiview as supported without
    \c{GL_OVR_multiview2} also being present.

    \note Multiview rendering is not supported in combination with tessellation
    or geometry shaders, even though some implementations of some graphics APIs
    may allow this.

    \since 6.7
 */

/*!
    \class QRhiTextureRenderTargetDescription
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Describes the color and depth or depth/stencil attachments of a render target.

    A texture render target has zero or more textures as color attachments,
    zero or one renderbuffer as combined depth/stencil buffer or zero or one
    texture as depth buffer.

    \note depthStencilBuffer() and depthTexture() cannot be both set (cannot be
    non-null at the same time).

    Let's look at some example usages in combination with
    QRhiTextureRenderTarget.

    Due to the constructors, the targeting a texture (and no depth/stencil
    buffer) is simple:

    \code
        QRhiTexture *texture = rhi->newTexture(QRhiTexture::RGBA8, QSize(256, 256), 1, QRhiTexture::RenderTarget);
        texture->create();
        QRhiTextureRenderTarget *rt = rhi->newTextureRenderTarget({ texture }));
    \endcode

    The following creates a texture render target that is set up to target mip
    level #2 of a texture:

    \code
        QRhiTexture *texture = rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 1, QRhiTexture::RenderTarget | QRhiTexture::MipMapped);
        texture->create();
        QRhiColorAttachment colorAtt(texture);
        colorAtt.setLevel(2);
        QRhiTextureRenderTarget *rt = rhi->newTextureRenderTarget({ colorAtt });
    \endcode

    Another example, this time to render into a depth texture:

    \code
        QRhiTexture *shadowMap = rhi->newTexture(QRhiTexture::D32F, QSize(1024, 1024), 1, QRhiTexture::RenderTarget);
        shadowMap->create();
        QRhiTextureRenderTargetDescription rtDesc;
        rtDesc.setDepthTexture(shadowMap);
        QRhiTextureRenderTarget *rt = rhi->newTextureRenderTarget(rtDesc);
    \endcode

    A very common case, having a texture as the color attachment and a
    renderbuffer as depth/stencil to enable depth testing:

    \code
        QRhiTexture *texture = rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 1, QRhiTexture::RenderTarget);
        texture->create();
        QRhiRenderBuffer *depthStencil = rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, QSize(512, 512));
        depthStencil->create();
        QRhiTextureRenderTargetDescription rtDesc({ texture }, depthStencil);
        QRhiTextureRenderTarget *rt = rhi->newTextureRenderTarget(rtDesc);
    \endcode

    Finally, to enable multisample rendering in a portable manner (so also
    supporting OpenGL ES 3.0), using a QRhiRenderBuffer as the (multisample)
    color buffer and then resolving into a regular (non-multisample) 2D
    texture. To enable depth testing, a depth-stencil buffer, which also must
    use the same sample count, is used as well:

    \code
        QRhiRenderBuffer *colorBuffer = rhi->newRenderBuffer(QRhiRenderBuffer::Color, QSize(512, 512), 4); // 4x MSAA
        colorBuffer->create();
        QRhiRenderBuffer *depthStencil = rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, QSize(512, 512), 4);
        depthStencil->create();
        QRhiTexture *texture = rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 1, QRhiTexture::RenderTarget);
        texture->create();
        QRhiColorAttachment colorAtt(colorBuffer);
        colorAtt.setResolveTexture(texture);
        QRhiTextureRenderTarget *rt = rhi->newTextureRenderTarget({ colorAtt, depthStencil });
    \endcode

    \note when multisample resolving is enabled, the multisample data may not be
    written out at all. This means that the multisample texture in a color
    attachment must not be used afterwards with shaders for sampling (or other
    purposes) whenever a resolve texture is set, since the multisample color
    buffer is merely an intermediate storage then that gets no data written back
    on some GPU architectures at all. See
    \l{QRhiTextureRenderTarget::Flag}{PreserveColorContents} for more details.

    \note When using setDepthTexture(), not setDepthStencilBuffer(), and the
    depth (stencil) data is not of interest afterwards, set the
    DoNotStoreDepthStencilContents flag on the QRhiTextureRenderTarget. This
    allows indicating to the underlying 3D API that the depth/stencil data can
    be discarded, leading potentially to better performance with tiled GPU
    architectures. When the depth-stencil buffer is a QRhiRenderBuffer (and also
    for the multisample color texture, see previous note) this is implicit, but
    with a depth (stencil) QRhiTexture the intention needs to be declared
    explicitly. By default QRhi assumes that the data is of interest (e.g., the
    depth texture is sampled in a shader afterwards).

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    \sa QRhiColorAttachment, QRhiTextureRenderTarget
 */

/*!
    \fn QRhiTextureRenderTargetDescription::QRhiTextureRenderTargetDescription() = default

    Constructs an empty texture render target description.
 */

/*!
    Constructs a texture render target description with one attachment
    described by \a colorAttachment.
 */
QRhiTextureRenderTargetDescription::QRhiTextureRenderTargetDescription(const QRhiColorAttachment &colorAttachment)
{
    m_colorAttachments.append(colorAttachment);
}

/*!
    Constructs a texture render target description with two attachments, a
    color attachment described by \a colorAttachment, and a depth/stencil
    attachment with \a depthStencilBuffer.
 */
QRhiTextureRenderTargetDescription::QRhiTextureRenderTargetDescription(const QRhiColorAttachment &colorAttachment,
                                                                       QRhiRenderBuffer *depthStencilBuffer)
    : m_depthStencilBuffer(depthStencilBuffer)
{
    m_colorAttachments.append(colorAttachment);
}

/*!
    Constructs a texture render target description with two attachments, a
    color attachment described by \a colorAttachment, and a depth attachment
    with \a depthTexture.

    \note \a depthTexture must have a suitable format, such as QRhiTexture::D16
    or QRhiTexture::D32F.
 */
QRhiTextureRenderTargetDescription::QRhiTextureRenderTargetDescription(const QRhiColorAttachment &colorAttachment,
                                                                       QRhiTexture *depthTexture)
    : m_depthTexture(depthTexture)
{
    m_colorAttachments.append(colorAttachment);
}

/*!
    \fn void QRhiTextureRenderTargetDescription::setColorAttachments(std::initializer_list<QRhiColorAttachment> list)
    Sets the \a list of color attachments.
 */

/*!
    \fn template<typename InputIterator> void QRhiTextureRenderTargetDescription::setColorAttachments(InputIterator first, InputIterator last)
    Sets the list of color attachments via the iterators \a first and \a last.
 */

/*!
    \fn const QRhiColorAttachment *QRhiTextureRenderTargetDescription::cbeginColorAttachments() const
    \return a const iterator pointing to the first item in the attachment list.
 */

/*!
    \fn const QRhiColorAttachment *QRhiTextureRenderTargetDescription::cendColorAttachments() const
    \return a const iterator pointing just after the last item in the attachment list.
 */

/*!
    \fn const QRhiColorAttachment *QRhiTextureRenderTargetDescription::colorAttachmentAt(qsizetype index) const
    \return the color attachment at the specified \a index.
 */

/*!
    \fn qsizetype QRhiTextureRenderTargetDescription::colorAttachmentCount() const
    \return the number of currently set color attachments.
 */

/*!
    \fn QRhiRenderBuffer *QRhiTextureRenderTargetDescription::depthStencilBuffer() const
    \return the renderbuffer used as depth-stencil buffer, or \nullptr if none was set.
 */

/*!
    \fn void QRhiTextureRenderTargetDescription::setDepthStencilBuffer(QRhiRenderBuffer *renderBuffer)

    Sets the \a renderBuffer for depth-stencil. Not mandatory, e.g. when no
    depth test/write or stencil-related features are used within any graphics
    pipelines in any of the render passes for this render target, it can be
    left set to \nullptr.

    \note depthStencilBuffer() and depthTexture() cannot be both set (cannot be
    non-null at the same time).

    Using a QRhiRenderBuffer over a 2D QRhiTexture as the depth or
    depth/stencil buffer is very common, and is the recommended approach for
    applications. Using a QRhiTexture, and so setDepthTexture() becomes
    relevant if the depth data is meant to be accessed (e.g. sampled in a
    shader) afterwards, or when
    \l{QRhiColorAttachment::setMultiViewCount()}{multiview rendering} is
    involved (because then the depth texture must be a texture array).

    \sa setDepthTexture()
 */

/*!
    \fn QRhiTexture *QRhiTextureRenderTargetDescription::depthTexture() const
    \return the currently referenced depth texture, or \nullptr if none was set.
 */

/*!
    \fn void QRhiTextureRenderTargetDescription::setDepthTexture(QRhiTexture *texture)

    Sets the \a texture for depth-stencil. This is an alternative to
    setDepthStencilBuffer(), where instead of a QRhiRenderBuffer a QRhiTexture
    with a suitable type (e.g., QRhiTexture::D32F) is provided.

    \note depthStencilBuffer() and depthTexture() cannot be both set (cannot be
    non-null at the same time).

    \a texture can either be a 2D texture or a 2D texture array (when texture
    arrays are supported). Specifying a texture array is relevant in particular
    with
    \l{QRhiColorAttachment::setMultiViewCount()}{multiview rendering}.

    \note If \a texture is a format with a stencil component, such as
    \l QRhiTexture::D24S8, it will serve as the stencil buffer as well.

    \sa setDepthStencilBuffer()
 */

/*!
    \fn QRhiTexture *QRhiTextureRenderTargetDescription::depthResolveTexture() const

    \return the texture to which a multisample depth (or depth-stencil) texture
    (or texture array) is resolved to. \nullptr if there is none, which is the
    most common case.

    \since 6.8
    \sa QRhiColorAttachment::resolveTexture(), depthTexture()
 */

/*!
    \fn void QRhiTextureRenderTargetDescription::setDepthResolveTexture(QRhiTexture *tex)

    Sets the depth (or depth-stencil) resolve texture \a tex.

    \a tex is expected to be a 2D texture or a 2D texture array with a format
    matching the texture set via setDepthTexture().

    \note Resolving depth (or depth-stencil) data is only functional when the
    \l QRhi::ResolveDepthStencil feature is reported as supported at run time.
    Support for depth-stencil resolve is not universally available among the
    graphics APIs. Designs assuming unconditional availability of depth-stencil
    resolve are therefore non-portable, and should be avoided.

    \note As an additional limitation for OpenGL ES in particular, setting a
    depth resolve texture may only be functional in combination with
    setDepthTexture(), not with setDepthStencilBuffer().

    \since 6.8
    \sa QRhiColorAttachment::setResolveTexture(), setDepthTexture()
 */

/*!
    \fn QRhiShadingRateMap *QRhiTextureRenderTargetDescription::shadingRateMap() const
    \return the currently set QRhiShadingRateMap. By default this is \nullptr.
    \since 6.9
 */

/*!
    \fn void QRhiTextureRenderTargetDescription::setShadingRateMap(QRhiShadingRateMap *map)

    Associates with the specified QRhiShadingRateMap \a map. This is functional
    only when the \l QRhi::VariableRateShadingMap feature is reported as
    supported.

    When QRhiCommandBuffer::setShadingRate() is also called, the higher of two
    the shading rates are used for each tile. There is currently no control
    offered over the combiner behavior.

    \note When the render target had already been built (create() was called
    successfully), setting a shading rate map implies that a different, new
    QRhiRenderPassDescriptor is needed and thus a rebuild is needed. Call
    setRenderPassDescriptor() again (outside of a render pass) and then rebuild
    by calling create(). This has other rolling consequences as well, for
    example for graphics pipelines: those also need to be associated with the
    new QRhiRenderPassDescriptor and then rebuilt. See \l
    QRhiRenderPassDescriptor::serializedFormat() for some suggestions on how to
    deal with this. Remember to set the QRhiGraphicsPipeline::UsesShadingRate
    flag as well.

    \since 6.9
 */

/*!
    \class QRhiTextureSubresourceUploadDescription
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Describes the source for one mip level in a layer in a texture upload operation.

    The source content is specified either as a QImage or as a raw blob. The
    former is only allowed for uncompressed textures with a format that can be
    mapped to QImage, while the latter is supported for all formats, including
    floating point and compressed.

    \note image() and data() cannot be both set at the same time.

    destinationTopLeft() specifies the top-left corner of the target
    rectangle. Defaults to (0, 0).

    An empty sourceSize() (the default) indicates that size is assumed to be
    the size of the subresource. With QImage-based uploads this implies that
    the size of the source image() must match the subresource. When providing
    raw data instead, sufficient number of bytes must be provided in data().

    sourceTopLeft() is supported only for QImage-based uploads, and specifies
    the top-left corner of the source rectangle.

    \note Setting sourceSize() or sourceTopLeft() may trigger a QImage copy
    internally, depending on the format and the backend.

    When providing raw data, and the stride is not specified via
    setDataStride(), the stride (row pitch, row length in bytes) of the
    provided data must be equal to \c{width * pixelSize} where \c pixelSize is
    the number of bytes used for one pixel, and there must be no additional
    padding between rows. There is no row start alignment requirement.

    When there is unused data at the end of each row in the input raw data,
    call setDataStride() with the total number of bytes per row. The stride
    must always be a multiple of the number of bytes for one pixel. The row
    stride is only applicable to image data for textures with an uncompressed
    format.

    \note The format of the source data must be compatible with the texture
    format. With many graphics APIs the data is copied as-is into a staging
    buffer, there is no intermediate format conversion provided by QRhi. This
    applies to floating point formats as well, with, for example, RGBA16F
    requiring half floats in the source data.

    \note Setting the stride via setDataStride() is only functional when
    QRhi::ImageDataStride is reported as
    \l{QRhi::isFeatureSupported()}{supported}. In practice this can be expected
    to be supported everywhere except for OpenGL ES 2.0.

    \note When a QImage is given, the stride returned from
    QImage::bytesPerLine() is taken into account automatically.

    \warning When a QImage is given and the QImage does not own the underlying
    pixel data, it is up to the caller to ensure that the associated data stays
    valid until the end of the frame. (just submitting the resource update batch
    is not sufficient, the data must stay valid until QRhi::endFrame() is called
    in order to be portable across all backends) If this cannot be ensured, the
    caller is strongly encouraged to call QImage::detach() on the image before
    passing it to uploadTexture().

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    \sa QRhiTextureUploadDescription
 */

/*!
    \fn QRhiTextureSubresourceUploadDescription::QRhiTextureSubresourceUploadDescription() = default

    Constructs an empty subresource description.

    \note an empty QRhiTextureSubresourceUploadDescription is not useful on its
    own and should not be submitted to a QRhiTextureUploadEntry. At minimum
    image or data must be set first.
 */

/*!
    Constructs a mip level description with a \a image.

    The \l{QImage::size()}{size} of \a image must match the size of the mip
    level. For level 0 that is the \l{QRhiTexture::pixelSize()}{texture size}.

    The bit depth of \a image must be compatible with the
    \l{QRhiTexture::Format}{texture format}.

    To describe a partial upload, call setSourceSize(), setSourceTopLeft(), or
    setDestinationTopLeft() afterwards.
 */
QRhiTextureSubresourceUploadDescription::QRhiTextureSubresourceUploadDescription(const QImage &image)
    : m_image(image)
{
}

/*!
    Constructs a mip level description with the image data is specified by \a
    data and \a size. This is suitable for floating point and compressed
    formats as well.

    \a data can safely be destroyed or changed once this function returns.
 */
QRhiTextureSubresourceUploadDescription::QRhiTextureSubresourceUploadDescription(const void *data, quint32 size)
    : m_data(reinterpret_cast<const char *>(data), size)
{
}

/*!
    Constructs a mip level description with the image data specified by \a
    data. This is suitable for floating point and compressed formats as well.
 */
QRhiTextureSubresourceUploadDescription::QRhiTextureSubresourceUploadDescription(const QByteArray &data)
    : m_data(data)
{
}

/*!
    \fn QImage QRhiTextureSubresourceUploadDescription::image() const
    \return the currently set QImage.
 */

/*!
    \fn void QRhiTextureSubresourceUploadDescription::setImage(const QImage &image)

    Sets \a image.
    Upon textures loading, the image data will be read as is, with no formats conversions.

    \note image() and data() cannot be both set at the same time.
 */

/*!
    \fn QByteArray QRhiTextureSubresourceUploadDescription::data() const
    \return the currently set raw pixel data.
 */

/*!
    \fn void QRhiTextureSubresourceUploadDescription::setData(const QByteArray &data)

    Sets \a data.

    \note image() and data() cannot be both set at the same time.
 */

/*!
    \fn quint32 QRhiTextureSubresourceUploadDescription::dataStride() const
    \return the currently set data stride.
 */

/*!
    \fn void QRhiTextureSubresourceUploadDescription::setDataStride(quint32 stride)

    Sets the data \a stride in bytes. By default this is 0 and not always
    relevant. When providing raw data(), and the stride is not specified via
    setDataStride(), the stride (row pitch, row length in bytes) of the
    provided data must be equal to \c{width * pixelSize} where \c pixelSize is
    the number of bytes used for one pixel, and there must be no additional
    padding between rows. Otherwise, if there is additional space between the
    lines, set a non-zero \a stride. All this is applicable only when raw image
    data is provided, and is not necessary when working QImage since that has
    its own \l{QImage::bytesPerLine()}{stride} value.

    \note Setting the stride via setDataStride() is only functional when
    QRhi::ImageDataStride is reported as
    \l{QRhi::isFeatureSupported()}{supported}.

    \note When a QImage is given, the stride returned from
    QImage::bytesPerLine() is taken into account automatically and therefore
    there is no need to set the data stride manually.
 */

/*!
    \fn QPoint QRhiTextureSubresourceUploadDescription::destinationTopLeft() const
    \return the currently set destination top-left position. Defaults to (0, 0).
 */

/*!
    \fn void QRhiTextureSubresourceUploadDescription::setDestinationTopLeft(const QPoint &p)
    Sets the destination top-left position \a p.
 */

/*!
    \fn QSize QRhiTextureSubresourceUploadDescription::sourceSize() const

    \return the source size in pixels. Defaults to a default-constructed QSize,
    which indicates the entire subresource.
 */

/*!
    \fn void QRhiTextureSubresourceUploadDescription::setSourceSize(const QSize &size)

    Sets the source \a size in pixels.

    \note Setting sourceSize() or sourceTopLeft() may trigger a QImage copy
    internally, depending on the format and the backend.
 */

/*!
    \fn QPoint QRhiTextureSubresourceUploadDescription::sourceTopLeft() const
    \return the currently set source top-left position. Defaults to (0, 0).
 */

/*!
    \fn void QRhiTextureSubresourceUploadDescription::setSourceTopLeft(const QPoint &p)

    Sets the source top-left position \a p.

    \note Setting sourceSize() or sourceTopLeft() may trigger a QImage copy
    internally, depending on the format and the backend.
 */

/*!
    \class QRhiTextureUploadEntry
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6

    \brief Describes one layer (face for cubemaps, slice for 3D textures,
    element for texture arrays) in a texture upload operation.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \fn QRhiTextureUploadEntry::QRhiTextureUploadEntry()

    Constructs an empty QRhiTextureUploadEntry targeting layer 0 and level 0.

    \note an empty QRhiTextureUploadEntry should not be submitted without
    setting a QRhiTextureSubresourceUploadDescription via setDescription()
    first.
 */

/*!
    Constructs a QRhiTextureUploadEntry targeting the given \a layer and mip
    \a level, with the subresource contents described by \a desc.
 */
QRhiTextureUploadEntry::QRhiTextureUploadEntry(int layer, int level,
                                               const QRhiTextureSubresourceUploadDescription &desc)
    : m_layer(layer),
      m_level(level),
      m_desc(desc)
{
}

/*!
    \fn int QRhiTextureUploadEntry::layer() const
    \return the currently set layer index (cubemap face, array layer). Defaults to 0.
 */

/*!
    \fn void QRhiTextureUploadEntry::setLayer(int layer)
    Sets the \a layer.
 */

/*!
    \fn int QRhiTextureUploadEntry::level() const
    \return the currently set mip level. Defaults to 0.
 */

/*!
    \fn void QRhiTextureUploadEntry::setLevel(int level)
    Sets the mip \a level.
 */

/*!
    \fn QRhiTextureSubresourceUploadDescription QRhiTextureUploadEntry::description() const
    \return the currently set subresource description.
 */

/*!
    \fn void QRhiTextureUploadEntry::setDescription(const QRhiTextureSubresourceUploadDescription &desc)
    Sets the subresource description \a desc.
 */

/*!
    \class QRhiTextureUploadDescription
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Describes a texture upload operation.

    Used with QRhiResourceUpdateBatch::uploadTexture(). That function has two
    variants: one taking a QImage and one taking a
    QRhiTextureUploadDescription. The former is a convenience version,
    internally creating a QRhiTextureUploadDescription with a single image
    targeting level 0 for layer 0.

    An example of the the common, simple case of wanting to upload the contents
    of a QImage to a QRhiTexture with a matching pixel size:

    \code
        QImage image(256, 256, QImage::Format_RGBA8888);
        image.fill(Qt::green); // or could use a QPainter targeting image
        QRhiTexture *texture = rhi->newTexture(QRhiTexture::RGBA8, QSize(256, 256));
        texture->create();
        QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
        u->uploadTexture(texture, image);
    \endcode

    When cubemaps, pre-generated mip images, compressed textures, or partial
    uploads are involved, applications will have to use this class instead.

    QRhiTextureUploadDescription also enables specifying batched uploads, which
    are useful for example when generating an atlas or glyph cache texture:
    multiple, partial uploads for the same subresource (meaning the same layer
    and level) are supported, and can be, depending on the backend and the
    underlying graphics API, more efficient when batched into the same
    QRhiTextureUploadDescription as opposed to issuing individual
    \l{QRhiResourceUpdateBatch::uploadTexture()}{uploadTexture()} commands for
    each of them.

    \note Cubemaps have one layer for each of the six faces in the order +X,
    -X, +Y, -Y, +Z, -Z.

    For example, specifying the faces of a cubemap could look like the following:

    \code
        QImage faces[6];
        // ...
        QVarLengthArray<QRhiTextureUploadEntry, 6> entries;
        for (int i = 0; i < 6; ++i)
          entries.append(QRhiTextureUploadEntry(i, 0, faces[i]));
        QRhiTextureUploadDescription desc;
        desc.setEntries(entries.cbegin(), entries.cend());
        resourceUpdates->uploadTexture(texture, desc);
    \endcode

    Another example that specifies mip images for a compressed texture:

    \code
        QList<QRhiTextureUploadEntry> entries;
        const int mipCount = rhi->mipLevelsForSize(compressedTexture->pixelSize());
        for (int level = 0; level < mipCount; ++level) {
            const QByteArray compressedDataForLevel = ..
            entries.append(QRhiTextureUploadEntry(0, level, compressedDataForLevel));
        }
        QRhiTextureUploadDescription desc;
        desc.setEntries(entries.cbegin(), entries.cend());
        resourceUpdates->uploadTexture(compressedTexture, desc);
    \endcode

    With partial uploads targeting the same subresource, it is recommended to
    batch them into a single upload request, whenever possible:

    \code
      QRhiTextureSubresourceUploadDescription subresDesc(image);
      subresDesc.setSourceSize(QSize(10, 10));
      subResDesc.setDestinationTopLeft(QPoint(50, 40));
      QRhiTextureUploadEntry entry(0, 0, subresDesc); // layer 0, level 0

      QRhiTextureSubresourceUploadDescription subresDesc2(image);
      subresDesc2.setSourceSize(QSize(30, 40));
      subResDesc2.setDestinationTopLeft(QPoint(100, 200));
      QRhiTextureUploadEntry entry2(0, 0, subresDesc2); // layer 0, level 0, i.e. same subresource

      QRhiTextureUploadDescription desc({ entry, entry2});
      resourceUpdates->uploadTexture(texture, desc);
    \endcode

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    \sa QRhiResourceUpdateBatch
 */

/*!
    \fn QRhiTextureUploadDescription::QRhiTextureUploadDescription()

    Constructs an empty texture upload description.
 */

/*!
    Constructs a texture upload description with a single subresource upload
    described by \a entry.
 */
QRhiTextureUploadDescription::QRhiTextureUploadDescription(const QRhiTextureUploadEntry &entry)
{
    m_entries.append(entry);
}

/*!
    Constructs a texture upload description with the specified \a list of entries.

    \note \a list can also contain multiple QRhiTextureUploadEntry elements
    with the same layer and level. This makes sense when those uploads are
    partial, meaning their subresource description has a source size or image
    smaller than the subresource dimensions, and can be more efficient than
    issuing separate uploadTexture()'s.
 */
QRhiTextureUploadDescription::QRhiTextureUploadDescription(std::initializer_list<QRhiTextureUploadEntry> list)
    : m_entries(list)
{
}

/*!
    \fn void QRhiTextureUploadDescription::setEntries(std::initializer_list<QRhiTextureUploadEntry> list)
    Sets the \a list of entries.
 */

/*!
    \fn template<typename InputIterator> void QRhiTextureUploadDescription::setEntries(InputIterator first, InputIterator last)
    Sets the list of entries using the iterators \a first and \a last.
 */

/*!
    \fn const QRhiTextureUploadEntry *QRhiTextureUploadDescription::cbeginEntries() const
    \return a const iterator pointing to the first item in the entry list.
 */

/*!
    \fn const QRhiTextureUploadEntry *QRhiTextureUploadDescription::cendEntries() const
    \return a const iterator pointing just after the last item in the entry list.
 */

/*!
    \fn const QRhiTextureUploadEntry *QRhiTextureUploadDescription::entryAt(qsizetype index) const
    \return the entry at \a index.
 */

/*!
    \fn qsizetype QRhiTextureUploadDescription::entryCount() const
    \return the number of entries.
 */

/*!
    \class QRhiTextureCopyDescription
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Describes a texture-to-texture copy operation.

    An empty pixelSize() indicates that the entire subresource is to be copied.
    A default constructed copy description therefore leads to copying the
    entire subresource at level 0 of layer 0.

    \note The source texture must be created with
    QRhiTexture::UsedAsTransferSource.

    \note The source and destination rectangles defined by pixelSize(),
    sourceTopLeft(), and destinationTopLeft() must fit the source and
    destination textures, respectively. The behavior is undefined otherwise.

    With cubemaps, 3D textures, and texture arrays one face or slice can be
    copied at a time. The face or slice is specified by the source and
    destination layer indices.  With mipmapped textures one mip level can be
    copied at a time. The source and destination layer and mip level indices can
    differ, but the size and position must be carefully controlled to avoid out
    of bounds copies, in which case the behavior is undefined.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \fn QRhiTextureCopyDescription::QRhiTextureCopyDescription()

    Constructs an empty texture copy description.
 */

/*!
    \fn QSize QRhiTextureCopyDescription::pixelSize() const
    \return the size of the region to copy.

    \note An empty pixelSize() indicates that the entire subresource is to be
    copied. A default constructed copy description therefore leads to copying
    the entire subresource at level 0 of layer 0.
 */

/*!
    \fn void QRhiTextureCopyDescription::setPixelSize(const QSize &sz)
    Sets the size of the region to copy to \a sz.
 */

/*!
    \fn int QRhiTextureCopyDescription::sourceLayer() const
    \return the source array layer (cubemap face or array layer index). Defaults to 0.
 */

/*!
    \fn void QRhiTextureCopyDescription::setSourceLayer(int layer)
    Sets the source array \a layer.
 */

/*!
    \fn int QRhiTextureCopyDescription::sourceLevel() const
    \return the source mip level. Defaults to 0.
 */

/*!
    \fn void QRhiTextureCopyDescription::setSourceLevel(int level)
    Sets the source mip \a level.
 */

/*!
    \fn QPoint QRhiTextureCopyDescription::sourceTopLeft() const
    \return the source top-left position (in pixels). Defaults to (0, 0).
 */

/*!
    \fn void QRhiTextureCopyDescription::setSourceTopLeft(const QPoint &p)
    Sets the source top-left position to \a p.
 */

/*!
    \fn int QRhiTextureCopyDescription::destinationLayer() const
    \return the destination array layer (cubemap face or array layer index). Default to 0.
 */

/*!
    \fn void QRhiTextureCopyDescription::setDestinationLayer(int layer)
    Sets the destination array \a layer.
 */

/*!
    \fn int QRhiTextureCopyDescription::destinationLevel() const
    \return the destionation mip level. Defaults to 0.
 */

/*!
    \fn void QRhiTextureCopyDescription::setDestinationLevel(int level)
    Sets the destination mip \a level.
 */

/*!
    \fn QPoint QRhiTextureCopyDescription::destinationTopLeft() const
    \return the destionation top-left position in pixels. Defaults to (0, 0).
 */

/*!
    \fn void QRhiTextureCopyDescription::setDestinationTopLeft(const QPoint &p)
    Sets the destination top-left position \a p.
 */

/*!
    \class QRhiReadbackDescription
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Describes a readback (reading back texture contents from possibly GPU-only memory) operation.

    The source of the readback operation is either a QRhiTexture or the
    current backbuffer of the currently targeted QRhiSwapChain. When
    texture() is not set, the swapchain is used. Otherwise the specified
    QRhiTexture is treated as the source.

    \note Textures used in readbacks must be created with
    QRhiTexture::UsedAsTransferSource.

    \note Swapchains used in readbacks must be created with
    QRhiSwapChain::UsedAsTransferSource.

    layer() and level() are only applicable when the source is a QRhiTexture.

    \note Multisample textures cannot be read back. Readbacks are supported for
    multisample swapchain buffers however.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \fn QRhiReadbackDescription::QRhiReadbackDescription() = default

    Constructs an empty texture readback description.

    \note The source texture is set to null by default, which is still a valid
    readback: it specifies that the backbuffer of the current swapchain is to
    be read back. (current meaning the frame's target swapchain at the time of
    committing the QRhiResourceUpdateBatch with the
    \l{QRhiResourceUpdateBatch::readBackTexture()}{texture readback} on it)
 */

/*!
    Constructs an texture readback description that specifies that level 0 of
    layer 0 of \a texture is to be read back.

    \note \a texture can also be null in which case this constructor is
    identical to the argumentless variant.
 */
QRhiReadbackDescription::QRhiReadbackDescription(QRhiTexture *texture)
    : m_texture(texture)
{
}

/*!
    \fn QRhiTexture *QRhiReadbackDescription::texture() const

    \return the QRhiTexture that is read back. Can be left set to \nullptr
    which indicates that the backbuffer of the current swapchain is to be used
    instead.
 */

/*!
    \fn void QRhiReadbackDescription::setTexture(QRhiTexture *tex)

    Sets the texture \a tex as the source of the readback operation.

    Setting \nullptr is valid too, in which case the current swapchain's
    current backbuffer is used. (but then the readback cannot be issued in a
    non-swapchain-based frame)

    \note Multisample textures cannot be read back. Readbacks are supported for
    multisample swapchain buffers however.

    \note Textures used in readbacks must be created with
    QRhiTexture::UsedAsTransferSource.

    \note Swapchains used in readbacks must be created with
    QRhiSwapChain::UsedAsTransferSource.
 */

/*!
    \fn int QRhiReadbackDescription::layer() const

    \return the currently set array layer (cubemap face, array index). Defaults to 0.

    Applicable only when the source of the readback is a QRhiTexture.
 */

/*!
    \fn void QRhiReadbackDescription::setLayer(int layer)
    Sets the array \a layer to read back.
 */

/*!
    \fn int QRhiReadbackDescription::level() const

    \return the currently set mip level. Defaults to 0.

    Applicable only when the source of the readback is a QRhiTexture.
 */

/*!
    \fn void QRhiReadbackDescription::setLevel(int level)
    Sets the mip \a level to read back.
 */

/*!
    \fn const QRect &QRhiReadbackDescription::rect() const
    \since 6.10

    \return the rectangle to read back. Defaults to an invalid rectangle.

    If invalid, the entire texture or swapchain backbuffer is read back.
 */

/*!
    \fn void QRhiReadbackDescription::setRect(const QRect &rectangle)
    \since 6.10

    Sets the \a rectangle to read back.
 */

/*!
    \class QRhiReadbackResult
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Describes the results of a potentially asynchronous buffer or texture readback operation.

    When \l completed is set, the function is invoked when the \l data is
    available. \l format and \l pixelSize are set upon completion together with
    \l data.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \variable QRhiReadbackResult::completed

    Callback that is invoked upon completion, on the thread the QRhi operates
    on. Can be left set to \nullptr, in which case no callback is invoked.
 */

/*!
    \variable QRhiReadbackResult::format

    Valid only for textures, the texture format.
 */

/*!
    \variable QRhiReadbackResult::pixelSize

    Valid only for textures, the size in pixels.
 */

/*!
    \variable QRhiReadbackResult::data

    The buffer or image data.

    \sa QRhiResourceUpdateBatch::readBackTexture(), QRhiResourceUpdateBatch::readBackBuffer()
 */


/*!
    \class QRhiNativeHandles
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Base class for classes exposing backend-specific collections of native resource objects.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \class QRhiResource
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Base class for classes encapsulating native resource objects.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \enum QRhiResource::Type
    Specifies type of the resource.

    \value Buffer
    \value Texture
    \value Sampler
    \value RenderBuffer
    \value RenderPassDescriptor
    \value SwapChainRenderTarget
    \value TextureRenderTarget
    \value ShaderResourceBindings
    \value GraphicsPipeline
    \value SwapChain
    \value ComputePipeline
    \value CommandBuffer
    \value ShadingRateMap
 */

/*!
    \fn virtual QRhiResource::Type QRhiResource::resourceType() const = 0

    \return the type of the resource.
 */

/*!
    \internal
 */
QRhiResource::QRhiResource(QRhiImplementation *rhi)
    : m_rhi(rhi)
{
    m_id = QRhiGlobalObjectIdGenerator::newId();
}

/*!
    Destructor.

    Releases (or requests deferred releasing of) the underlying native graphics
    resources, if there are any.

    \note Resources referenced by commands for the current frame should not be
    released until the frame is submitted by QRhi::endFrame().

    \sa destroy()
 */
QRhiResource::~QRhiResource()
{
    // destroy() cannot be called here, due to virtuals; it is up to the
    // subclasses to do that.
}

/*!
    \fn virtual void QRhiResource::destroy() = 0

    Releases (or requests deferred releasing of) the underlying native graphics
    resources. Safe to call multiple times, subsequent invocations will be a
    no-op then.

    Once destroy() is called, the QRhiResource instance can be reused, by
    calling \c create() again. That will then result in creating new native
    graphics resources underneath.

    \note Resources referenced by commands for the current frame should not be
    released until the frame is submitted by QRhi::endFrame().

    The QRhiResource destructor also performs the same task, so calling this
    function is not necessary before deleting a QRhiResource.

    \sa deleteLater()
 */

/*!
    When called without a frame being recorded, this function is equivalent to
    deleting the object. Between a QRhi::beginFrame() and QRhi::endFrame()
    however the behavior is different: the QRhiResource will not be destroyed
    until the frame is submitted via QRhi::endFrame(), thus satisfying the QRhi
    requirement of not altering QRhiResource objects that are referenced by the
    frame being recorded.

    If the QRhi that created this object is already destroyed, the object is
    deleted immediately.

    Using deleteLater() can be a useful convenience in many cases, and it
    complements the low-level guarantee (that the underlying native graphics
    objects are never destroyed until it is safe to do so and it is known for
    sure that they are not used by the GPU in an still in-flight frame), by
    offering a way to make sure the C++ object instances (of QRhiBuffer,
    QRhiTexture, etc.) themselves also stay valid until the end of the current
    frame.

    The following example shows a convenient way of creating a throwaway buffer
    that is only used in one frame and gets automatically released in
    endFrame(). (when it comes to the underlying native buffer(s), the usual
    guarantee applies: the QRhi backend defers the releasing of those until it
    is guaranteed that the frame in which the buffer is accessed by the GPU has
    completed)

    \code
        rhi->beginFrame(swapchain);
        QRhiBuffer *buf = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, 256);
        buf->deleteLater(); // !
        u = rhi->nextResourceUpdateBatch();
        u->uploadStaticBuffer(buf, data);
        // ... draw with buf
        rhi->endFrame();
    \endcode

    \sa destroy()
 */
void QRhiResource::deleteLater()
{
    if (m_rhi)
        m_rhi->addDeleteLater(this);
    else
        delete this;
}

/*!
    \return the currently set object name. By default the name is empty.
 */
QByteArray QRhiResource::name() const
{
    return m_objectName;
}

/*!
    Sets a \a name for the object.

    This allows getting descriptive names for the native graphics
    resources visible in graphics debugging tools, such as
    \l{https://renderdoc.org/}{RenderDoc} and
    \l{https://developer.apple.com/xcode/}{XCode}.

    When it comes to naming native objects by relaying the name via the
    appropriate graphics API, note that the name is ignored when
    QRhi::DebugMarkers are not supported, and may, depending on the backend,
    also be ignored when QRhi::EnableDebugMarkers is not set.

    \note The name may be ignored for objects other than buffers,
    renderbuffers, and textures, depending on the backend.

    \note The name may be modified. For slotted resources, such as a QRhiBuffer
    backed by multiple native buffers, QRhi will append a suffix to make the
    underlying native buffers easily distinguishable from each other.
 */
void QRhiResource::setName(const QByteArray &name)
{
    m_objectName = name;
}

/*!
    \return the global, unique identifier of this QRhiResource.

    User code rarely needs to deal with the value directly. It is used
    internally for tracking and bookkeeping purposes.
 */
quint64 QRhiResource::globalResourceId() const
{
    return m_id;
}

/*!
    \return the QRhi that created this resource.

    If the QRhi that created this object is already destroyed, the result is
    \nullptr.
 */
QRhi *QRhiResource::rhi() const
{
    return m_rhi ? m_rhi->q : nullptr;
}

/*!
    \class QRhiBuffer
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Vertex, index, or uniform (constant) buffer resource.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    A QRhiBuffer encapsulates zero, one, or more native buffer objects (such as
    a \c VkBuffer or \c MTLBuffer). With some graphics APIs and backends
    certain types of buffers may not use a native buffer object at all (e.g.
    OpenGL if uniform buffer objects are not used), but this is transparent to
    the user of the QRhiBuffer API. Similarly, the fact that some types of
    buffers may use two or three native buffers underneath, in order to allow
    efficient per-frame content update without stalling the GPU pipeline, is
    mostly invisible to the applications and libraries.

    A QRhiBuffer instance is always created by calling
    \l{QRhi::newBuffer()}{the QRhi's newBuffer() function}. This creates no
    native graphics resources. To do that, call create() after setting the
    appropriate options, such as the type, usage flags, size, although in most cases these
    are already set based on the arguments passed to
    \l{QRhi::newBuffer()}{newBuffer()}.

    \section2 Example usage

    To create a uniform buffer for a shader where the GLSL uniform block
    contains a single \c mat4 member, and update the contents:

    \code
        QRhiBuffer *ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64);
        if (!ubuf->create()) { error(); }
        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QMatrix4x4 mvp;
        // ... set up the modelview-projection matrix
        batch->updateDynamicBuffer(ubuf, 0, 64, mvp.constData());
        // ...
        commandBuffer->resourceUpdate(batch); // or, alternatively, pass 'batch' to a beginPass() call
    \endcode

    An example of creating a buffer with vertex data:

    \code
        const float vertices[] = { -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 1.0f };
        QRhiBuffer *vbuf = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertices));
        if (!vbuf->create()) { error(); }
        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        batch->uploadStaticBuffer(vbuf, vertices);
        // ...
        commandBuffer->resourceUpdate(batch); // or, alternatively, pass 'batch' to a beginPass() call
    \endcode

    An index buffer:

    \code
        static const quint16 indices[] = { 0, 1, 2 };
        QRhiBuffer *ibuf = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(indices));
        if (!ibuf->create()) { error(); }
        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        batch->uploadStaticBuffer(ibuf, indices);
        // ...
        commandBuffer->resourceUpdate(batch); // or, alternatively, pass 'batch' to a beginPass() call
    \endcode

    \section2 Common patterns

    A call to create() destroys any existing native resources if create() was
    successfully called before. If those native resources are still in use by
    an in-flight frame (i.e., there's a chance they are still read by the GPU),
    the destroying of those resources is deferred automatically. Thus a very
    common and convenient pattern to safely increase the size of an already
    initialized buffer is the following. In practice this drops and creates a
    whole new set of native resources underneath, so it is not necessarily a
    cheap operation, but is more convenient and still faster than the
    alternatives, because by not destroying the \c buf object itself, all
    references to it stay valid in other data structures (e.g., in any
    QRhiShaderResourceBinding the QRhiBuffer is referenced from).

    \code
        if (buf->size() < newSize) {
            buf->setSize(newSize);
            if (!buf->create()) { error(); }
        }
        // continue using buf, fill it with new data
    \endcode

    When working with uniform buffers, it will sometimes be necessary to
    combine data for multiple draw calls into a single buffer for efficiency
    reasons. Be aware of the aligment requirements: with some graphics APIs
    offsets for a uniform buffer must be aligned to 256 bytes. This applies
    both to QRhiShaderResourceBinding and to the dynamic offsets passed to
    \l{QRhiCommandBuffer::setShaderResources()}{setShaderResources()}. Use the
    \l{QRhi::ubufAlignment()}{ubufAlignment()} and
    \l{QRhi::ubufAligned()}{ubufAligned()} functions to create portable code.
    As an example, the following is an outline for issuing multiple (\c N) draw
    calls with the same pipeline and geometry, but with a different data in the
    uniform buffers exposed at binding point 0. This assumes the buffer is
    exposed via
    \l{QRhiShaderResourceBinding::uniformBufferWithDynamicOffset()}{uniformBufferWithDynamicOffset()}
    which allows passing a QRhiCommandBuffer::DynamicOffset list to
    \l{QRhiCommandBuffer::setShaderResources()}{setShaderResources()}.

    \code
        const int N = 2;
        const int UB_SIZE = 64 + 4; // assuming a uniform block with { mat4 matrix; float opacity; }
        const int ONE_UBUF_SIZE = rhi->ubufAligned(UB_SIZE);
        const int TOTAL_UBUF_SIZE = N * ONE_UBUF_SIZE;
        QRhiBuffer *ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, TOTAL_UBUF_SIZE);
        if (!ubuf->create()) { error(); }
        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        for (int i = 0; i < N; ++i) {
            batch->updateDynamicBuffer(ubuf, i * ONE_UBUF_SIZE, 64, matrix.constData());
            batch->updateDynamicBuffer(ubuf, i * ONE_UBUF_SIZE + 64, 4, &opacity);
        }
        // ...
        // beginPass(), set pipeline, etc., and then:
        for (int i = 0; i < N; ++i) {
            QRhiCommandBuffer::DynamicOffset dynOfs[] = { { 0, i * ONE_UBUF_SIZE } };
            cb->setShaderResources(srb, 1, dynOfs);
            cb->draw(36);
        }
    \endcode

    \sa QRhiResourceUpdateBatch, QRhi, QRhiCommandBuffer
 */

/*!
    \enum QRhiBuffer::Type
    Specifies storage type of buffer resource.

    \value Immutable Indicates that the data is not expected to change ever
    after the initial upload. Under the hood such buffer resources are
    typically placed in device local (GPU) memory (on systems where
    applicable). Uploading new data is possible, but may be expensive. The
    upload typically happens by copying to a separate, host visible staging
    buffer from which a GPU buffer-to-buffer copy is issued into the actual
    GPU-only buffer.

    \value Static Indicates that the data is expected to change only
    infrequently. Typically placed in device local (GPU) memory, where
    applicable. On backends where host visible staging buffers are used for
    uploading, the staging buffers are kept around for this type, unlike with
    Immutable, so subsequent uploads do not suffer in performance. Frequent
    updates, especially updates in consecutive frames, should be avoided.

    \value Dynamic Indicates that the data is expected to change frequently.
    Not recommended for large buffers. Typically backed by host visible memory
    in 2 copies in order to allow for changing without stalling the graphics
    pipeline. The double buffering is managed transparently to the applications
    and is not exposed in the API here in any form. This is the recommended,
    and, with some backends, the only possible, type for buffers with
    UniformBuffer usage.
 */

/*!
    \enum QRhiBuffer::UsageFlag
    Flag values to specify how the buffer is going to be used.

    \value VertexBuffer Vertex buffer. This allows the QRhiBuffer to be used in
    \l{QRhiCommandBuffer::setVertexInput()}{setVertexInput()}.

    \value IndexBuffer Index buffer. This allows the QRhiBuffer to be used in
    \l{QRhiCommandBuffer::setVertexInput()}{setVertexInput()}.

    \value UniformBuffer Uniform buffer (also called constant buffer). This
    allows the QRhiBuffer to be used in combination with
    \l{QRhiShaderResourceBinding::UniformBuffer}{UniformBuffer}. When
    \l{QRhi::NonDynamicUniformBuffers}{NonDynamicUniformBuffers} is reported as
    not supported, this usage can only be combined with the type Dynamic.

    \value StorageBuffer Storage buffer. This allows the QRhiBuffer to be used
    in combination with \l{QRhiShaderResourceBinding::BufferLoad}{BufferLoad},
    \l{QRhiShaderResourceBinding::BufferStore}{BufferStore}, or
    \l{QRhiShaderResourceBinding::BufferLoadStore}{BufferLoadStore}. This usage
    can only be combined with the types Immutable or Static, and is only
    available when the \l{QRhi::Compute}{Compute feature} is reported as
    supported.
 */

/*!
    \class QRhiBuffer::NativeBuffer
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \brief Contains information about the underlying native resources of a buffer.
 */

/*!
    \variable QRhiBuffer::NativeBuffer::objects
    \brief an array with pointers to the native object handles.

    With OpenGL, the native handle is a GLuint value, so the elements in the \c
    objects array are pointers to a GLuint. With Vulkan, the native handle is a
    VkBuffer, so the elements of the array are pointers to a VkBuffer. With
    Direct3D 11 and Metal the elements are pointers to a ID3D11Buffer or
    MTLBuffer pointer, respectively. With Direct3D 12, the elements are
    pointers to a ID3D12Resource.

    \note Pay attention to the fact that the elements are always pointers to
    the native buffer handle type, even if the native type itself is a pointer.
    (so the elements are \c{VkBuffer *} on Vulkan, even though VkBuffer itself
    is a pointer on 64-bit architectures).
 */

/*!
    \variable QRhiBuffer::NativeBuffer::slotCount
    \brief Specifies the number of valid elements in the objects array.

    The value can be 0, 1, 2, or 3 in practice. 0 indicates that the QRhiBuffer
    is not backed by any native buffer objects. This can happen with
    QRhiBuffers with the usage UniformBuffer when the underlying API does not
    support (or the backend chooses not to use) native uniform buffers. 1 is
    commonly used for Immutable and Static types (but some backends may
    differ). 2 or 3 is typical when the type is Dynamic (but some backends may
    differ).

    \sa QRhi::currentFrameSlot(), QRhi::FramesInFlight
 */

/*!
    \internal
 */
QRhiBuffer::QRhiBuffer(QRhiImplementation *rhi, Type type_, UsageFlags usage_, quint32 size_)
    : QRhiResource(rhi),
      m_type(type_), m_usage(usage_), m_size(size_)
{
}

/*!
    \return the resource type.
 */
QRhiResource::Type QRhiBuffer::resourceType() const
{
    return Buffer;
}

/*!
    \fn virtual bool QRhiBuffer::create() = 0

    Creates the corresponding native graphics resources. If there are already
    resources present due to an earlier create() with no corresponding
    destroy(), then destroy() is called implicitly first.

    \return \c true when successful, \c false when a graphics operation failed.
    Regardless of the return value, calling destroy() is always safe.
 */

/*!
    \fn QRhiBuffer::Type QRhiBuffer::type() const
    \return the buffer type.
 */

/*!
    \fn void QRhiBuffer::setType(Type t)
    Sets the buffer's type to \a t.
 */

/*!
    \fn QRhiBuffer::UsageFlags QRhiBuffer::usage() const
    \return the buffer's usage flags.
 */

/*!
    \fn void QRhiBuffer::setUsage(UsageFlags u)
    Sets the buffer's usage flags to \a u.
 */

/*!
    \fn quint32 QRhiBuffer::size() const

    \return the buffer's size in bytes.

    This is always the value that was passed to setSize() or QRhi::newBuffer().
    Internally, the native buffers may be bigger if that is required by the
    underlying graphics API.
 */

/*!
    \fn void QRhiBuffer::setSize(quint32 sz)

    Sets the size of the buffer in bytes. The size is normally specified in
    QRhi::newBuffer() so this function is only used when the size has to be
    changed. As with other setters, the size only takes effect when calling
    create(), and for already created buffers this involves releasing the previous
    native resource and creating new ones under the hood.

    Backends may choose to allocate buffers bigger than \a sz in order to
    fulfill alignment requirements. This is hidden from the applications and
    size() will always report the size requested in \a sz.
 */

/*!
    \return the underlying native resources for this buffer. The returned value
    will be empty if exposing the underlying native resources is not supported by
    the backend.

    A QRhiBuffer may be backed by multiple native buffer objects, depending on
    the type() and the QRhi backend in use. When this is the case, all of them
    are returned in the objects array in the returned struct, with slotCount
    specifying the number of native buffer objects. While
    \l{QRhi::beginFrame()}{recording a frame}, QRhi::currentFrameSlot() can be
    used to determine which of the native buffers QRhi is using for operations
    that read or write from this QRhiBuffer within the frame being recorded.

    In some cases a QRhiBuffer will not be backed by a native buffer object at
    all. In this case slotCount will be set to 0 and no valid native objects
    are returned. This is not an error, and is perfectly valid when a given
    backend does not use native buffers for QRhiBuffers with certain types or
    usages.

    \note Be aware that QRhi backends may employ various buffer update
    strategies. Unlike textures, where uploading image data always means
    recording a buffer-to-image (or similar) copy command on the command
    buffer, buffers, in particular Dynamic and UniformBuffer ones, can operate
    in many different ways. For example, a QRhiBuffer with usage type
    UniformBuffer may not even be backed by a native buffer object at all if
    uniform buffers are not used or supported by a given backend and graphics
    API. There are also differences to how data is written to the buffer and
    the type of backing memory used. For buffers backed by host visible memory,
    calling this function guarantees that pending host writes are executed for
    all the returned native buffers.

    \sa QRhi::currentFrameSlot(), QRhi::FramesInFlight
 */
QRhiBuffer::NativeBuffer QRhiBuffer::nativeBuffer()
{
    return { {}, 0 };
}

/*!
    \return a pointer to a memory block with the host visible buffer data.

    This is a shortcut for medium-to-large dynamic uniform buffers that have
    their \b entire contents (or at least all regions that are read by the
    shaders in the current frame) changed \b{in every frame} and the
    QRhiResourceUpdateBatch-based update mechanism is seen too heavy due to the
    amount of data copying involved.

    The call to this function must be eventually followed by a call to
    endFullDynamicUniformBufferUpdateForCurrentFrame(), before recording any
    render or compute pass that relies on this buffer.

    \warning Updating data via this method is not compatible with
    QRhiResourceUpdateBatch-based updates and readbacks. Unexpected behavior
    may occur when attempting to combine the two update models for the same
    buffer. Similarly, the data updated this direct way may not be visible to
    \l{QRhiResourceUpdateBatch::readBackBuffer()}{readBackBuffer operations},
    depending on the backend.

    \warning When updating buffer data via this method, the update must be done
    in every frame, otherwise backends that perform double or triple buffering
    of resources may end up in unexpected behavior.

    \warning Partial updates are not possible with this approach since some
    backends may choose a strategy where the previous contents of the buffer is
    lost upon calling this function. Data must be written to all regions that
    are read by shaders in the frame currently being prepared.

    \warning This function can only be called when recording a frame, so
    between QRhi::beginFrame() and QRhi::endFrame().

    \warning This function can only be called on Dynamic buffers.
 */
char *QRhiBuffer::beginFullDynamicBufferUpdateForCurrentFrame()
{
    return nullptr;
}

/*!
    To be called when the entire contents of the buffer data has been updated
    in the memory block returned from
    beginFullDynamicBufferUpdateForCurrentFrame().
 */
void QRhiBuffer::endFullDynamicBufferUpdateForCurrentFrame()
{
}

/*!
    \internal
 */
void QRhiBuffer::fullDynamicBufferUpdateForCurrentFrame(const void *data, quint32 size)
{
    char *p = beginFullDynamicBufferUpdateForCurrentFrame();
    if (p) {
        memcpy(p, data, size > 0 ? size : m_size);
        endFullDynamicBufferUpdateForCurrentFrame();
    }
}

/*!
    \class QRhiRenderBuffer
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Renderbuffer resource.

    Renderbuffers cannot be sampled or read but have some benefits over
    textures in some cases:

    A \l DepthStencil renderbuffer may be lazily allocated and be backed by
    transient memory with some APIs. On some platforms this may mean the
    depth/stencil buffer uses no physical backing at all.

    \l Color renderbuffers are useful since QRhi::MultisampleRenderBuffer may be
    supported even when QRhi::MultisampleTexture is not.

    How the renderbuffer is implemented by a backend is not exposed to the
    applications. In some cases it may be backed by ordinary textures, while in
    others there may be a different kind of native resource used.

    Renderbuffers that are used as (and are only used as) depth-stencil buffers
    in combination with a QRhiSwapChain's color buffers should have the
    UsedWithSwapChainOnly flag set. This serves a double purpose: such buffers,
    depending on the backend and the underlying APIs, be more efficient, and
    QRhi provides automatic sizing behavior to match the color buffers, which
    means calling setPixelSize() and create() are not necessary for such
    renderbuffers.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \enum QRhiRenderBuffer::Type
    Specifies the type of the renderbuffer

    \value DepthStencil Combined depth/stencil
    \value Color Color
 */

/*!
    \struct QRhiRenderBuffer::NativeRenderBuffer
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \brief Wraps a native renderbuffer object.
 */

/*!
    \variable QRhiRenderBuffer::NativeRenderBuffer::object
    \brief 64-bit integer containing the native object handle.

    Used with QRhiRenderBuffer::createFrom().

    With OpenGL the native handle is a GLuint value. \c object is expected to
    be a valid OpenGL renderbuffer object ID.
 */

/*!
    \enum QRhiRenderBuffer::Flag
    Flag values for flags() and setFlags()

    \value UsedWithSwapChainOnly For DepthStencil renderbuffers this indicates
    that the renderbuffer is only used in combination with a QRhiSwapChain, and
    never in any other way. This provides automatic sizing and resource
    rebuilding, so calling setPixelSize() or create() is not needed whenever
    this flag is set. This flag value may also trigger backend-specific
    behavior, for example with OpenGL, where a separate windowing system
    interface API is in use (EGL, GLX, etc.), the flag is especially important
    as it avoids creating any actual renderbuffer resource as there is already
    a windowing system provided depth/stencil buffer as requested by
    QSurfaceFormat.
 */

/*!
    \internal
 */
QRhiRenderBuffer::QRhiRenderBuffer(QRhiImplementation *rhi, Type type_, const QSize &pixelSize_,
                                   int sampleCount_, Flags flags_,
                                   QRhiTexture::Format backingFormatHint_)
    : QRhiResource(rhi),
      m_type(type_), m_pixelSize(pixelSize_), m_sampleCount(sampleCount_), m_flags(flags_),
      m_backingFormatHint(backingFormatHint_)
{
}

/*!
    \return the resource type.
 */
QRhiResource::Type QRhiRenderBuffer::resourceType() const
{
    return RenderBuffer;
}

/*!
    \fn virtual bool QRhiRenderBuffer::create() = 0

    Creates the corresponding native graphics resources. If there are already
    resources present due to an earlier create() with no corresponding
    destroy(), then destroy() is called implicitly first.

    \return \c true when successful, \c false when a graphics operation failed.
    Regardless of the return value, calling destroy() is always safe.
 */

/*!
    Similar to create() except that no new native renderbuffer objects are
    created. Instead, the native renderbuffer object specified by \a src is
    used.

    This allows importing an existing renderbuffer object (which must belong to
    the same device or sharing context, depending on the graphics API) from an
    external graphics engine.

    \note This is currently applicable to OpenGL only. This function exists
    solely to allow importing a renderbuffer object that is bound to some
    special, external object, such as an EGLImageKHR. Once the application
    performed the glEGLImageTargetRenderbufferStorageOES call, the renderbuffer
    object can be passed to this function to create a wrapping
    QRhiRenderBuffer, which in turn can be passed in as a color attachment to
    a QRhiTextureRenderTarget to enable rendering to the EGLImage.

    \note pixelSize(), sampleCount(), and flags() must still be set correctly.
    Passing incorrect sizes and other values to QRhi::newRenderBuffer() and
    then following it with a createFrom() expecting that the native
    renderbuffer object alone is sufficient to deduce such values is \b wrong
    and will lead to problems.

    \note QRhiRenderBuffer does not take ownership of the native object, and
    destroy() will not release that object.

    \note This function is only implemented when the QRhi::RenderBufferImport
    feature is reported as \l{QRhi::isFeatureSupported()}{supported}. Otherwise,
    the function does nothing and the return value is \c false.

    \return \c true when successful, \c false when not supported.
 */
bool QRhiRenderBuffer::createFrom(NativeRenderBuffer src)
{
    Q_UNUSED(src);
    return false;
}

/*!
    \fn QRhiRenderBuffer::Type QRhiRenderBuffer::type() const
    \return the renderbuffer type.
 */

/*!
    \fn void QRhiRenderBuffer::setType(Type t)
    Sets the type to \a t.
 */

/*!
    \fn QSize QRhiRenderBuffer::pixelSize() const
    \return the pixel size.
 */

/*!
    \fn void QRhiRenderBuffer::setPixelSize(const QSize &sz)
    Sets the size (in pixels) to \a sz.
 */

/*!
    \fn int QRhiRenderBuffer::sampleCount() const
    \return the sample count. 1 means no multisample antialiasing.
 */

/*!
    \fn void QRhiRenderBuffer::setSampleCount(int s)
    Sets the sample count to \a s.
 */

/*!
    \fn QRhiRenderBuffer::Flags QRhiRenderBuffer::flags() const
    \return the flags.
 */

/*!
    \fn void QRhiRenderBuffer::setFlags(Flags f)
    Sets the flags to \a f.
 */

/*!
    \fn virtual QRhiTexture::Format QRhiRenderBuffer::backingFormat() const = 0

    \internal
 */

/*!
    \class QRhiTexture
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Texture resource.

    A QRhiTexture encapsulates a native texture object, such as a \c VkImage or
    \c MTLTexture.

    A QRhiTexture instance is always created by calling
    \l{QRhi::newTexture()}{the QRhi's newTexture() function}. This creates no
    native graphics resources. To do that, call create() after setting the
    appropriate options, such as the format and size, although in most cases
    these are already set based on the arguments passed to
    \l{QRhi::newTexture()}{newTexture()}.

    Setting the \l{QRhiTexture::Flags}{flags} correctly is essential, otherwise
    various errors can occur depending on the underlying QRhi backend and
    graphics API. For example, when a texture will be rendered into from a
    render pass via QRhiTextureRenderTarget, the texture must be created with
    the \l RenderTarget flag set. Similarly, when the texture is going to be
    \l{QRhiResourceUpdateBatch::readBackTexture()}{read back}, the \l
    UsedAsTransferSource flag must be set upfront. Mipmapped textures must have
    the MipMapped flag set. And so on. It is not possible to change the flags
    once create() has succeeded. To release the existing and create a new
    native texture object with the changed settings, call the setters and call
    create() again. This then might be a potentially expensive operation.

    \section2 Example usage

    To create a 2D texture with a size of 512x512 pixels and set its contents to all green:

    \code
        QRhiTexture *texture = rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512));
        if (!texture->create()) { error(); }
        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QImage image(512, 512, QImage::Format_RGBA8888);
        image.fill(Qt::green);
        batch->uploadTexture(texture, image);
        // ...
        commandBuffer->resourceUpdate(batch); // or, alternatively, pass 'batch' to a beginPass() call
    \endcode

    \section2 Common patterns

    A call to create() destroys any existing native resources if create() was
    successfully called before. If those native resources are still in use by
    an in-flight frame (i.e., there's a chance they are still read by the GPU),
    the destroying of those resources is deferred automatically. Thus a very
    common and convenient pattern to safely change the size of an already
    existing texture is the following. In practice this drops and creates a
    whole new native texture resource underneath, so it is not necessarily a
    cheap operation, but is more convenient and still faster than the
    alternatives, because by not destroying the \c texture object itself, all
    references to it stay valid in other data structures (e.g., in any
    QShaderResourceBinding the QRhiTexture is referenced from).

    \code
        // determine newSize, e.g. based on the swapchain's output size or other factors
        if (texture->pixelSize() != newSize) {
            texture->setPixelSize(newSize);
            if (!texture->create()) { error(); }
        }
        // continue using texture, fill it with new data
    \endcode

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    \sa QRhiResourceUpdateBatch, QRhi, QRhiTextureRenderTarget
 */

/*!
    \enum QRhiTexture::Flag

    Flag values to specify how the texture is going to be used. Not honoring
    the flags set before create() and attempting to use the texture in ways that
    was not declared upfront can lead to unspecified behavior or decreased
    performance depending on the backend and the underlying graphics API.

    \value RenderTarget The texture going to be used in combination with
    QRhiTextureRenderTarget.

    \value CubeMap The texture is a cubemap. Such textures have 6 layers, one
    for each face in the order of +X, -X, +Y, -Y, +Z, -Z. Cubemap textures
    cannot be multisample.

     \value MipMapped The texture has mipmaps. The appropriate mip count is
     calculated automatically and can also be retrieved via
     QRhi::mipLevelsForSize(). The images for the mip levels have to be
     provided in the texture uploaded or generated via
     QRhiResourceUpdateBatch::generateMips(). Multisample textures cannot have
     mipmaps.

    \value sRGB Use an sRGB format.

    \value UsedAsTransferSource The texture is used as the source of a texture
    copy or readback, meaning the texture is given as the source in
    QRhiResourceUpdateBatch::copyTexture() or
    QRhiResourceUpdateBatch::readBackTexture().

     \value UsedWithGenerateMips The texture is going to be used with
     QRhiResourceUpdateBatch::generateMips().

     \value UsedWithLoadStore The texture is going to be used with image
     load/store operations, for example, in a compute shader.

     \value UsedAsCompressedAtlas The texture has a compressed format and the
     dimensions of subresource uploads may not match the texture size.

     \value ExternalOES The texture should use the GL_TEXTURE_EXTERNAL_OES
     target with OpenGL. This flag is ignored with other graphics APIs.

     \value ThreeDimensional The texture is a 3D texture. Such textures should
     be created with the QRhi::newTexture() overload taking a depth in addition
     to width and height. A 3D texture can have mipmaps but cannot be
     multisample. When rendering into, or uploading data to a 3D texture, the \c
     layer specified in the render target's color attachment or the upload
     description refers to a single slice in range [0..depth-1]. The underlying
     graphics API may not support 3D textures at run time. Support is indicated
     by the QRhi::ThreeDimensionalTextures feature.

     \value TextureRectangleGL The texture should use the GL_TEXTURE_RECTANGLE
     target with OpenGL. This flag is ignored with other graphics APIs. Just
     like ExternalOES, this flag is useful when working with platform APIs where
     native OpenGL texture objects received from the platform are wrapped in a
     QRhiTexture, and the platform can only provide textures for a non-2D
     texture target.

     \value TextureArray The texture is a texture array, i.e. a single texture
     object that is a homogeneous array of 2D textures. Texture arrays are
     created with QRhi::newTextureArray(). The underlying graphics API may not
     support texture array objects at run time. Support is indicated by the
     QRhi::TextureArrays feature. When rendering into, or uploading data to a
     texture array, the \c layer specified in the render target's color
     attachment or the upload description selects a single element in the array.

     \value OneDimensional The texture is a 1D texture. Such textures can be
     created by passing a 0 height and depth to QRhi::newTexture(). Note that
     there can be limitations on one dimensional textures depending on the
     underlying graphics API. For example, rendering to them or using them with
     mipmap-based filtering may be unsupported. This is indicated by the
     QRhi::OneDimensionalTextures and QRhi::OneDimensionalTextureMipmaps
     feature flags.

     \value UsedAsShadingRateMap
 */

/*!
    \enum QRhiTexture::Format

    Specifies the texture format. See also QRhi::isTextureFormatSupported() and
    note that flags() can modify the format when QRhiTexture::sRGB is set.

    \value UnknownFormat Not a valid format. This cannot be passed to setFormat().

    \value RGBA8 Four components, unsigned normalized 8-bit per component. Always supported. (32 bits total)

    \value BGRA8 Four components, unsigned normalized 8-bit per component. (32 bits total)

    \value R8 One component, unsigned normalized 8-bit. (8 bits total)

    \value RG8 Two components, unsigned normalized 8-bit. (16 bits total)

    \value R16 One component, unsigned normalized 16-bit. (16 bits total)

    \value RG16 Two components, unsigned normalized 16-bit. (32 bits total)

    \value RED_OR_ALPHA8 Either same as R8, or is a similar format with the component swizzled to alpha,
    depending on \l{QRhi::RedOrAlpha8IsRed}{RedOrAlpha8IsRed}. (8 bits total)

    \value RGBA16F Four components, 16-bit float. (64 bits total)

    \value RGBA32F Four components, 32-bit float. (128 bits total)

    \value R16F One component, 16-bit float. (16 bits total)

    \value R32F One component, 32-bit float. (32 bits total)

    \value RGB10A2 Four components, unsigned normalized 10 bit R, G, and B,
    2-bit alpha. This is a packed format so native endianness applies. Note
    that there is no BGR10A2. This is because RGB10A2 maps to
    DXGI_FORMAT_R10G10B10A2_UNORM with D3D, MTLPixelFormatRGB10A2Unorm with
    Metal, VK_FORMAT_A2B10G10R10_UNORM_PACK32 with Vulkan, and
    GL_RGB10_A2/GL_RGB/GL_UNSIGNED_INT_2_10_10_10_REV on OpenGL (ES). This is
    the only universally supported RGB30 option. The corresponding QImage
    formats are QImage::Format_BGR30 and QImage::Format_A2BGR30_Premultiplied.
    (32 bits total)

    \value D16 16-bit depth (normalized unsigned integer)

    \value D24 24-bit depth (normalized unsigned integer)

    \value D24S8 24-bit depth (normalized unsigned integer), 8 bit stencil

    \value D32F 32-bit depth (32-bit float)

    \value [since 6.9] D32FS8 32-bit depth (32-bit float), 8 bits of stencil, 24 bits unused
    (64 bits total)

    \value BC1
    \value BC2
    \value BC3
    \value BC4
    \value BC5
    \value BC6H
    \value BC7

    \value ETC2_RGB8
    \value ETC2_RGB8A1
    \value ETC2_RGBA8

    \value ASTC_4x4
    \value ASTC_5x4
    \value ASTC_5x5
    \value ASTC_6x5
    \value ASTC_6x6
    \value ASTC_8x5
    \value ASTC_8x6
    \value ASTC_8x8
    \value ASTC_10x5
    \value ASTC_10x6
    \value ASTC_10x8
    \value ASTC_10x10
    \value ASTC_12x10
    \value ASTC_12x12

    \value [since 6.9] R8UI One component, unsigned 8-bit. (8 bits total)
    \value [since 6.9] R32UI One component, unsigned 32-bit. (32 bits total)
    \value [since 6.9] RG32UI Two components, unsigned 32-bit. (64 bits total)
    \value [since 6.9] RGBA32UI Four components, unsigned 32-bit. (128 bits total)

    \value [since 6.10] R8SI One component, signed 8-bit. (8 bits total)
    \value [since 6.10] R32SI One component, signed 32-bit. (32 bits total)
    \value [since 6.10] RG32SI Two components, signed 32-bit. (64 bits total)
    \value [since 6.10] RGBA32SI Four components, signed 32-bit. (128 bits total)
 */

// When adding new texture formats, update void tst_QRhi::textureFormats_data().

/*!
    \struct QRhiTexture::NativeTexture
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \brief Contains information about the underlying native resources of a texture.
 */

/*!
    \variable QRhiTexture::NativeTexture::object
    \brief 64-bit integer containing the native object handle.

    With OpenGL, the native handle is a GLuint value, so \c object can then be
    cast to a GLuint. With Vulkan, the native handle is a VkImage, so \c object
    can be cast to a VkImage. With Direct3D 11 and Metal \c object contains a
    ID3D11Texture2D or MTLTexture pointer, respectively. With Direct3D 12
    \c object contains a ID3D12Resource pointer.
 */

/*!
    \variable QRhiTexture::NativeTexture::layout
    \brief Specifies the current image layout for APIs like Vulkan.

    For Vulkan, \c layout contains a \c VkImageLayout value.
 */

/*!
    \internal
 */
QRhiTexture::QRhiTexture(QRhiImplementation *rhi, Format format_, const QSize &pixelSize_, int depth_,
                         int arraySize_, int sampleCount_, Flags flags_)
    : QRhiResource(rhi),
      m_format(format_), m_pixelSize(pixelSize_), m_depth(depth_),
      m_arraySize(arraySize_), m_sampleCount(sampleCount_), m_flags(flags_)
{
}

/*!
    \return the resource type.
 */
QRhiResource::Type QRhiTexture::resourceType() const
{
    return Texture;
}

/*!
    \fn virtual bool QRhiTexture::create() = 0

    Creates the corresponding native graphics resources. If there are already
    resources present due to an earlier create() with no corresponding
    destroy(), then destroy() is called implicitly first.

    \return \c true when successful, \c false when a graphics operation failed.
    Regardless of the return value, calling destroy() is always safe.
 */

/*!
    \return the underlying native resources for this texture. The returned value
    will be empty if exposing the underlying native resources is not supported by
    the backend.

    \sa createFrom()
 */
QRhiTexture::NativeTexture QRhiTexture::nativeTexture()
{
    return {};
}

/*!
    Similar to create(), except that no new native textures are created.
    Instead, the native texture resources specified by \a src is used.

    This allows importing an existing native texture object (which must belong
    to the same device or sharing context, depending on the graphics API) from
    an external graphics engine.

    \return true if the specified existing native texture object has been
    successfully wrapped as a non-owning QRhiTexture.

    \note format(), pixelSize(), sampleCount(), and flags() must still be set
    correctly. Passing incorrect sizes and other values to QRhi::newTexture()
    and then following it with a createFrom() expecting that the native texture
    object alone is sufficient to deduce such values is \b wrong and will lead
    to problems.

    \note QRhiTexture does not take ownership of the texture object. destroy()
    does not free the object or any associated memory.

    The opposite of this operation, exposing a QRhiTexture-created native
    texture object to a foreign engine, is possible via nativeTexture().

    \note When importing a 3D texture, or a texture array object, or, with
    OpenGL ES, an external texture, it is then especially important to set the
    corresponding flags (ThreeDimensional, TextureArray, ExternalOES) via
    setFlags() before calling this function.
*/
bool QRhiTexture::createFrom(QRhiTexture::NativeTexture src)
{
    Q_UNUSED(src);
    return false;
}

/*!
    With some graphics APIs, such as Vulkan, integrating custom rendering code
    that uses the graphics API directly needs special care when it comes to
    image layouts. This function allows communicating the expected \a layout the
    image backing the QRhiTexture is in after the native rendering commands.

    For example, consider rendering into a QRhiTexture's VkImage directly with
    Vulkan in a code block enclosed by QRhiCommandBuffer::beginExternal() and
    QRhiCommandBuffer::endExternal(), followed by using the image for texture
    sampling in a QRhi-based render pass. To avoid potentially incorrect image
    layout transitions, this function can be used to indicate what the image
    layout will be once the commands recorded in said code block complete.

    Calling this function makes sense only after
    QRhiCommandBuffer::endExternal() and before a subsequent
    QRhiCommandBuffer::beginPass().

    This function has no effect with QRhi backends where the underlying
    graphics API does not expose a concept of image layouts.

    \note With Vulkan \a layout is a \c VkImageLayout. With Direct 3D 12 \a
    layout is a value composed of the bits from \c D3D12_RESOURCE_STATES.
 */
void QRhiTexture::setNativeLayout(int layout)
{
    Q_UNUSED(layout);
}

/*!
    \fn QRhiTexture::Format QRhiTexture::format() const
    \return the texture format.
 */

/*!
    \fn void QRhiTexture::setFormat(QRhiTexture::Format fmt)

    Sets the requested texture format to \a fmt.

    \note The value set is only taken into account upon the next call to
    create(), i.e. when the underlying graphics resource are (re)created.
    Setting a new value is futile otherwise and must be avoided since it can
    lead to inconsistent state.
 */

/*!
    \fn QSize QRhiTexture::pixelSize() const
    \return the size in pixels.
 */

/*!
    \fn void QRhiTexture::setPixelSize(const QSize &sz)

    Sets the texture size, specified in pixels, to \a sz.

    \note The value set is only taken into account upon the next call to
    create(), i.e. when the underlying graphics resource are (re)created.
    Setting a new value is futile otherwise and must be avoided since it can
    lead to inconsistent state. The same applies to all other setters as well.
 */

/*!
    \fn int QRhiTexture::depth() const
    \return the depth for 3D textures.
 */

/*!
    \fn void QRhiTexture::setDepth(int depth)
    Sets the \a depth for a 3D texture.
 */

/*!
    \fn int QRhiTexture::arraySize() const
    \return the texture array size.
 */

/*!
    \fn void QRhiTexture::setArraySize(int arraySize)
    Sets the texture \a arraySize.
 */

/*!
    \fn int QRhiTexture::arrayRangeStart() const

    \return the first array layer when setArrayRange() was called.

    \sa setArrayRange()
 */

/*!
    \fn int QRhiTexture::arrayRangeLength() const

    \return the exposed array range size when setArrayRange() was called.

    \sa setArrayRange()
*/

/*!
    \fn void QRhiTexture::setArrayRange(int startIndex, int count)

    Normally all array layers are exposed and it is up to the shader to select
    the layer via the third coordinate passed to the \c{texture()} GLSL
    function when sampling the \c sampler2DArray. When QRhi::TextureArrayRange
    is reported as supported, calling setArrayRange() before create() or
    createFrom() requests selecting only the specified range, \a count elements
    starting from \a startIndex. The shader logic can then be written with this
    in mind.

    \sa QRhi::TextureArrayRange
 */

/*!
    \fn Flags QRhiTexture::flags() const
    \return the texture flags.
 */

/*!
    \fn void QRhiTexture::setFlags(Flags f)
    Sets the texture flags to \a f.
 */

/*!
    \fn int QRhiTexture::sampleCount() const
    \return the sample count. 1 means no multisample antialiasing.
 */

/*!
    \fn void QRhiTexture::setSampleCount(int s)
    Sets the sample count to \a s.
 */

/*!
    \struct QRhiTexture::ViewFormat
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.8
    \brief Specifies the view format for reading or writing from or to the texture.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \variable QRhiTexture::ViewFormat::format
 */

/*!
    \variable QRhiTexture::ViewFormat::srgb
 */

/*!
    \fn QRhiTexture::ViewFormat QRhiTexture::readViewFormat() const
    \since 6.8
    \return the view format used when sampling the texture. When not called, the view
    format is assumed to be the same as format().
 */

/*!
    \fn void QRhiTexture::setReadViewFormat(const ViewFormat &fmt)
    \since 6.8

    Sets the shader resource view format (or the format of the view used for
    sampling the texture) to \a fmt. By default the same format (and sRGB-ness)
    is used as the texture itself, and in most cases this function does not need
    to be called.

    This setting is only taken into account when the \l QRhi::TextureViewFormat
    feature is reported as supported.

    \note This functionality is provided to allow "casting" between
    non-sRGB and sRGB in order to get the shader reads perform, or not perform,
    the implicit sRGB conversions. Other types of casting may or may not be
    functional.
 */

/*!
    \fn QRhiTexture::ViewFormat QRhiTexture::writeViewFormat() const
    \since 6.8
    \return the view format used when writing to the texture and when using it
    with image load/store. When not called, the view format is assumed to be the
    same as format().
 */

/*!
    \fn void QRhiTexture::setWriteViewFormat(const ViewFormat &fmt)
    \since 6.8

    Sets the render target view format to \a fmt. By default the same format
    (and sRGB-ness) is used as the texture itself, and in most cases this
    function does not need to be called.

    One common use case for providing a write view format is working with
    externally provided textures that, outside of our control, use an sRGB
    format with 3D APIs such as Vulkan or Direct 3D, but the rendering engine is
    already prepared to handle linearization and conversion to sRGB at the end
    of its shading pipeline. In this case what is wanted when rendering into
    such a texture is a render target view (e.g. VkImageView) that has the same,
    but non-sRGB format. (if e.g. from an OpenXR implementation one gets a
    VK_FORMAT_R8G8B8A8_SRGB texture, it is likely that rendering into it should
    be done using a VK_FORMAT_R8G8B8A8_UNORM view, if that is what the rendering
    engine's pipeline requires; in this example one would call this function
    with a ViewFormat that has a format of QRhiTexture::RGBA8 and \c srgb set to
    \c false).

    This setting is only taken into account when the \l QRhi::TextureViewFormat
    feature is reported as supported.

    \note This functionality is provided to allow "casting" between
    non-sRGB and sRGB in order to get the shader write not perform, or perform,
    the implicit sRGB conversions. Other types of casting may or may not be
    functional.
 */

/*!
    \class QRhiSampler
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Sampler resource.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \enum QRhiSampler::Filter
    Specifies the minification, magnification, or mipmap filtering

    \value None Applicable only for mipmapMode(), indicates no mipmaps to be used
    \value Nearest
    \value Linear
 */

/*!
    \enum QRhiSampler::AddressMode
    Specifies the addressing mode

    \value Repeat
    \value ClampToEdge
    \value Mirror
 */

/*!
    \enum QRhiSampler::CompareOp
    Specifies the texture comparison function.

    \value Never (default)
    \value Less
    \value Equal
    \value LessOrEqual
    \value Greater
    \value NotEqual
    \value GreaterOrEqual
    \value Always
 */

/*!
    \internal
 */
QRhiSampler::QRhiSampler(QRhiImplementation *rhi,
                         Filter magFilter_, Filter minFilter_, Filter mipmapMode_,
                         AddressMode u_, AddressMode v_, AddressMode w_)
    : QRhiResource(rhi),
      m_magFilter(magFilter_), m_minFilter(minFilter_), m_mipmapMode(mipmapMode_),
      m_addressU(u_), m_addressV(v_), m_addressW(w_),
      m_compareOp(QRhiSampler::Never)
{
}

/*!
    \return the resource type.
 */
QRhiResource::Type QRhiSampler::resourceType() const
{
    return Sampler;
}

/*!
    \fn QRhiSampler::Filter QRhiSampler::magFilter() const
    \return the magnification filter mode.
 */

/*!
    \fn void QRhiSampler::setMagFilter(Filter f)
    Sets the magnification filter mode to \a f.
 */

/*!
    \fn QRhiSampler::Filter QRhiSampler::minFilter() const
    \return the minification filter mode.
 */

/*!
    \fn void QRhiSampler::setMinFilter(Filter f)
    Sets the minification filter mode to \a f.
 */

/*!
    \fn QRhiSampler::Filter QRhiSampler::mipmapMode() const
    \return the mipmap filter mode.
 */

/*!
    \fn void QRhiSampler::setMipmapMode(Filter f)

    Sets the mipmap filter mode to \a f.

    Leave this set to None when the texture has no mip levels, or when the mip
    levels are not to be taken into account.
 */

/*!
    \fn QRhiSampler::AddressMode QRhiSampler::addressU() const
    \return the horizontal wrap mode.
 */

/*!
    \fn void QRhiSampler::setAddressU(AddressMode mode)
    Sets the horizontal wrap \a mode.
 */

/*!
    \fn QRhiSampler::AddressMode QRhiSampler::addressV() const
    \return the vertical wrap mode.
 */

/*!
    \fn void QRhiSampler::setAddressV(AddressMode mode)
    Sets the vertical wrap \a mode.
 */

/*!
    \fn QRhiSampler::AddressMode QRhiSampler::addressW() const
    \return the depth wrap mode.
 */

/*!
    \fn void QRhiSampler::setAddressW(AddressMode mode)
    Sets the depth wrap \a mode.
 */

/*!
    \fn QRhiSampler::CompareOp QRhiSampler::textureCompareOp() const
    \return the texture comparison function.
 */

/*!
    \fn void QRhiSampler::setTextureCompareOp(CompareOp op)
    Sets the texture comparison function \a op.
 */

/*!
    \class QRhiShadingRateMap
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.9
    \brief An object that wraps a texture or another kind of native 3D API object.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    For an introduction to Variable Rate Shading (VRS), see
    \l{https://learn.microsoft.com/en-us/windows/win32/direct3d12/vrs}. Qt
    supports a subset of the VRS features offered by Direct 3D 12 and Vulkan. In
    addition, Metal's somewhat different mechanism is supported by making it
    possible to set up a QRhiShadingRateMap with an existing
    MTLRasterizationRateMap object.
 */

/*!
    \struct QRhiShadingRateMap::NativeShadingRateMap
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.9
    \brief Wraps a native shading rate map.

    An example is MTLRasterizationRateMap with Metal. Other 3D APIs that use
    textures for image-based VRS do not use this struct since those can function
    via the QRhiTexture-based overload of QRhiShadingRate::createFrom().
 */

/*!
    \variable QRhiShadingRateMap::NativeShadingRateMap::object
    \brief 64-bit integer containing the native object handle.

    Used with QRhiShadingRateMap::createFrom(). For example, with Metal,
    \c object is expected to be an id<MTLRasterizationRateMap>.
 */

/*!
    \internal
 */
QRhiShadingRateMap::QRhiShadingRateMap(QRhiImplementation *rhi)
    : QRhiResource(rhi)
{
}

/*!
    \return the resource type.
 */
QRhiResource::Type QRhiShadingRateMap::resourceType() const
{
    return ShadingRateMap;
}

/*!
    Sets up the shading rate map to use a native 3D API shading rate object
    \a src.

    \return \c true when successful, \c false when not supported.

    \note This is functional only when the QRhi::VariableRateShadingMap feature
    is reported as supported, while QRhi::VariableShadingRateMapWithTexture
    feature is not. Currently this is true for Metal, assuming variable rate
    shading is supported by the GPU.

    \note With Metal, the \c object field of \a src is expected to contain an
    id<MTLRasterizationRateMap>. Note that Qt does not perform anything else
    apart from passing the MTLRasterizationRateMap on to the
    MTLRenderPassDescriptor. If any special scaling is required, it is up to the
    application (or the XR compositor) to perform that.
 */
bool QRhiShadingRateMap::createFrom(NativeShadingRateMap src)
{
    Q_UNUSED(src);
    return false;
}

/*!
    Sets up the shading rate map to use the texture \a src as the
    image containing the per-tile shading rates.

    \return \c true when successful, \c false when not supported.

    The QRhiShadingRateMap does not take ownership of \a src.

    \note This is functional only when the
    QRhi::VariableRateShadingMapWithTexture feature is reported as supported. In
    practice may be supported on Vulkan and Direct 3D 12 when using modern
    graphics cards. It will never be supported on OpenGL or Metal, for example.

    \note \a src must have a format of QRhiTexture::R8UI.

    \note \a src must have a width of \c{ceil(render_target_pixel_width /
    (float)tile_width)} and a height of \c{ceil(render_target_pixel_height /
    (float)tile_height)}. It is up to the application to ensure the size of the
    texture is as expected, using the above formula, at all times. The tile size
    can be queried via \l QRhi::resourceLimit() and
    QRhi::ShadingRateImageTileSize.

    Each byte (texel) in the texture corresponds to the shading rate value for
    one tile. 0 indicates 1x1, while a value of 10 indicates 4x4. See
    \l{https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_shading_rate}{D3D12_SHADING_RATE}
    for other possible values.
 */
bool QRhiShadingRateMap::createFrom(QRhiTexture *src)
{
    Q_UNUSED(src);
    return false;
}

/*!
    \class QRhiRenderPassDescriptor
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Render pass resource.

    A render pass, if such a concept exists in the underlying graphics API, is
    a collection of attachments (color, depth, stencil) and describes how those
    attachments are used.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \internal
 */
QRhiRenderPassDescriptor::QRhiRenderPassDescriptor(QRhiImplementation *rhi)
    : QRhiResource(rhi)
{
}

/*!
    \return the resource type.
 */
QRhiResource::Type QRhiRenderPassDescriptor::resourceType() const
{
    return RenderPassDescriptor;
}

/*!
    \fn virtual bool QRhiRenderPassDescriptor::isCompatible(const QRhiRenderPassDescriptor *other) const = 0

    \return true if the \a other QRhiRenderPassDescriptor is compatible with
    this one, meaning \c this and \a other can be used interchangebly in
    QRhiGraphicsPipeline::setRenderPassDescriptor().

    The concept of the compatibility of renderpass descriptors is similar to
    the \l{QRhiShaderResourceBindings::isLayoutCompatible}{layout
    compatibility} of QRhiShaderResourceBindings instances. They allow better
    reuse of QRhiGraphicsPipeline instances: for example, a
    QRhiGraphicsPipeline instance cache is expected to use these functions to
    look for a matching pipeline, instead of just comparing pointers, thus
    allowing a different QRhiRenderPassDescriptor and
    QRhiShaderResourceBindings to be used in combination with the pipeline, as
    long as they are compatible.

    The exact details of compatibility depend on the underlying graphics API.
    Two renderpass descriptors
    \l{QRhiTextureRenderTarget::newCompatibleRenderPassDescriptor()}{created}
    from the same QRhiTextureRenderTarget are always compatible.

    Similarly to QRhiShaderResourceBindings, compatibility can also be tested
    without having two existing objects available. Extracting the opaque blob by
    calling serializedFormat() allows testing for compatibility by comparing the
    returned vector to another QRhiRenderPassDescriptor's
    serializedFormat(). This has benefits in certain situations, because it
    allows testing the compatibility of a QRhiRenderPassDescriptor with a
    QRhiGraphicsPipeline even when the QRhiRenderPassDescriptor the pipeline was
    originally built was is no longer available (but the data returned from its
    serializedFormat() still is).

    \sa newCompatibleRenderPassDescriptor(), serializedFormat()
 */

/*!
    \fn virtual QRhiRenderPassDescriptor *QRhiRenderPassDescriptor::newCompatibleRenderPassDescriptor() const = 0

    \return a new QRhiRenderPassDescriptor that is
    \l{isCompatible()}{compatible} with this one.

    This function allows cloning a QRhiRenderPassDescriptor. The returned
    object is ready to be used, and the ownership is transferred to the caller.
    Cloning a QRhiRenderPassDescriptor object can become useful in situations
    where the object is stored in data structures related to graphics pipelines
    (in order to allow creating new pipelines which in turn requires a
    renderpass descriptor object), and the lifetime of the renderpass
    descriptor created from a render target may be shorter than the pipelines.
    (for example, because the engine manages and destroys renderpasses together
    with the textures and render targets it was created from) In such a
    situation, it can be beneficial to store a cloned version in the data
    structures, and thus transferring ownership as well.

    \sa isCompatible()
 */

/*!
    \fn virtual QVector<quint32> QRhiRenderPassDescriptor::serializedFormat() const = 0

    \return a vector of integers containing an opaque blob describing the data
    relevant for \l{isCompatible()}{compatibility}.

    Given two QRhiRenderPassDescriptor objects \c rp1 and \c rp2, if the data
    returned from this function is identical, then \c{rp1->isCompatible(rp2)},
    and vice versa hold true as well.

    \note The returned data is meant to be used for storing in memory and
    comparisons during the lifetime of the QRhi the object belongs to. It is not
    meant for storing on disk, reusing between processes, or using with multiple
    QRhi instances with potentially different backends.

    \note Calling this function is expected to be a cheap operation since the
    backends are not supposed to calculate the data in this function, but rather
    return an already calculated series of data.

    When creating reusable components as part of a library, where graphics
    pipelines are created and maintained while targeting a QRhiRenderTarget (be
    it a swapchain or a texture) managed by the client of the library, the
    components must be able to deal with a changing QRhiRenderPassDescriptor.
    For example, because the render target changes and so invalidates the
    previously QRhiRenderPassDescriptor (with regards to the new render target
    at least) due to having a potentially different color format and attachments
    now. Or because \l{QRhiShadingRateMap}{variable rate shading} is taken into
    use dynamically. A simple pattern that helps dealing with this is performing
    the following check on every frame, to recognize the case when the pipeline
    needs to be associated with a new QRhiRenderPassDescriptor, because
    something is different about the render target now, compared to earlier
    frames:

    \code
        QRhiRenderPassDescriptor *rp = m_renderTarget->renderPassDescriptor();
        if (m_pipeline && rp->serializedFormat() != m_renderPassFormat) {
            m_pipeline->setRenderPassDescriptor(rp);
            m_renderPassFormat = rp->serializedFormat();
            m_pipeline->create();
        }
        // remember to store m_renderPassFormat also when creating m_pipeline the first time
    \endcode

    \sa isCompatible()
 */

/*!
    \return a pointer to a backend-specific QRhiNativeHandles subclass, such as
    QRhiVulkanRenderPassNativeHandles. The returned value is \nullptr when exposing
    the underlying native resources is not supported by the backend.

    \sa QRhiVulkanRenderPassNativeHandles
 */
const QRhiNativeHandles *QRhiRenderPassDescriptor::nativeHandles()
{
    return nullptr;
}

/*!
    \class QRhiRenderTarget
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Represents an onscreen (swapchain) or offscreen (texture) render target.

    Applications do not create an instance of this class directly. Rather, it
    is the subclass QRhiTextureRenderTarget that is instantiable by clients of
    the API via \l{QRhi::newTextureRenderTarget()}{newTextureRenderTarget()}.
    The other subclass is QRhiSwapChainRenderTarget, which is the type
    QRhiSwapChain returns when calling
    \l{QRhiSwapChain::currentFrameRenderTarget()}{currentFrameRenderTarget()}.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    \sa QRhiSwapChainRenderTarget, QRhiTextureRenderTarget
 */

/*!
    \internal
 */
QRhiRenderTarget::QRhiRenderTarget(QRhiImplementation *rhi)
    : QRhiResource(rhi)
{
}

/*!
    \fn virtual QSize QRhiRenderTarget::pixelSize() const = 0

    \return the size in pixels.

    Valid only after create() has been called successfully. Until then the
    result is a default-constructed QSize.

    With QRhiTextureRenderTarget the returned size is the size of the
    associated attachments at the time of create(), in practice the size of the
    first color attachment, or the depth/stencil buffer if there are no color
    attachments. If the associated textures or renderbuffers are resized and
    rebuilt afterwards, then pixelSize() performs an implicit call to create()
    in order to rebuild the underlying data structures. This implicit check is
    similar to what QRhiCommandBuffer::beginPass() does, and ensures that the
    returned size is always up-to-date.
 */

/*!
    \fn virtual float QRhiRenderTarget::devicePixelRatio() const = 0

    \return the device pixel ratio. For QRhiTextureRenderTarget this is always
    1. For targets retrieved from a QRhiSwapChain the value reflects the
    \l{QWindow::devicePixelRatio()}{device pixel ratio} of the targeted
    QWindow.
 */

/*!
    \fn virtual int QRhiRenderTarget::sampleCount() const = 0

    \return the sample count or 1 if multisample antialiasing is not relevant for
    this render target.
 */

/*!
    \fn QRhiRenderPassDescriptor *QRhiRenderTarget::renderPassDescriptor() const

    \return the associated QRhiRenderPassDescriptor.
 */

/*!
    \fn void QRhiRenderTarget::setRenderPassDescriptor(QRhiRenderPassDescriptor *desc)

    Sets the QRhiRenderPassDescriptor \a desc for use with this render target.
 */

/*!
    \internal
 */
QRhiSwapChainRenderTarget::QRhiSwapChainRenderTarget(QRhiImplementation *rhi, QRhiSwapChain *swapchain_)
    : QRhiRenderTarget(rhi),
      m_swapchain(swapchain_)
{
}

/*!
    \class QRhiSwapChainRenderTarget
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Swapchain render target resource.

    When targeting the color buffers of a swapchain, active render target is a
    QRhiSwapChainRenderTarget. This is what
    QRhiSwapChain::currentFrameRenderTarget() returns.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    \sa QRhiSwapChain
 */

/*!
    \return the resource type.
 */
QRhiResource::Type QRhiSwapChainRenderTarget::resourceType() const
{
    return SwapChainRenderTarget;
}

/*!
    \fn QRhiSwapChain *QRhiSwapChainRenderTarget::swapChain() const

    \return the swapchain object.
 */

/*!
    \class QRhiTextureRenderTarget
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Texture render target resource.

    A texture render target allows rendering into one or more textures,
    optionally with a depth texture or depth/stencil renderbuffer.

    For multisample rendering the common approach is to use a renderbuffer as
    the color attachment and set the non-multisample destination texture as the
    \c{resolve texture}. For more information, read the detailed description of
    the \l QRhiColorAttachment class.

    \note Textures used in combination with QRhiTextureRenderTarget must be
    created with the QRhiTexture::RenderTarget flag.

    The simplest example of creating a render target with a texture as its
    single color attachment:

    \code
        QRhiTexture *texture = rhi->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::RenderTarget);
        texture->create();
        QRhiTextureRenderTarget *rt = rhi->newTextureRenderTarget({ texture });
        rp = rt->newCompatibleRenderPassDescriptor();
        rt->setRenderPassDescriptor(rp);
        rt->create();
        // rt can now be used with beginPass()
    \endcode

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \enum QRhiTextureRenderTarget::Flag

    Flag values describing the load/store behavior for the render target. The
    load/store behavior may be baked into native resources under the hood,
    depending on the backend, and therefore it needs to be known upfront and
    cannot be changed without rebuilding (and so releasing and creating new
    native resources).

    \value PreserveColorContents Indicates that the contents of the color
    attachments is to be loaded when starting a render pass, instead of
    clearing. This is potentially more expensive, especially on mobile (tiled)
    GPUs, but allows preserving the existing contents between passes. When doing
    multisample rendering with a resolve texture set, setting this flag also
    requests the multisample color data to be stored (written out) to the
    multisample texture or render buffer. (for non-multisample rendering the
    color data is always stored, but for MSAA storing the multisample data
    decreases efficiency for certain GPU architectures, hence defaulting to not
    writing it out) Note however that this is non-portable: in some cases there
    is no intermediate multisample texture on the graphics API level, e.g. when
    using OpenGL ES's \c{GL_EXT_multisampled_render_to_texture} as it is all
    implicit, handled by the OpenGL ES implementation. In that case,
    PreserveColorContents will likely have no effect. Therefore, avoid relying
    on this flag when using multisample rendering and the color attachment is
    using a multisample QRhiTexture (not QRhiRenderBuffer).

    \value PreserveDepthStencilContents Indicates that the contents of the
    depth texture is to be loaded when starting a render pass, instead
    clearing. Only applicable when a texture is used as the depth buffer
    (QRhiTextureRenderTargetDescription::depthTexture() is set) because
    depth/stencil renderbuffers may not have any physical backing and data may
    not be written out in the first place.

    \value DoNotStoreDepthStencilContents Indicates that the contents of the
    depth texture does not need to be written out. Relevant only when a
    QRhiTexture, not QRhiRenderBuffer, is used as the depth-stencil buffer,
    because for QRhiRenderBuffer this is implicit. When a depthResolveTexture is
    set, the flag is not relevant, because the behavior is then as if the flag
    was set. This enum value is introduced in Qt 6.8.
 */

/*!
    \internal
 */
QRhiTextureRenderTarget::QRhiTextureRenderTarget(QRhiImplementation *rhi,
                                                 const QRhiTextureRenderTargetDescription &desc_,
                                                 Flags flags_)
    : QRhiRenderTarget(rhi),
      m_desc(desc_),
      m_flags(flags_)
{
}

/*!
    \return the resource type.
 */
QRhiResource::Type QRhiTextureRenderTarget::resourceType() const
{
    return TextureRenderTarget;
}

/*!
    \fn virtual QRhiRenderPassDescriptor *QRhiTextureRenderTarget::newCompatibleRenderPassDescriptor() = 0

    \return a new QRhiRenderPassDescriptor that is compatible with this render
    target.

    The returned value is used in two ways: it can be passed to
    setRenderPassDescriptor() and
    QRhiGraphicsPipeline::setRenderPassDescriptor(). A render pass descriptor
    describes the attachments (color, depth/stencil) and the load/store
    behavior that can be affected by flags(). A QRhiGraphicsPipeline can only
    be used in combination with a render target that has a
    \l{QRhiRenderPassDescriptor::isCompatible()}{compatible}
    QRhiRenderPassDescriptor set.

    Two QRhiTextureRenderTarget instances can share the same render pass
    descriptor as long as they have the same number and type of attachments.
    The associated QRhiTexture or QRhiRenderBuffer instances are not part of
    the render pass descriptor so those can differ in the two
    QRhiTextureRenderTarget instances.

    \note resources, such as QRhiTexture instances, referenced in description()
    must already have create() called on them.

    \sa create()
 */

/*!
    \fn virtual bool QRhiTextureRenderTarget::create() = 0

    Creates the corresponding native graphics resources. If there are already
    resources present due to an earlier create() with no corresponding
    destroy(), then destroy() is called implicitly first.

    \note renderPassDescriptor() must be set before calling create(). To obtain
    a QRhiRenderPassDescriptor compatible with the render target, call
    newCompatibleRenderPassDescriptor() before create() but after setting all
    other parameters, such as description() and flags(). To save resources,
    reuse the same QRhiRenderPassDescriptor with multiple
    QRhiTextureRenderTarget instances, whenever possible. Sharing the same
    render pass descriptor is only possible when the render targets have the
    same number and type of attachments (the actual textures can differ) and
    the same flags.

    \note resources, such as QRhiTexture instances, referenced in description()
    must already have create() called on them.

    \return \c true when successful, \c false when a graphics operation failed.
    Regardless of the return value, calling destroy() is always safe.
 */

/*!
    \fn QRhiTextureRenderTargetDescription QRhiTextureRenderTarget::description() const
    \return the render target description.
 */

/*!
    \fn void QRhiTextureRenderTarget::setDescription(const QRhiTextureRenderTargetDescription &desc)
    Sets the render target description \a desc.
 */

/*!
    \fn QRhiTextureRenderTarget::Flags QRhiTextureRenderTarget::flags() const
    \return the currently set flags.
 */

/*!
    \fn void QRhiTextureRenderTarget::setFlags(Flags f)
    Sets the flags to \a f.
 */

/*!
    \class QRhiShaderResourceBindings
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Encapsulates resources for making buffer, texture, sampler resources visible to shaders.

    A QRhiShaderResourceBindings is a collection of QRhiShaderResourceBinding
    objects, each of which describe a single binding.

    Take a fragment shader with the following interface:

    \badcode
        layout(std140, binding = 0) uniform buf {
            mat4 mvp;
            int flip;
        } ubuf;

        layout(binding = 1) uniform sampler2D tex;
    \endcode

    To make resources visible to the shader, the following
    QRhiShaderResourceBindings could be created and then passed to
    QRhiGraphicsPipeline::setShaderResourceBindings():

    \code
        QRhiShaderResourceBindings *srb = rhi->newShaderResourceBindings();
        srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, ubuf),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture, sampler)
        });
        srb->create();
        // ...
        QRhiGraphicsPipeline *ps = rhi->newGraphicsPipeline();
        // ...
        ps->setShaderResourceBindings(srb);
        ps->create();
        // ...
        cb->setGraphicsPipeline(ps);
        cb->setShaderResources(); // binds srb
    \endcode

    This assumes that \c ubuf is a QRhiBuffer, \c texture is a QRhiTexture,
    while \a sampler is a QRhiSampler. The example also assumes that the
    uniform block is present in the vertex shader as well so the same buffer is
    made visible to the vertex stage too.

    \section3 Advanced usage

    Building on the above example, let's assume that a pass now needs to use
    the exact same pipeline and shaders with a different texture. Creating a
    whole separate QRhiGraphicsPipeline just for this would be an overkill.
    This is why QRhiCommandBuffer::setShaderResources() allows specifying a \a
    srb argument. As long as the layouts (so the number of bindings and the
    binding points) match between two QRhiShaderResourceBindings, they can both
    be used with the same pipeline, assuming the pipeline was created with one of
    them in the first place. See isLayoutCompatible() for more details.

    \code
        QRhiShaderResourceBindings *srb2 = rhi->newShaderResourceBindings();
        // ...
        cb->setGraphicsPipeline(ps);
        cb->setShaderResources(srb2); // binds srb2
    \endcode

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \typedef QRhiShaderResourceBindingSet
    \relates QRhi
    \since 6.7

    Synonym for QRhiShaderResourceBindings.
*/

/*!
    \internal
 */
QRhiShaderResourceBindings::QRhiShaderResourceBindings(QRhiImplementation *rhi)
    : QRhiResource(rhi)
{
    m_layoutDesc.reserve(BINDING_PREALLOC * QRhiShaderResourceBinding::LAYOUT_DESC_ENTRIES_PER_BINDING);
}

/*!
    \return the resource type.
 */
QRhiResource::Type QRhiShaderResourceBindings::resourceType() const
{
    return ShaderResourceBindings;
}

/*!
    \return \c true if the layout is compatible with \a other. The layout does
    not include the actual resource (such as, buffer or texture) and related
    parameters (such as, offset or size). It does include the binding point,
    pipeline stage, and resource type, however. The number and order of the
    bindings must also match in order to be compatible.

    When there is a QRhiGraphicsPipeline created with this
    QRhiShaderResourceBindings, and the function returns \c true, \a other can
    then safely be passed to QRhiCommandBuffer::setShaderResources(), and so
    be used with the pipeline in place of this QRhiShaderResourceBindings.

    \note This function must only be called after a successful create(), because
    it relies on data generated during the baking of the underlying data
    structures. This way the function can implement a comparison approach that
    is more efficient than iterating through two binding lists and calling
    QRhiShaderResourceBinding::isLayoutCompatible() on each pair. This becomes
    relevant especially when this function is called at a high frequency.

    \sa serializedLayoutDescription()
 */
bool QRhiShaderResourceBindings::isLayoutCompatible(const QRhiShaderResourceBindings *other) const
{
    if (other == this)
        return true;

    if (!other)
        return false;

    // This can become a hot code path. Therefore we do not iterate and call
    // isLayoutCompatible() on m_bindings, but rather check a pre-calculated
    // hash code and then, if the hash matched, do a uint array comparison
    // (that's still more cache friendly).

    return m_layoutDescHash == other->m_layoutDescHash
            && m_layoutDesc == other->m_layoutDesc;
}

/*!
    \fn QVector<quint32> QRhiShaderResourceBindings::serializedLayoutDescription() const

    \return a vector of integers containing an opaque blob describing the layout
    of the binding list, i.e. the data relevant for
    \l{isLayoutCompatible()}{layout compatibility tests}.

    Given two objects \c srb1 and \c srb2, if the data returned from this
    function is identical, then \c{srb1->isLayoutCompatible(srb2)}, and vice
    versa hold true as well.

    \note The returned data is meant to be used for storing in memory and
    comparisons during the lifetime of the QRhi the object belongs to. It is not
    meant for storing on disk, reusing between processes, or using with multiple
    QRhi instances with potentially different backends.

    \sa isLayoutCompatible()
 */

void QRhiImplementation::updateLayoutDesc(QRhiShaderResourceBindings *srb)
{
    srb->m_layoutDescHash = 0;
    srb->m_layoutDesc.clear();
    auto layoutDescAppender = std::back_inserter(srb->m_layoutDesc);
    for (const QRhiShaderResourceBinding &b : std::as_const(srb->m_bindings)) {
        const QRhiShaderResourceBinding::Data *d = &b.d;
        srb->m_layoutDescHash ^= uint(d->binding) ^ uint(d->stage) ^ uint(d->type)
            ^ uint(d->arraySize());
        layoutDescAppender = d->serialize(layoutDescAppender);
    }
}

/*!
    \fn virtual bool QRhiShaderResourceBindings::create() = 0

    Creates the corresponding resource binding set. Depending on the underlying
    graphics API, this may involve creating native graphics resources, and
    therefore it should not be assumed that this is a cheap operation.

    If create() has been called before with no corresponding destroy(), then
    destroy() is called implicitly first.

    \return \c true when successful, \c false when failed.
    Regardless of the return value, calling destroy() is always safe.
 */

/*!
    \fn void QRhiShaderResourceBindings::setBindings(std::initializer_list<QRhiShaderResourceBinding> list)
    Sets the \a list of bindings.
 */

/*!
    \fn template<typename InputIterator> void QRhiShaderResourceBindings::setBindings(InputIterator first, InputIterator last)
    Sets the list of bindings from the iterators \a first and \a last.
 */

/*!
    \fn const QRhiShaderResourceBinding *QRhiShaderResourceBindings::cbeginBindings() const
    \return a const iterator pointing to the first item in the binding list.
 */

/*!
    \fn const QRhiShaderResourceBinding *QRhiShaderResourceBindings::cendBindings() const
    \return a const iterator pointing just after the last item in the binding list.
 */

/*!
    \fn const QRhiShaderResourceBinding *QRhiShaderResourceBindings::bindingAt(qsizetype index) const
    \return the binding at the specified \a index.
 */

/*!
    \fn qsizetype QRhiShaderResourceBindings::bindingCount() const
    \return the number of bindings.
 */

/*!
    \class QRhiShaderResourceBinding
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Describes the shader resource for a single binding point.

    A QRhiShaderResourceBinding cannot be constructed directly. Instead, use the
    static functions such as uniformBuffer() or sampledTexture() to get an
    instance.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \enum QRhiShaderResourceBinding::Type
    Specifies type of the shader resource bound to a binding point

    \value UniformBuffer Uniform buffer

    \value SampledTexture Combined image sampler (a texture and sampler pair).
    Even when the shading language associated with the underlying 3D API has no
    support for this concept (e.g. D3D and HLSL), this is still supported
    because the shader translation layer takes care of the appropriate
    translation and remapping of binding points or shader registers.

    \value Texture Texture (separate)

    \value Sampler Sampler (separate)

    \value ImageLoad Image load (with GLSL this maps to doing imageLoad() on a
    single level - and either one or all layers - of a texture exposed to the
    shader as an image object)

    \value ImageStore Image store (with GLSL this maps to doing imageStore() or
    imageAtomic*() on a single level - and either one or all layers - of a
    texture exposed to the shader as an image object)

    \value ImageLoadStore Image load and store

    \value BufferLoad Storage buffer load (with GLSL this maps to reading from
    a shader storage buffer)

    \value BufferStore Storage buffer store (with GLSL this maps to writing to
    a shader storage buffer)

    \value BufferLoadStore Storage buffer load and store
 */

/*!
    \enum QRhiShaderResourceBinding::StageFlag
    Flag values to indicate which stages the shader resource is visible in

    \value VertexStage Vertex stage
    \value TessellationControlStage Tessellation control (hull shader) stage
    \value TessellationEvaluationStage Tessellation evaluation (domain shader) stage
    \value FragmentStage Fragment (pixel shader) stage
    \value ComputeStage Compute stage
    \value GeometryStage Geometry stage
 */

/*!
    \return \c true if the layout is compatible with \a other. The layout does not
    include the actual resource (such as, buffer or texture) and related
    parameters (such as, offset or size).

    For example, \c a and \c b below are not equal, but are compatible layout-wise:

    \code
        auto a = QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, buffer);
        auto b = QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, someOtherBuffer, 256);
    \endcode
 */
bool QRhiShaderResourceBinding::isLayoutCompatible(const QRhiShaderResourceBinding &other) const
{
    // everything that goes into a VkDescriptorSetLayoutBinding must match
    return d.binding == other.d.binding
            && d.stage == other.d.stage
            && d.type == other.d.type
            && d.arraySize() == other.d.arraySize();
}

/*!
    \return a shader resource binding for the given binding number, pipeline
    stages, and buffer specified by \a binding, \a stage, and \a buf.

    \note When \a buf is not null, it must have been created with
    QRhiBuffer::UniformBuffer.

    \note \a buf can be null. It is valid to create a
    QRhiShaderResourceBindings with unspecified resources, but such an object
    cannot be used with QRhiCommandBuffer::setShaderResources(). It is however
    suitable for creating pipelines. Such a pipeline must then always be used
    together with another, layout compatible QRhiShaderResourceBindings with
    resources present passed to QRhiCommandBuffer::setShaderResources().

    \note If the size of \a buf exceeds the limit reported for
    QRhi::MaxUniformBufferRange, unexpected errors may occur.
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::uniformBuffer(
        int binding, StageFlags stage, QRhiBuffer *buf)
{
    QRhiShaderResourceBinding b;
    b.d.binding = binding;
    b.d.stage = stage;
    b.d.type = UniformBuffer;
    b.d.u.ubuf.buf = buf;
    b.d.u.ubuf.offset = 0;
    b.d.u.ubuf.maybeSize = 0; // entire buffer
    b.d.u.ubuf.hasDynamicOffset = false;
    return b;
}

/*!
    \return a shader resource binding for the given binding number, pipeline
    stages, and buffer specified by \a binding, \a stage, and \a buf. This
    overload binds a region only, as specified by \a offset and \a size.

    \note It is up to the user to ensure the offset is aligned to
    QRhi::ubufAlignment().

    \note \a size must be greater than 0.

    \note When \a buf is not null, it must have been created with
    QRhiBuffer::UniformBuffer.

    \note \a buf can be null. It is valid to create a
    QRhiShaderResourceBindings with unspecified resources, but such an object
    cannot be used with QRhiCommandBuffer::setShaderResources(). It is however
    suitable for creating pipelines. Such a pipeline must then always be used
    together with another, layout compatible QRhiShaderResourceBindings with
    resources present passed to QRhiCommandBuffer::setShaderResources().

    \note If \a size exceeds the limit reported for QRhi::MaxUniformBufferRange,
    unexpected errors may occur.
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::uniformBuffer(
        int binding, StageFlags stage, QRhiBuffer *buf, quint32 offset, quint32 size)
{
    Q_ASSERT(size > 0);
    QRhiShaderResourceBinding b;
    b.d.binding = binding;
    b.d.stage = stage;
    b.d.type = UniformBuffer;
    b.d.u.ubuf.buf = buf;
    b.d.u.ubuf.offset = offset;
    b.d.u.ubuf.maybeSize = size;
    b.d.u.ubuf.hasDynamicOffset = false;
    return b;
}

/*!
    \return a shader resource binding for the given binding number, pipeline
    stages, and buffer specified by \a binding, \a stage, and \a buf. The
    uniform buffer is assumed to have dynamic offset. The dynamic offset can be
    specified in QRhiCommandBuffer::setShaderResources(), thus allowing using
    varying offset values without creating new bindings for the buffer. The
    size of the bound region is specified by \a size. Like with non-dynamic
    offsets, \c{offset + size} cannot exceed the size of \a buf.

    \note When \a buf is not null, it must have been created with
    QRhiBuffer::UniformBuffer.

    \note \a buf can be null. It is valid to create a
    QRhiShaderResourceBindings with unspecified resources, but such an object
    cannot be used with QRhiCommandBuffer::setShaderResources(). It is however
    suitable for creating pipelines. Such a pipeline must then always be used
    together with another, layout compatible QRhiShaderResourceBindings with
    resources present passed to QRhiCommandBuffer::setShaderResources().

    \note If \a size exceeds the limit reported for QRhi::MaxUniformBufferRange,
    unexpected errors may occur.
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
        int binding, StageFlags stage, QRhiBuffer *buf, quint32 size)
{
    Q_ASSERT(size > 0);
    QRhiShaderResourceBinding b;
    b.d.binding = binding;
    b.d.stage = stage;
    b.d.type = UniformBuffer;
    b.d.u.ubuf.buf = buf;
    b.d.u.ubuf.offset = 0;
    b.d.u.ubuf.maybeSize = size;
    b.d.u.ubuf.hasDynamicOffset = true;
    return b;
}

/*!
    \return a shader resource binding for the given binding number, pipeline
    stages, texture, and sampler specified by \a binding, \a stage, \a tex,
    \a sampler.

    \note This function is equivalent to calling sampledTextures() with a
    \c count of 1.

    \note \a tex and \a sampler can be null. It is valid to create a
    QRhiShaderResourceBindings with unspecified resources, but such an object
    cannot be used with QRhiCommandBuffer::setShaderResources(). It is however
    suitable for creating pipelines. Such a pipeline must then always be used
    together with another, layout compatible QRhiShaderResourceBindings with
    resources present passed to QRhiCommandBuffer::setShaderResources().

    \note A shader may not be able to consume more than 16 textures/samplers,
    depending on the underlying graphics API. This hard limit must be kept in
    mind in renderer design. This does not apply to texture arrays which
    consume a single binding point (shader register) and can contain 256-2048
    textures, depending on the underlying graphics API. Arrays of textures (see
    sampledTextures()) are however no different in this regard than using the
    same number of individual textures.

    \sa sampledTextures()
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::sampledTexture(
        int binding, StageFlags stage, QRhiTexture *tex, QRhiSampler *sampler)
{
    QRhiShaderResourceBinding b;
    b.d.binding = binding;
    b.d.stage = stage;
    b.d.type = SampledTexture;
    b.d.u.stex.count = 1;
    b.d.u.stex.texSamplers[0] = { tex, sampler };
    return b;
}

/*!
    \return a shader resource binding for the given binding number, pipeline
    stages, and the array of texture-sampler pairs specified by \a binding, \a
    stage, \a count, and \a texSamplers.

    \note \a count must be at least 1, and not larger than 16.

    \note When \a count is 1, this function is equivalent to sampledTexture().

    This function is relevant when arrays of combined image samplers are
    involved. For example, in GLSL \c{layout(binding = 5) uniform sampler2D
    shadowMaps[8];} declares an array of combined image samplers. The
    application is then expected provide a QRhiShaderResourceBinding for
    binding point 5, set up by calling this function with \a count set to 8 and
    a valid texture and sampler for each element of the array.

    \warning All elements of the array must be specified. With the above
    example, the only valid, portable approach is calling this function with a
    \a count of 8. Additionally, all QRhiTexture and QRhiSampler instances must
    be valid, meaning nullptr is not an accepted value. This is due to some of
    the underlying APIs, such as, Vulkan, that require a valid image and
    sampler object for each element in descriptor arrays. Applications are
    advised to provide "dummy" samplers and textures if some array elements are
    not relevant (due to not being accessed in the shader).

    \note \a texSamplers can be null. It is valid to create a
    QRhiShaderResourceBindings with unspecified resources, but such an object
    cannot be used with QRhiCommandBuffer::setShaderResources(). It is however
    suitable for creating pipelines. Such a pipeline must then always be used
    together with another, layout compatible QRhiShaderResourceBindings with
    resources present passed to QRhiCommandBuffer::setShaderResources().

    \sa sampledTexture()
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::sampledTextures(
        int binding, StageFlags stage, int count, const TextureAndSampler *texSamplers)
{
    Q_ASSERT(count >= 1 && count <= Data::MAX_TEX_SAMPLER_ARRAY_SIZE);
    QRhiShaderResourceBinding b;
    b.d.binding = binding;
    b.d.stage = stage;
    b.d.type = SampledTexture;
    b.d.u.stex.count = count;
    for (int i = 0; i < count; ++i) {
        if (texSamplers)
            b.d.u.stex.texSamplers[i] = texSamplers[i];
        else
            b.d.u.stex.texSamplers[i] = { nullptr, nullptr };
    }
    return b;
}

/*!
    \return a shader resource binding for the given binding number, pipeline
    stages, and texture specified by \a binding, \a stage, \a tex.

    \note This function is equivalent to calling textures() with a
    \c count of 1.

    \note \a tex can be null. It is valid to create a
    QRhiShaderResourceBindings with unspecified resources, but such an object
    cannot be used with QRhiCommandBuffer::setShaderResources(). It is however
    suitable for creating pipelines. Such a pipeline must then always be used
    together with another, layout compatible QRhiShaderResourceBindings with
    resources present passed to QRhiCommandBuffer::setShaderResources().

    This creates a binding for a separate texture (image) object, whereas
    sampledTexture() is suitable for combined image samplers. In
    Vulkan-compatible GLSL code separate textures are declared as \c texture2D
    as opposed to \c sampler2D: \c{layout(binding = 1) uniform texture2D tex;}

    \note A shader may not be able to consume more than 16 textures, depending
    on the underlying graphics API. This hard limit must be kept in mind in
    renderer design. This does not apply to texture arrays which consume a
    single binding point (shader register) and can contain 256-2048 textures,
    depending on the underlying graphics API. Arrays of textures (see
    sampledTextures()) are however no different in this regard than using the
    same number of individual textures.

    \sa textures(), sampler()
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::texture(int binding, StageFlags stage, QRhiTexture *tex)
{
    QRhiShaderResourceBinding b;
    b.d.binding = binding;
    b.d.stage = stage;
    b.d.type = Texture;
    b.d.u.stex.count = 1;
    b.d.u.stex.texSamplers[0] = { tex, nullptr };
    return b;
}

/*!
    \return a shader resource binding for the given binding number, pipeline
    stages, and the array of (separate) textures specified by \a binding, \a
    stage, \a count, and \a tex.

    \note \a count must be at least 1, and not larger than 16.

    \note When \a count is 1, this function is equivalent to texture().

    \warning All elements of the array must be specified.

    \note \a tex can be null. It is valid to create a
    QRhiShaderResourceBindings with unspecified resources, but such an object
    cannot be used with QRhiCommandBuffer::setShaderResources(). It is however
    suitable for creating pipelines. Such a pipeline must then always be used
    together with another, layout compatible QRhiShaderResourceBindings with
    resources present passed to QRhiCommandBuffer::setShaderResources().

    \sa texture(), sampler()
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::textures(int binding, StageFlags stage, int count, QRhiTexture **tex)
{
    Q_ASSERT(count >= 1 && count <= Data::MAX_TEX_SAMPLER_ARRAY_SIZE);
    QRhiShaderResourceBinding b;
    b.d.binding = binding;
    b.d.stage = stage;
    b.d.type = Texture;
    b.d.u.stex.count = count;
    for (int i = 0; i < count; ++i) {
        if (tex)
            b.d.u.stex.texSamplers[i] = { tex[i], nullptr };
        else
            b.d.u.stex.texSamplers[i] = { nullptr, nullptr };
    }
    return b;
}

/*!
    \return a shader resource binding for the given binding number, pipeline
    stages, and sampler specified by \a binding, \a stage, \a sampler.

    \note \a sampler can be null. It is valid to create a
    QRhiShaderResourceBindings with unspecified resources, but such an object
    cannot be used with QRhiCommandBuffer::setShaderResources(). It is however
    suitable for creating pipelines. Such a pipeline must then always be used
    together with another, layout compatible QRhiShaderResourceBindings with
    resources present passed to QRhiCommandBuffer::setShaderResources().

    Arrays of separate samplers are not supported.

    This creates a binding for a separate sampler object, whereas
    sampledTexture() is suitable for combined image samplers. In
    Vulkan-compatible GLSL code separate samplers are declared as \c sampler
    as opposed to \c sampler2D: \c{layout(binding = 2) uniform sampler samp;}

    With both a \c texture2D and \c sampler present, they can be used together
    to sample the texture: \c{fragColor = texture(sampler2D(tex, samp),
    texcoord);}.

    \note A shader may not be able to consume more than 16 samplers, depending
    on the underlying graphics API. This hard limit must be kept in mind in
    renderer design.

    \sa texture()
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::sampler(int binding, StageFlags stage, QRhiSampler *sampler)
{
    QRhiShaderResourceBinding b;
    b.d.binding = binding;
    b.d.stage = stage;
    b.d.type = Sampler;
    b.d.u.stex.count = 1;
    b.d.u.stex.texSamplers[0] = { nullptr, sampler };
    return b;
}

/*!
   \return a shader resource binding for a read-only storage image with the
   given \a binding number and pipeline \a stage. The image load operations
   will have access to all layers of the specified \a level. (so if the texture
   is a cubemap, the shader must use imageCube instead of image2D)

   \note When \a tex is not null, it must have been created with
   QRhiTexture::UsedWithLoadStore.

   \note \a tex can be null. It is valid to create a QRhiShaderResourceBindings
   with unspecified resources, but such an object cannot be used with
   QRhiCommandBuffer::setShaderResources(). It is however suitable for creating
   pipelines. Such a pipeline must then always be used together with another,
   layout compatible QRhiShaderResourceBindings with resources present passed
   to QRhiCommandBuffer::setShaderResources().

   \note Image load/store is only available within the compute and fragment stages.
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::imageLoad(
        int binding, StageFlags stage, QRhiTexture *tex, int level)
{
    QRhiShaderResourceBinding b;
    b.d.binding = binding;
    b.d.stage = stage;
    b.d.type = ImageLoad;
    b.d.u.simage.tex = tex;
    b.d.u.simage.level = level;
    return b;
}

/*!
   \return a shader resource binding for a write-only storage image with the
   given \a binding number and pipeline \a stage. The image store operations
   will have access to all layers of the specified \a level. (so if the texture
   is a cubemap, the shader must use imageCube instead of image2D)

   \note When \a tex is not null, it must have been created with
   QRhiTexture::UsedWithLoadStore.

   \note \a tex can be null. It is valid to create a QRhiShaderResourceBindings
   with unspecified resources, but such an object cannot be used with
   QRhiCommandBuffer::setShaderResources(). It is however suitable for creating
   pipelines. Such a pipeline must then always be used together with another,
   layout compatible QRhiShaderResourceBindings with resources present passed
   to QRhiCommandBuffer::setShaderResources().

   \note Image load/store is only available within the compute and fragment stages.
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::imageStore(
        int binding, StageFlags stage, QRhiTexture *tex, int level)
{
    QRhiShaderResourceBinding b;
    b.d.binding = binding;
    b.d.stage = stage;
    b.d.type = ImageStore;
    b.d.u.simage.tex = tex;
    b.d.u.simage.level = level;
    return b;
}

/*!
   \return a shader resource binding for a read/write storage image with the
   given \a binding number and pipeline \a stage. The image load/store operations
   will have access to all layers of the specified \a level. (so if the texture
   is a cubemap, the shader must use imageCube instead of image2D)

   \note When \a tex is not null, it must have been created with
   QRhiTexture::UsedWithLoadStore.

   \note \a tex can be null. It is valid to create a QRhiShaderResourceBindings
   with unspecified resources, but such an object cannot be used with
   QRhiCommandBuffer::setShaderResources(). It is however suitable for creating
   pipelines. Such a pipeline must then always be used together with another,
   layout compatible QRhiShaderResourceBindings with resources present passed
   to QRhiCommandBuffer::setShaderResources().

   \note Image load/store is only available within the compute and fragment stages.
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::imageLoadStore(
        int binding, StageFlags stage, QRhiTexture *tex, int level)
{
    QRhiShaderResourceBinding b;
    b.d.binding = binding;
    b.d.stage = stage;
    b.d.type = ImageLoadStore;
    b.d.u.simage.tex = tex;
    b.d.u.simage.level = level;
    return b;
}

/*!
    \return a shader resource binding for a read-only storage buffer with the
    given \a binding number and pipeline \a stage.

    \note When \a buf is not null, must have been created with
    QRhiBuffer::StorageBuffer.

    \note \a buf can be null. It is valid to create a
    QRhiShaderResourceBindings with unspecified resources, but such an object
    cannot be used with QRhiCommandBuffer::setShaderResources(). It is however
    suitable for creating pipelines. Such a pipeline must then always be used
    together with another, layout compatible QRhiShaderResourceBindings with
    resources present passed to QRhiCommandBuffer::setShaderResources().

    \note Buffer load/store is only guaranteed to be available within a compute
    pipeline. While some backends may support using these resources in a
    graphics pipeline as well, this is not universally supported, and even when
    it is, unexpected problems may arise when it comes to barriers and
    synchronization. Therefore, avoid using such resources with shaders other
    than compute.
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::bufferLoad(
        int binding, StageFlags stage, QRhiBuffer *buf)
{
    QRhiShaderResourceBinding b;
    b.d.binding = binding;
    b.d.stage = stage;
    b.d.type = BufferLoad;
    b.d.u.sbuf.buf = buf;
    b.d.u.sbuf.offset = 0;
    b.d.u.sbuf.maybeSize = 0; // entire buffer
    return b;
}

/*!
    \return a shader resource binding for a read-only storage buffer with the
    given \a binding number and pipeline \a stage. This overload binds a region
    only, as specified by \a offset and \a size.

    \note When \a buf is not null, must have been created with
    QRhiBuffer::StorageBuffer.

    \note \a buf can be null. It is valid to create a
    QRhiShaderResourceBindings with unspecified resources, but such an object
    cannot be used with QRhiCommandBuffer::setShaderResources(). It is however
    suitable for creating pipelines. Such a pipeline must then always be used
    together with another, layout compatible QRhiShaderResourceBindings with
    resources present passed to QRhiCommandBuffer::setShaderResources().

    \note Buffer load/store is only guaranteed to be available within a compute
    pipeline. While some backends may support using these resources in a
    graphics pipeline as well, this is not universally supported, and even when
    it is, unexpected problems may arise when it comes to barriers and
    synchronization. Therefore, avoid using such resources with shaders other
    than compute.
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::bufferLoad(
        int binding, StageFlags stage, QRhiBuffer *buf, quint32 offset, quint32 size)
{
    Q_ASSERT(size > 0);
    QRhiShaderResourceBinding b;
    b.d.binding = binding;
    b.d.stage = stage;
    b.d.type = BufferLoad;
    b.d.u.sbuf.buf = buf;
    b.d.u.sbuf.offset = offset;
    b.d.u.sbuf.maybeSize = size;
    return b;
}

/*!
    \return a shader resource binding for a write-only storage buffer with the
    given \a binding number and pipeline \a stage.

    \note When \a buf is not null, must have been created with
    QRhiBuffer::StorageBuffer.

    \note \a buf can be null. It is valid to create a
    QRhiShaderResourceBindings with unspecified resources, but such an object
    cannot be used with QRhiCommandBuffer::setShaderResources(). It is however
    suitable for creating pipelines. Such a pipeline must then always be used
    together with another, layout compatible QRhiShaderResourceBindings with
    resources present passed to QRhiCommandBuffer::setShaderResources().

    \note Buffer load/store is only guaranteed to be available within a compute
    pipeline. While some backends may support using these resources in a
    graphics pipeline as well, this is not universally supported, and even when
    it is, unexpected problems may arise when it comes to barriers and
    synchronization. Therefore, avoid using such resources with shaders other
    than compute.
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::bufferStore(
        int binding, StageFlags stage, QRhiBuffer *buf)
{
    QRhiShaderResourceBinding b;
    b.d.binding = binding;
    b.d.stage = stage;
    b.d.type = BufferStore;
    b.d.u.sbuf.buf = buf;
    b.d.u.sbuf.offset = 0;
    b.d.u.sbuf.maybeSize = 0; // entire buffer
    return b;
}

/*!
    \return a shader resource binding for a write-only storage buffer with the
    given \a binding number and pipeline \a stage. This overload binds a region
    only, as specified by \a offset and \a size.

    \note When \a buf is not null, must have been created with
    QRhiBuffer::StorageBuffer.

    \note \a buf can be null. It is valid to create a
    QRhiShaderResourceBindings with unspecified resources, but such an object
    cannot be used with QRhiCommandBuffer::setShaderResources(). It is however
    suitable for creating pipelines. Such a pipeline must then always be used
    together with another, layout compatible QRhiShaderResourceBindings with
    resources present passed to QRhiCommandBuffer::setShaderResources().

    \note Buffer load/store is only guaranteed to be available within a compute
    pipeline. While some backends may support using these resources in a
    graphics pipeline as well, this is not universally supported, and even when
    it is, unexpected problems may arise when it comes to barriers and
    synchronization. Therefore, avoid using such resources with shaders other
    than compute.
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::bufferStore(
        int binding, StageFlags stage, QRhiBuffer *buf, quint32 offset, quint32 size)
{
    Q_ASSERT(size > 0);
    QRhiShaderResourceBinding b;
    b.d.binding = binding;
    b.d.stage = stage;
    b.d.type = BufferStore;
    b.d.u.sbuf.buf = buf;
    b.d.u.sbuf.offset = offset;
    b.d.u.sbuf.maybeSize = size;
    return b;
}

/*!
    \return a shader resource binding for a read-write storage buffer with the
    given \a binding number and pipeline \a stage.

    \note When \a buf is not null, must have been created with
    QRhiBuffer::StorageBuffer.

    \note \a buf can be null. It is valid to create a
    QRhiShaderResourceBindings with unspecified resources, but such an object
    cannot be used with QRhiCommandBuffer::setShaderResources(). It is however
    suitable for creating pipelines. Such a pipeline must then always be used
    together with another, layout compatible QRhiShaderResourceBindings with
    resources present passed to QRhiCommandBuffer::setShaderResources().

    \note Buffer load/store is only guaranteed to be available within a compute
    pipeline. While some backends may support using these resources in a
    graphics pipeline as well, this is not universally supported, and even when
    it is, unexpected problems may arise when it comes to barriers and
    synchronization. Therefore, avoid using such resources with shaders other
    than compute.
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::bufferLoadStore(
        int binding, StageFlags stage, QRhiBuffer *buf)
{
    QRhiShaderResourceBinding b;
    b.d.binding = binding;
    b.d.stage = stage;
    b.d.type = BufferLoadStore;
    b.d.u.sbuf.buf = buf;
    b.d.u.sbuf.offset = 0;
    b.d.u.sbuf.maybeSize = 0; // entire buffer
    return b;
}

/*!
    \return a shader resource binding for a read-write storage buffer with the
    given \a binding number and pipeline \a stage. This overload binds a region
    only, as specified by \a offset and \a size.

    \note When \a buf is not null, must have been created with
    QRhiBuffer::StorageBuffer.

    \note \a buf can be null. It is valid to create a
    QRhiShaderResourceBindings with unspecified resources, but such an object
    cannot be used with QRhiCommandBuffer::setShaderResources(). It is however
    suitable for creating pipelines. Such a pipeline must then always be used
    together with another, layout compatible QRhiShaderResourceBindings with
    resources present passed to QRhiCommandBuffer::setShaderResources().

    \note Buffer load/store is only guaranteed to be available within a compute
    pipeline. While some backends may support using these resources in a
    graphics pipeline as well, this is not universally supported, and even when
    it is, unexpected problems may arise when it comes to barriers and
    synchronization. Therefore, avoid using such resources with shaders other
    than compute.
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::bufferLoadStore(
        int binding, StageFlags stage, QRhiBuffer *buf, quint32 offset, quint32 size)
{
    Q_ASSERT(size > 0);
    QRhiShaderResourceBinding b;
    b.d.binding = binding;
    b.d.stage = stage;
    b.d.type = BufferLoadStore;
    b.d.u.sbuf.buf = buf;
    b.d.u.sbuf.offset = offset;
    b.d.u.sbuf.maybeSize = size;
    return b;
}

/*!
    \return \c true if the contents of the two QRhiShaderResourceBinding
    objects \a a and \a b are equal. This includes the resources (buffer,
    texture) and related parameters (offset, size) as well. To only compare
    layouts (binding point, pipeline stage, resource type), use
    \l{QRhiShaderResourceBinding::isLayoutCompatible()}{isLayoutCompatible()}
    instead.

    \relates QRhiShaderResourceBinding
 */
bool operator==(const QRhiShaderResourceBinding &a, const QRhiShaderResourceBinding &b) noexcept
{
    const QRhiShaderResourceBinding::Data *da = QRhiImplementation::shaderResourceBindingData(a);
    const QRhiShaderResourceBinding::Data *db = QRhiImplementation::shaderResourceBindingData(b);

    if (da == db)
        return true;


    if (da->binding != db->binding
            || da->stage != db->stage
            || da->type != db->type)
    {
        return false;
    }

    switch (da->type) {
    case QRhiShaderResourceBinding::UniformBuffer:
        if (da->u.ubuf.buf != db->u.ubuf.buf
                || da->u.ubuf.offset != db->u.ubuf.offset
                || da->u.ubuf.maybeSize != db->u.ubuf.maybeSize)
        {
            return false;
        }
        break;
    case QRhiShaderResourceBinding::SampledTexture:
        if (da->u.stex.count != db->u.stex.count)
            return false;
        for (int i = 0; i < da->u.stex.count; ++i) {
            if (da->u.stex.texSamplers[i].tex != db->u.stex.texSamplers[i].tex
                    || da->u.stex.texSamplers[i].sampler != db->u.stex.texSamplers[i].sampler)
            {
                return false;
            }
        }
        break;
    case QRhiShaderResourceBinding::Texture:
        if (da->u.stex.count != db->u.stex.count)
            return false;
        for (int i = 0; i < da->u.stex.count; ++i) {
            if (da->u.stex.texSamplers[i].tex != db->u.stex.texSamplers[i].tex)
                return false;
        }
        break;
    case QRhiShaderResourceBinding::Sampler:
        if (da->u.stex.texSamplers[0].sampler != db->u.stex.texSamplers[0].sampler)
            return false;
        break;
    case QRhiShaderResourceBinding::ImageLoad:
    case QRhiShaderResourceBinding::ImageStore:
    case QRhiShaderResourceBinding::ImageLoadStore:
        if (da->u.simage.tex != db->u.simage.tex
                || da->u.simage.level != db->u.simage.level)
        {
            return false;
        }
        break;
    case QRhiShaderResourceBinding::BufferLoad:
    case QRhiShaderResourceBinding::BufferStore:
    case QRhiShaderResourceBinding::BufferLoadStore:
        if (da->u.sbuf.buf != db->u.sbuf.buf
                || da->u.sbuf.offset != db->u.sbuf.offset
                || da->u.sbuf.maybeSize != db->u.sbuf.maybeSize)
        {
            return false;
        }
        break;
    default:
        Q_UNREACHABLE_RETURN(false);
    }

    return true;
}

/*!
    \return \c false if all the bindings in the two QRhiShaderResourceBinding
    objects \a a and \a b are equal; otherwise returns \c true.

    \relates QRhiShaderResourceBinding
 */
bool operator!=(const QRhiShaderResourceBinding &a, const QRhiShaderResourceBinding &b) noexcept
{
    return !(a == b);
}

/*!
    \fn size_t qHash(const QRhiShaderResourceBinding &key, size_t seed)
    \qhashold{QRhiShaderResourceBinding}
 */
size_t qHash(const QRhiShaderResourceBinding &b, size_t seed) noexcept
{
    const QRhiShaderResourceBinding::Data *d = QRhiImplementation::shaderResourceBindingData(b);
    QtPrivate::QHashCombineWithSeed hash(seed);
    seed = hash(seed, d->binding);
    seed = hash(seed, d->stage);
    seed = hash(seed, d->type);
    switch (d->type) {
    case QRhiShaderResourceBinding::UniformBuffer:
        seed = hash(seed, reinterpret_cast<quintptr>(d->u.ubuf.buf));
        break;
    case QRhiShaderResourceBinding::SampledTexture:
        seed = hash(seed, reinterpret_cast<quintptr>(d->u.stex.texSamplers[0].tex));
        seed = hash(seed, reinterpret_cast<quintptr>(d->u.stex.texSamplers[0].sampler));
        break;
    case QRhiShaderResourceBinding::Texture:
        seed = hash(seed, reinterpret_cast<quintptr>(d->u.stex.texSamplers[0].tex));
        break;
    case QRhiShaderResourceBinding::Sampler:
        seed = hash(seed, reinterpret_cast<quintptr>(d->u.stex.texSamplers[0].sampler));
        break;
    case QRhiShaderResourceBinding::ImageLoad:
    case QRhiShaderResourceBinding::ImageStore:
    case QRhiShaderResourceBinding::ImageLoadStore:
        seed = hash(seed, reinterpret_cast<quintptr>(d->u.simage.tex));
        break;
    case QRhiShaderResourceBinding::BufferLoad:
    case QRhiShaderResourceBinding::BufferStore:
    case QRhiShaderResourceBinding::BufferLoadStore:
        seed = hash(seed, reinterpret_cast<quintptr>(d->u.sbuf.buf));
        break;
    }
    return seed;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QRhiShaderResourceBinding &b)
{
    QDebugStateSaver saver(dbg);
    const QRhiShaderResourceBinding::Data *d = QRhiImplementation::shaderResourceBindingData(b);
    dbg.nospace() << "QRhiShaderResourceBinding("
                  << "binding=" << d->binding
                  << " stage=" << d->stage
                  << " type=" << d->type;
    switch (d->type) {
    case QRhiShaderResourceBinding::UniformBuffer:
        dbg.nospace() << " UniformBuffer("
                      << "buffer=" << d->u.ubuf.buf
                      << " offset=" << d->u.ubuf.offset
                      << " maybeSize=" << d->u.ubuf.maybeSize
                      << ')';
        break;
    case QRhiShaderResourceBinding::SampledTexture:
        dbg.nospace() << " SampledTextures("
                      << "count=" << d->u.stex.count;
        for (int i = 0; i < d->u.stex.count; ++i) {
            dbg.nospace() << " texture=" << d->u.stex.texSamplers[i].tex
                          << " sampler=" << d->u.stex.texSamplers[i].sampler;
        }
        dbg.nospace() << ')';
        break;
    case QRhiShaderResourceBinding::Texture:
        dbg.nospace() << " Textures("
                      << "count=" << d->u.stex.count;
        for (int i = 0; i < d->u.stex.count; ++i)
            dbg.nospace() << " texture=" << d->u.stex.texSamplers[i].tex;
        dbg.nospace() << ')';
        break;
    case QRhiShaderResourceBinding::Sampler:
        dbg.nospace() << " Sampler("
                      << " sampler=" << d->u.stex.texSamplers[0].sampler
                      << ')';
        break;
    case QRhiShaderResourceBinding::ImageLoad:
        dbg.nospace() << " ImageLoad("
                      << "texture=" << d->u.simage.tex
                      << " level=" << d->u.simage.level
                      << ')';
        break;
    case QRhiShaderResourceBinding::ImageStore:
        dbg.nospace() << " ImageStore("
                      << "texture=" << d->u.simage.tex
                      << " level=" << d->u.simage.level
                      << ')';
        break;
    case QRhiShaderResourceBinding::ImageLoadStore:
        dbg.nospace() << " ImageLoadStore("
                      << "texture=" << d->u.simage.tex
                      << " level=" << d->u.simage.level
                      << ')';
        break;
    case QRhiShaderResourceBinding::BufferLoad:
        dbg.nospace() << " BufferLoad("
                      << "buffer=" << d->u.sbuf.buf
                      << " offset=" << d->u.sbuf.offset
                      << " maybeSize=" << d->u.sbuf.maybeSize
                      << ')';
        break;
    case QRhiShaderResourceBinding::BufferStore:
        dbg.nospace() << " BufferStore("
                      << "buffer=" << d->u.sbuf.buf
                      << " offset=" << d->u.sbuf.offset
                      << " maybeSize=" << d->u.sbuf.maybeSize
                      << ')';
        break;
    case QRhiShaderResourceBinding::BufferLoadStore:
        dbg.nospace() << " BufferLoadStore("
                      << "buffer=" << d->u.sbuf.buf
                      << " offset=" << d->u.sbuf.offset
                      << " maybeSize=" << d->u.sbuf.maybeSize
                      << ')';
        break;
    default:
        dbg.nospace() << " UNKNOWN()";
        break;
    }
    dbg.nospace() << ')';
    return dbg;
}
#endif

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QRhiShaderResourceBindings &srb)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QRhiShaderResourceBindings("
                  << srb.m_bindings
                  << ')';
    return dbg;
}
#endif

/*!
    \class QRhiGraphicsPipeline
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Graphics pipeline state resource.

    Represents a graphics pipeline. What exactly this map to in the underlying
    native graphics API, varies. Where there is a concept of pipeline objects,
    for example with Vulkan, the QRhi backend will create such an object upon
    calling create(). Elsewhere, for example with OpenGL, the
    QRhiGraphicsPipeline may merely collect the various state, and create()'s
    main task is to set up the corresponding shader program, but deferring
    looking at any of the requested state to a later point.

    As with all QRhiResource subclasses, the two-phased initialization pattern
    applies: setting any values via the setters, for example setDepthTest(), is
    only effective after calling create(). Avoid changing any values once the
    QRhiGraphicsPipeline has been initialized via create(). To change some
    state, set the new value and call create() again. However, that will
    effectively release all underlying native resources and create new ones. As
    a result, it may be a heavy, expensive operation. Rather, prefer creating
    multiple pipelines with the different states, and
    \l{QRhiCommandBuffer::setGraphicsPipeline()}{switch between them} when
    recording the render pass.

    \note Setting the shader stages is mandatory. There must be at least one
    stage, and there must be a vertex stage.

    \note Setting the shader resource bindings is mandatory. The referenced
    QRhiShaderResourceBindings must already have create() called on it by the
    time create() is called. Associating with a QRhiShaderResourceBindings that
    has no bindings is also valid, as long as no shader in any stage expects any
    resources. Using a QRhiShaderResourceBindings object that does not specify
    any actual resources (i.e., the buffers, textures, etc. for the binding
    points are set to \nullptr) is valid as well, as long as a
    \l{QRhiShaderResourceBindings::isLayoutCompatible()}{layout-compatible}
    QRhiShaderResourceBindings, that specifies resources for all the bindings,
    is going to be set via
    \l{QRhiCommandBuffer::setShaderResources()}{setShaderResources()} when
    recording the render pass.

    \note Setting the render pass descriptor is mandatory. To obtain a
    QRhiRenderPassDescriptor that can be passed to setRenderPassDescriptor(),
    use either QRhiTextureRenderTarget::newCompatibleRenderPassDescriptor() or
    QRhiSwapChain::newCompatibleRenderPassDescriptor().

    \note Setting the vertex input layout is mandatory.

    \note sampleCount() defaults to 1 and must match the sample count of the
    render target's color and depth stencil attachments.

    \note The depth test, depth write, and stencil test are disabled by
    default. The face culling mode defaults to no culling.

    \note stencilReadMask() and stencilWriteMask() apply to both faces. They
    both default to 0xFF.

    \section2 Example usage

    All settings of a graphics pipeline have defaults which might be suitable
    to many applications. Therefore a minimal example of creating a graphics
    pipeline could be the following. This assumes that the vertex shader takes
    a single \c{vec3 position} input at the input location 0. With the
    QRhiShaderResourceBindings and QRhiRenderPassDescriptor objects, plus the
    QShader collections for the vertex and fragment stages, a pipeline could be
    created like this:

    \code
        QRhiShaderResourceBindings *srb;
        QRhiRenderPassDescriptor *rpDesc;
        QShader vs, fs;
        // ...

        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({ { 3 * sizeof(float) } });
        inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float3, 0 } });

        QRhiGraphicsPipeline *ps = rhi->newGraphicsPipeline();
        ps->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
        ps->setVertexInputLayout(inputLayout);
        ps->setShaderResourceBindings(srb);
        ps->setRenderPassDescriptor(rpDesc);
        if (!ps->create()) { error(); }
    \endcode

    The above code creates a pipeline object that uses the defaults for many
    settings and states. For example, it will use a \l Triangles topology, no
    backface culling, blending is disabled but color write is enabled for all
    four channels, depth test/write are disabled, stencil operations are
    disabled.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    \sa QRhiCommandBuffer, QRhi
 */

/*!
    \enum QRhiGraphicsPipeline::Flag

    Flag values for describing the dynamic state of the pipeline, and other
    options. The viewport is always dynamic.

    \value UsesBlendConstants Indicates that a blend color constant will be set
    via QRhiCommandBuffer::setBlendConstants()

    \value UsesStencilRef Indicates that a stencil reference value will be set
    via QRhiCommandBuffer::setStencilRef()

    \value UsesScissor Indicates that a scissor rectangle will be set via
    QRhiCommandBuffer::setScissor()

    \value CompileShadersWithDebugInfo Requests compiling shaders with debug
    information enabled. This is relevant only when runtime shader compilation
    from source code is involved, and only when the underlying infrastructure
    supports this. With concrete examples, this is not relevant with Vulkan and
    SPIR-V, because the GLSL-to-SPIR-V compilation does not happen at run
    time. On the other hand, consider Direct3D and HLSL, where there are
    multiple options: when the QShader packages ship with pre-compiled bytecode
    (\c DXBC), debug information is to be requested through the tool that
    generates the \c{.qsb} file, similarly to the case of Vulkan and
    SPIR-V. However, when having HLSL source code in the pre- or
    runtime-generated QShader packages, the first phase of compilation (HLSL
    source to intermediate format) happens at run time too, with this flag taken
    into account. Debug information is relevant in particular with tools like
    RenderDoc since it allows seeing the original source code when investigating
    the pipeline and when performing vertex or fragment shader debugging.

    \value UsesShadingRate Indicates that a per-draw (per-pipeline) shading rate
    value will be set via QRhiCommandBuffer::setShadingRate(). Not specifying
    this flag and still calling setShadingRate() may lead to varying, unexpected
    results depending on the underlying graphics API.
 */

/*!
    \enum QRhiGraphicsPipeline::Topology
    Specifies the primitive topology

    \value Triangles (default)
    \value TriangleStrip
    \value TriangleFan (only available if QRhi::TriangleFanTopology is supported)
    \value Lines
    \value LineStrip
    \value Points

    \value Patches (only available if QRhi::Tessellation is supported, and
    requires the tessellation stages to be present in the pipeline)
 */

/*!
    \enum QRhiGraphicsPipeline::CullMode
    Specifies the culling mode

    \value None No culling (default)
    \value Front Cull front faces
    \value Back Cull back faces
 */

/*!
    \enum QRhiGraphicsPipeline::FrontFace
    Specifies the front face winding order

    \value CCW Counter clockwise (default)
    \value CW Clockwise
 */

/*!
    \enum QRhiGraphicsPipeline::ColorMaskComponent
    Flag values for specifying the color write mask

    \value R
    \value G
    \value B
    \value A
 */

/*!
    \enum QRhiGraphicsPipeline::BlendFactor
    Specifies the blend factor

    \value Zero
    \value One
    \value SrcColor
    \value OneMinusSrcColor
    \value DstColor
    \value OneMinusDstColor
    \value SrcAlpha
    \value OneMinusSrcAlpha
    \value DstAlpha
    \value OneMinusDstAlpha
    \value ConstantColor
    \value OneMinusConstantColor
    \value ConstantAlpha
    \value OneMinusConstantAlpha
    \value SrcAlphaSaturate
    \value Src1Color
    \value OneMinusSrc1Color
    \value Src1Alpha
    \value OneMinusSrc1Alpha
 */

/*!
    \enum QRhiGraphicsPipeline::BlendOp
    Specifies the blend operation

    \value Add
    \value Subtract
    \value ReverseSubtract
    \value Min
    \value Max
 */

/*!
    \enum QRhiGraphicsPipeline::CompareOp
    Specifies the depth or stencil comparison function

    \value Never
    \value Less (default for depth)
    \value Equal
    \value LessOrEqual
    \value Greater
    \value NotEqual
    \value GreaterOrEqual
    \value Always (default for stencil)
 */

/*!
    \enum QRhiGraphicsPipeline::StencilOp
    Specifies the stencil operation

    \value StencilZero
    \value Keep (default)
    \value Replace
    \value IncrementAndClamp
    \value DecrementAndClamp
    \value Invert
    \value IncrementAndWrap
    \value DecrementAndWrap
 */

/*!
    \enum QRhiGraphicsPipeline::PolygonMode
    \brief Specifies the polygon rasterization mode

    Polygon Mode (Triangle Fill Mode in Metal, Fill Mode in D3D) specifies
    the fill mode used when rasterizing polygons.  Polygons may be drawn as
    solids (Fill), or as a wire mesh (Line).

    Support for non-fill polygon modes is optional and is indicated by the
    QRhi::NonFillPolygonMode feature. With OpenGL ES and some Vulkan
    implementations the feature will likely be reported as unsupported, which
    then means values other than Fill cannot be used.

    \value Fill The interior of the polygon is filled (default)
    \value Line Boundary edges of the polygon are drawn as line segments.
 */

/*!
    \struct QRhiGraphicsPipeline::TargetBlend
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Describes the blend state for one color attachment.

    Defaults to color write enabled, blending disabled. The blend values are
    set up for pre-multiplied alpha (One, OneMinusSrcAlpha, One,
    OneMinusSrcAlpha) by default. This means that to get the alpha blending
    mode Qt Quick uses, it is enough to set the \c enable flag to true while
    leaving other values at their defaults.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \variable QRhiGraphicsPipeline::TargetBlend::colorWrite
 */

/*!
    \variable QRhiGraphicsPipeline::TargetBlend::enable
 */

/*!
    \variable QRhiGraphicsPipeline::TargetBlend::srcColor
 */

/*!
    \variable QRhiGraphicsPipeline::TargetBlend::dstColor
 */

/*!
    \variable QRhiGraphicsPipeline::TargetBlend::opColor
 */

/*!
    \variable QRhiGraphicsPipeline::TargetBlend::srcAlpha
 */

/*!
    \variable QRhiGraphicsPipeline::TargetBlend::dstAlpha
 */

/*!
    \variable QRhiGraphicsPipeline::TargetBlend::opAlpha
 */

/*!
    \struct QRhiGraphicsPipeline::StencilOpState
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Describes the stencil operation state.

    The default-constructed StencilOpState has the following set:
    \list
    \li failOp - \l Keep
    \li depthFailOp - \l Keep
    \li passOp - \l Keep
    \li compareOp \l Always
    \endlist

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \variable QRhiGraphicsPipeline::StencilOpState::failOp
 */

/*!
    \variable QRhiGraphicsPipeline::StencilOpState::depthFailOp
 */

/*!
    \variable QRhiGraphicsPipeline::StencilOpState::passOp
 */

/*!
    \variable QRhiGraphicsPipeline::StencilOpState::compareOp
 */

/*!
    \internal
 */
QRhiGraphicsPipeline::QRhiGraphicsPipeline(QRhiImplementation *rhi)
    : QRhiResource(rhi)
{
}

/*!
    \return the resource type.
 */
QRhiResource::Type QRhiGraphicsPipeline::resourceType() const
{
    return GraphicsPipeline;
}

/*!
    \fn virtual bool QRhiGraphicsPipeline::create() = 0

    Creates the corresponding native graphics resources. If there are already
    resources present due to an earlier create() with no corresponding
    destroy(), then destroy() is called implicitly first.

    \return \c true when successful, \c false when a graphics operation failed.
    Regardless of the return value, calling destroy() is always safe.

    \note This may be, depending on the underlying graphics API, an expensive
    operation, especially when shaders get compiled/optimized from source or
    from an intermediate bytecode format to the GPU's own instruction set.
    Where applicable, the QRhi backend automatically sets up the relevant
    non-persistent facilities to accelerate this, for example the Vulkan
    backend automatically creates a \c VkPipelineCache to improve data reuse
    during the lifetime of the application.

    \note Drivers may also employ various persistent (disk-based) caching
    strategies for shader and pipeline data, which is hidden to and is outside
    of Qt's control. In some cases, depending on the graphics API and the QRhi
    backend, there are facilities within QRhi for manually managing such a
    cache, allowing the retrieval of a serializable blob that can then be
    reloaded in the future runs of the application to ensure faster pipeline
    creation times. See QRhi::pipelineCacheData() and
    QRhi::setPipelineCacheData() for details. Note also that when working with
    a QRhi instance managed by a higher level Qt framework, such as Qt Quick,
    it is possible that such disk-based caching is taken care of automatically,
    for example QQuickWindow uses a disk-based pipeline cache by default (which
    comes in addition to any driver-level caching).
 */

/*!
    \fn QRhiGraphicsPipeline::Flags QRhiGraphicsPipeline::flags() const
    \return the currently set flags.
 */

/*!
    \fn void QRhiGraphicsPipeline::setFlags(Flags f)
    Sets the flags \a f.
 */

/*!
    \fn QRhiGraphicsPipeline::Topology QRhiGraphicsPipeline::topology() const
    \return the currently set primitive topology.
 */

/*!
    \fn void QRhiGraphicsPipeline::setTopology(Topology t)
    Sets the primitive topology \a t.
 */

/*!
    \fn QRhiGraphicsPipeline::CullMode QRhiGraphicsPipeline::cullMode() const
    \return the currently set face culling mode.
 */

/*!
    \fn void QRhiGraphicsPipeline::setCullMode(CullMode mode)
    Sets the specified face culling \a mode.
 */

/*!
    \fn QRhiGraphicsPipeline::FrontFace QRhiGraphicsPipeline::frontFace() const
    \return the currently set front face mode.
 */

/*!
    \fn void QRhiGraphicsPipeline::setFrontFace(FrontFace f)
    Sets the front face mode \a f.
 */

/*!
    \fn void QRhiGraphicsPipeline::setTargetBlends(std::initializer_list<TargetBlend> list)

    Sets the \a list of render target blend settings. This is a list because
    when multiple render targets are used (i.e., a QRhiTextureRenderTarget with
    more than one QRhiColorAttachment), there needs to be a TargetBlend
    structure per render target (color attachment).

    By default there is one default-constructed TargetBlend set.

    \sa QRhi::MaxColorAttachments
 */

/*!
    \fn template<typename InputIterator> void QRhiGraphicsPipeline::setTargetBlends(InputIterator first, InputIterator last)
    Sets the list of render target blend settings from the iterators \a first and \a last.
 */

/*!
    \fn const QRhiGraphicsPipeline::TargetBlend *QRhiGraphicsPipeline::cbeginTargetBlends() const
    \return a const iterator pointing to the first item in the render target blend setting list.
 */

/*!
    \fn const QRhiGraphicsPipeline::TargetBlend *QRhiGraphicsPipeline::cendTargetBlends() const
    \return a const iterator pointing just after the last item in the render target blend setting list.
 */

/*!
    \fn const QRhiGraphicsPipeline::TargetBlend *QRhiGraphicsPipeline::targetBlendAt(qsizetype index) const
    \return the render target blend setting at the specified \a index.
 */

/*!
    \fn qsizetype QRhiGraphicsPipeline::targetBlendCount() const
    \return the number of render target blend settings.
 */

/*!
    \fn bool QRhiGraphicsPipeline::hasDepthTest() const
    \return true if depth testing is enabled.
 */

/*!
    \fn void QRhiGraphicsPipeline::setDepthTest(bool enable)

    Enables or disables depth testing based on \a enable. Both depth test and
    the writing out of depth data are disabled by default.

    \sa setDepthWrite()
 */

/*!
    \fn bool QRhiGraphicsPipeline::hasDepthWrite() const
    \return true if depth write is enabled.
 */

/*!
    \fn void QRhiGraphicsPipeline::setDepthWrite(bool enable)

    Controls the writing out of depth data into the depth buffer based on
    \a enable. By default this is disabled. Depth write is typically enabled
    together with the depth test.

    \note Enabling depth write without having depth testing enabled may not
    lead to the desired result, and should be avoided.

    \sa setDepthTest()
 */

/*!
    \fn QRhiGraphicsPipeline::CompareOp QRhiGraphicsPipeline::depthOp() const
    \return the depth comparison function.
 */

/*!
    \fn void QRhiGraphicsPipeline::setDepthOp(CompareOp op)
    Sets the depth comparison function \a op.
 */

/*!
    \fn bool QRhiGraphicsPipeline::hasStencilTest() const
    \return true if stencil testing is enabled.
 */

/*!
    \fn void QRhiGraphicsPipeline::setStencilTest(bool enable)
    Enables or disables stencil tests based on \a enable.
    By default this is disabled.
 */

/*!
    \fn QRhiGraphicsPipeline::StencilOpState QRhiGraphicsPipeline::stencilFront() const
    \return the current stencil test state for front faces.
 */

/*!
    \fn void QRhiGraphicsPipeline::setStencilFront(const StencilOpState &state)
    Sets the stencil test \a state for front faces.
 */

/*!
    \fn QRhiGraphicsPipeline::StencilOpState QRhiGraphicsPipeline::stencilBack() const
    \return the current stencil test state for back faces.
 */

/*!
    \fn void QRhiGraphicsPipeline::setStencilBack(const StencilOpState &state)
    Sets the stencil test \a state for back faces.
 */

/*!
    \fn quint32 QRhiGraphicsPipeline::stencilReadMask() const
    \return the currrent stencil read mask.
 */

/*!
    \fn void QRhiGraphicsPipeline::setStencilReadMask(quint32 mask)
    Sets the stencil read \a mask. The default value is 0xFF.
 */

/*!
    \fn quint32 QRhiGraphicsPipeline::stencilWriteMask() const
    \return the current stencil write mask.
 */

/*!
    \fn void QRhiGraphicsPipeline::setStencilWriteMask(quint32 mask)
    Sets the stencil write \a mask. The default value is 0xFF.
 */

/*!
    \fn int QRhiGraphicsPipeline::sampleCount() const
    \return the currently set sample count. 1 means no multisample antialiasing.
 */

/*!
    \fn void QRhiGraphicsPipeline::setSampleCount(int s)

    Sets the sample count. Typical values for \a s are 1, 4, or 8. The pipeline
    must always be compatible with the render target, i.e. the sample counts
    must match.

    \sa QRhi::supportedSampleCounts()
 */

/*!
    \fn float QRhiGraphicsPipeline::lineWidth() const
    \return the currently set line width. The default is 1.0f.
 */

/*!
    \fn void QRhiGraphicsPipeline::setLineWidth(float width)

    Sets the line \a width. If the QRhi::WideLines feature is reported as
    unsupported at runtime, values other than 1.0f are ignored.
 */

/*!
    \fn int QRhiGraphicsPipeline::depthBias() const
    \return the currently set depth bias.
 */

/*!
    \fn void QRhiGraphicsPipeline::setDepthBias(int bias)
    Sets the depth \a bias. The default value is 0.
 */

/*!
    \fn float QRhiGraphicsPipeline::slopeScaledDepthBias() const
    \return the currently set slope scaled depth bias.
 */

/*!
    \fn void QRhiGraphicsPipeline::setSlopeScaledDepthBias(float bias)
    Sets the slope scaled depth \a bias. The default value is 0.
 */

/*!
    \fn void QRhiGraphicsPipeline::setShaderStages(std::initializer_list<QRhiShaderStage> list)
    Sets the \a list of shader stages.
 */

/*!
    \fn template<typename InputIterator> void QRhiGraphicsPipeline::setShaderStages(InputIterator first, InputIterator last)
    Sets the list of shader stages from the iterators \a first and \a last.
 */

/*!
    \fn const QRhiShaderStage *QRhiGraphicsPipeline::cbeginShaderStages() const
    \return a const iterator pointing to the first item in the shader stage list.
 */

/*!
    \fn const QRhiShaderStage *QRhiGraphicsPipeline::cendShaderStages() const
    \return a const iterator pointing just after the last item in the shader stage list.
 */

/*!
    \fn const QRhiShaderStage *QRhiGraphicsPipeline::shaderStageAt(qsizetype index) const
    \return the shader stage at the specified \a index.
 */

/*!
    \fn qsizetype QRhiGraphicsPipeline::shaderStageCount() const
    \return the number of shader stages in this pipeline.
 */

/*!
    \fn QRhiVertexInputLayout QRhiGraphicsPipeline::vertexInputLayout() const
    \return the currently set vertex input layout specification.
 */

/*!
    \fn void QRhiGraphicsPipeline::setVertexInputLayout(const QRhiVertexInputLayout &layout)
    Specifies the vertex input \a layout.
 */

/*!
    \fn QRhiShaderResourceBindings *QRhiGraphicsPipeline::shaderResourceBindings() const
    \return the currently associated QRhiShaderResourceBindings object.
 */

/*!
    \fn void QRhiGraphicsPipeline::setShaderResourceBindings(QRhiShaderResourceBindings *srb)

    Associates with \a srb describing the resource binding layout and the
    resources (QRhiBuffer, QRhiTexture) themselves. The latter is optional,
    because only the layout matters during pipeline creation. Therefore, the \a
    srb passed in here can leave the actual buffer or texture objects
    unspecified (\nullptr) as long as there is another,
    \l{QRhiShaderResourceBindings::isLayoutCompatible()}{layout-compatible}
    QRhiShaderResourceBindings bound via
    \l{QRhiCommandBuffer::setShaderResources()}{setShaderResources()} before
    recording the draw calls.
 */

/*!
    \fn QRhiRenderPassDescriptor *QRhiGraphicsPipeline::renderPassDescriptor() const
    \return the currently set QRhiRenderPassDescriptor.
 */

/*!
    \fn void QRhiGraphicsPipeline::setRenderPassDescriptor(QRhiRenderPassDescriptor *desc)
    Associates with the specified QRhiRenderPassDescriptor \a desc.
 */

/*!
    \fn int QRhiGraphicsPipeline::patchControlPointCount() const
    \return the currently set patch control point count.
 */

/*!
    \fn void QRhiGraphicsPipeline::setPatchControlPointCount(int count)

    Sets the number of patch control points to \a count. The default value is
    3. This is used only when the topology is set to \l Patches.
 */

/*!
    \fn QRhiGraphicsPipeline::PolygonMode QRhiGraphicsPipeline::polygonMode() const
    \return the polygon mode.
 */

/*!
    \fn void QRhiGraphicsPipeline::setPolygonMode(PolygonMode mode)
    Sets the polygon \a mode. The default is Fill.

    \sa QRhi::NonFillPolygonMode
 */

/*!
    \fn int QRhiGraphicsPipeline::multiViewCount() const
    \return the view count. The default is 0, indicating no multiview rendering.
    \since 6.7
 */

/*!
    \fn void QRhiGraphicsPipeline::setMultiViewCount(int count)
    Sets the view \a count for multiview rendering. The default is 0,
    indicating no multiview rendering.
    \a count must be 2 or larger to trigger multiview rendering.

    Multiview is only available when the \l{QRhi::MultiView}{MultiView feature}
    is reported as supported. The render target must be a 2D texture array, and
    the color attachment for the render target must have the same \a count set.

    See QRhiColorAttachment::setMultiViewCount() for further details on
    multiview rendering.

    \since 6.7
    \sa QRhi::MultiView, QRhiColorAttachment::setMultiViewCount()
 */

/*!
    \class QRhiSwapChain
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Swapchain resource.

    A swapchain enables presenting rendering results to a surface. A swapchain
    is typically backed by a set of color buffers. Of these, one is displayed
    at a time.

    Below is a typical pattern for creating and managing a swapchain and some
    associated resources in order to render onto a QWindow:

    \code
      void init()
      {
          sc = rhi->newSwapChain();
          ds = rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil,
                                    QSize(), // no need to set the size here due to UsedWithSwapChainOnly
                                    1,
                                    QRhiRenderBuffer::UsedWithSwapChainOnly);
          sc->setWindow(window);
          sc->setDepthStencil(ds);
          rp = sc->newCompatibleRenderPassDescriptor();
          sc->setRenderPassDescriptor(rp);
          resizeSwapChain();
      }

      void resizeSwapChain()
      {
          hasSwapChain = sc->createOrResize();
      }

      void render()
      {
          if (!hasSwapChain || notExposed)
              return;

          if (sc->currentPixelSize() != sc->surfacePixelSize() || newlyExposed) {
              resizeSwapChain();
              if (!hasSwapChain)
                  return;
              newlyExposed = false;
          }

          rhi->beginFrame(sc);
          // ...
          rhi->endFrame(sc);
      }
    \endcode

    Avoid relying on QWindow resize events to resize swapchains, especially
    considering that surface sizes may not always fully match the QWindow
    reported dimensions. The safe, cross-platform approach is to do the check
    via surfacePixelSize() whenever starting a new frame.

    Releasing the swapchain must happen while the QWindow and the underlying
    native window is fully up and running. Building on the previous example:

    \code
        void releaseSwapChain()
        {
            if (hasSwapChain) {
                sc->destroy();
                hasSwapChain = false;
            }
        }

        // assuming Window is our QWindow subclass
        bool Window::event(QEvent *e)
        {
            switch (e->type()) {
            case QEvent::UpdateRequest: // for QWindow::requestUpdate()
                render();
                break;
            case QEvent::PlatformSurface:
                if (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
                    releaseSwapChain();
                break;
            default:
                break;
            }
            return QWindow::event(e);
        }
    \endcode

    Initializing the swapchain and starting to render the first frame cannot
    start at any time. The safe, cross-platform approach is to rely on expose
    events. QExposeEvent is a loosely specified event that is sent whenever a
    window gets mapped, obscured, and resized, depending on the platform.

    \code
        void Window::exposeEvent(QExposeEvent *)
        {
            // initialize and start rendering when the window becomes usable for graphics purposes
            if (isExposed() && !running) {
                running = true;
                init();
            }

            // stop pushing frames when not exposed or size becomes 0
            if ((!isExposed() || (hasSwapChain && sc->surfacePixelSize().isEmpty())) && running)
                notExposed = true;

            // continue when exposed again and the surface has a valid size
            if (isExposed() && running && notExposed && !sc->surfacePixelSize().isEmpty()) {
                notExposed = false;
                newlyExposed = true;
            }

            if (isExposed() && !sc->surfacePixelSize().isEmpty())
                render();
        }
    \endcode

    Once the rendering has started, a simple way to request a new frame is
    QWindow::requestUpdate(). While on some platforms this is merely a small
    timer, on others it has a specific implementation: for instance on macOS or
    iOS it may be backed by
    \l{https://developer.apple.com/documentation/corevideo/cvdisplaylink?language=objc}{CVDisplayLink}.
    The example above is already prepared for update requests by handling
    QEvent::UpdateRequest.

    While acting as a QRhiRenderTarget, QRhiSwapChain also manages a
    QRhiCommandBuffer. Calling QRhi::endFrame() submits the recorded commands
    and also enqueues a \c present request. The default behavior is to do this
    with a swap interval of 1, meaning synchronizing to the display's vertical
    refresh is enabled. Thus the rendering thread calling beginFrame() and
    endFrame() will get throttled to vsync. On some backends this can be
    disabled by passing QRhiSwapChain:NoVSync in flags().

    Multisampling (MSAA) is handled transparently to the applications when
    requested via setSampleCount(). Where applicable, QRhiSwapChain will take
    care of creating additional color buffers and issuing a multisample resolve
    command at the end of a frame. For OpenGL, it is necessary to request the
    appropriate sample count also via QSurfaceFormat, by calling
    QSurfaceFormat::setDefaultFormat() before initializing the QRhi.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \enum QRhiSwapChain::Flag
    Flag values to describe swapchain properties

    \value SurfaceHasPreMulAlpha Indicates that the target surface has
    transparency with premultiplied alpha. For example, this is what Qt Quick
    uses when the alpha channel is enabled on the target QWindow, because the
    scenegraph rendrerer always outputs fragments with alpha multiplied into
    the red, green, and blue values. To ensure identical behavior across
    platforms, always set QSurfaceFormat::alphaBufferSize() to a non-zero value
    on the target QWindow whenever this flag is set on the swapchain.

    \value SurfaceHasNonPreMulAlpha Indicates the target surface has
    transparency with non-premultiplied alpha. Be aware that this may not be
    supported on some systems, if the system compositor always expects content
    with premultiplied alpha. In that case the behavior with this flag set is
    expected to be equivalent to SurfaceHasPreMulAlpha.

    \value sRGB Requests to pick an sRGB format for the swapchain's color
    buffers and/or render target views, where applicable. Note that this
    implies that sRGB framebuffer update and blending will get enabled for all
    content targeting this swapchain, and opting out is not possible. For
    OpenGL, set \l{QSurfaceFormat::sRGBColorSpace}{sRGBColorSpace} on the
    QSurfaceFormat of the QWindow in addition. Applicable only when the
    swapchain format is set to QRhiSwapChain::SDR.

    \value UsedAsTransferSource Indicates the swapchain will be used as the
    source of a readback in QRhiResourceUpdateBatch::readBackTexture().

    \value NoVSync Requests disabling waiting for vertical sync, also avoiding
    throttling the rendering thread. The behavior is backend specific and
    applicable only where it is possible to control this. Some may ignore the
    request altogether. For OpenGL, try instead setting the swap interval to 0
    on the QWindow via QSurfaceFormat::setSwapInterval().

    \value MinimalBufferCount Requests creating the swapchain with the minimum
    number of buffers, which is in practice 2, unless the graphics
    implementation has a higher minimum number than that. Only applicable with
    backends where such control is available via the graphics API, for example,
    Vulkan. By default it is up to the backend to decide what number of buffers
    it requests (in practice this is almost always either 2 or 3), and it is
    not the applications' concern. However, on Vulkan for instance the backend
    will likely prefer the higher number (3), for example to avoid odd
    performance issues with some Vulkan implementations on mobile devices. It
    could be that on some platforms it can prove to be beneficial to force the
    lower buffer count (2), so this flag allows forcing that. Note that all
    this has no effect on the number of frames kept in flight, so the CPU
    (QRhi) will still prepare frames at most \c{N - 1} frames ahead of the GPU,
    even when the swapchain image buffer count larger than \c N. (\c{N} =
    QRhi::FramesInFlight and typically 2).
 */

/*!
    \enum QRhiSwapChain::Format
    Describes the swapchain format. The default format is SDR.

    This enum is used with
    \l{QRhiSwapChain::isFormatSupported()}{isFormatSupported()} to check
    upfront if creating the swapchain with the given format is supported by the
    platform and the window's associated screen, and with
    \l{QRhiSwapChain::setFormat()}{setFormat()}
    to set the requested format in the swapchain before calling
    \l{QRhiSwapChain::createOrResize()}{createOrResize()} for the first time.

    \value SDR 8-bit RGBA or BGRA, depending on the backend and platform. With
    OpenGL ES in particular, it could happen that the platform provides less
    than 8 bits (e.g. due to EGL and the QSurfaceFormat choosing a 565 or 444
    format - this is outside the control of QRhi). Standard dynamic range. May
    be combined with setting the QRhiSwapChain::sRGB flag.

    \value HDRExtendedSrgbLinear 16-bit float RGBA, high dynamic range,
    extended linear sRGB (scRGB) color space. This involves Rec. 709 primaries
    (same as SDR/sRGB) and linear colors. Conversion to the display's native
    color space (such as, HDR10) is performed by the windowing system. On
    Windows this is the canonical color space of the system compositor, and is
    the recommended format for HDR swapchains in general on desktop platforms.

    \value HDR10 10-bit unsigned int RGB or BGR with 2 bit alpha, high dynamic
    range, HDR10 (Rec. 2020) color space with an ST2084 PQ transfer function.

    \value HDRExtendedDisplayP3Linear 16-bit float RGBA, high dynamic range,
    extended linear Display P3 color space. The primary choice for HDR on
    platforms such as iOS and VisionOS.
 */

/*!
    \internal
 */
QRhiSwapChain::QRhiSwapChain(QRhiImplementation *rhi)
    : QRhiResource(rhi)
{
}

/*!
    \return the resource type.
 */
QRhiResource::Type QRhiSwapChain::resourceType() const
{
    return SwapChain;
}

/*!
    \fn QSize QRhiSwapChain::currentPixelSize() const

    \return the size with which the swapchain was last successfully built. Use
    this to decide if createOrResize() needs to be called again: if
    \c{currentPixelSize() != surfacePixelSize()} then the swapchain needs to be
    resized.

    \note Typical rendering logic will call this function to get the output
    size when starting to prepare a new frame, and base dependent calculations
    (such as, the viewport) on the size returned from this function.

    While in many cases the value is the same as \c{QWindow::size() *
    QWindow::devicePixelRatio()}, relying on the QWindow-reported size is not
    guaranteed to be correct on all platforms and graphics API implementations.
    Using this function is therefore strongly recommended whenever there is a
    need to identify the dimensions, in pixels, of the output layer or surface.

    This also has the added benefit of avoiding potential data races when QRhi
    is used on a dedicated rendering thread, because the need to call QWindow
    functions, that may then access data updated on the main thread, is
    avoided.

    \sa surfacePixelSize()
  */

/*!
    \fn virtual QSize QRhiSwapChain::surfacePixelSize() = 0

    \return The size of the window's associated surface or layer.

    \warning Do not assume this is the same as \c{QWindow::size() *
    QWindow::devicePixelRatio()}. With some graphics APIs and windowing system
    interfaces (for example, Vulkan) there is a theoretical possibility for a
    surface to assume a size different from the associated window. To support
    these cases, \b{rendering logic must always base size-derived calculations
    (such as, viewports) on the size reported from QRhiSwapChain, and never on
    the size queried from QWindow}.

    \note \b{Can also be called before createOrResize(), if at least window() is
    already set. This in combination with currentPixelSize() allows to detect
    when a swapchain needs to be resized.} However, watch out for the fact that
    the size of the underlying native object (surface, layer, or similar) is
    "live", so whenever this function is called, it returns the latest value
    reported by the underlying implementation, without any atomicity guarantee.
    Therefore, using this function to determine pixel sizes for graphics
    resources that are used in a frame is strongly discouraged. Rely on
    currentPixelSize() instead which returns a size that is atomic and will not
    change between createOrResize() invocations.

    \note For depth-stencil buffers used in combination with the swapchain's
    color buffers, it is strongly recommended to rely on the automatic sizing
    and rebuilding behavior provided by the
    QRhiRenderBuffer:UsedWithSwapChainOnly flag. Avoid querying the surface
    size via this function just to get a size that can be passed to
    QRhiRenderBuffer::setPixelSize() as that would suffer from the lack of
    atomicity as described above.

    \sa currentPixelSize()
  */

/*!
    \fn virtual bool QRhiSwapChain::isFormatSupported(Format f) = 0

    \return true if the given swapchain format \a f is supported. SDR is always
    supported.

    \note Can be called independently of createOrResize(), but window() must
    already be set. Calling without the window set may lead to unexpected
    results depending on the backend and platform (most likely false for any
    HDR format), because HDR format support is usually tied to the output
    (screen) to which the swapchain's associated window belongs at any given
    time. If the result is true for a HDR format, then creating the swapchain
    with that format is expected to succeed as long as the window is not moved
    to another screen in the meantime.

    The main use of this function is to call it before the first
    createOrResize() after the window is already set. This allow the QRhi
    backends to perform platform or windowing system specific queries to
    determine if the window (and the screen it is on) is capable of true HDR
    output with the specified format.

    When the format is reported as supported, call setFormat() to set the
    requested format and call createOrResize(). Be aware of the consequences
    however: successfully requesting a HDR format will involve having to deal
    with a different color space, possibly doing white level correction for
    non-HDR-aware content, adjusting tonemapping methods, adjusting offscreen
    render target settings, etc.

    \sa setFormat()
 */

/*!
    \fn virtual QRhiCommandBuffer *QRhiSwapChain::currentFrameCommandBuffer() = 0

    \return a command buffer on which rendering commands and resource updates
    can be recorded within a \l{QRhi::beginFrame()}{beginFrame} -
    \l{QRhi::endFrame()}{endFrame} block, assuming beginFrame() was called with
    this swapchain.

    \note The returned object is valid also after endFrame(), up until the next
    beginFrame(), but the returned command buffer should not be used to record
    any commands then. Rather, it can be used to query data collected during
    the frame (or previous frames), for example by calling
    \l{QRhiCommandBuffer::lastCompletedGpuTime()}{lastCompletedGpuTime()}.

    \note The value must not be cached and reused between frames. The caller
    should not hold on to the returned object once
    \l{QRhi::beginFrame()}{beginFrame()} is called again. Instead, the command
    buffer object should be queried again by calling this function.
*/

/*!
    \fn virtual QRhiRenderTarget *QRhiSwapChain::currentFrameRenderTarget() = 0

    \return a render target that can used with beginPass() in order to render
    the swapchain's current backbuffer. Only valid within a
    QRhi::beginFrame() - QRhi::endFrame() block where beginFrame() was called
    with this swapchain.

    \note the value must not be cached and reused between frames
 */

/*!
    \enum QRhiSwapChain::StereoTargetBuffer
    Selects the backbuffer to use with a stereoscopic swapchain.

    \value LeftBuffer
    \value RightBuffer
 */

/*!
    \return a render target that can be used with beginPass() in order to
    render to the swapchain's left or right backbuffer. This overload should be
    used only with stereoscopic rendering, that is, when the associated QWindow
    is backed by two color buffers, one for each eye, instead of just one.

    When stereoscopic rendering is not supported, the return value will be
    the default target. It is supported by all hardware backends except for Metal, in
    combination with \l QSurfaceFormat::StereoBuffers, assuming it is supported
    by the graphics and display driver stack at run time. Metal and Null backends
    are going to return the default render target from this overload.

    \note the value must not be cached and reused between frames
 */
QRhiRenderTarget *QRhiSwapChain::currentFrameRenderTarget(StereoTargetBuffer targetBuffer)
{
    Q_UNUSED(targetBuffer);
    return currentFrameRenderTarget();
}

/*!
    \fn virtual bool QRhiSwapChain::createOrResize() = 0

    Creates the swapchain if not already done and resizes the swapchain buffers
    to match the current size of the targeted surface. Call this whenever the
    size of the target surface is different than before.

    \note call destroy() only when the swapchain needs to be released
    completely, typically upon
    QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed. To perform resizing, just
    call createOrResize().

    \return \c true when successful, \c false when a graphics operation failed.
    Regardless of the return value, calling destroy() is always safe.
 */

/*!
    \fn QWindow *QRhiSwapChain::window() const
    \return the currently set window.
 */

/*!
    \fn void QRhiSwapChain::setWindow(QWindow *window)
    Sets the \a window.
 */

/*!
    \fn QRhiSwapChainProxyData QRhiSwapChain::proxyData() const
    \return the currently set proxy data.
 */

/*!
    \fn void QRhiSwapChain::setProxyData(const QRhiSwapChainProxyData &d)
    Sets the proxy data \a d.

    \sa QRhi::updateSwapChainProxyData()
 */

/*!
    \fn QRhiSwapChain::Flags QRhiSwapChain::flags() const
    \return the currently set flags.
 */

/*!
    \fn void QRhiSwapChain::setFlags(Flags f)
    Sets the flags \a f.
 */

/*!
    \fn QRhiSwapChain::Format QRhiSwapChain::format() const
    \return the currently set format.
 */

/*!
    \fn void QRhiSwapChain::setFormat(Format f)
    Sets the format \a f.

    Avoid setting formats that are reported as unsupported from
    isFormatSupported(). Note that support for a given format may depend on the
    screen the swapchain's associated window is opened on. On some platforms,
    such as Windows and macOS, for HDR output to work it is necessary to have
    HDR output enabled in the display settings.

    See isFormatSupported(), \l QRhiSwapChainHdrInfo, and \l Format for more
    information on high dynamic range output.
 */

/*!
    \fn QRhiRenderBuffer *QRhiSwapChain::depthStencil() const
    \return the currently associated renderbuffer for depth-stencil.
 */

/*!
    \fn void QRhiSwapChain::setDepthStencil(QRhiRenderBuffer *ds)
    Sets the renderbuffer \a ds for use as a depth-stencil buffer.
 */

/*!
    \fn int QRhiSwapChain::sampleCount() const
    \return the currently set sample count. 1 means no multisample antialiasing.
 */

/*!
    \fn void QRhiSwapChain::setSampleCount(int samples)

    Sets the sample count. Common values for \a samples are 1 (no MSAA), 4 (4x
    MSAA), or 8 (8x MSAA).

    \sa QRhi::supportedSampleCounts()
 */

/*!
    \fn QRhiRenderPassDescriptor *QRhiSwapChain::renderPassDescriptor() const
    \return the currently associated QRhiRenderPassDescriptor object.
 */

/*!
    \fn void QRhiSwapChain::setRenderPassDescriptor(QRhiRenderPassDescriptor *desc)
    Associates with the QRhiRenderPassDescriptor \a desc.
 */

/*!
    \fn virtual QRhiRenderPassDescriptor *QRhiSwapChain::newCompatibleRenderPassDescriptor() = 0;

    \return a new QRhiRenderPassDescriptor that is compatible with this swapchain.

    The returned value is used in two ways: it can be passed to
    setRenderPassDescriptor() and
    QRhiGraphicsPipeline::setRenderPassDescriptor(). A render pass descriptor
    describes the attachments (color, depth/stencil) and the load/store
    behavior that can be affected by flags(). A QRhiGraphicsPipeline can only
    be used in combination with a swapchain that has a
    \l{QRhiRenderPassDescriptor::isCompatible()}{compatible}
    QRhiRenderPassDescriptor set.

    \sa createOrResize()
 */

/*!
    \fn QRhiShadingRateMap *QRhiSwapChain::shadingRateMap() const
    \return the currently set QRhiShadingRateMap. By default this is \nullptr.
    \since 6.9
 */

/*!
    \fn void QRhiSwapChain::setShadingRateMap(QRhiShadingRateMap *map)

    Associates with the specified QRhiShadingRateMap \a map. This is functional
    only when the \l QRhi::VariableRateShadingMap feature is reported as
    supported.

    When QRhiCommandBuffer::setShadingRate() is also called, the higher of two
    the shading rates are used for each tile. There is currently no control
    offered over the combiner behavior.

    \note Setting a shading rate map implies that a different, new
    QRhiRenderPassDescriptor is needed and some of the native swapchain objects
    must be rebuilt. Therefore, if the swapchain is already set up, call
    newCompatibleRenderPassDescriptor() and setRenderPassDescriptor() right
    after setShadingRateMap(). Then, createOrResize() must also be called again.
    This has rolling consequences, for example for graphics pipelines: those
    also need to be associated with the new QRhiRenderPassDescriptor and then
    rebuilt. See \l QRhiRenderPassDescriptor::serializedFormat() for some
    suggestions on how to deal with this. Remember to set the
    QRhiGraphicsPipeline::UsesShadingRate flag for them as well.

    \since 6.9
 */

/*!
    \struct QRhiSwapChainHdrInfo
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6

    \brief Describes the high dynamic range related information of the
    swapchain's associated output.

    To perform HDR-compatible tonemapping, where the target range is not [0,1],
    one often needs to know the maximum luminance of the display the
    swapchain's window is associated with. While this is often made
    user-configurable (think brightness, gamma and similar settings in games),
    it can be highly useful to set defaults based on the values reported by the
    display itself, thus providing a decent starting point.

    There are some problems however: the information is exposed in different
    forms on different platforms, whereas with cross-platform graphics APIs
    there is often no associated solution at all, because managing such
    information is not in the scope of the API (and may rather be retrievable
    via other platform-specific means, if any).

    With Metal on macOS/iOS, there is no luminance values exposed in the
    platform APIs. Instead, the maximum color component value, that would be
    1.0 in a non-HDR setup, is provided. The \c limitsType field indicates what
    kind of information is available. It is then up to the clients of QRhi to
    access the correct data from the \c limits union and use it as they see
    fit.

    With an API like Vulkan, where there is no way to get such information, the
    values are always the built-in defaults.

    Therefore, the struct returned from QRhiSwapChain::hdrInfo() contains
    either some hard-coded defaults or real values received from an API such as
    DXGI (IDXGIOutput6) or Cocoa (NSScreen). When no platform queries are
    available (or needs using platform facilities out of scope for QRhi), the
    hard-coded defaults are a maximum luminance of 1000 nits and an SDR white
    level of 200.

    The struct also exposes the presumed luminance behavior of the platform and
    its compositor, to indicate what a color component value of 1.0 is treated
    as in a HDR color buffer. In some cases it will be necessary to perform
    color correction of non-HDR content composited with HDR content. To enable
    this, the SDR white level is queried from the system on some platforms
    (Windows) and exposed here.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    \sa QRhiSwapChain::hdrInfo()
 */

/*!
    \enum QRhiSwapChainHdrInfo::LimitsType

    \value LuminanceInNits Indicates that the \l limits union has its
    \c luminanceInNits struct set

    \value ColorComponentValue Indicates that the \l limits union has its
    \c colorComponentValue struct set
*/

/*!
    \enum QRhiSwapChainHdrInfo::LuminanceBehavior

    \value SceneReferred Indicates that the color value of 1.0 is interpreted
    as 80 nits. This is the behavior of HDR-enabled windows with the Windows
    compositor. See
    \l{https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range}{this
    page} for more information on HDR on Windows.

    \value DisplayReferred Indicates that the color value of 1.0 is interpreted
    as the value of the SDR white. (which can be e.g. 200 nits, but will vary
    depending on screen brightness) This is the behavior of HDR-enabled windows
    on Apple platforms. See
    \l{https://developer.apple.com/documentation/metal/hdr_content/displaying_hdr_content_in_a_metal_layer}{this
    page} for more information on Apple's EDR system.
*/

/*!
    \variable QRhiSwapChainHdrInfo::limitsType

    With Metal on macOS/iOS, there is no luminance values exposed in the
    platform APIs. Instead, the maximum color component value, that would be
    1.0 in a non-HDR setup, is provided. This value indicates what kind of
    information is available in \l limits.

    \sa QRhiSwapChain::hdrInfo()
*/

/*!
    \variable QRhiSwapChainHdrInfo::limits

    Contains the actual values queried from the graphics API or the platform.
    The type of data is indicated by \l limitsType. This is therefore a union.
    There are currently two options:

    Luminance values in nits:

    \code
        struct {
            float minLuminance;
            float maxLuminance;
        } luminanceInNits;
    \endcode

    On Windows the minimum and maximum luminance depends on the screen
    brightness. While not relevant for desktops, on laptops the screen
    brightness may change at any time. Increasing brightness implies decreased
    maximum luminance. In addition, the results may also be dependent on the
    HDR Content Brightness set in Windows Settings' System/Display/HDR view,
    if there is such a setting.

    Note however that the changes made to the laptop screen's brightness or in
    the system settings while the application is running are not necessarily
    reflected in the returned values, meaning calling hdrInfo() again may still
    return the same luminance range as before for the rest of the process'
    lifetime. The exact behavior is up to DXGI and Qt has no control over it.

    \note The Windows compositor works in scene-referred mode for HDR content.
    A color component value of 1.0 corresponds to a luminance of 80 nits. When
    rendering non-HDR content (e.g. 2D UI elements), the correction of the
    white level is often necessary. (e.g., outputting the fragment color (1, 1,
    1) will likely lead to showing a shade of white that is too dim on-screen)
    See \l sdrWhiteLevel.

    For macOS/iOS, the current maximum and potential maximum color
    component values are provided:

    \code
        struct {
            float maxColorComponentValue;
            float maxPotentialColorComponentValue;
        } colorComponentValue;
    \endcode

    The value may depend on the screen brightness, which on laptops means that
    the result may change in the next call to hdrInfo() if the brightness was
    changed in the meantime. The maximum screen brightness implies a maximum
    color value of 1.0.

    \note Apple's EDR is display-referred. 1.0 corresponds to a luminance level
    of SDR white (e.g. 200 nits), the value of which varies based on the screen
    brightness and possibly other settings. The exact luminance value for that,
    or the maximum luminance of the display, are not exposed to the
    applications.

    \note It has been observed that the color component values are not set to
    the correct larger-than-1 value right away on startup on some macOS
    systems, but the values tend to change during or after the first frame.

    \sa QRhiSwapChain::hdrInfo()
*/

/*!
    \variable QRhiSwapChainHdrInfo::luminanceBehavior

    Describes the platform's presumed behavior with regards to color values.

    \sa sdrWhiteLevel
 */

/*!
    \variable QRhiSwapChainHdrInfo::sdrWhiteLevel

    On Windows this is the dynamic SDR white level in nits. The value is
    dependent on the screen brightness (on laptops), and the SDR or HDR Content
    Brightness settings in the Windows settings' System/Display/HDR view.

    To perform white level correction for non-HDR (SDR) content, such as 2D UI
    elemenents, multiply the final color with sdrWhiteLevel / 80.0 whenever
    \l luminanceBehavior is SceneReferred. (assuming Windows and a linear
    extended sRGB (scRGB) color space)

    On other platforms the value is always a pre-defined value, 200. This may
    not match the system's actual SDR white level, but the value of this
    variable is not relevant in practice when the \l luminanceBehavior is
    DisplayReferred, because then the color component value of 1.0 refers to
    the SDR white by default.

    \sa luminanceBehavior
*/

/*!
    \return the HDR information for the associated display.

    Do not assume that this is a cheap operation. Depending on the platform,
    this function makes various platform queries which may have a performance
    impact.

    \note Can be called before createOrResize() as long as the window is
    \l{setWindow()}{set}.

    \note What happens when moving a window with an initialized swapchain
    between displays (HDR to HDR with different characteristics, HDR to SDR,
    etc.) is not currently well-defined and depends heavily on the windowing
    system and compositor, with potentially varying behavior between platforms.
    Currently QRhi only guarantees that hdrInfo() returns valid data, if
    available, for the display to which the swapchain's associated window
    belonged at the time of createOrResize().

    \sa QRhiSwapChainHdrInfo
 */
QRhiSwapChainHdrInfo QRhiSwapChain::hdrInfo()
{
    QRhiSwapChainHdrInfo info;
    info.limitsType = QRhiSwapChainHdrInfo::LuminanceInNits;
    info.limits.luminanceInNits.minLuminance = 0.0f;
    info.limits.luminanceInNits.maxLuminance = 1000.0f;
    info.luminanceBehavior = QRhiSwapChainHdrInfo::SceneReferred;
    info.sdrWhiteLevel = 200.0f;
    return info;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QRhiSwapChainHdrInfo &info)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QRhiSwapChainHdrInfo(";
    switch (info.limitsType) {
    case QRhiSwapChainHdrInfo::LuminanceInNits:
        dbg.nospace() << " minLuminance=" << info.limits.luminanceInNits.minLuminance
                      << " maxLuminance=" << info.limits.luminanceInNits.maxLuminance;
        break;
    case QRhiSwapChainHdrInfo::ColorComponentValue:
        dbg.nospace() << " maxColorComponentValue=" << info.limits.colorComponentValue.maxColorComponentValue;
        dbg.nospace() << " maxPotentialColorComponentValue=" << info.limits.colorComponentValue.maxPotentialColorComponentValue;
        break;
    }
    switch (info.luminanceBehavior) {
    case QRhiSwapChainHdrInfo::SceneReferred:
        dbg.nospace() << " scene-referred, SDR white level=" << info.sdrWhiteLevel;
        break;
    case QRhiSwapChainHdrInfo::DisplayReferred:
        dbg.nospace() << " display-referred";
        break;
    }
    dbg.nospace() << ')';
    return dbg;
}
#endif

/*!
    \class QRhiComputePipeline
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Compute pipeline state resource.

    \note Setting the shader resource bindings is mandatory. The referenced
    QRhiShaderResourceBindings must already have created() called on it by the
    time create() is called.

    \note Setting the shader is mandatory.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \enum QRhiComputePipeline::Flag

    Flag values for describing pipeline options.

    \value CompileShadersWithDebugInfo Requests compiling shaders with debug
    information enabled, when applicable. See
    QRhiGraphicsPipeline::CompileShadersWithDebugInfo for more information.
 */

/*!
    \return the resource type.
 */
QRhiResource::Type QRhiComputePipeline::resourceType() const
{
    return ComputePipeline;
}

/*!
    \internal
 */
QRhiComputePipeline::QRhiComputePipeline(QRhiImplementation *rhi)
    : QRhiResource(rhi)
{
}

/*!
    \fn QRhiComputePipeline::Flags QRhiComputePipeline::flags() const
    \return the currently set flags.
 */

/*!
    \fn void QRhiComputePipeline::setFlags(Flags f)
    Sets the flags \a f.
 */

/*!
    \fn QRhiShaderStage QRhiComputePipeline::shaderStage() const
    \return the currently set shader.
 */

/*!
    \fn void QRhiComputePipeline::setShaderStage(const QRhiShaderStage &stage)

    Sets the shader to use. \a stage can only refer to the
    \l{QRhiShaderStage::Compute}{compute stage}.
 */

/*!
    \fn QRhiShaderResourceBindings *QRhiComputePipeline::shaderResourceBindings() const
    \return the currently associated QRhiShaderResourceBindings object.
 */

/*!
    \fn void QRhiComputePipeline::setShaderResourceBindings(QRhiShaderResourceBindings *srb)

    Associates with \a srb describing the resource binding layout and the
    resources (QRhiBuffer, QRhiTexture) themselves. The latter is optional. As
    with graphics pipelines, the \a srb passed in here can leave the actual
    buffer or texture objects unspecified (\nullptr) as long as there is
    another,
    \l{QRhiShaderResourceBindings::isLayoutCompatible()}{layout-compatible}
    QRhiShaderResourceBindings bound via
    \l{QRhiCommandBuffer::setShaderResources()}{setShaderResources()} before
    recording the dispatch call.
 */

/*!
    \class QRhiCommandBuffer
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Command buffer resource.

    Not creatable by applications at the moment. The only ways to obtain a
    valid QRhiCommandBuffer are to get it from the targeted swapchain via
    QRhiSwapChain::currentFrameCommandBuffer(), or, in case of rendering
    completely offscreen, initializing one via QRhi::beginOffscreenFrame().

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \enum QRhiCommandBuffer::IndexFormat
    Specifies the index data type

    \value IndexUInt16 Unsigned 16-bit (quint16)
    \value IndexUInt32 Unsigned 32-bit (quint32)
 */

/*!
    \enum QRhiCommandBuffer::BeginPassFlag
    Flag values for QRhi::beginPass()

    \value ExternalContent Specifies that there will be a call to
    QRhiCommandBuffer::beginExternal() in this pass. Some backends, Vulkan in
    particular, will fail if this flag is not set and beginExternal() is still
    called.

    \value DoNotTrackResourcesForCompute Specifies that there is no need to
    track resources used in this pass if the only purpose of such tracking is
    to generate barriers for compute. Implies that there are no compute passes
    in the frame. This is an optimization hint that may be taken into account
    by certain backends, OpenGL in particular, allowing them to skip certain
    operations. When this flag is set for a render pass in a frame, calling
    \l{QRhiCommandBuffer::beginComputePass()}{beginComputePass()} in that frame
    may lead to unexpected behavior, depending on the resource dependencies
    between the render and compute passes.
 */

/*!
    \typedef QRhiCommandBuffer::DynamicOffset

    Synonym for std::pair<int, quint32>. The first entry is the binding, the second
    is the offset in the buffer.
*/

/*!
    \typedef QRhiCommandBuffer::VertexInput

    Synonym for std::pair<QRhiBuffer *, quint32>. The second entry is an offset in
    the buffer specified by the first.
*/

/*!
    \internal
 */
QRhiCommandBuffer::QRhiCommandBuffer(QRhiImplementation *rhi)
    : QRhiResource(rhi)
{
}

/*!
    \return the resource type.
 */
QRhiResource::Type QRhiCommandBuffer::resourceType() const
{
    return CommandBuffer;
}

static const char *resourceTypeStr(const QRhiResource *res)
{
    switch (res->resourceType()) {
    case QRhiResource::Buffer:
        return "Buffer";
    case QRhiResource::Texture:
        return "Texture";
    case QRhiResource::Sampler:
        return "Sampler";
    case QRhiResource::RenderBuffer:
        return "RenderBuffer";
    case QRhiResource::RenderPassDescriptor:
        return "RenderPassDescriptor";
    case QRhiResource::SwapChainRenderTarget:
        return "SwapChainRenderTarget";
    case QRhiResource::TextureRenderTarget:
        return "TextureRenderTarget";
    case QRhiResource::ShaderResourceBindings:
        return "ShaderResourceBindings";
    case QRhiResource::GraphicsPipeline:
        return "GraphicsPipeline";
    case QRhiResource::SwapChain:
        return "SwapChain";
    case QRhiResource::ComputePipeline:
        return "ComputePipeline";
    case QRhiResource::CommandBuffer:
        return "CommandBuffer";
    case QRhiResource::ShadingRateMap:
        return "ShadingRateMap";
    }

    Q_UNREACHABLE_RETURN("");
}

QRhiImplementation::~QRhiImplementation()
{
    qDeleteAll(resUpdPool);

    // Be nice and show something about leaked stuff. Though we may not get
    // this far with some backends where the allocator or the api may check
    // and freak out for unfreed graphics objects in the derived dtor already.
#ifndef QT_NO_DEBUG
    // debug builds: just do it always
    static bool leakCheck = true;
#else
    // release builds: opt-in
    static bool leakCheck = qEnvironmentVariableIntValue("QT_RHI_LEAK_CHECK");
#endif
    if (!resources.isEmpty()) {
        if (leakCheck) {
            qWarning("QRhi %p going down with %d unreleased resources that own native graphics objects. This is not nice.",
                     q, int(resources.size()));
        }
        for (auto it = resources.cbegin(), end = resources.cend(); it != end; ++it) {
            QRhiResource *res = it.key();
            const bool ownsNativeResources = it.value();
            if (leakCheck && ownsNativeResources)
                qWarning("  %s resource %p (%s)", resourceTypeStr(res), res, res->m_objectName.constData());

            // Null out the resource's rhi pointer. This is why it makes sense to do null
            // checks in the destroy() implementations of the various resource types. It
            // allows to survive in bad applications that somehow manage to destroy a
            // resource of a QRhi after the QRhi itself.
            res->m_rhi = nullptr;
        }
    }
}

bool QRhiImplementation::isCompressedFormat(QRhiTexture::Format format) const
{
    return (format >= QRhiTexture::BC1 && format <= QRhiTexture::BC7)
            || (format >= QRhiTexture::ETC2_RGB8 && format <= QRhiTexture::ETC2_RGBA8)
            || (format >= QRhiTexture::ASTC_4x4 && format <= QRhiTexture::ASTC_12x12);
}

void QRhiImplementation::compressedFormatInfo(QRhiTexture::Format format, const QSize &size,
                                              quint32 *bpl, quint32 *byteSize,
                                              QSize *blockDim) const
{
    int xdim = 4;
    int ydim = 4;
    quint32 blockSize = 0;

    switch (format) {
    case QRhiTexture::BC1:
        blockSize = 8;
        break;
    case QRhiTexture::BC2:
        blockSize = 16;
        break;
    case QRhiTexture::BC3:
        blockSize = 16;
        break;
    case QRhiTexture::BC4:
        blockSize = 8;
        break;
    case QRhiTexture::BC5:
        blockSize = 16;
        break;
    case QRhiTexture::BC6H:
        blockSize = 16;
        break;
    case QRhiTexture::BC7:
        blockSize = 16;
        break;

    case QRhiTexture::ETC2_RGB8:
        blockSize = 8;
        break;
    case QRhiTexture::ETC2_RGB8A1:
        blockSize = 8;
        break;
    case QRhiTexture::ETC2_RGBA8:
        blockSize = 16;
        break;

    case QRhiTexture::ASTC_4x4:
        blockSize = 16;
        break;
    case QRhiTexture::ASTC_5x4:
        blockSize = 16;
        xdim = 5;
        break;
    case QRhiTexture::ASTC_5x5:
        blockSize = 16;
        xdim = ydim = 5;
        break;
    case QRhiTexture::ASTC_6x5:
        blockSize = 16;
        xdim = 6;
        ydim = 5;
        break;
    case QRhiTexture::ASTC_6x6:
        blockSize = 16;
        xdim = ydim = 6;
        break;
    case QRhiTexture::ASTC_8x5:
        blockSize = 16;
        xdim = 8;
        ydim = 5;
        break;
    case QRhiTexture::ASTC_8x6:
        blockSize = 16;
        xdim = 8;
        ydim = 6;
        break;
    case QRhiTexture::ASTC_8x8:
        blockSize = 16;
        xdim = ydim = 8;
        break;
    case QRhiTexture::ASTC_10x5:
        blockSize = 16;
        xdim = 10;
        ydim = 5;
        break;
    case QRhiTexture::ASTC_10x6:
        blockSize = 16;
        xdim = 10;
        ydim = 6;
        break;
    case QRhiTexture::ASTC_10x8:
        blockSize = 16;
        xdim = 10;
        ydim = 8;
        break;
    case QRhiTexture::ASTC_10x10:
        blockSize = 16;
        xdim = ydim = 10;
        break;
    case QRhiTexture::ASTC_12x10:
        blockSize = 16;
        xdim = 12;
        ydim = 10;
        break;
    case QRhiTexture::ASTC_12x12:
        blockSize = 16;
        xdim = ydim = 12;
        break;

    default:
        Q_UNREACHABLE();
        break;
    }

    const quint32 wblocks = uint((size.width() + xdim - 1) / xdim);
    const quint32 hblocks = uint((size.height() + ydim - 1) / ydim);

    if (bpl)
        *bpl = wblocks * blockSize;
    if (byteSize)
        *byteSize = wblocks * hblocks * blockSize;
    if (blockDim)
        *blockDim = QSize(xdim, ydim);
}

void QRhiImplementation::textureFormatInfo(QRhiTexture::Format format, const QSize &size,
                                           quint32 *bpl, quint32 *byteSize, quint32 *bytesPerPixel) const
{
    if (isCompressedFormat(format)) {
        compressedFormatInfo(format, size, bpl, byteSize, nullptr);
        return;
    }

    quint32 bpc = 0;
    switch (format) {
    case QRhiTexture::RGBA8:
        bpc = 4;
        break;
    case QRhiTexture::BGRA8:
        bpc = 4;
        break;
    case QRhiTexture::R8:
        bpc = 1;
        break;
    case QRhiTexture::RG8:
        bpc = 2;
        break;
    case QRhiTexture::R16:
        bpc = 2;
        break;
    case QRhiTexture::RG16:
        bpc = 4;
        break;
    case QRhiTexture::RED_OR_ALPHA8:
        bpc = 1;
        break;

    case QRhiTexture::RGBA16F:
        bpc = 8;
        break;
    case QRhiTexture::RGBA32F:
        bpc = 16;
        break;
    case QRhiTexture::R16F:
        bpc = 2;
        break;
    case QRhiTexture::R32F:
        bpc = 4;
        break;

    case QRhiTexture::RGB10A2:
        bpc = 4;
        break;

    case QRhiTexture::D16:
        bpc = 2;
        break;
    case QRhiTexture::D24:
    case QRhiTexture::D24S8:
    case QRhiTexture::D32F:
        bpc = 4;
        break;

    case QRhiTexture::D32FS8:
        bpc = 8;
        break;

    case QRhiTexture::R8SI:
    case QRhiTexture::R8UI:
        bpc = 1;
        break;
    case QRhiTexture::R32SI:
    case QRhiTexture::R32UI:
        bpc = 4;
        break;
    case QRhiTexture::RG32SI:
    case QRhiTexture::RG32UI:
        bpc = 8;
        break;
    case QRhiTexture::RGBA32SI:
    case QRhiTexture::RGBA32UI:
        bpc = 16;
        break;

    default:
        Q_UNREACHABLE();
        break;
    }

    if (bpl)
        *bpl = uint(size.width()) * bpc;
    if (byteSize)
        *byteSize = uint(size.width() * size.height()) * bpc;
    if (bytesPerPixel)
        *bytesPerPixel = bpc;
}

bool QRhiImplementation::isStencilSupportingFormat(QRhiTexture::Format format) const
{
    switch (format) {
    case QRhiTexture::D24S8:
    case QRhiTexture::D32FS8:
        return true;
    default:
        break;
    }
    return false;
}

bool QRhiImplementation::sanityCheckGraphicsPipeline(QRhiGraphicsPipeline *ps)
{
    if (ps->cbeginShaderStages() == ps->cendShaderStages()) {
        qWarning("Cannot build a graphics pipeline without any stages");
        return false;
    }

    bool hasVertexStage = false;
    for (auto it = ps->cbeginShaderStages(), itEnd = ps->cendShaderStages(); it != itEnd; ++it) {
        if (!it->shader().isValid()) {
            qWarning("Empty shader passed to graphics pipeline");
            return false;
        }
        if (it->type() == QRhiShaderStage::Vertex)
            hasVertexStage = true;
    }
    if (!hasVertexStage) {
        qWarning("Cannot build a graphics pipeline without a vertex stage");
        return false;
    }

    if (!ps->renderPassDescriptor()) {
        qWarning("Cannot build a graphics pipeline without a QRhiRenderPassDescriptor");
        return false;
    }

    if (!ps->shaderResourceBindings()) {
        qWarning("Cannot build a graphics pipeline without QRhiShaderResourceBindings");
        return false;
    }

    return true;
}

bool QRhiImplementation::sanityCheckShaderResourceBindings(QRhiShaderResourceBindings *srb)
{
#ifndef QT_NO_DEBUG
    bool bindingsOk = true;
    const int CHECKED_BINDINGS_COUNT = 64;
    bool bindingSeen[CHECKED_BINDINGS_COUNT] = {};
    for (auto it = srb->cbeginBindings(), end = srb->cendBindings(); it != end; ++it) {
        const int binding = shaderResourceBindingData(*it)->binding;
        if (binding >= CHECKED_BINDINGS_COUNT)
            continue;
        if (binding < 0) {
            qWarning("Invalid binding number %d", binding);
            bindingsOk = false;
            continue;
        }
        switch (shaderResourceBindingData(*it)->type) {
        case QRhiShaderResourceBinding::UniformBuffer:
            if (!bindingSeen[binding]) {
                bindingSeen[binding] = true;
            } else {
                qWarning("Uniform buffer duplicates an existing binding number %d", binding);
                bindingsOk = false;
            }
            break;
        case QRhiShaderResourceBinding::SampledTexture:
            if (!bindingSeen[binding]) {
                bindingSeen[binding] = true;
            } else {
                qWarning("Combined image sampler duplicates an existing binding number %d", binding);
                bindingsOk = false;
            }
            break;
        case QRhiShaderResourceBinding::Texture:
            if (!bindingSeen[binding]) {
                bindingSeen[binding] = true;
            } else {
                qWarning("Texture duplicates an existing binding number %d", binding);
                bindingsOk = false;
            }
            break;
        case QRhiShaderResourceBinding::Sampler:
            if (!bindingSeen[binding]) {
                bindingSeen[binding] = true;
            } else {
                qWarning("Sampler duplicates an existing binding number %d", binding);
                bindingsOk = false;
            }
            break;
        case QRhiShaderResourceBinding::ImageLoad:
        case QRhiShaderResourceBinding::ImageStore:
        case QRhiShaderResourceBinding::ImageLoadStore:
            if (!bindingSeen[binding]) {
                bindingSeen[binding] = true;
            } else {
                qWarning("Image duplicates an existing binding number %d", binding);
                bindingsOk = false;
            }
            break;
        case QRhiShaderResourceBinding::BufferLoad:
        case QRhiShaderResourceBinding::BufferStore:
        case QRhiShaderResourceBinding::BufferLoadStore:
            if (!bindingSeen[binding]) {
                bindingSeen[binding] = true;
            } else {
                qWarning("Buffer duplicates an existing binding number %d", binding);
                bindingsOk = false;
            }
            break;
        default:
            qWarning("Unknown binding type %d", int(shaderResourceBindingData(*it)->type));
            bindingsOk = false;
            break;
        }
    }

    if (!bindingsOk) {
        qWarning() << *srb;
        return false;
    }
#else
    Q_UNUSED(srb);
#endif
    return true;
}

int QRhiImplementation::effectiveSampleCount(int sampleCount) const
{
    // Stay compatible with QSurfaceFormat and friends where samples == 0 means the same as 1.
    const int s = qBound(1, sampleCount, 64);
    const QList<int> supported = supportedSampleCounts();
    int result = 1;

    // Stay compatible with Qt 5 in that requesting an unsupported sample count
    // is not an error (although we still do a categorized debug print about
    // this), and rather a supported value, preferably a close one, not just 1,
    // is used instead. This is actually deviating from Qt 5 as that performs a
    // clamping only and does not handle cases such as when sample count 2 is
    // not supported but 4 is. (OpenGL handles things like that gracefully,
    // other APIs may not, so improve this by picking the next largest, or in
    // absence of that, the largest value; this with the goal to not reduce
    // quality by rather picking a larger-than-requested value than a smaller one)

    for (int i = 0, ie = supported.count(); i != ie; ++i) {
        // assumes the 'supported' list is sorted
        if (supported[i] >= s) {
            result = supported[i];
            break;
        }
    }

    if (result != s) {
        if (result == 1 && !supported.isEmpty())
            result = supported.last();
        qCDebug(QRHI_LOG_INFO, "Attempted to set unsupported sample count %d, using %d instead",
                sampleCount, result);
    }

    return result;
}

/*!
    \internal
 */
QRhi::QRhi()
{
}

/*!
    Destructor. Destroys the backend and releases resources.
 */
QRhi::~QRhi()
{
    if (!d)
        return;

    d->runCleanup();

    qDeleteAll(d->pendingDeleteResources);
    d->pendingDeleteResources.clear();

    d->destroy();
    delete d;
}

QRhiImplementation *QRhiImplementation::newInstance(QRhi::Implementation impl, QRhiInitParams *params, QRhiNativeHandles *importDevice)
{
    QRhiImplementation *d = nullptr;

    switch (impl) {
    case QRhi::Null:
        d = new QRhiNull(static_cast<QRhiNullInitParams *>(params));
        break;
    case QRhi::Vulkan:
#if QT_CONFIG(vulkan)
        d = new QRhiVulkan(static_cast<QRhiVulkanInitParams *>(params),
                           static_cast<QRhiVulkanNativeHandles *>(importDevice));
        break;
#else
        Q_UNUSED(importDevice);
        qWarning("This build of Qt has no Vulkan support");
        break;
#endif
    case QRhi::OpenGLES2:
#ifndef QT_NO_OPENGL
        d = new QRhiGles2(static_cast<QRhiGles2InitParams *>(params),
                          static_cast<QRhiGles2NativeHandles *>(importDevice));
        break;
#else
        qWarning("This build of Qt has no OpenGL support");
        break;
#endif
    case QRhi::D3D11:
#ifdef Q_OS_WIN
        d = new QRhiD3D11(static_cast<QRhiD3D11InitParams *>(params),
                          static_cast<QRhiD3D11NativeHandles *>(importDevice));
        break;
#else
        qWarning("This platform has no Direct3D 11 support");
        break;
#endif
    case QRhi::Metal:
#if QT_CONFIG(metal)
        d = new QRhiMetal(static_cast<QRhiMetalInitParams *>(params),
                          static_cast<QRhiMetalNativeHandles *>(importDevice));
        break;
#else
        qWarning("This platform has no Metal support");
        break;
#endif
    case QRhi::D3D12:
#ifdef Q_OS_WIN
#ifdef QRHI_D3D12_AVAILABLE
        d = new QRhiD3D12(static_cast<QRhiD3D12InitParams *>(params),
                          static_cast<QRhiD3D12NativeHandles *>(importDevice));
        break;
#else
        qWarning("Qt was built without Direct3D 12 support. "
                 "This is likely due to having ancient SDK headers (such as d3d12.h) in the Qt build environment. "
                 "Rebuild Qt with an SDK supporting D3D12 features introduced in Windows 10 version 1703, "
                 "or use an MSVC build as those typically are built with more up-to-date SDKs.");
        break;
#endif
#else
        qWarning("This platform has no Direct3D 12 support");
        break;
#endif
    }

    return d;
}

void QRhiImplementation::prepareForCreate(QRhi *rhi, QRhi::Implementation impl, QRhi::Flags flags, QRhiAdapter *adapter)
{
    q = rhi;

    debugMarkers = flags.testFlag(QRhi::EnableDebugMarkers);

    implType = impl;
    implThread = QThread::currentThread();

    requestedRhiAdapter = adapter;
}

QRhi::AdapterList QRhiImplementation::enumerateAdaptersBeforeCreate(QRhiNativeHandles *) const
{
    return {};
}

/*!
    \overload

    Equivalent to create(\a impl, \a params, \a flags, \a importDevice, \c nullptr).
 */
QRhi *QRhi::create(Implementation impl, QRhiInitParams *params, Flags flags, QRhiNativeHandles *importDevice)
{
    return create(impl, params, flags, importDevice, nullptr);
}

/*!
    \return a new QRhi instance with a backend for the graphics API specified
    by \a impl with the specified \a flags. \return \c nullptr if the
    function fails.

    \a params must point to an instance of one of the backend-specific
    subclasses of QRhiInitParams, such as, QRhiVulkanInitParams,
    QRhiMetalInitParams, QRhiD3D11InitParams, QRhiD3D12InitParams,
    QRhiGles2InitParams. See these classes for examples on creating a QRhi.

    QRhi by design does not implement any fallback logic: if the specified API
    cannot be initialized, create() will fail, with warnings printed on the
    debug output by the backends. The clients of QRhi, for example Qt Quick,
    may however provide additional logic that allow falling back to an API
    different than what was requested, depending on the platform. If the
    intention is just to test if initialization would succeed when calling
    create() at later point, it is preferable to use probe() instead of
    create(), because with some backends probing can be implemented in a more
    lightweight manner as opposed to create(), which performs full
    initialization of the infrastructure and is wasteful if that QRhi instance
    is then thrown immediately away.

    \a importDevice allows using an already existing graphics device, without
    QRhi creating its own. When not null, this parameter must point to an
    instance of one of the subclasses of QRhiNativeHandles:
    QRhiVulkanNativeHandles, QRhiD3D11NativeHandles, QRhiD3D12NativeHandles,
    QRhiMetalNativeHandles, QRhiGles2NativeHandles. The exact details and
    semantics depend on the backand and the underlying graphics API.

    Specifying a QRhiAdapter in \a adapter offers a transparent, cross-API
    alternative to passing in a \c VkPhysicalDevice via QRhiVulkanNativeHandles,
    or an adapter LUID via QRhiD3D12NativeHandles. The ownership of \a adapter
    is not taken. See enumerateAdapters() for more information on this approach.

    \note \a importDevice and \a adapter cannot be both specified.

    \sa probe()
 */
QRhi *QRhi::create(Implementation impl, QRhiInitParams *params, Flags flags, QRhiNativeHandles *importDevice, QRhiAdapter *adapter)
{
    if (adapter && importDevice)
        qWarning("adapter and importDevice should not both be non-null in QRhi::create()");

    std::unique_ptr<QRhiImplementation> rd(QRhiImplementation::newInstance(impl, params, importDevice));
    if (!rd)
        return nullptr;

    std::unique_ptr<QRhi> r(new QRhi);
    r->d = rd.release();
    r->d->prepareForCreate(r.get(), impl, flags, adapter);
    if (!r->d->create(flags))
        return nullptr;

    return r.release();
}

/*!
    \return true if create() can be expected to succeed when called the given
    \a impl and \a params.

    For some backends this is equivalent to calling create(), checking its
    return value, and then destroying the resulting QRhi.

    For others, in particular with Metal, there may be a specific probing
    implementation, which allows testing in a more lightweight manner without
    polluting the debug output with warnings upon failures.

    \sa create()
 */
bool QRhi::probe(QRhi::Implementation impl, QRhiInitParams *params)
{
    bool ok = false;

    // The only place currently where this makes sense is Metal, where the API
    // is simple enough so that a special probing function - doing nothing but
    // a MTLCreateSystemDefaultDevice - is reasonable. Elsewhere, just call
    // create() and then drop the result.

    if (impl == Metal) {
#if QT_CONFIG(metal)
        ok = QRhiMetal::probe(static_cast<QRhiMetalInitParams *>(params));
#endif
    } else {
        QRhi *rhi = create(impl, params);
        ok = rhi != nullptr;
        delete rhi;
    }
    return ok;
}

/*!
    \typedef QRhi::AdapterList
    \relates QRhi
    \since 6.10

    Synonym for QVector<QRhiAdapter *>.
*/

/*!
    \return the list of adapters (physical devices) present, or an empty list
    when such control is not available with a given graphics API.

    Backends where such level of control is not available, the returned list is
    always empty. Thus an empty list does not indicate there are no graphics
    devices in the system, but that fine-grained control over selecting which
    one to use is not available.

    Backends for Direct 3D 11, Direct 3D 12, and Vulkan can be expected to fully
    support enumerating adapters. Others may not. The backend is specified by \a
    impl. A QRhiAdapter returned from this function must only be used in a
    create() call with the same \a impl. Some underlying APIs may present
    further limitations, with Vulkan in particular the QRhiAdapter is specified
    to the QVulkanInstance (\c VkInstance).

    The caller is expected to destroy the QRhiAdapter objects in the list. Apart
    from querying \l{QRhiAdapter::}{info()}, the only purpose of these objects is
    to be passed on to create(), or the corresponding functions in higher layers
    such as Qt Quick.

    The following snippet, written specifically for Vulkan, shows how to
    enumerate the available physical devices and request to create a QRhi for
    the chosen one. This in practice is equivalent to passing in a \c
    VkPhysicalDevice via a QRhiVulkanNativeHandles to create(), but it involves
    less API-specific code on the application side:

    \code
      QRhiVulkanInitParams initParams;
      initParams.inst = &vulkanInstance;
      QRhi::AdapterList adapters = QRhi::enumerateAdapters(QRhi::Vulkan, &initParams);
      QRhiAdapter *chosenAdapter = nullptr;
      for (QRhiAdapter *adapter : adapters) {
          if (looksGood(adapter->info())) {
              chosenAdapter = adapter;
              break;
          }
      }
      QRhi *rhi = QRhi::create(QRhi::Vulkan, &initParams, {}, nullptr, chosenAdapter);
      qDeleteAll(adapters);
    \endcode

    Passing in \a params is required due to some of the underlying graphics
    APIs' design. With Vulkan in particular, the QVulkanInstance must be
    provided, since enumerating is not possible without it. Other fields in the
    backend-specific \a params will not actually be used by this function.

    \a nativeHandles is optional. When specified, it must be a valid
    QRhiD3D11NativeHandles, QRhiD3D12NativeHandles, or QRhiVulkanNativeHandles,
    similarly to create(). However, unlike create(), only the physical device
    (in case of Vulkan) or the adapter LUID (in case of D3D) fields are used,
    all other fields are ignored. This can be used the restrict the results to a
    given adapter. The returned list will contain 1 or 0 elements in this case.

    Note how in the previous code snippet the looksGood() function
    implementation cannot perform any platform-specific filtering based on the
    true adapter / physical device identity, such as the adapter LUID on Windows
    or the VkPhysicalDevice with Vulkan. This is because QRhiDriverInfo does not
    contain platform-specific data. Instead, use \a nativeHandles to get the
    results filtered already inside enumerateAdapters().

    The following two snippets, using Direct 3D 12 as an example, are equivalent
    in practice:

    \code
      // enumerateAdapters-based approach from Qt 6.10 on
      QRhiD3D12InitParams initParams;
      QRhiD3D12NativeHandles nativeHandles;
      nativeHandles.adapterLuidLow = luid.LowPart; // retrieved a LUID from somewhere, now pass it on to Qt
      nativeHandles.adapterLuidHigh = luid.HighPart;
      QRhi::AdapterList adapters = QRhi::enumerateAdapters(QRhi::D3D12, &initParams, &nativeHandles);
      if (adapters.isEmpty()) { qWarning("Requested adapter was not found"); }
      QRhi *rhi = QRhi::create(QRhi::D3D12, &initParams, {}, nullptr, adapters[0]);
      qDeleteAll(adapters);
    \endcode

    \code
      // traditional approach, more lightweight
      QRhiD3D12InitParams initParams;
      QRhiD3D12NativeHandles nativeHandles;
      nativeHandles.adapterLuidLow = luid.LowPart; // retrieved a LUID from somewhere, now pass it on to Qt
      nativeHandles.adapterLuidHigh = luid.HighPart;
      QRhi *rhi = QRhi::create(QRhi::D3D12, &initParams, {}, &nativeHandles, nullptr);
    \endcode

    \since 6.10
    \sa create()
 */
QRhi::AdapterList QRhi::enumerateAdapters(Implementation impl, QRhiInitParams *params, QRhiNativeHandles *nativeHandles)
{
    std::unique_ptr<QRhiImplementation> rd(QRhiImplementation::newInstance(impl, params, nullptr));
    if (!rd)
        return {};

    return rd->enumerateAdaptersBeforeCreate(nativeHandles);
}

/*!
    \struct QRhiSwapChainProxyData
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6

    \brief Opaque data describing native objects needed to set up a swapchain.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    \sa QRhi::updateSwapChainProxyData()
 */

/*!
    Generates and returns a QRhiSwapChainProxyData struct containing opaque
    data specific to the backend and graphics API specified by \a impl. \a
    window is the QWindow a swapchain is targeting.

    The returned struct can be passed to QRhiSwapChain::setProxyData(). This
    makes sense in threaded rendering systems: this static function is expected
    to be called on the \b{main (gui) thread}, unlike all QRhi operations, then
    transferred to the thread working with the QRhi and QRhiSwapChain and passed
    on to the swapchain. This allows doing native platform queries that are
    only safe to be called on the main thread, for example to query the
    CAMetalLayer from a NSView, and then passing on the data to the
    QRhiSwapChain living on the rendering thread. With the Metal example, doing
    the view.layer access on a dedicated rendering thread causes a warning in
    the Xcode Thread Checker. With the data proxy mechanism, this is avoided.

    When threads are not involved, generating and passing on the
    QRhiSwapChainProxyData is not required: backends are guaranteed to be able
    to query whatever is needed on their own, and if everything lives on the
    main (gui) thread, that should be sufficient.

    \note \a impl should match what the QRhi is created with. For example,
    calling with QRhi::Metal on a non-Apple platform will not generate any
    useful data.
 */
QRhiSwapChainProxyData QRhi::updateSwapChainProxyData(QRhi::Implementation impl, QWindow *window)
{
#if QT_CONFIG(metal)
    if (impl == Metal)
        return QRhiMetal::updateSwapChainProxyData(window);
#else
    Q_UNUSED(impl);
    Q_UNUSED(window);
#endif
    return {};
}

/*!
    \return the backend type for this QRhi.
 */
QRhi::Implementation QRhi::backend() const
{
    return d->implType;
}

/*!
    \return a friendly name for the backend \a impl, usually the name of the 3D
    API in use.
 */
const char *QRhi::backendName(Implementation impl)
{
    switch (impl) {
    case QRhi::Null:
        return "Null";
    case QRhi::Vulkan:
        return "Vulkan";
    case QRhi::OpenGLES2:
        return "OpenGL";
    case QRhi::D3D11:
        return "D3D11";
    case QRhi::Metal:
        return "Metal";
    case QRhi::D3D12:
        return "D3D12";
    }

    Q_UNREACHABLE_RETURN("Unknown");
}

/*!
    \return the backend type as string for this QRhi.
 */
const char *QRhi::backendName() const
{
    return backendName(d->implType);
}

/*!
    \enum QRhiDriverInfo::DeviceType
    Specifies the graphics device's type, when the information is available.

    In practice this is only applicable with Vulkan and Metal. With Direct 3D
    11 and 12, using an adapter with the software flag set leads to the value
    \c CpuDevice. Otherwise, and with OpenGL, the value is always UnknownDevice.

    \value UnknownDevice
    \value IntegratedDevice
    \value DiscreteDevice
    \value ExternalDevice
    \value VirtualDevice
    \value CpuDevice
*/

/*!
    \struct QRhiDriverInfo
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6

    \brief Describes the physical device, adapter, or graphics API
    implementation that is used by an initialized QRhi.

    Graphics APIs offer different levels and kinds of information. The only
    value that is available across all APIs is the deviceName, which is a
    freetext description of the physical device, adapter, or is a combination
    of the strings reported for \c{GL_VENDOR} + \c{GL_RENDERER} +
    \c{GL_VERSION}. The deviceId is always 0 for OpenGL. vendorId is always 0
    for OpenGL and Metal. deviceType is always UnknownDevice for OpenGL and
    Direct 3D.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \variable QRhiDriverInfo::deviceName

    \sa QRhi::driverInfo()
*/

/*!
    \variable QRhiDriverInfo::deviceId

    \sa QRhi::driverInfo()
*/

/*!
    \variable QRhiDriverInfo::vendorId

    \sa QRhi::driverInfo()
*/

/*!
    \variable QRhiDriverInfo::deviceType

    \sa QRhi::driverInfo(), QRhiDriverInfo::DeviceType
*/

#ifndef QT_NO_DEBUG_STREAM
static inline const char *deviceTypeStr(QRhiDriverInfo::DeviceType type)
{
    switch (type) {
    case QRhiDriverInfo::UnknownDevice:
        return "Unknown";
    case QRhiDriverInfo::IntegratedDevice:
        return "Integrated";
    case QRhiDriverInfo::DiscreteDevice:
        return "Discrete";
    case QRhiDriverInfo::ExternalDevice:
        return "External";
    case QRhiDriverInfo::VirtualDevice:
        return "Virtual";
    case QRhiDriverInfo::CpuDevice:
        return "Cpu";
    }

    Q_UNREACHABLE_RETURN(nullptr);
}
QDebug operator<<(QDebug dbg, const QRhiDriverInfo &info)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QRhiDriverInfo(deviceName=" << info.deviceName
                  << " deviceId=0x" << Qt::hex << info.deviceId
                  << " vendorId=0x" << info.vendorId
                  << " deviceType=" << deviceTypeStr(info.deviceType)
                  << ')';
    return dbg;
}
#endif

/*!
    \return metadata for the graphics device used by this successfully
    initialized QRhi instance.
 */
QRhiDriverInfo QRhi::driverInfo() const
{
    return d->driverInfo();
}

/*!
    \class QRhiAdapter
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.10

    \brief Represents a physical graphics device.

    Some QRhi backends target graphics APIs that expose the concept of \c
    adapters or \c{physical devices}. Call the static \l
    {QRhi::}{enumerateAdapters()} function to retrieve a list of the adapters
    present in the system. Pass one of the returned QRhiAdapter objects to \l
    {QRhi::}{create()} in order to request using the adapter or physical device
    the QRhiAdapter corresponds to. Other than exposing the QRhiDriverInfo,
    QRhiAdapter is to be treated as an opaque handle.

    \note With Vulkan, the QRhiAdapter is valid only as long as the
    QVulkanInstance that was used for \l{QRhi::}{enumerateAdapters()} is valid.
    This also means that a QRhiAdapter is tied to the Vulkan instance
    (QVulkanInstance, \c VkInstance) and cannot be used in the context of
    another Vulkan instance.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \fn virtual QRhiDriverInfo QRhiAdapter::info() const = 0

    \return the corresponding QRhiDriverInfo.
 */

/*!
    \internal
 */
QRhiAdapter::~QRhiAdapter()
{
}

/*!
    \return the thread on which the QRhi was \l{QRhi::create()}{initialized}.
 */
QThread *QRhi::thread() const
{
    return d->implThread;
}

/*!
    Registers a \a callback that is invoked when the QRhi is destroyed.

    The callback will run with the graphics resource still available, so this
    provides an opportunity for the application to cleanly release QRhiResource
    instances belonging to the QRhi. This is particularly useful for managing
    the lifetime of resources stored in \c cache type of objects, where the
    cache holds QRhiResources or objects containing QRhiResources.

    \sa ~QRhi()
 */
void QRhi::addCleanupCallback(const CleanupCallback &callback)
{
    d->addCleanupCallback(callback);
}

/*!
    \overload

    Registers \a callback to be invoked when the QRhi is destroyed. This
    overload takes an opaque pointer, \a key, that is used to ensure that a
    given callback is registered (and so called) only once.

    \sa removeCleanupCallback()
 */
void QRhi::addCleanupCallback(const void *key, const CleanupCallback &callback)
{
    d->addCleanupCallback(key, callback);
}

/*!
    Deregisters the callback with \a key. If no cleanup callback was registered
    with \a key, the function does nothing. Callbacks registered without a key
    cannot be removed.

    \sa addCleanupCallback()
 */
void QRhi::removeCleanupCallback(const void *key)
{
    d->removeCleanupCallback(key);
}

void QRhiImplementation::runCleanup()
{
    for (const QRhi::CleanupCallback &f : std::as_const(cleanupCallbacks))
        f(q);

    cleanupCallbacks.clear();

    for (auto it = keyedCleanupCallbacks.cbegin(), end = keyedCleanupCallbacks.cend(); it != end; ++it)
        it.value()(q);

    keyedCleanupCallbacks.clear();
}

/*!
    \class QRhiResourceUpdateBatch
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6
    \brief Records upload and copy type of operations.

    With QRhi it is no longer possible to perform copy type of operations at
    arbitrary times. Instead, all such operations are recorded into batches
    that are then passed, most commonly, to QRhiCommandBuffer::beginPass().
    What then happens under the hood is hidden from the application: the
    underlying implementations can defer and implement these operations in
    various different ways.

    A resource update batch owns no graphics resources and does not perform any
    actual operations on its own. It should rather be viewed as a command
    buffer for update, upload, and copy type of commands.

    To get an available, empty batch from the pool, call
    QRhi::nextResourceUpdateBatch().

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \internal
 */
QRhiResourceUpdateBatch::QRhiResourceUpdateBatch(QRhiImplementation *rhi)
    : d(new QRhiResourceUpdateBatchPrivate)
{
    d->q = this;
    d->rhi = rhi;
}

QRhiResourceUpdateBatch::~QRhiResourceUpdateBatch()
{
    delete d;
}

/*!
    \return the batch to the pool. This should only be used when the batch is
    not passed to one of QRhiCommandBuffer::beginPass(),
    QRhiCommandBuffer::endPass(), or QRhiCommandBuffer::resourceUpdate()
    because these implicitly call destroy().

    \note QRhiResourceUpdateBatch instances must never by \c deleted by
    applications.
 */
void QRhiResourceUpdateBatch::release()
{
    d->free();
}

/*!
    Copies all queued operations from the \a other batch into this one.

    \note \a other may no longer contain valid data after the merge operation,
    and must not be submitted, but it will still need to be released by calling
    release().

    This allows for a convenient pattern where resource updates that are
    already known during the initialization step are collected into a batch
    that is then merged into another when starting to first render pass later
    on:

    \code
        void init()
        {
            initialUpdates = rhi->nextResourceUpdateBatch();
            initialUpdates->uploadStaticBuffer(vbuf, vertexData);
            initialUpdates->uploadStaticBuffer(ibuf, indexData);
            // ...
        }

        void render()
        {
            QRhiResourceUpdateBatch *resUpdates = rhi->nextResourceUpdateBatch();
            if (initialUpdates) {
                resUpdates->merge(initialUpdates);
                initialUpdates->release();
                initialUpdates = nullptr;
            }
            // resUpdates->updateDynamicBuffer(...);
            cb->beginPass(rt, clearCol, clearDs, resUpdates);
        }
    \endcode
 */
void QRhiResourceUpdateBatch::merge(QRhiResourceUpdateBatch *other)
{
    d->merge(other->d);
}

/*!
    \return true until the number of buffer and texture operations enqueued
    onto this batch is below a reasonable limit.

    The return value is false when the number of buffer and/or texture
    operations added to this batch have reached, or are about to reach, a
    certain limit. The batch is fully functional afterwards as well, but may
    need to allocate additional memory. Therefore, a renderer that collects
    lots of buffer and texture updates in a single batch when preparing a frame
    may want to consider \l{QRhiCommandBuffer::resourceUpdate()}{submitting the
    batch} and \l{QRhi::nextResourceUpdateBatch()}{starting a new one} when
    this function returns false.
 */
bool QRhiResourceUpdateBatch::hasOptimalCapacity() const
{
    return d->hasOptimalCapacity();
}

/*!
    Enqueues updating a region of a QRhiBuffer \a buf created with the type
    QRhiBuffer::Dynamic.

    The region is specified \a offset and \a size. The actual bytes to write
    are specified by \a data which must have at least \a size bytes available.

    \a data is copied and can safely be destroyed or changed once this function
    returns.

    \note If host writes are involved, which is the case with
    updateDynamicBuffer() typically as such buffers are backed by host visible
    memory with most backends, they may accumulate within a frame. Thus pass 1
    reading a region changed by a batch passed to pass 2 may see the changes
    specified in pass 2's update batch.

    \note QRhi transparently manages double buffering in order to prevent
    stalling the graphics pipeline. The fact that a QRhiBuffer may have
    multiple native buffer objects underneath can be safely ignored when using
    the QRhi and QRhiResourceUpdateBatch.
 */
void QRhiResourceUpdateBatch::updateDynamicBuffer(QRhiBuffer *buf, quint32 offset, quint32 size, const void *data)
{
    if (size > 0) {
        const int idx = d->activeBufferOpCount++;
        const int opListSize = d->bufferOps.size();
        if (idx < opListSize)
            QRhiResourceUpdateBatchPrivate::BufferOp::changeToDynamicUpdate(&d->bufferOps[idx], buf, offset, size, data);
        else
            d->bufferOps.append(QRhiResourceUpdateBatchPrivate::BufferOp::dynamicUpdate(buf, offset, size, data));
    }
}

/*!
    \overload
    \since 6.10

    Enqueues updating a region of a QRhiBuffer \a buf created with the type
    QRhiBuffer::Dynamic.

    \a data is moved into the batch instead of copied with this overload.
 */
void QRhiResourceUpdateBatch::updateDynamicBuffer(QRhiBuffer *buf, quint32 offset, QByteArray data)
{
    if (!data.isEmpty()) {
        const int idx = d->activeBufferOpCount++;
        const int opListSize = d->bufferOps.size();
        if (idx < opListSize)
            QRhiResourceUpdateBatchPrivate::BufferOp::changeToDynamicUpdate(&d->bufferOps[idx], buf, offset, std::move(data));
        else
            d->bufferOps.append(QRhiResourceUpdateBatchPrivate::BufferOp::dynamicUpdate(buf, offset, std::move(data)));
    }
}

/*!
    Enqueues updating a region of a QRhiBuffer \a buf created with the type
    QRhiBuffer::Immutable or QRhiBuffer::Static.

    The region is specified \a offset and \a size. The actual bytes to write
    are specified by \a data which must have at least \a size bytes available.

    \a data is copied and can safely be destroyed or changed once this function
    returns.
 */
void QRhiResourceUpdateBatch::uploadStaticBuffer(QRhiBuffer *buf, quint32 offset, quint32 size, const void *data)
{
    if (size > 0) {
        const int idx = d->activeBufferOpCount++;
        if (idx < d->bufferOps.size())
            QRhiResourceUpdateBatchPrivate::BufferOp::changeToStaticUpload(&d->bufferOps[idx], buf, offset, size, data);
        else
            d->bufferOps.append(QRhiResourceUpdateBatchPrivate::BufferOp::staticUpload(buf, offset, size, data));
    }
}

/*!
    \overload
    \since 6.10

    Enqueues updating a region of a QRhiBuffer \a buf created with the type
    QRhiBuffer::Immutable or QRhiBuffer::Static.

    \a data is moved into the batch instead of copied with this overload.
 */
void QRhiResourceUpdateBatch::uploadStaticBuffer(QRhiBuffer *buf, quint32 offset, QByteArray data)
{
    if (!data.isEmpty()) {
        const int idx = d->activeBufferOpCount++;
        if (idx < d->bufferOps.size())
            QRhiResourceUpdateBatchPrivate::BufferOp::changeToStaticUpload(&d->bufferOps[idx], buf, offset, std::move(data));
        else
            d->bufferOps.append(QRhiResourceUpdateBatchPrivate::BufferOp::staticUpload(buf, offset, std::move(data)));
    }
}

/*!
    \overload

    Enqueues updating the entire QRhiBuffer \a buf created with the type
    QRhiBuffer::Immutable or QRhiBuffer::Static.
 */
void QRhiResourceUpdateBatch::uploadStaticBuffer(QRhiBuffer *buf, const void *data)
{
    if (buf->size() > 0) {
        const int idx = d->activeBufferOpCount++;
        if (idx < d->bufferOps.size())
            QRhiResourceUpdateBatchPrivate::BufferOp::changeToStaticUpload(&d->bufferOps[idx], buf, 0, 0, data);
        else
            d->bufferOps.append(QRhiResourceUpdateBatchPrivate::BufferOp::staticUpload(buf, 0, 0, data));
    }
}

/*!
    \overload
    \since 6.10

    Enqueues updating the entire QRhiBuffer \a buf created with the type
    QRhiBuffer::Immutable or QRhiBuffer::Static.

    \a data is moved into the batch instead of copied with this overload.

    \a data size must equal the size of \a buf.
 */
void QRhiResourceUpdateBatch::uploadStaticBuffer(QRhiBuffer *buf, QByteArray data)
{
    if (buf->size() > 0 && quint32(data.size()) == buf->size()) {
        const int idx = d->activeBufferOpCount++;
        if (idx < d->bufferOps.size())
            QRhiResourceUpdateBatchPrivate::BufferOp::changeToStaticUpload(&d->bufferOps[idx], buf, 0, std::move(data));
        else
            d->bufferOps.append(QRhiResourceUpdateBatchPrivate::BufferOp::staticUpload(buf, 0, std::move(data)));
    }
}

/*!
    Enqueues reading back a region of the QRhiBuffer \a buf. The size of the
    region is specified by \a size in bytes, \a offset is the offset in bytes
    to start reading from.

    A readback is asynchronous. \a result contains a callback that is invoked
    when the operation has completed. The data is provided in
    QRhiReadbackResult::data. Upon successful completion that QByteArray
    will have a size equal to \a size. On failure the QByteArray will be empty.

    \note Reading buffers with a usage different than QRhiBuffer::UniformBuffer
    is supported only when the QRhi::ReadBackNonUniformBuffer feature is
    reported as supported.

   \note The asynchronous readback is guaranteed to have completed when one of
   the following conditions is met: \l{QRhi::finish()}{finish()} has been
   called; or, at least \c N frames have been \l{QRhi::endFrame()}{submitted},
   including the frame that issued the readback operation, and the
   \l{QRhi::beginFrame()}{recording of a new frame} has been started, where \c
   N is the \l{QRhi::resourceLimit()}{resource limit value} returned for
   QRhi::MaxAsyncReadbackFrames.

   \sa readBackTexture(), QRhi::isFeatureSupported(), QRhi::resourceLimit()
 */
void QRhiResourceUpdateBatch::readBackBuffer(QRhiBuffer *buf, quint32 offset, quint32 size, QRhiReadbackResult *result)
{
    const int idx = d->activeBufferOpCount++;
    if (idx < d->bufferOps.size())
        d->bufferOps[idx] = QRhiResourceUpdateBatchPrivate::BufferOp::read(buf, offset, size, result);
    else
        d->bufferOps.append(QRhiResourceUpdateBatchPrivate::BufferOp::read(buf, offset, size, result));
}

/*!
    Enqueues uploading the image data for one or more mip levels in one or more
    layers of the texture \a tex.

    The details of the copy (source QImage or compressed texture data, regions,
    target layers and levels) are described in \a desc.
 */
void QRhiResourceUpdateBatch::uploadTexture(QRhiTexture *tex, const QRhiTextureUploadDescription &desc)
{
    if (desc.cbeginEntries() != desc.cendEntries()) {
        const int idx = d->activeTextureOpCount++;
        if (idx < d->textureOps.size())
            d->textureOps[idx] = QRhiResourceUpdateBatchPrivate::TextureOp::upload(tex, desc);
        else
            d->textureOps.append(QRhiResourceUpdateBatchPrivate::TextureOp::upload(tex, desc));
    }
}

/*!
    Enqueues uploading the image data for mip level 0 of layer 0 of the texture
    \a tex.

    \a tex must have an uncompressed format. Its format must also be compatible
    with the QImage::format() of \a image. The source data is given in \a
    image.
 */
void QRhiResourceUpdateBatch::uploadTexture(QRhiTexture *tex, const QImage &image)
{
    uploadTexture(tex,
                  QRhiTextureUploadEntry(0, 0, QRhiTextureSubresourceUploadDescription(image)));
}

/*!
   Enqueues a texture-to-texture copy operation from \a src into \a dst as
   described by \a desc.

   \note The source texture \a src must be created with
   QRhiTexture::UsedAsTransferSource.

   \note The format of the textures must match. With most graphics
   APIs the data is copied as-is without any format conversions. If
   \a dst and \a src are created with different formats, unspecified
   issues may arise.
 */
void QRhiResourceUpdateBatch::copyTexture(QRhiTexture *dst, QRhiTexture *src, const QRhiTextureCopyDescription &desc)
{
    const int idx = d->activeTextureOpCount++;
    if (idx < d->textureOps.size())
        d->textureOps[idx] = QRhiResourceUpdateBatchPrivate::TextureOp::copy(dst, src, desc);
    else
        d->textureOps.append(QRhiResourceUpdateBatchPrivate::TextureOp::copy(dst, src, desc));
}

/*!
   Enqueues a texture-to-host copy operation as described by \a rb.

   Normally \a rb will specify a QRhiTexture as the source. However, when the
   swapchain in the current frame was created with
   QRhiSwapChain::UsedAsTransferSource, it can also be the source of the
   readback. For this, leave the texture set to null in \a rb.

   Unlike other operations, the results here need to be processed by the
   application. Therefore, \a result provides not just the data but also a
   callback as operations on the batch are asynchronous by nature:

   \code
      rhi->beginFrame(swapchain);
      cb->beginPass(swapchain->currentFrameRenderTarget(), colorClear, dsClear);
      // ...
      QRhiReadbackResult *rbResult = new QRhiReadbackResult;
      rbResult->completed = [rbResult] {
          {
              const QImage::Format fmt = QImage::Format_RGBA8888_Premultiplied; // fits QRhiTexture::RGBA8
              const uchar *p = reinterpret_cast<const uchar *>(rbResult->data.constData());
              QImage image(p, rbResult->pixelSize.width(), rbResult->pixelSize.height(), fmt);
              image.save("result.png");
          }
          delete rbResult;
      };
      QRhiResourceUpdateBatch *u = nextResourceUpdateBatch();
      QRhiReadbackDescription rb; // no texture -> uses the current backbuffer of sc
      u->readBackTexture(rb, rbResult);
      cb->endPass(u);
      rhi->endFrame(swapchain);
   \endcode

   \note The texture must be created with QRhiTexture::UsedAsTransferSource.

   \note Multisample textures cannot be read back.

   \note The readback returns raw byte data, in order to allow the applications
   to interpret it in any way they see fit. Be aware of the blending settings
   of rendering code: if the blending is set up to rely on premultiplied alpha,
   the results of the readback must also be interpreted as Premultiplied.

   \note When interpreting the resulting raw data, be aware that the readback
   happens with a byte ordered format. A \l{QRhiTexture::RGBA8}{RGBA8} texture
   maps therefore to byte ordered QImage formats, such as,
   QImage::Format_RGBA8888.

   \note The asynchronous readback is guaranteed to have completed when one of
   the following conditions is met: \l{QRhi::finish()}{finish()} has been
   called; or, at least \c N frames have been \l{QRhi::endFrame()}{submitted},
   including the frame that issued the readback operation, and the
   \l{QRhi::beginFrame()}{recording of a new frame} has been started, where \c
   N is the \l{QRhi::resourceLimit()}{resource limit value} returned for
   QRhi::MaxAsyncReadbackFrames.

   A single readback operation copies one mip level of one layer (cubemap face
   or 3D slice or texture array element) at a time. The level and layer are
   specified by the respective fields in \a rb.

   \sa readBackBuffer(), QRhi::resourceLimit()
 */
void QRhiResourceUpdateBatch::readBackTexture(const QRhiReadbackDescription &rb, QRhiReadbackResult *result)
{
    const int idx = d->activeTextureOpCount++;
    if (idx < d->textureOps.size())
        d->textureOps[idx] = QRhiResourceUpdateBatchPrivate::TextureOp::read(rb, result);
    else
        d->textureOps.append(QRhiResourceUpdateBatchPrivate::TextureOp::read(rb, result));
}

/*!
   Enqueues a mipmap generation operation for the specified texture \a tex.

   Both 2D and cube textures are supported.

   \note The texture must be created with QRhiTexture::MipMapped and
   QRhiTexture::UsedWithGenerateMips.

   \warning QRhi cannot guarantee that mipmaps can be generated for all
   supported texture formats. For example, QRhiTexture::RGBA32F is not a \c
   filterable format in OpenGL ES 3.0 and Metal on iOS, and therefore the
   mipmap generation request may fail. RGBA8 and RGBA16F are typically
   filterable, so it is recommended to use these formats when mipmap generation
   is desired.
 */
void QRhiResourceUpdateBatch::generateMips(QRhiTexture *tex)
{
    const int idx = d->activeTextureOpCount++;
    if (idx < d->textureOps.size())
        d->textureOps[idx] = QRhiResourceUpdateBatchPrivate::TextureOp::genMips(tex);
    else
        d->textureOps.append(QRhiResourceUpdateBatchPrivate::TextureOp::genMips(tex));
}

/*!
   \return an available, empty batch to which copy type of operations can be
   recorded.

   \note the return value is not owned by the caller and must never be
   destroyed. Instead, the batch is returned the pool for reuse by passing
   it to QRhiCommandBuffer::beginPass(), QRhiCommandBuffer::endPass(), or
   QRhiCommandBuffer::resourceUpdate(), or by calling
   QRhiResourceUpdateBatch::release() on it.

   \note Can be called outside beginFrame() - endFrame() as well since a batch
   instance just collects data on its own, it does not perform any operations.

   Due to not being tied to a frame being recorded, the following sequence is
   valid for example:

   \code
      rhi->beginFrame(swapchain);
      QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
      u->uploadStaticBuffer(buf, data);
      // ... do not commit the batch
      rhi->endFrame();
      // u stays valid (assuming buf stays valid as well)
      rhi->beginFrame(swapchain);
      swapchain->currentFrameCommandBuffer()->resourceUpdate(u);
      // ... draw with buf
      rhi->endFrame();
   \endcode

   \warning The maximum number of batches per QRhi is 64. When this limit is
   reached, the function will return null until a batch is returned to the
   pool.
 */
QRhiResourceUpdateBatch *QRhi::nextResourceUpdateBatch()
{
    // By default we prefer spreading out the utilization of the worst case 64
    // (but typically 4) batches as much as possible, meaning we won't pick the
    // first one even if it's free, but prefer picking one after the last picked
    // one. Relevant due to implicit sharing (the backend may hold on to the
    // QRhiBufferData until frame no. current+FramesInFlight-1, but
    // implementations may vary), combined with the desire to reuse container
    // and QRhiBufferData allocations in bufferOps instead of flooding every
    // frame with allocs. See free(). In typical Qt Quick scenes this leads to
    // eventually seeding all 4 (or more) resource batches with buffer operation
    // data allocations which may (*) then be reused in subsequent frames. This
    // comes at the expense of using more memory, but has proven good results
    // when (CPU) profiling typical Quick/Quick3D apps.
    //
    // (*) Due to implicit sharing(ish), the exact behavior is unpredictable. If
    // a backend holds on to the QRhiBufferData for, e.g., a dynamic buffer
    // update, and then there is a new assign() for that same QRhiBufferData
    // while the refcount is still 2, it will "detach" (without contents) and
    // there is no reuse of the alloc. This is mitigated by the 'choose the one
    // afer the last picked one' logic when handing out batches.

    auto nextFreeBatch = [this]() -> QRhiResourceUpdateBatch * {
        auto isFree = [this](int i) -> QRhiResourceUpdateBatch * {
            const quint64 mask = 1ULL << quint64(i);
            if (!(d->resUpdPoolMap & mask)) {
                d->resUpdPoolMap |= mask;
                QRhiResourceUpdateBatch *u = d->resUpdPool[i];
                QRhiResourceUpdateBatchPrivate::get(u)->poolIndex = i;
                d->lastResUpdIdx = i;
                return u;
            }
            return nullptr;
        };
        const int poolSize = d->resUpdPool.size();
        for (int i = d->lastResUpdIdx + 1; i < poolSize; ++i) {
            if (QRhiResourceUpdateBatch *u = isFree(i))
                return u;
        }
        for (int i = 0; i <= d->lastResUpdIdx; ++i) {
            if (QRhiResourceUpdateBatch *u = isFree(i))
                return u;
        }
        return nullptr;
    };

    QRhiResourceUpdateBatch *u = nextFreeBatch();
    if (!u) {
        const int oldSize = d->resUpdPool.size();
        // 4, 8, 12, ..., up to 64
        const int newSize = oldSize + qMin(4, qMax(0, 64 - oldSize));
        d->resUpdPool.resize(newSize);
        for (int i = oldSize; i < newSize; ++i)
            d->resUpdPool[i] = new QRhiResourceUpdateBatch(d);
        u = nextFreeBatch();
        if (!u)
            qWarning("Resource update batch pool exhausted (max is 64)");
    }

    return u;
}

void QRhiResourceUpdateBatchPrivate::free()
{
    Q_ASSERT(poolIndex >= 0 && rhi->resUpdPool[poolIndex] == q);

    quint32 bufferDataTotal = 0;
    quint32 bufferLargeAllocTotal = 0;
    for (const BufferOp &op : std::as_const(bufferOps)) {
        bufferDataTotal += op.data.size();
        bufferLargeAllocTotal += op.data.largeAlloc(); // alloc when > 1 KB
    }

    if (QRHI_LOG_RUB().isDebugEnabled()) {
        qDebug() << "[rub] release to pool upd.batch #" << poolIndex
                << "/ bufferOps active" << activeBufferOpCount
                << "of" << bufferOps.count()
                << "data" << bufferDataTotal
                << "largeAlloc" << bufferLargeAllocTotal
                << "textureOps active" << activeTextureOpCount
                << "of" << textureOps.count();
    }

    activeBufferOpCount = 0;
    activeTextureOpCount = 0;

    const quint64 mask = 1ULL << quint64(poolIndex);
    rhi->resUpdPoolMap &= ~mask;
    poolIndex = -1;

    // textureOps is cleared, to not keep the potentially large image pixel
    // data alive, but it is expected that the container keeps the list alloc
    // at least. Only trimOpList() goes for the more aggressive route with squeeze.
    textureOps.clear();

    // bufferOps is not touched in many cases, to allow reusing allocations
    // (incl. in the elements' QRhiBufferData) as much as possible when this
    // batch is used again in the future, which is important for performance, in
    // particular with Qt Quick where it is easy for scenes to produce lots of,
    // typically small buffer changes on every frame.
    //
    // However, ensure that even in the unlikely case of having the max number
    // of batches (64) created in resUpdPool, no more than 64 MB in total is
    // used up by buffer data just to help future reuse. For simplicity, if
    // there is more than 1 MB data -> clear. Applications with frequent, huge
    // buffer updates probably have other bottlenecks anyway.
    if (bufferLargeAllocTotal > 1024 * 1024)
        bufferOps.clear();
}

void QRhiResourceUpdateBatchPrivate::merge(QRhiResourceUpdateBatchPrivate *other)
{
    int combinedSize = activeBufferOpCount + other->activeBufferOpCount;
    if (bufferOps.size() < combinedSize)
        bufferOps.resize(combinedSize);
    for (int i = activeBufferOpCount; i < combinedSize; ++i)
        bufferOps[i] = std::move(other->bufferOps[i - activeBufferOpCount]);
    activeBufferOpCount += other->activeBufferOpCount;

    combinedSize = activeTextureOpCount + other->activeTextureOpCount;
    if (textureOps.size() < combinedSize)
        textureOps.resize(combinedSize);
    for (int i = activeTextureOpCount; i < combinedSize; ++i)
        textureOps[i] = std::move(other->textureOps[i - activeTextureOpCount]);
    activeTextureOpCount += other->activeTextureOpCount;
}

bool QRhiResourceUpdateBatchPrivate::hasOptimalCapacity() const
{
    return activeBufferOpCount < BUFFER_OPS_STATIC_ALLOC - 4
            && activeTextureOpCount < TEXTURE_OPS_STATIC_ALLOC - 4;
}

void QRhiResourceUpdateBatchPrivate::trimOpLists()
{
    // Unlike free(), this is expected to aggressively deallocate all memory
    // used by both the buffer and texture operation lists. (i.e. using
    // squeeze() to only keep the stack prealloc of the QVLAs)
    //
    // This (e.g. just the destruction of bufferOps elements) may have a
    // non-negligible performance impact e.g. with Qt Quick with scenes where
    // there are lots of buffer operations per frame.

    activeBufferOpCount = 0;
    bufferOps.clear();
    bufferOps.squeeze();

    activeTextureOpCount = 0;
    textureOps.clear();
    textureOps.squeeze();
}

/*!
    Sometimes committing resource updates is necessary or just more convenient
    without starting a render pass. Calling this function with \a
    resourceUpdates is an alternative to passing \a resourceUpdates to a
    beginPass() call (or endPass(), which would be typical in case of readbacks).

    \note Cannot be called inside a pass.
 */
void QRhiCommandBuffer::resourceUpdate(QRhiResourceUpdateBatch *resourceUpdates)
{
    if (resourceUpdates)
        m_rhi->resourceUpdate(this, resourceUpdates);
}

/*!
    Records starting a new render pass targeting the render target \a rt.

    \a resourceUpdates, when not null, specifies a resource update batch that
    is to be committed and then released.

    The color and depth/stencil buffers of the render target are normally
    cleared. The clear values are specified in \a colorClearValue and \a
    depthStencilClearValue. The exception is when the render target was created
    with QRhiTextureRenderTarget::PreserveColorContents and/or
    QRhiTextureRenderTarget::PreserveDepthStencilContents. The clear values are
    ignored then.

    \note Enabling preserved color or depth contents leads to decreased
    performance depending on the underlying hardware. Mobile GPUs with tiled
    architecture benefit from not having to reload the previous contents into
    the tile buffer. Similarly, a QRhiTextureRenderTarget with a QRhiTexture as
    the depth buffer is less efficient than a QRhiRenderBuffer since using a
    depth texture triggers requiring writing the data out to it, while with
    renderbuffers this is not needed (as the API does not allow sampling or
    reading from a renderbuffer).

    \note Do not assume that any state or resource bindings persist between
    passes.

    \note The QRhiCommandBuffer's \c set and \c draw functions can only be
    called inside a pass. Also, with the exception of setGraphicsPipeline(),
    they expect to have a pipeline set already on the command buffer.
    Unspecified issues may arise otherwise, depending on the backend.

    If \a rt is a QRhiTextureRenderTarget, beginPass() performs a check to see
    if the texture and renderbuffer objects referenced from the render target
    are up-to-date. This is similar to what setShaderResources() does for
    QRhiShaderResourceBindings. If any of the attachments had been rebuilt
    since QRhiTextureRenderTarget::create(), an implicit call to create() is
    made on \a rt. Therefore, if \a rt has a QRhiTexture color attachment \c
    texture, and one needs to make the texture a different size, the following
    is then valid:
    \code
      QRhiTextureRenderTarget *rt = rhi->newTextureRenderTarget({ { texture } });
      rt->create();
      // ...
      texture->setPixelSize(new_size);
      texture->create();
      cb->beginPass(rt, colorClear, dsClear); // this is ok, no explicit rt->create() is required before
    \endcode

    \a flags allow controlling certain advanced functionality. One commonly used
    flag is \c ExternalContents. This should be specified whenever
    beginExternal() will be called within the pass started by this function.

    \sa endPass(), BeginPassFlags
 */
void QRhiCommandBuffer::beginPass(QRhiRenderTarget *rt,
                                  const QColor &colorClearValue,
                                  const QRhiDepthStencilClearValue &depthStencilClearValue,
                                  QRhiResourceUpdateBatch *resourceUpdates,
                                  BeginPassFlags flags)
{
    m_rhi->beginPass(this, rt, colorClearValue, depthStencilClearValue, resourceUpdates, flags);
}

/*!
    Records ending the current render pass.

    \a resourceUpdates, when not null, specifies a resource update batch that
    is to be committed and then released.

    \sa beginPass()
 */
void QRhiCommandBuffer::endPass(QRhiResourceUpdateBatch *resourceUpdates)
{
    m_rhi->endPass(this, resourceUpdates);
}

/*!
    Records setting a new graphics pipeline \a ps.

    \note This function must be called before recording other \c set or \c draw
    commands on the command buffer.

    \note QRhi will optimize out unnecessary invocations within a pass, so
    therefore overoptimizing to avoid calls to this function is not necessary
    on the applications' side.

    \note This function can only be called inside a render pass, meaning
    between a beginPass() and endPass() call.

    \note The new graphics pipeline \a ps must be a valid pointer.
 */
void QRhiCommandBuffer::setGraphicsPipeline(QRhiGraphicsPipeline *ps)
{
    Q_ASSERT(ps != nullptr);
    m_rhi->setGraphicsPipeline(this, ps);
}

/*!
    Records binding a set of shader resources, such as, uniform buffers or
    textures, that are made visible to one or more shader stages.

    \a srb can be null in which case the current graphics or compute pipeline's
    associated QRhiShaderResourceBindings is used. When \a srb is non-null, it
    must be
    \l{QRhiShaderResourceBindings::isLayoutCompatible()}{layout-compatible},
    meaning the layout (number of bindings, the type and binding number of each
    binding) must fully match the QRhiShaderResourceBindings that was
    associated with the pipeline at the time of calling the pipeline's create().

    There are cases when a seemingly unnecessary setShaderResources() call is
    mandatory: when rebuilding a resource referenced from \a srb, for example
    changing the size of a QRhiBuffer followed by a QRhiBuffer::create(), this
    is the place where associated native objects (such as descriptor sets in
    case of Vulkan) are updated to refer to the current native resources that
    back the QRhiBuffer, QRhiTexture, QRhiSampler objects referenced from \a
    srb. In this case setShaderResources() must be called even if \a srb is
    the same as in the last call.

    When \a srb is not null, the QRhiShaderResourceBindings object the pipeline
    was built with in create() is guaranteed to be not accessed in any form. In
    fact, it does not need to be valid even at this point: destroying the
    pipeline's associated srb after create() and instead explicitly specifying
    another, \l{QRhiShaderResourceBindings::isLayoutCompatible()}{layout
    compatible} one in every setShaderResources() call is valid.

    \a dynamicOffsets allows specifying buffer offsets for uniform buffers that
    were associated with \a srb via
    QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(). This is
    different from providing the offset in the \a srb itself: dynamic offsets
    do not require building a new QRhiShaderResourceBindings for every
    different offset, can avoid writing the underlying descriptors (with
    backends where applicable), and so they may be more efficient. Each element
    of \a dynamicOffsets is a \c binding - \c offset pair.
    \a dynamicOffsetCount specifies the number of elements in \a dynamicOffsets.

    \note All offsets in \a dynamicOffsets must be byte aligned to the value
    returned from QRhi::ubufAlignment().

    \note Some backends may limit the number of supported dynamic offsets.
    Avoid using a \a dynamicOffsetCount larger than 8.

    \note QRhi will optimize out unnecessary invocations within a pass (taking
    the conditions described above into account), so therefore overoptimizing
    to avoid calls to this function is not necessary on the applications' side.

    \note This function can only be called inside a render or compute pass,
    meaning between a beginPass() and endPass(), or beginComputePass() and
    endComputePass().
 */
void QRhiCommandBuffer::setShaderResources(QRhiShaderResourceBindings *srb,
                                           int dynamicOffsetCount,
                                           const DynamicOffset *dynamicOffsets)
{
    m_rhi->setShaderResources(this, srb, dynamicOffsetCount, dynamicOffsets);
}

/*!
    Records vertex input bindings.

    The index buffer used by subsequent drawIndexed() commands is specified by
    \a indexBuf, \a indexOffset, and \a indexFormat. \a indexBuf can be set to
    null when indexed drawing is not needed.

    Vertex buffer bindings are batched. \a startBinding specifies the first
    binding number. The recorded command then binds each buffer from \a
    bindings to the binding point \c{startBinding + i} where \c i is the index
    in \a bindings. Each element in \a bindings specifies a QRhiBuffer and an
    offset.

    \note Some backends may limit the number of vertex buffer bindings. Avoid
    using a \a bindingCount larger than 8.

    Superfluous vertex input and index changes in the same pass are ignored
    automatically with most backends and therefore applications do not need to
    overoptimize to avoid calls to this function.

    \note This function can only be called inside a render pass, meaning
    between a beginPass() and endPass() call.

    As a simple example, take a vertex shader with two inputs:

    \badcode
        layout(location = 0) in vec4 position;
        layout(location = 1) in vec3 color;
    \endcode

    and assume we have the data available in interleaved format, using only 2
    floats for position (so 5 floats per vertex: x, y, r, g, b). A QRhiGraphicsPipeline for
    this shader can then be created using the input layout:

    \code
        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({
            { 5 * sizeof(float) }
        });
        inputLayout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
            { 0, 1, QRhiVertexInputAttribute::Float3, 2 * sizeof(float) }
        });
    \endcode

    Here there is one buffer binding (binding number 0), with two inputs
    referencing it. When recording the pass, once the pipeline is set, the
    vertex bindings can be specified simply like the following, assuming vbuf
    is the QRhiBuffer with all the interleaved position+color data:

    \code
        const QRhiCommandBuffer::VertexInput vbufBinding(vbuf, 0);
        cb->setVertexInput(0, 1, &vbufBinding);
    \endcode
 */
void QRhiCommandBuffer::setVertexInput(int startBinding, int bindingCount, const VertexInput *bindings,
                                       QRhiBuffer *indexBuf, quint32 indexOffset,
                                       IndexFormat indexFormat)
{
    m_rhi->setVertexInput(this, startBinding, bindingCount, bindings, indexBuf, indexOffset, indexFormat);
}

/*!
    Records setting the active viewport rectangle specified in \a viewport.

    With backends where the underlying graphics API has scissoring always
    enabled, this function also sets the scissor to match the viewport whenever
    the active QRhiGraphicsPipeline does not have
    \l{QRhiGraphicsPipeline::UsesScissor}{UsesScissor} set.

    \note QRhi assumes OpenGL-style viewport coordinates, meaning x and y are
    bottom-left.

    \note This function can only be called inside a render pass, meaning
    between a beginPass() and endPass() call.
 */
void QRhiCommandBuffer::setViewport(const QRhiViewport &viewport)
{
    m_rhi->setViewport(this, viewport);
}

/*!
    Records setting the active scissor rectangle specified in \a scissor.

    This can only be called when the bound pipeline has
    \l{QRhiGraphicsPipeline::UsesScissor}{UsesScissor} set. When the flag is
    set on the active pipeline, this function must be called because scissor
    testing will get enabled and so a scissor rectangle must be provided.

    \note QRhi assumes OpenGL-style viewport coordinates, meaning x and y are
    bottom-left.

    \note This function can only be called inside a render pass, meaning
    between a beginPass() and endPass() call.
 */
void QRhiCommandBuffer::setScissor(const QRhiScissor &scissor)
{
    m_rhi->setScissor(this, scissor);
}

/*!
    Records setting the active blend constants to \a c.

    This can only be called when the bound pipeline has
    QRhiGraphicsPipeline::UsesBlendConstants set.

    \note This function can only be called inside a render pass, meaning
    between a beginPass() and endPass() call.
 */
void QRhiCommandBuffer::setBlendConstants(const QColor &c)
{
    m_rhi->setBlendConstants(this, c);
}

/*!
    Records setting the active stencil reference value to \a refValue.

    This can only be called when the bound pipeline has
    QRhiGraphicsPipeline::UsesStencilRef set.

    \note This function can only be called inside a render pass, meaning between
    a beginPass() and endPass() call.
 */
void QRhiCommandBuffer::setStencilRef(quint32 refValue)
{
    m_rhi->setStencilRef(this, refValue);
}

/*!
    Sets the shading rate for the following draw calls to \a coarsePixelSize.

    The default is 1x1.

    Functional only when the \l QRhi::VariableRateShading feature is reported as
    supported and the QRhiGraphicsPipeline(s) bound on the command buffer were
    declaring \l QRhiGraphicsPipeline::UsesShadingRate when creating them.

    Call \l QRhi::supportedShadingRates() to check what shading rates are
    supported for a given sample count.

    When both a QRhiShadingRateMap and this function is in use, the higher of
    two the shading rates are used for each tile. There is currently no control
    offered over the combiner behavior.

    \since 6.9
 */
void QRhiCommandBuffer::setShadingRate(const QSize &coarsePixelSize)
{
    m_rhi->setShadingRate(this, coarsePixelSize);
}

/*!
    Records a non-indexed draw.

    The number of vertices is specified in \a vertexCount. For instanced
    drawing set \a instanceCount to a value other than 1. \a firstVertex is the
    index of the first vertex to draw. When drawing multiple instances, the
    first instance ID is specified by \a firstInstance.

    \note \a firstInstance may not be supported, and is ignored when the
    QRhi::BaseInstance feature is reported as not supported. The first instance
    ID is always 0 in that case. QRhi::BaseInstance is never supported with
    OpenGL at the moment, mainly due to OpenGL ES limitations, and therefore
    portable applications should not be designed to rely on this argument.

    \note Shaders that need to access the index of the current vertex or
    instance must use \c gl_VertexIndex and \c gl_InstanceIndex, i.e., the
    Vulkan-compatible built-in variables, instead of \c gl_VertexID and \c
    gl_InstanceID.

    \note This function can only be called inside a render pass, meaning
    between a beginPass() and endPass() call.
 */
void QRhiCommandBuffer::draw(quint32 vertexCount,
                             quint32 instanceCount,
                             quint32 firstVertex,
                             quint32 firstInstance)
{
    m_rhi->draw(this, vertexCount, instanceCount, firstVertex, firstInstance);
}

/*!
    Records an indexed draw.

    The number of vertices is specified in \a indexCount. \a firstIndex is the
    base index. The effective offset in the index buffer is given by
    \c{indexOffset + firstIndex * n} where \c n is 2 or 4 depending on the
    index element type. \c indexOffset is specified in setVertexInput().

    \note The effective offset in the index buffer must be 4 byte aligned with
    some backends (for example, Metal). With these backends the
    \l{QRhi::NonFourAlignedEffectiveIndexBufferOffset}{NonFourAlignedEffectiveIndexBufferOffset}
    feature will be reported as not-supported.

    \a vertexOffset (also called \c{base vertex}) is a signed value that is
    added to the element index before indexing into the vertex buffer. Support
    for this is not always available, and the value is ignored when the feature
    QRhi::BaseVertex is reported as unsupported.

    For instanced drawing set \a instanceCount to a value other than 1. When
    drawing multiple instances, the first instance ID is specified by \a
    firstInstance.

    \note \a firstInstance may not be supported, and is ignored when the
    QRhi::BaseInstance feature is reported as not supported. The first instance
    ID is always 0 in that case. QRhi::BaseInstance is never supported with
    OpenGL at the moment, mainly due to OpenGL ES limitations, and therefore
    portable applications should not be designed to rely on this argument.

    \note Shaders that need to access the index of the current vertex or
    instance must use \c gl_VertexIndex and \c gl_InstanceIndex, i.e., the
    Vulkan-compatible built-in variables, instead of \c gl_VertexID and \c
    gl_InstanceID.

    \note This function can only be called inside a render pass, meaning
    between a beginPass() and endPass() call.
 */
void QRhiCommandBuffer::drawIndexed(quint32 indexCount,
                                    quint32 instanceCount,
                                    quint32 firstIndex,
                                    qint32 vertexOffset,
                                    quint32 firstInstance)
{
    m_rhi->drawIndexed(this, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

/*!
    Records a named debug group on the command buffer with the specified \a
    name. This is shown in graphics debugging tools such as
    \l{https://renderdoc.org/}{RenderDoc} and
    \l{https://developer.apple.com/xcode/}{XCode}. The end of the grouping is
    indicated by debugMarkEnd().

    \note Ignored when QRhi::DebugMarkers are not supported or
    QRhi::EnableDebugMarkers is not set.

    \note Can be called anywhere within the frame, both inside and outside of passes.
 */
void QRhiCommandBuffer::debugMarkBegin(const QByteArray &name)
{
    m_rhi->debugMarkBegin(this, name);
}

/*!
    Records the end of a debug group.

    \note Ignored when QRhi::DebugMarkers are not supported or
    QRhi::EnableDebugMarkers is not set.

    \note Can be called anywhere within the frame, both inside and outside of passes.
 */
void QRhiCommandBuffer::debugMarkEnd()
{
    m_rhi->debugMarkEnd(this);
}

/*!
    Inserts a debug message \a msg into the command stream.

    \note Ignored when QRhi::DebugMarkers are not supported or
    QRhi::EnableDebugMarkers is not set.

    \note With some backends debugMarkMsg() is only supported inside a pass and
    is ignored when called outside a pass. With others it is recorded anywhere
    within the frame.
 */
void QRhiCommandBuffer::debugMarkMsg(const QByteArray &msg)
{
    m_rhi->debugMarkMsg(this, msg);
}

/*!
    Records starting a new compute pass.

    \a resourceUpdates, when not null, specifies a resource update batch that
    is to be committed and then released.

    \note Do not assume that any state or resource bindings persist between
    passes.

    \note A compute pass can record setComputePipeline(), setShaderResources(),
    and dispatch() calls, not graphics ones. General functionality, such as,
    debug markers and beginExternal() is available both in render and compute
    passes.

    \note Compute is only available when the \l{QRhi::Compute}{Compute} feature
    is reported as supported.

    \a flags is not currently used.
 */
void QRhiCommandBuffer::beginComputePass(QRhiResourceUpdateBatch *resourceUpdates, BeginPassFlags flags)
{
    m_rhi->beginComputePass(this, resourceUpdates, flags);
}

/*!
    Records ending the current compute pass.

    \a resourceUpdates, when not null, specifies a resource update batch that
    is to be committed and then released.
 */
void QRhiCommandBuffer::endComputePass(QRhiResourceUpdateBatch *resourceUpdates)
{
    m_rhi->endComputePass(this, resourceUpdates);
}

/*!
    Records setting a new compute pipeline \a ps.

    \note This function must be called before recording setShaderResources() or
    dispatch() commands on the command buffer.

    \note QRhi will optimize out unnecessary invocations within a pass, so
    therefore overoptimizing to avoid calls to this function is not necessary
    on the applications' side.

    \note This function can only be called inside a compute pass, meaning
    between a beginComputePass() and endComputePass() call.
 */
void QRhiCommandBuffer::setComputePipeline(QRhiComputePipeline *ps)
{
    m_rhi->setComputePipeline(this, ps);
}

/*!
    Records dispatching compute work items, with \a x, \a y, and \a z
    specifying the number of local workgroups in the corresponding dimension.

    \note This function can only be called inside a compute pass, meaning
    between a beginComputePass() and endComputePass() call.

    \note \a x, \a y, and \a z must fit the limits from the underlying graphics
    API implementation at run time. The maximum values are typically 65535.

    \note Watch out for possible limits on the local workgroup size as well.
    This is specified in the shader, for example: \c{layout(local_size_x = 16,
    local_size_y = 16) in;}. For example, with OpenGL the minimum value mandated
    by the specification for the number of invocations in a single local work
    group (the product of \c local_size_x, \c local_size_y, and \c local_size_z)
    is 1024, while with OpenGL ES (3.1) the value may be as low as 128. This
    means that the example given above may be rejected by some OpenGL ES
    implementations as the number of invocations is 256.
 */
void QRhiCommandBuffer::dispatch(int x, int y, int z)
{
    m_rhi->dispatch(this, x, y, z);
}

/*!
    \return a pointer to a backend-specific QRhiNativeHandles subclass, such as
    QRhiVulkanCommandBufferNativeHandles. The returned value is \nullptr when
    exposing the underlying native resources is not supported by, or not
    applicable to, the backend.

    \sa QRhiVulkanCommandBufferNativeHandles,
    QRhiMetalCommandBufferNativeHandles, beginExternal(), endExternal()
 */
const QRhiNativeHandles *QRhiCommandBuffer::nativeHandles()
{
    return m_rhi->nativeHandles(this);
}

/*!
    To be called when the application before the application is about to
    enqueue commands to the current pass' command buffer by calling graphics
    API functions directly.

    \note This is only available when the intent was declared upfront in
    beginPass() or beginComputePass(). Therefore this function must only be
    called when the pass recording was started with specifying
    QRhiCommandBuffer::ExternalContent.

    With Vulkan, Metal, or Direct3D 12 one can query the native command buffer
    or encoder objects via nativeHandles() and enqueue commands to them. With
    OpenGL or Direct3D 11 the (device) context can be retrieved from
    QRhi::nativeHandles(). However, this must never be done without ensuring
    the QRhiCommandBuffer's state stays up-to-date. Hence the requirement for
    wrapping any externally added command recording between beginExternal() and
    endExternal(). Conceptually this is the same as QPainter's
    \l{QPainter::beginNativePainting()}{beginNativePainting()} and
    \l{QPainter::endNativePainting()}{endNativePainting()} functions.

    For OpenGL in particular, this function has an additional task: it makes
    sure the context is made current on the current thread.

    \note Once beginExternal() is called, no other render pass specific
    functions (\c set* or \c draw*) must be called on the
    QRhiCommandBuffer until endExternal().

    \warning Some backends may return a native command buffer object from
    QRhiCommandBuffer::nativeHandles() that is different from the primary one
    when inside a beginExternal() - endExternal() block. Therefore it is
    important to (re)query the native command buffer object after calling
    beginExternal(). In practical terms this means that with Vulkan for example
    the externally recorded Vulkan commands are placed onto a secondary command
    buffer (with VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT).
    nativeHandles() returns this secondary command buffer when called between
    begin/endExternal.

    \sa endExternal(), nativeHandles()
 */
void QRhiCommandBuffer::beginExternal()
{
    m_rhi->beginExternal(this);
}

/*!
    To be called once the externally added commands are recorded to the command
    buffer or context.

    \note All QRhiCommandBuffer state must be assumed as invalid after calling
    this function. Pipelines, vertex and index buffers, and other state must be
    set again if more draw calls are recorded after the external commands.

    \sa beginExternal(), nativeHandles()
 */
void QRhiCommandBuffer::endExternal()
{
    m_rhi->endExternal(this);
}

/*!
    \return the last available timestamp, in seconds, when
    \l QRhi::EnableTimestamps was enabled when creating the QRhi. The value
    indicates the elapsed time on the GPU during the last completed frame.

    \note Do not expect results other than 0 when the QRhi::Timestamps feature
    is not reported as supported, or when QRhi::EnableTimestamps was not passed
    to QRhi::create(). There are exceptions to this, because with some graphics
    APIs (Metal) timings are available without having to perform extra
    operations (timestamp queries), but portable applications should always
    consciously opt-in to timestamp collection when they know it is needed, and
    call this function accordingly.

    Care must be exercised with the interpretation of the value, as its
    precision and granularity is often not controlled by Qt, and depends on the
    underlying graphics API and its implementation. In particular, comparing
    the values between different graphics APIs and hardware is discouraged and
    may be meaningless.

    When the frame was recorded with \l{QRhi::beginFrame()}{beginFrame()} and
    \l{QRhi::endFrame()}{endFrame()}, i.e., with a swapchain, the timing values
    will likely become available asynchronously. The returned value may
    therefore be 0 (e.g., for the first 1-2 frames) or the last known value
    referring to some previous frame. The value my also
    become 0 again under certain conditions, such as when resizing the window.
    It can be expected that the most up-to-date available value is retrieved in
    beginFrame() and becomes queriable via this function once beginFrame()
    returns.

    \note Do not assume that the value refers to the previous
    (\c{currently_recorded - 1}) frame. It may refer to \c{currently_recorded -
    2} or \c{currently_recorded - 3} as well. The exact behavior may depend on
    the graphics API and its implementation.

    On the other hand, with offscreen frames the returned value is up-to-date
    once \l{QRhi::endOffscreenFrame()}{endOffscreenFrame()} returns, because
    offscreen frames reduce GPU pipelining and wait the the commands to be
    complete.

    \note This means that, unlike with swapchain frames, with offscreen frames
    the returned value is guaranteed to refer to the frame that has just been
    submitted and completed. (assuming this function is called after
    endOffscreenFrame() but before the next beginOffscreenFrame())

    Watch out for the consequences of GPU frequency scaling and GPU clock
    changes, depending on the platform. For example, on Windows the returned
    timing may vary in a quite wide range between frames with modern graphics
    cards, even when submitting frames with a similar, or the same workload.
    This is out of scope for Qt to control and solve, generally speaking.
    However, the D3D12 backend automatically calls
    \l{https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-setstablepowerstate}{ID3D12Device::SetStablePowerState()}
    whenever the environment variable \c QT_D3D_STABLE_POWER_STATE is set to a
    non-zero value. This can greatly stabilize the result. It can also have a
    non-insignificant effect on the CPU-side timings measured via QElapsedTimer
    for example, especially when offscreen frames are involved.

    \note Do not and never ship applications to production with
    \c QT_D3D_STABLE_POWER_STATE set. See the Windows API documentation for details.

    \sa QRhi::Timestamps, QRhi::EnableTimestamps
 */
double QRhiCommandBuffer::lastCompletedGpuTime()
{
    return m_rhi->lastCompletedGpuTime(this);
}

/*!
    \return the value (typically an offset) \a v aligned to the uniform buffer
    alignment given by by ubufAlignment().
 */
int QRhi::ubufAligned(int v) const
{
    const int byteAlign = ubufAlignment();
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

/*!
    \return the number of mip levels for a given \a size.
 */
int QRhi::mipLevelsForSize(const QSize &size)
{
    return qFloor(std::log2(qMax(size.width(), size.height()))) + 1;
}

/*!
    \return the texture image size for a given \a mipLevel, calculated based on
    the level 0 size given in \a baseLevelSize.
 */
QSize QRhi::sizeForMipLevel(int mipLevel, const QSize &baseLevelSize)
{
    const int w = qMax(1, baseLevelSize.width() >> mipLevel);
    const int h = qMax(1, baseLevelSize.height() >> mipLevel);
    return QSize(w, h);
}

/*!
    \return \c true if the underlying graphics API has the Y axis pointing up
    in framebuffers and images.

    In practice this is \c true for OpenGL only.
 */
bool QRhi::isYUpInFramebuffer() const
{
    return d->isYUpInFramebuffer();
}

/*!
    \return \c true if the underlying graphics API has the Y axis pointing up
    in its normalized device coordinate system.

    In practice this is \c false for Vulkan only.

    \note clipSpaceCorrMatrix() includes the corresponding adjustment (to make
    Y point up) in its returned matrix.
 */
bool QRhi::isYUpInNDC() const
{
    return d->isYUpInNDC();
}

/*!
    \return \c true if the underlying graphics API uses depth range [0, 1] in
    clip space.

    In practice this is \c false for OpenGL only, because OpenGL uses a
    post-projection depth range of [-1, 1]. (not to be confused with the
    NDC-to-window mapping controlled by glDepthRange(), which uses a range of
    [0, 1], unless overridden by the QRhiViewport) In some OpenGL versions
    glClipControl() could be used to change this, but the OpenGL backend of
    QRhi does not use that function as it is not available in OpenGL ES or
    OpenGL versions lower than 4.5.

    \note clipSpaceCorrMatrix() includes the corresponding adjustment in its
    returned matrix. Therefore, many users of QRhi do not need to take any
    further measures apart from pre-multiplying their projection matrices with
    clipSpaceCorrMatrix(). However, some graphics techniques, such as, some
    types of shadow mapping, involve working with and outputting depth values
    in the shaders. These will need to query and take the value of this
    function into account as appropriate.
 */
bool QRhi::isClipDepthZeroToOne() const
{
    return d->isClipDepthZeroToOne();
}

/*!
    \return a matrix that can be used to allow applications keep using
    OpenGL-targeted vertex data and perspective projection matrices (such as,
    the ones generated by QMatrix4x4::perspective()), regardless of the active
    QRhi backend.

    In a typical renderer, once \c{this_matrix * mvp} is used instead of just
    \c mvp, vertex data with Y up and viewports with depth range 0 - 1 can be
    used without considering what backend (and so graphics API) is going to be
    used at run time. This way branching based on isYUpInNDC() and
    isClipDepthZeroToOne() can be avoided (although such logic may still become
    required when implementing certain advanced graphics techniques).

    See
    \l{https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/}{this
    page} for a discussion of the topic from Vulkan perspective.
 */
QMatrix4x4 QRhi::clipSpaceCorrMatrix() const
{
    return d->clipSpaceCorrMatrix();
}

/*!
    \return \c true if the specified texture \a format modified by \a flags is
    supported.

    The query is supported both for uncompressed and compressed formats.
 */
bool QRhi::isTextureFormatSupported(QRhiTexture::Format format, QRhiTexture::Flags flags) const
{
    return d->isTextureFormatSupported(format, flags);
}

/*!
    \return \c true if the specified \a feature is supported
 */
bool QRhi::isFeatureSupported(QRhi::Feature feature) const
{
    return d->isFeatureSupported(feature);
}

/*!
    \return the value for the specified resource \a limit.

    The values are expected to be queried by the backends upon initialization,
    meaning calling this function is a light operation.
 */
int QRhi::resourceLimit(ResourceLimit limit) const
{
    return d->resourceLimit(limit);
}

/*!
    \return a pointer to the backend-specific collection of native objects
    for the device, context, and similar concepts used by the backend.

    Cast to QRhiVulkanNativeHandles, QRhiD3D11NativeHandles,
    QRhiD3D12NativeHandles, QRhiGles2NativeHandles, or QRhiMetalNativeHandles
    as appropriate.

    \note No ownership is transferred, neither for the returned pointer nor for
    any native objects.
 */
const QRhiNativeHandles *QRhi::nativeHandles()
{
    return d->nativeHandles();
}

/*!
    With OpenGL this makes the OpenGL context current on the current thread.
    The function has no effect with other backends.

    Calling this function is relevant typically in Qt framework code, when one
    has to ensure external OpenGL code provided by the application can still
    run like it did before with direct usage of OpenGL, as long as the QRhi is
    using the OpenGL backend.

    \return false when failed, similarly to QOpenGLContext::makeCurrent(). When
    the operation failed, isDeviceLost() can be called to determine if there
    was a loss of context situation. Such a check is equivalent to checking via
    QOpenGLContext::isValid().

    \sa QOpenGLContext::makeCurrent(), QOpenGLContext::isValid()
 */
bool QRhi::makeThreadLocalNativeContextCurrent()
{
    return d->makeThreadLocalNativeContextCurrent();
}

/*!
    With backends and graphics APIs where applicable, this function allows to
    provide additional arguments to the \b next submission of commands to the
    graphics command queue.

    In particular, with Vulkan this allows passing in a list of Vulkan semaphore
    objects for \c vkQueueSubmit() to signal and wait on. \a params must then be
    a \l QRhiVulkanQueueSubmitParams. This becomes essential in certain advanced
    use cases, such as when performing native Vulkan calls that involve having
    to wait on and signal VkSemaphores that the application's custom Vulkan
    rendering or compute code manages. In addition, this also allows specifying
    additional semaphores to wait on in the next \c vkQueuePresentKHR().

    \note This function affects the next queue submission only, which will
    happen in endFrame(), endOffscreenFrame(), or finish(). The enqueuing of
    present happens in endFrame().

    With many other backends the implementation of this function is a no-op.

    \since 6.9
 */
void QRhi::setQueueSubmitParams(QRhiNativeHandles *params)
{
    d->setQueueSubmitParams(params);
}

/*!
    Attempts to release resources in the backend's caches. This can include both
    CPU and GPU resources.  Only memory and resources that can be recreated
    automatically are in scope. As an example, if the backend's
    QRhiGraphicsPipeline implementation maintains a cache of shader compilation
    results, calling this function leads to emptying that cache, thus
    potentially freeing up memory and graphics resources.

    Calling this function makes sense in resource constrained environments,
    where at a certain point there is a need to ensure minimal resource usage,
    at the expense of performance.
 */
void QRhi::releaseCachedResources()
{
    d->releaseCachedResources();

    for (QRhiResourceUpdateBatch *u : d->resUpdPool) {
        if (u->d->poolIndex < 0)
            u->d->trimOpLists();
    }
}

/*!
    \return true if the graphics device was lost.

    The loss of the device is typically detected in beginFrame(), endFrame() or
    QRhiSwapChain::createOrResize(), depending on the backend and the underlying
    native APIs. The most common is endFrame() because that is where presenting
    happens. With some backends QRhiSwapChain::createOrResize() can also fail
    due to a device loss. Therefore this function is provided as a generic way
    to check if a device loss was detected by a previous operation.

    When the device is lost, no further operations should be done via the QRhi.
    Rather, all QRhi resources should be released, followed by destroying the
    QRhi. A new QRhi can then be attempted to be created. If successful, all
    graphics resources must be reinitialized. If not, try again later,
    repeatedly.

    While simple applications may decide to not care about device loss,
    on the commonly used desktop platforms a device loss can happen
    due to a variety of reasons, including physically disconnecting the
    graphics adapter, disabling the device or driver, uninstalling or upgrading
    the graphics driver, or due to errors that lead to a graphics device reset.
    Some of these can happen under perfectly normal circumstances as well, for
    example the upgrade of the graphics driver to a newer version is a common
    task that can happen at any time while a Qt application is running. Users
    may very well expect applications to be able to survive this, even when the
    application is actively using an API like OpenGL or Direct3D.

    Qt's own frameworks built on top of QRhi, such as, Qt Quick, can be
    expected to handle and take appropriate measures when a device loss occurs.
    If the data for graphics resources, such as textures and buffers, are still
    available on the CPU side, such an event may not be noticeable on the
    application level at all since graphics resources can seamlessly be
    reinitialized then. However, applications and libraries working directly
    with QRhi are expected to be prepared to check and handle device loss
    situations themselves.

    \note With OpenGL, applications may need to opt-in to context reset
    notifications by setting QSurfaceFormat::ResetNotification on the
    QOpenGLContext. This is typically done by enabling the flag in
    QRhiGles2InitParams::format. Keep in mind however that some systems may
    generate context resets situations even when this flag is not set.
 */
bool QRhi::isDeviceLost() const
{
    return d->isDeviceLost();
}

/*!
    \return a binary data blob with data collected from the
    QRhiGraphicsPipeline and QRhiComputePipeline successfully created during
    the lifetime of this QRhi.

    By saving and then, in subsequent runs of the same application, reloading
    the cache data, pipeline and shader creation times can potentially be
    reduced. What exactly the cache and its serialized version includes is not
    specified, is always specific to the backend used, and in some cases also
    dependent on the particular implementation of the graphics API.

    When the PipelineCacheDataLoadSave is reported as unsupported, the returned
    QByteArray is empty.

    When the EnablePipelineCacheDataSave flag was not specified when calling
    create(), the returned QByteArray may be empty, even when the
    PipelineCacheDataLoadSave feature is supported.

    When the returned data is non-empty, it is always specific to the Qt
    version and QRhi backend. In addition, in some cases there is a strong
    dependency to the graphics device and the exact driver version used. QRhi
    takes care of adding the appropriate header and safeguards that ensure that
    the data can always be passed safely to setPipelineCacheData(), therefore
    attempting to load data from a run on another version of a driver will be
    handled safely and gracefully.

    \note Calling releaseCachedResources() may, depending on the backend, clear
    the pipeline data collected. A subsequent call to this function may then
    not return any data.

    See EnablePipelineCacheDataSave for further details about this feature.

    \note Minimize the number of calls to this function. Retrieving the blob is
    not always a cheap operation, and therefore this function should only be
    called at a low frequency, ideally only once e.g. when closing the
    application.

    \sa setPipelineCacheData(), create(), isFeatureSupported()
 */
QByteArray QRhi::pipelineCacheData()
{
    return d->pipelineCacheData();
}

/*!
    Loads \a data into the pipeline cache, when applicable.

    When the PipelineCacheDataLoadSave is reported as unsupported, the function
    is safe to call, but has no effect.

    The blob returned by pipelineCacheData() is always specific to the Qt
    version, the QRhi backend, and, in some cases, also to the graphics device,
    and a given version of the graphics driver. QRhi takes care of adding the
    appropriate header and safeguards that ensure that the data can always be
    passed safely to this function. If there is a mismatch, e.g. because the
    driver has been upgraded to a newer version, or because the data was
    generated from a different QRhi backend, a warning is printed and \a data
    is safely ignored.

    With Vulkan, this maps directly to VkPipelineCache. Calling this function
    creates a new Vulkan pipeline cache object, with its initial data sourced
    from \a data. The pipeline cache object is then used by all subsequently
    created QRhiGraphicsPipeline and QRhiComputePipeline objects, thus
    accelerating, potentially, the pipeline creation.

    With other APIs there is no real pipeline cache, but they may provide a
    cache with bytecode from shader compilations (D3D) or program binaries
    (OpenGL). In applications that perform a lot of shader compilation from
    source at run time this can provide a significant boost in subsequent runs
    if the "pipeline cache" is pre-seeded from an earlier run using this
    function.

    \note QRhi cannot give any guarantees that \a data has an effect on the
    pipeline and shader creation performance. With APIs like Vulkan, it is up
    to the driver to decide if \a data is used for some purpose, or if it is
    ignored.

    See EnablePipelineCacheDataSave for further details about this feature.

    \note This mechanism offered by QRhi is independent of the drivers' own
    internal caching mechanism, if any. This means that, depending on the
    graphics API and its implementation, the exact effects of retrieving and
    then reloading \a data are not predictable. Improved performance may not be
    visible at all in case other caching mechanisms outside of Qt's control are
    already active.

    \note Minimize the number of calls to this function. Loading the blob is
    not always a cheap operation, and therefore this function should only be
    called at a low frequency, ideally only once e.g. when starting the
    application.

    \warning Serialized pipeline cache data is assumed to be trusted content. Qt
    performs robust parsing of the header and metadata included in \a data,
    application developers are however advised to never pass in data from
    untrusted sources.

    \sa pipelineCacheData(), isFeatureSupported()
 */
void QRhi::setPipelineCacheData(const QByteArray &data)
{
    d->setPipelineCacheData(data);
}

/*!
    \struct QRhiStats
    \inmodule QtGuiPrivate
    \inheaderfile rhi/qrhi.h
    \since 6.6

    \brief Statistics provided from the underlying memory allocator.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \variable QRhiStats::totalPipelineCreationTime

    The total time in milliseconds spent in graphics and compute pipeline
    creation, which usually involves shader compilation or cache lookups, and
    potentially expensive processing.

    \note The value should not be compared between different backends since the
    concept of "pipelines" and what exactly happens under the hood during, for
    instance, a call to QRhiGraphicsPipeline::create(), differ greatly between
    graphics APIs and their implementations.

    \sa QRhi::statistics()
*/

/*!
    \variable QRhiStats::blockCount

    Statistic reported from the Vulkan or D3D12 memory allocator.

    \sa QRhi::statistics()
*/

/*!
    \variable QRhiStats::allocCount

    Statistic reported from the Vulkan or D3D12 memory allocator.

    \sa QRhi::statistics()
*/

/*!
    \variable QRhiStats::usedBytes

    Statistic reported from the Vulkan or D3D12 memory allocator.

    \sa QRhi::statistics()
*/

/*!
    \variable QRhiStats::unusedBytes

    Statistic reported from the Vulkan or D3D12 memory allocator.

    \sa QRhi::statistics()
*/

/*!
    \variable QRhiStats::totalUsageBytes

    Valid only with D3D12 currently. Matches IDXGIAdapter3::QueryVideoMemoryInfo().

    \sa QRhi::statistics()
*/

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QRhiStats &info)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QRhiStats("
                  << "totalPipelineCreationTime=" << info.totalPipelineCreationTime
                  << " blockCount=" << info.blockCount
                  << " allocCount=" << info.allocCount
                  << " usedBytes=" << info.usedBytes
                  << " unusedBytes=" << info.unusedBytes
                  << " totalUsageBytes=" << info.totalUsageBytes
                  << ')';
    return dbg;
}
#endif

/*!
    Gathers and returns statistics about the timings and allocations of
    graphics resources.

    Data about memory allocations is only available with some backends, where
    such operations are under Qt's control. With graphics APIs where there is
    no lower level control over resource memory allocations, this will never be
    supported and all relevant fields in the results are 0.

    With Vulkan in particular, the values are valid always, and are queried
    from the underlying memory allocator library. This gives an insight into
    the memory requirements of the active buffers and textures.

    The same is true for Direct 3D 12. In addition to the memory allocator
    library's statistics, here the result also includes a \c totalUsageBytes
    field which reports the total size including additional resources that are
    not under the memory allocator library's control (swapchain buffers,
    descriptor heaps, etc.), as reported by DXGI.

    The values correspond to all types of memory used, combined. (i.e. video +
    system in case of a discreet GPU)

    Additional data, such as the total time in milliseconds spent in graphics
    and compute pipeline creation (which usually involves shader compilation or
    cache lookups, and potentially expensive processing) is available with most
    backends.

    \note The elapsed times for operations such as pipeline creation may be
    affected by various factors. The results should not be compared between
    different backends since the concept of "pipelines" and what exactly
    happens under the hood during, for instance, a call to
    QRhiGraphicsPipeline::create(), differ greatly between graphics APIs and
    their implementations.

    \note Additionally, many drivers will likely employ various caching
    strategies for shaders, programs, pipelines. (independently of Qt's own
    similar facilities, such as setPipelineCacheData() or the OpenGL-specific
    program binary disk cache). Because such internal behavior is transparent
    to the API client, Qt and QRhi have no knowledge or control over the exact
    caching strategy, persistency, invalidation of the cached data, etc. When
    reading timings, such as the time spent on pipeline creation, the potential
    presence and unspecified behavior of driver-level caching mechanisms should
    be kept in mind.
 */
QRhiStats QRhi::statistics() const
{
    return d->statistics();
}

/*!
    \return a new graphics pipeline resource.

    \sa QRhiResource::destroy()
 */
QRhiGraphicsPipeline *QRhi::newGraphicsPipeline()
{
    return d->createGraphicsPipeline();
}

/*!
    \return a new compute pipeline resource.

    \note Compute is only available when the \l{QRhi::Compute}{Compute} feature
    is reported as supported.

    \sa QRhiResource::destroy()
 */
QRhiComputePipeline *QRhi::newComputePipeline()
{
    return d->createComputePipeline();
}

/*!
    \return a new shader resource binding collection resource.

    \sa QRhiResource::destroy()
 */
QRhiShaderResourceBindings *QRhi::newShaderResourceBindings()
{
    return d->createShaderResourceBindings();
}

/*!
    \return a new buffer with the specified \a type, \a usage, and \a size.

    \note Some \a usage and \a type combinations may not be supported by all
    backends. See \l{QRhiBuffer::UsageFlag}{UsageFlags} and
    \l{QRhi::NonDynamicUniformBuffers}{the feature flags}.

    \note Backends may choose to allocate buffers bigger than \a size. This is
    done transparently to applications, so there are no special restrictions on
    the value of \a size. QRhiBuffer::size() will always report back the value
    that was requested in \a size.

    \sa QRhiResource::destroy()
 */
QRhiBuffer *QRhi::newBuffer(QRhiBuffer::Type type,
                            QRhiBuffer::UsageFlags usage,
                            quint32 size)
{
    return d->createBuffer(type, usage, size);
}

/*!
    \return a new renderbuffer with the specified \a type, \a pixelSize, \a
    sampleCount, and \a flags.

    When \a backingFormatHint is set to a texture format other than
    QRhiTexture::UnknownFormat, it may be used by the backend to decide what
    format to use for the storage backing the renderbuffer.

    \note \a backingFormatHint becomes relevant typically when multisampling
    and floating point texture formats are involved: rendering into a
    multisample QRhiRenderBuffer and then resolving into a non-RGBA8
    QRhiTexture implies (with some graphics APIs) that the storage backing the
    QRhiRenderBuffer uses the matching non-RGBA8 format. That means that
    passing a format like QRhiTexture::RGBA32F is important, because backends
    will typically opt for QRhiTexture::RGBA8 by default, which would then
    break later on due to attempting to set up RGBA8->RGBA32F multisample
    resolve in the color attachment(s) of the QRhiTextureRenderTarget.

    \sa QRhiResource::destroy()
 */
QRhiRenderBuffer *QRhi::newRenderBuffer(QRhiRenderBuffer::Type type,
                                        const QSize &pixelSize,
                                        int sampleCount,
                                        QRhiRenderBuffer::Flags flags,
                                        QRhiTexture::Format backingFormatHint)
{
    return d->createRenderBuffer(type, pixelSize, sampleCount, flags, backingFormatHint);
}

/*!
    \return a new 1D or 2D texture with the specified \a format, \a pixelSize, \a
    sampleCount, and \a flags.

    A 1D texture array must have QRhiTexture::OneDimensional set in \a flags.  This
    function will implicitly set this flag if the \a pixelSize height is 0.

    \note \a format specifies the requested internal and external format,
    meaning the data to be uploaded to the texture will need to be in a
    compatible format, while the native texture may (but is not guaranteed to,
    in case of OpenGL at least) use this format internally.

    \note 1D textures are only functional when the OneDimensionalTextures feature is
    reported as supported at run time. Further, mipmaps on 1D textures are only
    functional when the OneDimensionalTextureMipmaps feature is reported at run time.

    \sa QRhiResource::destroy()
 */
QRhiTexture *QRhi::newTexture(QRhiTexture::Format format,
                              const QSize &pixelSize,
                              int sampleCount,
                              QRhiTexture::Flags flags)
{
    if (pixelSize.height() == 0)
        flags |= QRhiTexture::OneDimensional;

    return d->createTexture(format, pixelSize, 1, 0, sampleCount, flags);
}

/*!
    \return a new 1D, 2D or 3D texture with the specified \a format, \a width, \a
    height, \a depth, \a sampleCount, and \a flags.

    This overload is suitable for 3D textures because it allows specifying \a
    depth. A 3D texture must have QRhiTexture::ThreeDimensional set in \a
    flags, but using this overload that can be omitted because the flag is set
    implicitly whenever \a depth is greater than 0. For 1D, 2D and cube textures \a
    depth should be set to 0.

    A 1D texture must have QRhiTexture::OneDimensional set in \a flags.  This overload
    will implicitly set this flag if both \a height and \a depth are 0.

    \note 3D textures are only functional when the ThreeDimensionalTextures
    feature is reported as supported at run time.

    \note 1D textures are only functional when the OneDimensionalTextures feature is
    reported as supported at run time. Further, mipmaps on 1D textures are only
    functional when the OneDimensionalTextureMipmaps feature is reported at run time.

    \overload
 */
QRhiTexture *QRhi::newTexture(QRhiTexture::Format format,
                              int width, int height, int depth,
                              int sampleCount,
                              QRhiTexture::Flags flags)
{
    if (depth > 0)
        flags |= QRhiTexture::ThreeDimensional;

    if (height == 0 && depth == 0)
        flags |= QRhiTexture::OneDimensional;

    return d->createTexture(format, QSize(width, height), depth, 0, sampleCount, flags);
}

/*!
    \return a new 1D or 2D texture array with the specified \a format, \a arraySize,
    \a pixelSize, \a sampleCount, and \a flags.

    This function implicitly sets QRhiTexture::TextureArray in \a flags.

    A 1D texture array must have QRhiTexture::OneDimensional set in \a flags.  This
    function will implicitly set this flag if the \a pixelSize height is 0.

    \note Do not confuse texture arrays with arrays of textures. A QRhiTexture
    created by this function is usable with 1D or 2D array samplers in the shader, for
    example: \c{layout(binding = 1) uniform sampler2DArray texArr;}. Arrays of
    textures refers to a list of textures that are exposed to the shader via
    QRhiShaderResourceBinding::sampledTextures() and a count > 1, and declared
    in the shader for example like this: \c{layout(binding = 1) uniform
    sampler2D textures[4];}

    \note This is only functional when the TextureArrays feature is reported as
    supported at run time.

    \note 1D textures are only functional when the OneDimensionalTextures feature is
    reported as supported at run time. Further, mipmaps on 1D textures are only
    functional when the OneDimensionalTextureMipmaps feature is reported at run time.


    \sa newTexture()
 */
QRhiTexture *QRhi::newTextureArray(QRhiTexture::Format format,
                                   int arraySize,
                                   const QSize &pixelSize,
                                   int sampleCount,
                                   QRhiTexture::Flags flags)
{
    flags |= QRhiTexture::TextureArray;

    if (pixelSize.height() == 0)
        flags |= QRhiTexture::OneDimensional;

    return d->createTexture(format, pixelSize, 1, arraySize, sampleCount, flags);
}

/*!
    \return a new sampler with the specified magnification filter \a magFilter,
    minification filter \a minFilter, mipmapping mode \a mipmapMode, and the
    addressing (wrap) modes \a addressU, \a addressV, and \a addressW.

    \note Setting \a mipmapMode to a value other than \c None implies that
    images for all relevant mip levels will be provided either via
    \l{QRhiResourceUpdateBatch::uploadTexture()}{texture uploads} or by calling
    \l{QRhiResourceUpdateBatch::generateMips()}{generateMips()} on the texture
    that is used with this sampler. Attempting to use the sampler with a
    texture that has no data for all relevant mip levels will lead to rendering
    errors, with the exact behavior dependent on the underlying graphics API.

    \sa QRhiResource::destroy()
 */
QRhiSampler *QRhi::newSampler(QRhiSampler::Filter magFilter,
                              QRhiSampler::Filter minFilter,
                              QRhiSampler::Filter mipmapMode,
                              QRhiSampler::AddressMode addressU,
                              QRhiSampler::AddressMode addressV,
                              QRhiSampler::AddressMode addressW)
{
    return d->createSampler(magFilter, minFilter, mipmapMode, addressU, addressV, addressW);
}

/*!
    \return a new shading rate map object.

    \since 6.9
 */
QRhiShadingRateMap *QRhi::newShadingRateMap()
{
    return d->createShadingRateMap();
}

/*!
    \return a new texture render target with color and depth/stencil
    attachments given in \a desc, and with the specified \a flags.

    \sa QRhiResource::destroy()
 */

QRhiTextureRenderTarget *QRhi::newTextureRenderTarget(const QRhiTextureRenderTargetDescription &desc,
                                                      QRhiTextureRenderTarget::Flags flags)
{
    return d->createTextureRenderTarget(desc, flags);
}

/*!
    \return a new swapchain.

    \sa QRhiResource::destroy(), QRhiSwapChain::createOrResize()
 */
QRhiSwapChain *QRhi::newSwapChain()
{
    return d->createSwapChain();
}

/*!
    Starts a new frame targeting the next available buffer of \a swapChain.

    A frame consists of resource updates and one or more render and compute
    passes.

    \a flags can indicate certain special cases.

    The high level pattern of rendering into a QWindow using a swapchain:

    \list

    \li Create a swapchain.

    \li Call QRhiSwapChain::createOrResize() whenever the surface size is
    different than before.

    \li Call QRhiSwapChain::destroy() on
    QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed.

    \li Then on every frame:
    \badcode
       beginFrame(sc);
       updates = nextResourceUpdateBatch();
       updates->...
       QRhiCommandBuffer *cb = sc->currentFrameCommandBuffer();
       cb->beginPass(sc->currentFrameRenderTarget(), colorClear, dsClear, updates);
       ...
       cb->endPass();
       ... // more passes as necessary
       endFrame(sc);
    \endcode

    \endlist

    \return QRhi::FrameOpSuccess on success, or another QRhi::FrameOpResult
    value on failure. Some of these should be treated as soft, "try again
    later" type of errors: When QRhi::FrameOpSwapChainOutOfDate is returned,
    the swapchain is to be resized or updated by calling
    QRhiSwapChain::createOrResize(). The application should then attempt to
    generate a new frame. QRhi::FrameOpDeviceLost means the graphics device is
    lost but this may also be recoverable by releasing all resources, including
    the QRhi itself, and then recreating all resources. See isDeviceLost() for
    further discussion.

    \sa endFrame(), beginOffscreenFrame(), isDeviceLost()
 */
QRhi::FrameOpResult QRhi::beginFrame(QRhiSwapChain *swapChain, BeginFrameFlags flags)
{
    if (d->inFrame)
        qWarning("Attempted to call beginFrame() within a still active frame; ignored");

    qCDebug(QRHI_LOG_RUB) << "[rub] new frame";

    QRhi::FrameOpResult r = !d->inFrame ? d->beginFrame(swapChain, flags) : FrameOpSuccess;
    if (r == FrameOpSuccess)
        d->inFrame = true;

    return r;
}

/*!
    Ends, commits, and presents a frame that was started in the last
    beginFrame() on \a swapChain.

    Double (or triple) buffering is managed internally by the QRhiSwapChain and
    QRhi.

    \a flags can optionally be used to change the behavior in certain ways.
    Passing QRhi::SkipPresent skips queuing the Present command or calling
    swapBuffers.

    \return QRhi::FrameOpSuccess on success, or another QRhi::FrameOpResult
    value on failure. Some of these should be treated as soft, "try again
    later" type of errors: When QRhi::FrameOpSwapChainOutOfDate is returned,
    the swapchain is to be resized or updated by calling
    QRhiSwapChain::createOrResize(). The application should then attempt to
    generate a new frame. QRhi::FrameOpDeviceLost means the graphics device is
    lost but this may also be recoverable by releasing all resources, including
    the QRhi itself, and then recreating all resources. See isDeviceLost() for
    further discussion.

    \sa beginFrame(), isDeviceLost()
 */
QRhi::FrameOpResult QRhi::endFrame(QRhiSwapChain *swapChain, EndFrameFlags flags)
{
    if (!d->inFrame)
        qWarning("Attempted to call endFrame() without an active frame; ignored");

    QRhi::FrameOpResult r = d->inFrame ? d->endFrame(swapChain, flags) : FrameOpSuccess;
    d->inFrame = false;
    // deleteLater is a high level QRhi concept the backends know
    // nothing about - handle it here.
    qDeleteAll(d->pendingDeleteResources);
    d->pendingDeleteResources.clear();

    return r;
}

/*!
    \return true when there is an active frame, meaning there was a
    beginFrame() (or beginOffscreenFrame()) with no corresponding endFrame()
    (or endOffscreenFrame()) yet.

    \sa currentFrameSlot(), beginFrame(), endFrame()
 */
bool QRhi::isRecordingFrame() const
{
    return d->inFrame;
}

/*!
    \return the current frame slot index while recording a frame. Unspecified
    when called outside an active frame (that is, when isRecordingFrame() is \c
    false).

    With backends like Vulkan or Metal, it is the responsibility of the QRhi
    backend to block whenever starting a new frame and finding the CPU is
    already \c{FramesInFlight - 1} frames ahead of the GPU (because the command
    buffer submitted in frame no. \c{current} - \c{FramesInFlight} has not yet
    completed).

    Resources that tend to change between frames (such as, the native buffer
    object backing a QRhiBuffer with type QRhiBuffer::Dynamic) exist in
    multiple versions, so that each frame, that can be submitted while a
    previous one is still being processed, works with its own copy, thus
    avoiding the need to stall the pipeline when preparing the frame. (The
    contents of a resource that may still be in use in the GPU should not be
    touched, but simply always waiting for the previous frame to finish would
    reduce GPU utilization and ultimately, performance and efficiency.)

    Conceptually this is somewhat similar to copy-on-write schemes used by some
    C++ containers and other types. It may also be similar to what an OpenGL or
    Direct 3D 11 implementation performs internally for certain type of objects.

    In practice, such double (or triple) buffering resources is realized in
    the Vulkan, Metal, and similar QRhi backends by having a fixed number of
    native resource (such as, VkBuffer) \c slots behind a QRhiResource. That
    can then be indexed by a frame slot index running 0, 1, ..,
    FramesInFlight-1, and then wrapping around.

    All this is managed transparently to the users of QRhi. However,
    applications that integrate rendering done directly with the graphics API
    may want to perform a similar double or triple buffering of their own
    graphics resources. That is then most easily achieved by knowing the values
    of the maximum number of in-flight frames (retrievable via resourceLimit())
    and the current frame (slot) index (returned by this function).

    \sa isRecordingFrame(), beginFrame(), endFrame()
 */
int QRhi::currentFrameSlot() const
{
    return d->currentFrameSlot;
}

/*!
    Starts a new offscreen frame. Provides a command buffer suitable for
    recording rendering commands in \a cb. \a flags is used to indicate
    certain special cases, just like with beginFrame().

    \note The QRhiCommandBuffer stored to *cb is not owned by the caller.

    Rendering without a swapchain is possible as well. The typical use case is
    to use it in completely offscreen applications, e.g. to generate image
    sequences by rendering and reading back without ever showing a window.

    Usage in on-screen applications (so beginFrame, endFrame,
    beginOffscreenFrame, endOffscreenFrame, beginFrame, ...) is possible too
    but it does reduce parallelism so it should be done only infrequently.

    Offscreen frames do not let the CPU potentially generate another frame
    while the GPU is still processing the previous one. This has the side
    effect that if readbacks are scheduled, the results are guaranteed to be
    available once endOffscreenFrame() returns. That is not the case with
    frames targeting a swapchain: there the GPU is potentially better utilized,
    but working with readback operations needs more care from the application
    because endFrame(), unlike endOffscreenFrame(), does not guarantee that the
    results from the readback are available at that point.

    The skeleton of rendering a frame without a swapchain and then reading the
    frame contents back could look like the following:

    \code
        QRhiReadbackResult rbResult;
        QRhiCommandBuffer *cb;
        rhi->beginOffscreenFrame(&cb);
        cb->beginPass(rt, colorClear, dsClear);
        // ...
        u = nextResourceUpdateBatch();
        u->readBackTexture(rb, &rbResult);
        cb->endPass(u);
        rhi->endOffscreenFrame();
        // image data available in rbResult
   \endcode

   \sa endOffscreenFrame(), beginFrame()
 */
QRhi::FrameOpResult QRhi::beginOffscreenFrame(QRhiCommandBuffer **cb, BeginFrameFlags flags)
{
    if (d->inFrame)
        qWarning("Attempted to call beginOffscreenFrame() within a still active frame; ignored");

    qCDebug(QRHI_LOG_RUB) << "[rub] new offscreen frame";

    QRhi::FrameOpResult r = !d->inFrame ? d->beginOffscreenFrame(cb, flags) : FrameOpSuccess;
    if (r == FrameOpSuccess)
        d->inFrame = true;

    return r;
}

/*!
    Ends, submits, and waits for the offscreen frame.

    \a flags is not currently used.

    \sa beginOffscreenFrame()
 */
QRhi::FrameOpResult QRhi::endOffscreenFrame(EndFrameFlags flags)
{
    if (!d->inFrame)
        qWarning("Attempted to call endOffscreenFrame() without an active frame; ignored");

    QRhi::FrameOpResult r = d->inFrame ? d->endOffscreenFrame(flags) : FrameOpSuccess;
    d->inFrame = false;
    qDeleteAll(d->pendingDeleteResources);
    d->pendingDeleteResources.clear();

    return r;
}

/*!
    Waits for any work on the graphics queue (where applicable) to complete,
    then executes all deferred operations, like completing readbacks and
    resource releases. Can be called inside and outside of a frame, but not
    inside a pass. Inside a frame it implies submitting any work on the
    command buffer.

    \note Avoid this function. One case where it may be needed is when the
    results of an enqueued readback in a swapchain-based frame are needed at a
    fixed given point and so waiting for the results is desired.
 */
QRhi::FrameOpResult QRhi::finish()
{
    return d->finish();
}

/*!
    \return the list of supported sample counts.

    A typical example would be (1, 2, 4, 8).

    With some backend this list of supported values is fixed in advance, while
    with some others the (physical) device properties indicate what is
    supported at run time.

    \sa QRhiRenderBuffer::setSampleCount(), QRhiTexture::setSampleCount(),
    QRhiGraphicsPipeline::setSampleCount(), QRhiSwapChain::setSampleCount()
 */
QList<int> QRhi::supportedSampleCounts() const
{
    return d->supportedSampleCounts();
}

/*!
    \return the minimum uniform buffer offset alignment in bytes. This is
    typically 256.

    Attempting to bind a uniform buffer region with an offset not aligned to
    this value will lead to failures depending on the backend and the
    underlying graphics API.

    \sa ubufAligned()
 */
int QRhi::ubufAlignment() const
{
    return d->ubufAlignment();
}

/*!
    \return The list of supported variable shading rates for the specified \a sampleCount.

    1x1 is always supported.

    \since 6.9
 */
QList<QSize> QRhi::supportedShadingRates(int sampleCount) const
{
    return d->supportedShadingRates(sampleCount);
}

Q_CONSTINIT static QBasicAtomicInteger<QRhiGlobalObjectIdGenerator::Type> counter = Q_BASIC_ATOMIC_INITIALIZER(0);

QRhiGlobalObjectIdGenerator::Type QRhiGlobalObjectIdGenerator::newId()
{
    return counter.fetchAndAddRelaxed(1) + 1;
}

bool QRhiPassResourceTracker::isEmpty() const
{
    return m_buffers.isEmpty() && m_textures.isEmpty();
}

void QRhiPassResourceTracker::reset()
{
    m_buffers.clear();
    m_textures.clear();
}

static inline QRhiPassResourceTracker::BufferStage earlierStage(QRhiPassResourceTracker::BufferStage a,
                                                                QRhiPassResourceTracker::BufferStage b)
{
    return QRhiPassResourceTracker::BufferStage(qMin(int(a), int(b)));
}

void QRhiPassResourceTracker::registerBuffer(QRhiBuffer *buf, int slot, BufferAccess *access, BufferStage *stage,
                                             const UsageState &state)
{
    auto it = m_buffers.find(buf);
    if (it != m_buffers.end()) {
        if (it->access != *access) {
            const QByteArray name = buf->name();
            qWarning("Buffer %p (%s) used with different accesses within the same pass, this is not allowed.",
                     buf, name.constData());
            return;
        }
        if (it->stage != *stage) {
            it->stage = earlierStage(it->stage, *stage);
            *stage = it->stage;
        }
        return;
    }

    Buffer b;
    b.slot = slot;
    b.access = *access;
    b.stage = *stage;
    b.stateAtPassBegin = state; // first use -> initial state
    m_buffers.insert(buf, b);
}

static inline QRhiPassResourceTracker::TextureStage earlierStage(QRhiPassResourceTracker::TextureStage a,
                                                                 QRhiPassResourceTracker::TextureStage b)
{
    return QRhiPassResourceTracker::TextureStage(qMin(int(a), int(b)));
}

static inline bool isImageLoadStore(QRhiPassResourceTracker::TextureAccess access)
{
    return access == QRhiPassResourceTracker::TexStorageLoad
            || access == QRhiPassResourceTracker::TexStorageStore
            || access == QRhiPassResourceTracker::TexStorageLoadStore;
}

void QRhiPassResourceTracker::registerTexture(QRhiTexture *tex, TextureAccess *access, TextureStage *stage,
                                              const UsageState &state)
{
    auto it = m_textures.find(tex);
    if (it != m_textures.end()) {
        if (it->access != *access) {
            // Different subresources of a texture may be used for both load
            // and store in the same pass. (think reading from one mip level
            // and writing to another one in a compute shader) This we can
            // handle by treating the entire resource as read-write.
            if (isImageLoadStore(it->access) && isImageLoadStore(*access)) {
                it->access = QRhiPassResourceTracker::TexStorageLoadStore;
                *access = it->access;
            } else {
                const QByteArray name = tex->name();
                qWarning("Texture %p (%s) used with different accesses within the same pass, this is not allowed.",
                         tex, name.constData());
            }
        }
        if (it->stage != *stage) {
            it->stage = earlierStage(it->stage, *stage);
            *stage = it->stage;
        }
        return;
    }

    Texture t;
    t.access = *access;
    t.stage = *stage;
    t.stateAtPassBegin = state; // first use -> initial state
    m_textures.insert(tex, t);
}

QRhiPassResourceTracker::BufferStage QRhiPassResourceTracker::toPassTrackerBufferStage(QRhiShaderResourceBinding::StageFlags stages)
{
    // pick the earlier stage (as this is going to be dstAccessMask)
    if (stages.testFlag(QRhiShaderResourceBinding::VertexStage))
        return QRhiPassResourceTracker::BufVertexStage;
    if (stages.testFlag(QRhiShaderResourceBinding::TessellationControlStage))
        return QRhiPassResourceTracker::BufTCStage;
    if (stages.testFlag(QRhiShaderResourceBinding::TessellationEvaluationStage))
        return QRhiPassResourceTracker::BufTEStage;
    if (stages.testFlag(QRhiShaderResourceBinding::FragmentStage))
        return QRhiPassResourceTracker::BufFragmentStage;
    if (stages.testFlag(QRhiShaderResourceBinding::ComputeStage))
        return QRhiPassResourceTracker::BufComputeStage;
    if (stages.testFlag(QRhiShaderResourceBinding::GeometryStage))
        return QRhiPassResourceTracker::BufGeometryStage;

    Q_UNREACHABLE_RETURN(QRhiPassResourceTracker::BufVertexStage);
}

QRhiPassResourceTracker::TextureStage QRhiPassResourceTracker::toPassTrackerTextureStage(QRhiShaderResourceBinding::StageFlags stages)
{
    // pick the earlier stage (as this is going to be dstAccessMask)
    if (stages.testFlag(QRhiShaderResourceBinding::VertexStage))
        return QRhiPassResourceTracker::TexVertexStage;
    if (stages.testFlag(QRhiShaderResourceBinding::TessellationControlStage))
        return QRhiPassResourceTracker::TexTCStage;
    if (stages.testFlag(QRhiShaderResourceBinding::TessellationEvaluationStage))
        return QRhiPassResourceTracker::TexTEStage;
    if (stages.testFlag(QRhiShaderResourceBinding::FragmentStage))
        return QRhiPassResourceTracker::TexFragmentStage;
    if (stages.testFlag(QRhiShaderResourceBinding::ComputeStage))
        return QRhiPassResourceTracker::TexComputeStage;
    if (stages.testFlag(QRhiShaderResourceBinding::GeometryStage))
        return QRhiPassResourceTracker::TexGeometryStage;

    Q_UNREACHABLE_RETURN(QRhiPassResourceTracker::TexVertexStage);
}

QT_END_NAMESPACE
