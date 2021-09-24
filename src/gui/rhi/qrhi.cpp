/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Gui module
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qrhi_p_p.h"
#include <qmath.h>
#include <QLoggingCategory>

#include "qrhinull_p_p.h"
#ifndef QT_NO_OPENGL
#include "qrhigles2_p_p.h"
#endif
#if QT_CONFIG(vulkan)
#include "qrhivulkan_p_p.h"
#endif
#ifdef Q_OS_WIN
#include "qrhid3d11_p_p.h"
#endif
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
#include "qrhimetal_p_p.h"
#endif

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QRHI_LOG_INFO, "qt.rhi.general")

/*!
    \class QRhi
    \internal
    \inmodule QtGui

    \brief Accelerated 2D/3D graphics API abstraction.

    The Qt Rendering Hardware Interface is an abstraction for hardware accelerated
    graphics APIs, such as, \l{https://www.khronos.org/opengl/}{OpenGL},
    \l{https://www.khronos.org/opengles/}{OpenGL ES},
    \l{https://docs.microsoft.com/en-us/windows/desktop/direct3d}{Direct3D},
    \l{https://developer.apple.com/metal/}{Metal}, and
    \l{https://www.khronos.org/vulkan/}{Vulkan}.

    Some of the main design goals are:

    \list

    \li Simple, minimal, understandable, extensible. Follow the proven path of the
    Qt Quick scenegraph.

    \li Aim to be a product - and in the bigger picture, part of a product (Qt) -
    that is usable out of the box both by internal (such as, Qt Quick) and,
    eventually, external users.

    \li Not a complete 1:1 wrapper for any of the underlying APIs. The feature set
    is tuned towards the needs of Qt's 2D and 3D offering (QPainter, Qt Quick, Qt
    3D Studio). Iterate and evolve in a sustainable manner.

    \li Intrinsically cross-platform, without reinventing: abstracting
    cross-platform aspects of certain APIs (such as, OpenGL context creation and
    windowing system interfaces, Vulkan instance and surface management) is not in
    scope here. These are delegated to the existing QtGui facilities (QWindow,
    QOpenGLContext, QVulkanInstance) and its backing QPA architecture.

    \endlist

    Each QRhi instance is backed by a backend for a specific graphics API. The
    selection of the backend is a run time choice and is up to the application
    or library that creates the QRhi instance. Some backends are available on
    multiple platforms (OpenGL, Vulkan, Null), while APIs specific to a given
    platform are only available when running on the platform in question (Metal
    on macOS/iOS/tvOS, Direct3D on Windows).

    The available backends currently are:

    \list

    \li OpenGL 2.1 or OpenGL ES 2.0 or newer. Some extensions are utilized when
    present, for example to enable multisample framebuffers.

    \li Direct3D 11.1

    \li Metal

    \li Vulkan 1.0, optionally with some extensions that are part of Vulkan 1.1

    \li Null - A "dummy" backend that issues no graphics calls at all.

    \endlist

    In order to allow shader code to be written once in Qt applications and
    libraries, all shaders are expected to be written in a single language
    which is then compiled into SPIR-V. Versions for various shading language
    are then generated from that, together with reflection information (inputs,
    outputs, shader resources). This is then packed into easily and efficiently
    serializable QShader instances. The compilers and tools to generate such
    shaders are not part of QRhi, but the core classes for using such shaders,
    QShader and QShaderDescription, are.

    \section2 Design Fundamentals

    A QRhi cannot be instantiated directly. Instead, use the create()
    function. Delete the QRhi instance normally to release the graphics device.

    \section3 Resources

    Instances of classes deriving from QRhiResource, such as, QRhiBuffer,
    QRhiTexture, etc., encapsulate zero, one, or more native graphics
    resources. Instances of such classes are always created via the \c new
    functions of the QRhi, such as, newBuffer(), newTexture(),
    newTextureRenderTarget(), newSwapChain().

    \badcode
        vbuf = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData));
        if (!vbuf->create()) { error }
        ...
        delete vbuf;
    \endcode

    \list

    \li The returned value from functions like newBuffer() is always owned by
    the caller.

    \li Just creating a QRhiResource subclass never allocates or initializes any
    native resources. That is only done when calling the \c create() function of a
    subclass, for example, QRhiBuffer::create() or QRhiTexture::create().

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
    \l{QRhiCommandBuffer::beginFrame()}{beginFrame()} -
    \l{QRhiCommandBuffer::endFrame()}{endFrame()} section). As a general rule,
    all referenced QRhiResource objects must stay unchanged until the frame is
    submitted by calling \l{QRhiCommandBuffer::endFrame()}{endFrame()}. To ease
    this, QRhiResource::deleteLater() is provided as a convenience.

    \endlist

    \section3 Command buffers and deferred command execution

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
    \l{QRhiCommandBuffer::endFrame()}{endFrame()}. On the other hand, calling
    \l{QRhiResource::destroy()}{destroy()} or deleting the QRhiResource are
    always safe once the frame is submitted, regardless of the status of the
    underlying native resources (which may still be in use by the GPU - but
    that is taken care of internally).

    Unlike APIs like OpenGL, upload and copy type of commands cannot be mixed
    with draw commands. The typical renderer will involve a sequence similar to
    the following: \c{(re)create resources} - \c{begin frame} - \c{record
    uploads and copies} - \c{start renderpass} - \c{record draw calls} - \c{end
    renderpass} - \c{end frame}. Recording copy type of operations happens via
    QRhiResourceUpdateBatch. Such operations are committed typically on
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

    \section3 Threading

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

    \section3 Resource synchronization

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

    \section3 Resource reuse

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

    \badcode
        ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 256);
        ubuf->create();

        srb = rhi->newShaderResourceBindings()
        srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, ubuf)
        });
        srb->create();

        ...

        // now in a later frame we need to grow the buffer to a larger size
        ubuf->setSize(512);
        ubuf->create(); // same as ubuf->destroy(); ubuf->create();

        // that's it, srb needs no changes whatsoever
    \endcode

    \section3 Pooled objects

    In addition to resources, there are pooled objects as well, such as,
    QRhiResourceUpdateBatch. An instance is retrieved via a \c next function,
    such as, nextResourceUpdateBatch(). The caller does not own the returned
    instance in this case. The only valid way of operating here is calling
    functions on the QRhiResourceUpdateBatch and then passing it to
    QRhiCommandBuffer::beginPass() or QRhiCommandBuffer::endPass(). These
    functions take care of returning the batch to the pool. Alternatively, a
    batch can be "canceled" and returned to the pool without processing by
    calling QRhiResourceUpdateBatch::destroy().

    A typical pattern is thus:

    \badcode
        QRhiResourceUpdateBatch *resUpdates = rhi->nextResourceUpdateBatch();
        ...
        resUpdates->updateDynamicBuffer(ubuf, 0, 64, mvp.constData());
        if (!image.isNull()) {
            resUpdates->uploadTexture(texture, image);
            image = QImage();
        }
        ...
        QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
        cb->beginPass(swapchain->currentFrameRenderTarget(), clearCol, clearDs, resUpdates);
    \endcode

    \section3 Swapchain specifics

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

    \section3 Ownership

    The general rule is no ownership transfer. Creating a QRhi with an already
    existing graphics device does not mean the QRhi takes ownership of the
    device object. Similarly, ownership is not given away when a device or
    texture object is "exported" via QRhi::nativeHandles() or
    QRhiTexture::nativeHandles(). Most importantly, passing pointers in structs
    and via setters does not transfer ownership.

    \section2 Troubleshooting

    Errors are printed to the output via qWarning(). Additional debug messages
    can be enabled via the following logging categories. Messages from these
    categories are not printed by default unless explicitly enabled via
    QRhi::EnableProfiling or the facilities of QLoggingCategory (such as, the
    \c QT_LOGGING_RULES environment variable).

    \list
    \li \c{qt.rhi.general}
    \endlist

    It is strongly advised to inspect the output with the logging categories
    (\c{qt.rhi.*}) enabled whenever a QRhi-based application is not behaving as
    expected.
 */

/*!
    \enum QRhi::Implementation
    Describes which graphics API-specific backend gets used by a QRhi instance.

    \value Null
    \value Vulkan
    \value OpenGLES2
    \value D3D11
    \value Metal
 */

/*!
    \enum QRhi::Flag
    Describes what special features to enable.

    \value EnableProfiling Enables gathering timing (CPU, GPU) and resource
    (QRhiBuffer, QRhiTexture, etc.) information and additional metadata. See
    QRhiProfiler. Avoid enabling in production builds as it may involve a
    performance penalty. Also enables debug messages from the \c{qt.rhi.*}
    logging categories.

    \value EnableDebugMarkers Enables debug marker groups. Without this frame
    debugging features like making debug groups and custom resource name
    visible in external GPU debugging tools will not be available and functions
    like QRhiCommandBuffer::debugMarkBegin() will become a no-op. Avoid
    enabling in production builds as it may involve a performance penalty.

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
    an empty blob always. Opting in is relevant in particular with backends
    where additional, potentially time consuming work is needed to maintain the
    data structures with the serialized, binary versions of shader programs. An
    example is OpenGL, where the "pipeline cache" is simulated by retrieving
    and loading shader program binaries. With backends where retrieving and
    restoring the pipeline cache contents is not supported, the flag has no
    effect. With some backends (such as, OpenGL) there are additional,
    disk-based caching mechanisms for shader binaries. Writing to those may get
    disabled whenever this flag is set since storing program binaries (OpenGL)
    to multiple caches is not sensible.
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
    Relevant for QRhiProfiler::gpuFrameTimes().

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
    backends (D3D11, Metal). With Vulkan, the value depends on the
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
    will be unsupported with OpenGL, and with Metal on older iOS devices,
    including the iOS Simulator.

    \value TriangleFanTopology Indicates that QRhiGraphicsPipeline::setTopology()
    supports QRhiGraphicsPipeline::TriangleFan. In practice this feature will be
    unsupported with Metal and Direct 3D 11.

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

    \value MaxUniformBufferRange The number of bytes that can be exposed from a
    uniform buffer to the shaders at once. On OpenGL ES 2.0 and 3.0
    implementations this may be as low as 3584 bytes (224 four component, 32
    bits per component vectors). Elsewhere the value is typically 16384 (1024
    vec4s) or 65536 (4096 vec4s).
 */

/*!
    \class QRhiInitParams
    \internal
    \inmodule QtGui
    \brief Base class for backend-specific initialization parameters.

    Contains fields that are relevant to all backends.
 */

/*!
    \class QRhiDepthStencilClearValue
    \internal
    \inmodule QtGui
    \brief Specifies clear values for a depth or stencil buffer.
 */

/*!
    \fn QRhiDepthStencilClearValue::QRhiDepthStencilClearValue()

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
    \return \c true if the values in the two QRhiDepthStencilClearValue objects
    \a a and \a b are equal.

    \relates QRhiDepthStencilClearValue
 */
bool operator==(const QRhiDepthStencilClearValue &a, const QRhiDepthStencilClearValue &b) noexcept
{
    return a.depthClearValue() == b.depthClearValue()
            && a.stencilClearValue() == b.stencilClearValue();
}

/*!
    \return \c false if the values in the two QRhiDepthStencilClearValue
    objects \a a and \a b are equal; otherwise returns \c true.

    \relates QRhiDepthStencilClearValue
*/
bool operator!=(const QRhiDepthStencilClearValue &a, const QRhiDepthStencilClearValue &b) noexcept
{
    return !(a == b);
}

/*!
    \return the hash value for \a v, using \a seed to seed the calculation.

    \relates QRhiDepthStencilClearValue
 */
size_t qHash(const QRhiDepthStencilClearValue &v, size_t seed) noexcept
{
    return seed * (uint(qFloor(qreal(v.depthClearValue()) * 100)) + v.stencilClearValue());
}

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
    \internal
    \inmodule QtGui
    \brief Specifies a viewport rectangle.

    Used with QRhiCommandBuffer::setViewport().

    QRhi assumes OpenGL-style viewport coordinates, meaning x and y are
    bottom-left. Negative width or height are not allowed.

    Typical usage is like the following:

    \badcode
      const QSize outputSizeInPixels = swapchain->currentPixelSize();
      const QRhiViewport viewport(0, 0, outputSizeInPixels.width(), outputSizeInPixels.height());
      cb->beginPass(swapchain->currentFrameRenderTarget(), { 0, 0, 0, 1 }, { 1, 0 });
      cb->setGraphicsPipeline(ps);
      cb->setViewport(viewport);
      ...
    \endcode

    \sa QRhiCommandBuffer::setViewport(), QRhi::clipSpaceCorrMatrix(), QRhiScissor
 */

/*!
    \fn QRhiViewport::QRhiViewport()

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
    \return \c true if the values in the two QRhiViewport objects
    \a a and \a b are equal.

    \relates QRhiViewport
 */
bool operator==(const QRhiViewport &a, const QRhiViewport &b) noexcept
{
    return a.viewport() == b.viewport()
            && a.minDepth() == b.minDepth()
            && a.maxDepth() == b.maxDepth();
}

/*!
    \return \c false if the values in the two QRhiViewport
    objects \a a and \a b are equal; otherwise returns \c true.

    \relates QRhiViewport
*/
bool operator!=(const QRhiViewport &a, const QRhiViewport &b) noexcept
{
    return !(a == b);
}

/*!
    \return the hash value for \a v, using \a seed to seed the calculation.

    \relates QRhiViewport
 */
size_t qHash(const QRhiViewport &v, size_t seed) noexcept
{
    const std::array<float, 4> r = v.viewport();
    return seed + uint(r[0]) + uint(r[1]) + uint(r[2]) + uint(r[3])
            + uint(qFloor(qreal(v.minDepth()) * 100)) + uint(qFloor(qreal(v.maxDepth()) * 100));
}

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
    \internal
    \inmodule QtGui
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

    \sa QRhiCommandBuffer::setScissor(), QRhiViewport
 */

/*!
    \fn QRhiScissor::QRhiScissor()

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
    \return \c true if the values in the two QRhiScissor objects
    \a a and \a b are equal.

    \relates QRhiScissor
 */
bool operator==(const QRhiScissor &a, const QRhiScissor &b) noexcept
{
    return a.scissor() == b.scissor();
}

/*!
    \return \c false if the values in the two QRhiScissor
    objects \a a and \a b are equal; otherwise returns \c true.

    \relates QRhiScissor
*/
bool operator!=(const QRhiScissor &a, const QRhiScissor &b) noexcept
{
    return !(a == b);
}

/*!
    \return the hash value for \a v, using \a seed to seed the calculation.

    \relates QRhiScissor
 */
size_t qHash(const QRhiScissor &v, size_t seed) noexcept
{
    const std::array<int, 4> r = v.scissor();
    return seed + uint(r[0]) + uint(r[1]) + uint(r[2]) + uint(r[3]);
}

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
    \internal
    \inmodule QtGui
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

    \badcode
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

    \badcode
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

    \sa QRhiCommandBuffer::setVertexInput()
 */

/*!
    \enum QRhiVertexInputBinding::Classification
    Describes the input data classification.

    \value PerVertex Data is per-vertex
    \value PerInstance Data is per-instance
 */

/*!
    \fn QRhiVertexInputBinding::QRhiVertexInputBinding()

    Constructs a default vertex input binding description.
 */

/*!
    Constructs a vertex input binding description with the specified \a stride,
    classification \a cls, and instance step rate \a stepRate.

    \note \a stepRate other than 1 is only supported when
    QRhi::CustomInstanceStepRate is reported to be supported.
 */
QRhiVertexInputBinding::QRhiVertexInputBinding(quint32 stride, Classification cls, int stepRate)
    : m_stride(stride),
      m_classification(cls),
      m_instanceStepRate(stepRate)
{
}

/*!
    \return \c true if the values in the two QRhiVertexInputBinding objects
    \a a and \a b are equal.

    \relates QRhiVertexInputBinding
 */
bool operator==(const QRhiVertexInputBinding &a, const QRhiVertexInputBinding &b) noexcept
{
    return a.stride() == b.stride()
            && a.classification() == b.classification()
            && a.instanceStepRate() == b.instanceStepRate();
}

/*!
    \return \c false if the values in the two QRhiVertexInputBinding
    objects \a a and \a b are equal; otherwise returns \c true.

    \relates QRhiVertexInputBinding
*/
bool operator!=(const QRhiVertexInputBinding &a, const QRhiVertexInputBinding &b) noexcept
{
    return !(a == b);
}

/*!
    \return the hash value for \a v, using \a seed to seed the calculation.

    \relates QRhiVertexInputBinding
 */
size_t qHash(const QRhiVertexInputBinding &v, size_t seed) noexcept
{
    return seed + v.stride() + v.classification();
}

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
    \internal
    \inmodule QtGui
    \brief Describes a single vertex input element.

    The members specify the binding number, location, format, and offset for a
    single vertex input element.

    \note For HLSL it is assumed that the vertex shader uses
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

    \badcode
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

    \badcode
        const QRhiCommandBuffer::VertexInput vbufBindings[] = {
            { cubeBuf, 0 },
            { cubeBuf, 36 * 3 * sizeof(float) }
        };
        cb->setVertexInput(0, 2, vbufBindings);
    \endcode

    When working with interleaved data, there will typically be just one
    binding, with multiple attributes referring to that same buffer binding
    point:

    \badcode
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

    \badcode
        const QRhiCommandBuffer::VertexInput vbufBinding(interleavedCubeBuf, 0);
        cb->setVertexInput(0, 1, &vbufBinding);
    \endcode

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
 */

/*!
    \fn QRhiVertexInputAttribute::QRhiVertexInputAttribute()

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
    \return \c true if the values in the two QRhiVertexInputAttribute objects
    \a a and \a b are equal.

    \relates QRhiVertexInputAttribute
 */
bool operator==(const QRhiVertexInputAttribute &a, const QRhiVertexInputAttribute &b) noexcept
{
    return a.binding() == b.binding()
            && a.location() == b.location()
            && a.format() == b.format()
            && a.offset() == b.offset();
}

/*!
    \return \c false if the values in the two QRhiVertexInputAttribute
    objects \a a and \a b are equal; otherwise returns \c true.

    \relates QRhiVertexInputAttribute
*/
bool operator!=(const QRhiVertexInputAttribute &a, const QRhiVertexInputAttribute &b) noexcept
{
    return !(a == b);
}

/*!
    \return the hash value for \a v, using \a seed to seed the calculation.

    \relates QRhiVertexInputAttribute
 */
size_t qHash(const QRhiVertexInputAttribute &v, size_t seed) noexcept
{
    return seed + uint(v.binding()) + uint(v.location()) + uint(v.format()) + v.offset();
}

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

/*!
    \class QRhiVertexInputLayout
    \internal
    \inmodule QtGui
    \brief Describes the layout of vertex inputs consumed by a vertex shader.

    The vertex input layout is defined by the collections of
    QRhiVertexInputBinding and QRhiVertexInputAttribute.
 */

/*!
    \fn QRhiVertexInputLayout::QRhiVertexInputLayout()

    Constructs an empty vertex input layout description.
 */

/*!
    \return \c true if the values in the two QRhiVertexInputLayout objects
    \a a and \a b are equal.

    \relates QRhiVertexInputLayout
 */
bool operator==(const QRhiVertexInputLayout &a, const QRhiVertexInputLayout &b) noexcept
{
    return a.m_bindings == b.m_bindings && a.m_attributes == b.m_attributes;
}

/*!
    \return \c false if the values in the two QRhiVertexInputLayout
    objects \a a and \a b are equal; otherwise returns \c true.

    \relates QRhiVertexInputLayout
*/
bool operator!=(const QRhiVertexInputLayout &a, const QRhiVertexInputLayout &b) noexcept
{
    return !(a == b);
}

/*!
    \return the hash value for \a v, using \a seed to seed the calculation.

    \relates QRhiVertexInputLayout
 */
size_t qHash(const QRhiVertexInputLayout &v, size_t seed) noexcept
{
    return qHash(v.m_bindings, seed) + qHash(v.m_attributes, seed);
}

#ifndef QT_NO_DEBUG_STREAM
template<typename T, qsizetype N>
QDebug operator<<(QDebug dbg, const QVarLengthArray<T, N> &vla)
{
    return QtPrivate::printSequentialContainer(dbg, "VLA", vla);
}

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
    \internal
    \inmodule QtGui
    \brief Specifies the type and the shader code for a shader stage in the pipeline.
 */

/*!
    \enum QRhiShaderStage::Type
    Specifies the type of the shader stage.

    \value Vertex Vertex stage
    \value Fragment Fragment (pixel) stage
    \value Compute Compute stage (this may not always be supported at run time)
 */

/*!
    \fn QRhiShaderStage::QRhiShaderStage()

    Constructs a shader stage description for the vertex stage with an empty
    QShader.
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
    \return \c true if the values in the two QRhiShaderStage objects
    \a a and \a b are equal.

    \relates QRhiShaderStage
 */
bool operator==(const QRhiShaderStage &a, const QRhiShaderStage &b) noexcept
{
    return a.type() == b.type()
            && a.shader() == b.shader()
            && a.shaderVariant() == b.shaderVariant();
}

/*!
    \return \c false if the values in the two QRhiShaderStage
    objects \a a and \a b are equal; otherwise returns \c true.

    \relates QRhiShaderStage
*/
bool operator!=(const QRhiShaderStage &a, const QRhiShaderStage &b) noexcept
{
    return !(a == b);
}

/*!
    \return the hash value for \a v, using \a seed to seed the calculation.

    \relates QRhiShaderStage
 */
size_t qHash(const QRhiShaderStage &v, size_t seed) noexcept
{
    return v.type() + qHash(v.shader(), seed) + v.shaderVariant();
}

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
    \internal
    \inmodule QtGui
    \brief Describes the a single color attachment of a render target.

    A color attachment is either a QRhiTexture or a QRhiRenderBuffer. The
    former, when texture() is set, is used in most cases.

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
    to render to.

    When texture() or renderBuffer() is multisample, resolveTexture() can be
    set optionally. When set, samples are resolved automatically into that
    (non-multisample) texture at the end of the render pass. When rendering
    into a multisample renderbuffers, this is the only way to get resolved,
    non-multisample content out of them. Multisample textures allow sampling in
    shaders so for them this is just one option.

    \note when resolving is enabled, the multisample data may not be written
    out at all. This means that the multisample texture() must not be used
    afterwards with shaders for sampling when resolveTexture() is set.
 */

/*!
    \fn QRhiColorAttachment::QRhiColorAttachment()

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
    \class QRhiTextureRenderTargetDescription
    \internal
    \inmodule QtGui
    \brief Describes the color and depth or depth/stencil attachments of a render target.

    A texture render target has zero or more textures as color attachments,
    zero or one renderbuffer as combined depth/stencil buffer or zero or one
    texture as depth buffer.

    \note depthStencilBuffer() and depthTexture() cannot be both set (cannot be
    non-null at the same time).
 */

/*!
    \fn QRhiTextureRenderTargetDescription::QRhiTextureRenderTargetDescription()

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
    \class QRhiTextureSubresourceUploadDescription
    \internal
    \inmodule QtGui
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
 */

/*!
    \fn QRhiTextureSubresourceUploadDescription::QRhiTextureSubresourceUploadDescription()

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
QRhiTextureSubresourceUploadDescription::QRhiTextureSubresourceUploadDescription(const void *data, int size)
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
    \class QRhiTextureUploadEntry
    \internal
    \inmodule QtGui
    \brief Describes one layer (face for cubemaps) in a texture upload operation.
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
    \class QRhiTextureUploadDescription
    \internal
    \inmodule QtGui
    \brief Describes a texture upload operation.

    Used with QRhiResourceUpdateBatch::uploadTexture(). That function has two
    variants: one taking a QImage and one taking a
    QRhiTextureUploadDescription. The former is a convenience version,
    internally creating a QRhiTextureUploadDescription with a single image
    targeting level 0 for layer 0. However, when cubemaps, pre-generated mip
    images, or compressed textures are involved, applications will have to work
    directly with this class instead.

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

    \badcode
        QImage faces[6];
        ...
        QList<QRhiTextureUploadEntry> entries;
        for (int i = 0; i < 6; ++i)
          entries.append(QRhiTextureUploadEntry(i, 0, faces[i]));
        QRhiTextureUploadDescription desc(entries);
        resourceUpdates->uploadTexture(texture, desc);
    \endcode

    Another example that specifies mip images for a compressed texture:

    \badcode
        QRhiTextureUploadDescription desc;
        const int mipCount = rhi->mipLevelsForSize(compressedTexture->pixelSize());
        for (int level = 0; level < mipCount; ++level) {
            const QByteArray compressedDataForLevel = ..
            desc.append(QRhiTextureUploadEntry(0, level, compressedDataForLevel));
        }
        resourceUpdates->uploadTexture(compressedTexture, desc);
    \endcode

    With partial uploads targeting the same subresource, it is recommended to
    batch them into a single upload request, whenever possible:

    \badcode
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
    \class QRhiTextureCopyDescription
    \internal
    \inmodule QtGui
    \brief Describes a texture-to-texture copy operation.

    An empty pixelSize() indicates that the entire subresource is to be copied.
    A default constructed copy description therefore leads to copying the
    entire subresource at level 0 of layer 0.

    \note The source texture must be created with
    QRhiTexture::UsedAsTransferSource.

    \note The source and destination rectangles defined by pixelSize(),
    sourceTopLeft(), and destinationTopLeft() must fit the source and
    destination textures, respectively. The behavior is undefined otherwise.

    With cubemap and 3D textures one face or slice can be copied at a time. The
    face or slice is specified by the source and destination layer indices.
    With mipmapped textures one mip level can be copied at a time. The source
    and destination layer and mip level indices can differ, but the size and
    position must be carefully controlled to avoid out of bounds copies, in
    which case the behavior is undefined.
 */

/*!
    \fn QRhiTextureCopyDescription::QRhiTextureCopyDescription()

    Constructs an empty texture copy description.
 */

/*!
    \class QRhiReadbackDescription
    \internal
    \inmodule QtGui
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
 */

/*!
    \fn QRhiReadbackDescription::QRhiReadbackDescription()

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
    \class QRhiReadbackResult
    \internal
    \inmodule QtGui
    \brief Describes the results of a potentially asynchronous readback operation.

    When \l completed is set, the function is invoked when the \l data is
    available. \l format and \l pixelSize are set upon completion together with
    \l data.
 */

/*!
    \class QRhiNativeHandles
    \internal
    \inmodule QtGui
    \brief Base class for classes exposing backend-specific collections of native resource objects.
 */

/*!
    \class QRhiResource
    \internal
    \inmodule QtGui
    \brief Base class for classes encapsulating native resource objects.
 */

/*!
    \fn QRhiResource::Type QRhiResource::resourceType() const

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
    \fn void QRhiResource::destroy()

    Releases (or requests deferred releasing of) the underlying native graphics
    resources. Safe to call multiple times, subsequent invocations will be a
    no-op then.

    Once destroy() is called, the QRhiResource instance can be reused, by
    calling \c create() again. That will then result in creating new native
    graphics resources underneath.

    \note Resources referenced by commands for the current frame should not be
    released until the frame is submitted by QRhi::endFrame().

    The QRhiResource destructor also performs the same task, so calling this
    function is not necessary before destroying a QRhiResource.

    \sa deleteLater()
 */

/*!
    When called without a frame being recorded, this function is equivalent to
    deleting the object. Between a QRhi::beginFrame() and QRhi::endFrame()
    however the behavior is different: the QRhiResource will not be destroyed
    until the frame is submitted via QRhi::endFrame(), thus satisfying the QRhi
    requirement of not altering QRhiResource objects that are referenced by the
    frame being recorded.

    \sa destroy()
 */
void QRhiResource::deleteLater()
{
    m_rhi->addDeleteLater(this);
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

    This has two uses: to get descriptive names for the native graphics
    resources visible in graphics debugging tools, such as
    \l{https://renderdoc.org/}{RenderDoc} and
    \l{https://developer.apple.com/xcode/}{XCode}, and in the output stream of
    QRhiProfiler.

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
    m_objectName.replace(',', '_'); // cannot contain comma for QRhiProfiler
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
    \class QRhiBuffer
    \internal
    \inmodule QtGui
    \brief Vertex, index, or uniform (constant) buffer resource.
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
    \l{setVertexInput()}{QRhiCommandBuffer::setVertexInput()}.

    \value IndexBuffer Index buffer. This allows the QRhiBuffer to be used in
    \l{setVertexInput()}{QRhiCommandBuffer::setVertexInput()}.

    \value UniformBuffer Uniform buffer (also called constant buffer). This
    allows the QRhiBuffer to be used in combination with
    \l{UniformBuffer}{QRhiShaderResourceBinding::UniformBuffer}. When
    \l{QRhi::NonDynamicUniformBuffers}{NonDynamicUniformBuffers} is reported as
    not supported, this usage can only be combined with the type Dynamic.

    \value StorageBuffer Storage buffer. This allows the QRhiBuffer to be used
    in combination with \l{BufferLoad}{QRhiShaderResourceBinding::BufferLoad},
    \l{BufferStore}{QRhiShaderResourceBinding::BufferStore}, or
    \l{BufferLoadStore}{QRhiShaderResourceBinding::BufferLoadStore}. This usage
    can only be combined with the types Immutable or Static, and is only
    available when the \l{QRhi::Compute}{Compute feature} is reported as
    supported.
 */

/*!
    \fn void QRhiBuffer::setSize(int sz)

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
    \class QRhiBuffer::NativeBuffer
    \brief Contains information about the underlying native resources of a buffer.
 */

/*!
    \variable QRhiBuffer::NativeBuffer::objects
    \brief an array with pointers to the native object handles.

    With OpenGL, the native handle is a GLuint value, so the elements in the \c
    objects array are pointers to a GLuint. With Vulkan, the native handle is a
    VkBuffer, so the elements of the array are pointers to a VkBuffer. With
    Direct3D 11 and Metal the elements are pointers to a ID3D11Buffer or
    MTLBuffer pointer, respectively.

    \note Pay attention to the fact that the elements are always pointers to
    the native buffer handle type, even if the native type itself is a pointer.
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
QRhiBuffer::QRhiBuffer(QRhiImplementation *rhi, Type type_, UsageFlags usage_, int size_)
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
    \fn bool QRhiBuffer::create()

    Creates the corresponding native graphics resources. If there are already
    resources present due to an earlier create() with no corresponding
    destroy(), then destroy() is called implicitly first.

    \return \c true when successful, \c false when a graphics operation failed.
    Regardless of the return value, calling destroy() is always safe.
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
    return {};
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
    \class QRhiRenderBuffer
    \internal
    \inmodule QtGui
    \brief Renderbuffer resource.

    Renderbuffers cannot be sampled or read but have some benefits over
    textures in some cases:

    A DepthStencil renderbuffer may be lazily allocated and be backed by
    transient memory with some APIs. On some platforms this may mean the
    depth/stencil buffer uses no physical backing at all.

    Color renderbuffers are useful since QRhi::MultisampleRenderBuffer may be
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
 */

/*!
    \enum QRhiRenderBuffer::Type
    Specifies the type of the renderbuffer

    \value DepthStencil Combined depth/stencil
    \value Color Color
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
    \fn bool QRhiRenderBuffer::create()

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
    \fn QRhiTexture::Format QRhiRenderBuffer::backingFormat() const

    \internal
 */

/*!
    \class QRhiTexture
    \internal
    \inmodule QtGui
    \brief Texture resource.
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
     multisample. When rendering into a 3D texture, the layer specified in the
     render target's color attachment refers to a slice in range [0..depth-1].
     The underlying graphics API may not support 3D textures at run time.
     Support is indicated by the QRhi::ThreeDimensionalTextures feature.

     \value TextureRectangleGL The texture should use the GL_TEXTURE_RECTANGLE
     target with OpenGL. This flag is ignored with other graphics APIs. Just
     like ExternalOES, this flag is useful when working with platform APIs where
     native OpenGL texture objects received from the platform are wrapped in a
     QRhiTexture, and the platform can only provide textures for a non-2D
     texture target.
 */

/*!
    \enum QRhiTexture::Format

    Specifies the texture format. See also QRhi::isTextureFormatSupported() and
    note that flags() can modify the format when QRhiTexture::sRGB is set.

    \value UnknownFormat Not a valid format. This cannot be passed to setFormat().

    \value RGBA8 Four component, unsigned normalized 8 bit per component. Always supported.

    \value BGRA8 Four component, unsigned normalized 8 bit per component.

    \value R8 One component, unsigned normalized 8 bit.

    \value RG8 Two components, unsigned normalized 8 bit.

    \value R16 One component, unsigned normalized 16 bit.

    \value RG16 Two component, unsigned normalized 16 bit.

    \value RED_OR_ALPHA8 Either same as R8, or is a similar format with the component swizzled to alpha,
    depending on \l{QRhi::RedOrAlpha8IsRed}{RedOrAlpha8IsRed}.

    \value RGBA16F Four components, 16-bit float per component.

    \value RGBA32F Four components, 32-bit float per component.

    \value D16 16-bit depth (normalized unsigned integer)

    \value D24 24-bit depth (normalized unsigned integer)

    \value D24S8 24-bit depth (normalized unsigned integer), 8 bit stencil

    \value D32F 32-bit depth (32-bit float)

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
 */

/*!
    \class QRhiTexture::NativeTexture
    \brief Contains information about the underlying native resources of a texture.
 */

/*!
    \variable QRhiTexture::NativeTexture::object
    \brief 64-bit integer containing the native object handle.

    With OpenGL, the native handle is a GLuint value, so \c object can then be
    cast to a GLuint. With Vulkan, the native handle is a VkImage, so \c
    object can be cast to a VkImage. With Direct3D 11 and Metal \c
    object contains a ID3D11Texture2D or MTLTexture pointer, respectively.
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
                         int sampleCount_, Flags flags_)
    : QRhiResource(rhi),
      m_format(format_), m_pixelSize(pixelSize_), m_depth(depth_), m_sampleCount(sampleCount_), m_flags(flags_)
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
    \fn bool QRhiTexture::create()

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
    Similar to create() except that no new native textures are created. Instead,
    the native texture resources specified by \a src is used.

    This allows importing an existing native texture object (which must belong
    to the same device or sharing context, depending on the graphics API) from
    an external graphics engine.

    \note format(), pixelSize(), sampleCount(), and flags() must still be set
    correctly. Passing incorrect sizes and other values to QRhi::newTexture()
    and then following it with a createFrom() expecting that the native texture
    object alone is sufficient to deduce such values is \b wrong and will lead
    to problems.

    \note QRhiTexture does not take ownership of the texture object. destroy()
    does not free the object or any associated memory.

    The opposite of this operation, exposing a QRhiTexture-created native
    texture object to a foreign engine, is possible via nativeTexture().

*/
bool QRhiTexture::createFrom(QRhiTexture::NativeTexture src)
{
    Q_UNUSED(src);
    return false;
}

/*!
    With some graphics APIs, such as Vulkan, integrating custom rendering code
    that uses the graphics API directly needs special care when it comes to
    image layouts. This function allows communicating the expected layout the
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
 */
void QRhiTexture::setNativeLayout(int layout)
{
    Q_UNUSED(layout);
}

/*!
    \class QRhiSampler
    \internal
    \inmodule QtGui
    \brief Sampler resource.
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
    \class QRhiRenderPassDescriptor
    \internal
    \inmodule QtGui
    \brief Render pass resource.

    A render pass, if such a concept exists in the underlying graphics API, is
    a collection of attachments (color, depth, stencil) and describes how those
    attachments are used.
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
    \fn bool QRhiRenderPassDescriptor::isCompatible(const QRhiRenderPassDescriptor *other) const

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
    \fn QRhiRenderPassDescriptor *QRhiRenderPassDescriptor::newCompatibleRenderPassDescriptor() const

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
    \fn QVector<quint32> QRhiRenderPassDescriptor::serializedFormat() const

    \return a vector of integers containing an opaque blob describing the data
    relevant for \l{isCompatible()}{compatibility}.

    Given two QRhiRenderPassDescriptor objects \c rp1 and \c rp2, if the data
    returned from this function is identical, then \c{rp1->isCompatible(rp2)},
    and vice versa hold true as well.

    \note The returned data is meant to be used for storing in memory and
    comparisons during the lifetime of the QRhi the object belongs to. It is not
    meant for storing on disk, reusing between processes, or using with multiple
    QRhi instances with potentially different backends.

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
    \internal
    \inmodule QtGui
    \brief Represents an onscreen (swapchain) or offscreen (texture) render target.
 */

/*!
    \internal
 */
QRhiRenderTarget::QRhiRenderTarget(QRhiImplementation *rhi)
    : QRhiResource(rhi)
{
}

/*!
    \return the resource type.
 */
QRhiResource::Type QRhiRenderTarget::resourceType() const
{
    return RenderTarget;
}

/*!
    \fn QSize QRhiRenderTarget::pixelSize() const

    \return the size in pixels.
 */

/*!
    \fn float QRhiRenderTarget::devicePixelRatio() const

    \return the device pixel ratio. For QRhiTextureRenderTarget this is always
    1. For targets retrieved from a QRhiSwapChain the value reflects the
    \l{QWindow::devicePixelRatio()}{device pixel ratio} of the targeted
    QWindow.
 */

/*!
    \class QRhiTextureRenderTarget
    \internal
    \inmodule QtGui
    \brief Texture render target resource.

    A texture render target allows rendering into one or more textures,
    optionally with a depth texture or depth/stencil renderbuffer.

    \note Textures used in combination with QRhiTextureRenderTarget must be
    created with the QRhiTexture::RenderTarget flag.

    The simplest example of creating a render target with a texture as its
    single color attachment:

    \badcode
        texture = rhi->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::RenderTarget);
        texture->create();
        rt = rhi->newTextureRenderTarget({ texture });
        rp = rt->newCompatibleRenderPassDescriptor();
        rt->setRenderPassDescriptor(rt);
        rt->create();
        // rt can now be used with beginPass()
    \endcode
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
    GPUs, but allows preserving the existing contents between passes.

    \value PreserveDepthStencilContents Indicates that the contents of the
    depth texture is to be loaded when starting a render pass, instead
    clearing. Only applicable when a texture is used as the depth buffer
    (QRhiTextureRenderTargetDescription::depthTexture() is set) because
    depth/stencil renderbuffers may not have any physical backing and data may
    not be written out in the first place.
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
    \fn QRhiRenderPassDescriptor *QRhiTextureRenderTarget::newCompatibleRenderPassDescriptor()

    \return a new QRhiRenderPassDescriptor that is compatible with this render
    target.

    The returned value is used in two ways: it can be passed to
    setRenderPassDescriptor() and
    QRhiGraphicsPipeline::setRenderPassDescriptor(). A render pass descriptor
    describes the attachments (color, depth/stencil) and the load/store
    behavior that can be affected by flags(). A QRhiGraphicsPipeline can only
    be used in combination with a render target that has the same
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
    \fn bool QRhiTextureRenderTarget::create()

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
    \class QRhiShaderResourceBindings
    \internal
    \inmodule QtGui
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

    \badcode
        srb = rhi->newShaderResourceBindings();
        srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, ubuf),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture, sampler)
        });
        srb->create();
        ...
        ps = rhi->newGraphicsPipeline();
        ...
        ps->setShaderResourceBindings(srb);
        ps->create();
        ...
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
    them in the first place.

    \badcode
        srb2 = rhi->newShaderResourceBindings();
        ...
        cb->setGraphicsPipeline(ps);
        cb->setShaderResources(srb2); // binds srb2
    \endcode
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
    function is identical, then \c{srb1->isLayoutCompatible(srb2), and vice
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
    for (const QRhiShaderResourceBinding &b : qAsConst(srb->m_bindings)) {
        const QRhiShaderResourceBinding::Data *d = b.data();
        srb->m_layoutDescHash ^= uint(d->binding) ^ uint(d->stage) ^ uint(d->type)
            ^ uint(d->type == QRhiShaderResourceBinding::SampledTexture ? d->u.stex.count : 1);
        layoutDescAppender = d->serialize(layoutDescAppender);
    }
}

/*!
    \class QRhiShaderResourceBinding
    \internal
    \inmodule QtGui
    \brief Describes the shader resource for a single binding point.

    A QRhiShaderResourceBinding cannot be constructed directly. Instead, use
    the static functions uniformBuffer(), sampledTexture() to get an instance.
 */

/*!
    \enum QRhiShaderResourceBinding::Type
    Specifies type of the shader resource bound to a binding point

    \value UniformBuffer Uniform buffer

    \value SampledTexture Combined image sampler

    \value ImageLoad Image load (with GLSL this maps to doing imageLoad() on a
    single level - and either one or all layers - of a texture exposed to the
    shader as an image object)

    \value ImageStore Image store (with GLSL this maps to doing imageStore() or
    imageAtomic*() on a single level - and either one or all layers - of a
    texture exposed to the shader as an image object)

    \value ImageLoadStore Image load and store

    \value BufferLoad Storage buffer store (with GLSL this maps to reading from
    a shader storage buffer)

    \value BufferStore Storage buffer store (with GLSL this maps to writing to
    a shader storage buffer)

    \value BufferLoadStore Storage buffer load and store
 */

/*!
    \enum QRhiShaderResourceBinding::StageFlag
    Flag values to indicate which stages the shader resource is visible in

    \value VertexStage Vertex stage
    \value FragmentStage Fragment (pixel) stage
    \value ComputeStage Compute stage
 */

/*!
    \return \c true if the layout is compatible with \a other. The layout does not
    include the actual resource (such as, buffer or texture) and related
    parameters (such as, offset or size).

    For example, \c a and \c b below are not equal, but are compatible layout-wise:

    \badcode
        auto a = QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, buffer);
        auto b = QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, someOtherBuffer, 256);
    \endcode
 */
bool QRhiShaderResourceBinding::isLayoutCompatible(const QRhiShaderResourceBinding &other) const
{
    // i.e. everything that goes into a VkDescriptorSetLayoutBinding must match
    const int thisCount = d.type == QRhiShaderResourceBinding::SampledTexture ? d.u.stex.count : 1;
    const int otherCount = other.d.type == QRhiShaderResourceBinding::SampledTexture ? other.d.u.stex.count : 1;
    return d.binding == other.d.binding && d.stage == other.d.stage && d.type == other.d.type && thisCount == otherCount;
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
        int binding, StageFlags stage, QRhiBuffer *buf, int offset, int size)
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
        int binding, StageFlags stage, QRhiBuffer *buf, int size)
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
    b.d.u.stex.texSamplers[0].tex = tex;
    b.d.u.stex.texSamplers[0].sampler = sampler;
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
            b.d.u.stex.texSamplers[i] = {};
    }
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
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::bufferLoad(
        int binding, StageFlags stage, QRhiBuffer *buf, int offset, int size)
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
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::bufferStore(
        int binding, StageFlags stage, QRhiBuffer *buf, int offset, int size)
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
 */
QRhiShaderResourceBinding QRhiShaderResourceBinding::bufferLoadStore(
        int binding, StageFlags stage, QRhiBuffer *buf, int offset, int size)
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
    const QRhiShaderResourceBinding::Data *da = a.data();
    const QRhiShaderResourceBinding::Data *db = b.data();

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
    case QRhiShaderResourceBinding::ImageLoad:
        Q_FALLTHROUGH();
    case QRhiShaderResourceBinding::ImageStore:
        Q_FALLTHROUGH();
    case QRhiShaderResourceBinding::ImageLoadStore:
        if (da->u.simage.tex != db->u.simage.tex
                || da->u.simage.level != db->u.simage.level)
        {
            return false;
        }
        break;
    case QRhiShaderResourceBinding::BufferLoad:
        Q_FALLTHROUGH();
    case QRhiShaderResourceBinding::BufferStore:
        Q_FALLTHROUGH();
    case QRhiShaderResourceBinding::BufferLoadStore:
        if (da->u.sbuf.buf != db->u.sbuf.buf
                || da->u.sbuf.offset != db->u.sbuf.offset
                || da->u.sbuf.maybeSize != db->u.sbuf.maybeSize)
        {
            return false;
        }
        break;
    default:
        Q_UNREACHABLE();
        return false;
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
    \return the hash value for \a b, using \a seed to seed the calculation.

    \relates QRhiShaderResourceBinding
 */
size_t qHash(const QRhiShaderResourceBinding &b, size_t seed) noexcept
{
    const QRhiShaderResourceBinding::Data *d = b.data();
    size_t h = uint(d->binding) ^ uint(d->stage) ^ uint(d->type) ^ seed;
    switch (d->type) {
    case QRhiShaderResourceBinding::UniformBuffer:
        h ^= qHash(reinterpret_cast<quintptr>(d->u.ubuf.buf));
        break;
    case QRhiShaderResourceBinding::SampledTexture:
        h ^= qHash(reinterpret_cast<quintptr>(d->u.stex.texSamplers[0].tex));
        h ^= qHash(reinterpret_cast<quintptr>(d->u.stex.texSamplers[0].sampler));
        break;
    case QRhiShaderResourceBinding::ImageLoad:
        Q_FALLTHROUGH();
    case QRhiShaderResourceBinding::ImageStore:
        Q_FALLTHROUGH();
    case QRhiShaderResourceBinding::ImageLoadStore:
        h ^= qHash(reinterpret_cast<quintptr>(d->u.simage.tex));
        break;
    case QRhiShaderResourceBinding::BufferLoad:
        Q_FALLTHROUGH();
    case QRhiShaderResourceBinding::BufferStore:
        Q_FALLTHROUGH();
    case QRhiShaderResourceBinding::BufferLoadStore:
        h ^= qHash(reinterpret_cast<quintptr>(d->u.sbuf.buf));
        break;
    default:
        break;
    }
    return h;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QRhiShaderResourceBinding &b)
{
    QDebugStateSaver saver(dbg);
    const QRhiShaderResourceBinding::Data *d = b.data();
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
    \internal
    \inmodule QtGui
    \brief Graphics pipeline state resource.

    \note Setting the shader stages is mandatory. There must be at least one
    stage, and there must be a vertex stage.

    \note Setting the shader resource bindings is mandatory. The referenced
    QRhiShaderResourceBindings must already have create() called on it by the
    time create() is called. Associating with a QRhiShaderResourceBindings that
    has no bindings is also valid, as long as no shader in any stage expects
    any resources.

    \note Setting the render pass descriptor is mandatory. To obtain a
    QRhiRenderPassDescriptor that can be passed to setRenderPassDescriptor(),
    use either QRhiTextureRenderTarget::newCompatibleRenderPassDescriptor() or
    QRhiSwapChain::newCompatibleRenderPassDescriptor().

    \note Setting the vertex input layout is mandatory.

    \note sampleCount() defaults to 1 and must match the sample count of the
    render target's color and depth stencil attachments.

    \note The depth test, depth write, and stencil test are disabled by
    default.

    \note stencilReadMask() and stencilWriteMask() apply to both faces. They
    both default to 0xFF.
 */

/*!
    \fn void QRhiGraphicsPipeline::setTargetBlends(const QList<TargetBlend> &blends)

    Sets the blend specification for color attachments. Each element in \a
    blends corresponds to a color attachment of the render target.

    By default no blends are set, which is a shortcut to disabling blending and
    enabling color write for all four channels.
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
    \class QRhiGraphicsPipeline::TargetBlend
    \internal
    \inmodule QtGui
    \brief Describes the blend state for one color attachment.

    Defaults to color write enabled, blending disabled. The blend values are
    set up for pre-multiplied alpha (One, OneMinusSrcAlpha, One,
    OneMinusSrcAlpha) by default.
 */

/*!
    \class QRhiGraphicsPipeline::StencilOpState
    \internal
    \inmodule QtGui
    \brief Describes the stencil operation state.
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
    \fn bool QRhiGraphicsPipeline::create()

    Creates the corresponding native graphics resources. If there are already
    resources present due to an earlier create() with no corresponding
    destroy(), then destroy() is called implicitly first.

    \return \c true when successful, \c false when a graphics operation failed.
    Regardless of the return value, calling destroy() is always safe.
 */

/*!
    \fn void QRhiGraphicsPipeline::setDepthTest(bool enable)

    Enables or disables depth testing. Both depth test and the writing out of
    depth data are disabled by default.

    \sa setDepthWrite()
 */

/*!
    \fn void QRhiGraphicsPipeline::setDepthWrite(bool enable)

    Controls the writing out of depth data into the depth buffer. By default
    this is disabled. Depth write is typically enabled together with the depth
    test.

    \note Enabling depth write without having depth testing enabled may not
    lead to the desired result, and should be avoided.

    \sa setDepthTest()
 */

/*!
    \class QRhiSwapChain
    \internal
    \inmodule QtGui
    \brief Swapchain resource.

    A swapchain enables presenting rendering results to a surface. A swapchain
    is typically backed by a set of color buffers. Of these, one is displayed
    at a time.

    Below is a typical pattern for creating and managing a swapchain and some
    associated resources in order to render onto a QWindow:

    \badcode
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

    \badcode
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

    \badcode
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

    \value sRGB Requests to pick an sRGB format for the swapchain and/or its
    render target views, where applicable. Note that this implies that sRGB
    framebuffer update and blending will get enabled for all content targeting
    this swapchain, and opting out is not possible. For OpenGL, set
    \l{QSurfaceFormat::sRGBColorSpace}{sRGBColorSpace} on the QSurfaceFormat of
    the QWindow in addition.

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
    \fn QSize QRhiSwapChain::surfacePixelSize()

    \return The size of the window's associated surface or layer.

    \warning Do not assume this is the same as \c{QWindow::size() *
    QWindow::devicePixelRatio()}. With some graphics APIs and windowing system
    interfaces (for example, Vulkan) there is a theoretical possibility for a
    surface to assume a size different from the associated window. To support
    these cases, rendering logic must always base size-derived calculations
    (such as, viewports) on the size reported from QRhiSwapChain, and never on
    the size queried from QWindow.

    \note Can also be called before createOrResize(), if at least window() is
    already set) This in combination with currentPixelSize() allows to detect
    when a swapchain needs to be resized. However, watch out for the fact that
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
    \fn QRhiCommandBuffer *QRhiSwapChain::currentFrameCommandBuffer()

    \return a command buffer on which rendering commands can be recorded. Only
    valid within a QRhi::beginFrame() - QRhi::endFrame() block where
    beginFrame() was called with this swapchain.

    \note the value must not be cached and reused between frames
*/

/*!
    \fn QRhiRenderTarget *QRhiSwapChain::currentFrameRenderTarget()

    \return a render target that can used with beginPass() in order to render
    the swapchain's current backbuffer. Only valid within a
    QRhi::beginFrame() - QRhi::endFrame() block where beginFrame() was called
    with this swapchain.

    \note the value must not be cached and reused between frames
 */

/*!
    \fn bool QRhiSwapChain::createOrResize()

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
    \class QRhiComputePipeline
    \internal
    \inmodule QtGui
    \brief Compute pipeline state resource.

    \note Setting the shader resource bindings is mandatory. The referenced
    QRhiShaderResourceBindings must already have created() called on it by the
    time create() is called.

    \note Setting the shader is mandatory.
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
    \class QRhiCommandBuffer
    \internal
    \inmodule QtGui
    \brief Command buffer resource.

    Not creatable by applications at the moment. The only ways to obtain a
    valid QRhiCommandBuffer are to get it from the targeted swapchain via
    QRhiSwapChain::currentFrameCommandBuffer(), or, in case of rendering
    completely offscreen, initializing one via QRhi::beginOffscreenFrame().
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

    Synonym for QPair<int, quint32>. The first entry is the binding, the second
    is the offset in the buffer.
*/

/*!
    \typedef QRhiCommandBuffer::VertexInput

    Synonym for QPair<QRhiBuffer *, quint32>. The second entry is an offset in
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

static const char *resourceTypeStr(QRhiResource *res)
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
    case QRhiResource::RenderTarget:
        return "RenderTarget";
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
    default:
        Q_UNREACHABLE();
        break;
    }
    return "";
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
                     q, int(resources.count()));
        }
        for (QRhiResource *res : qAsConst(resources)) {
            if (leakCheck)
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

    case QRhiTexture::D16:
        bpc = 2;
        break;
    case QRhiTexture::D24:
    case QRhiTexture::D24S8:
    case QRhiTexture::D32F:
        bpc = 4;
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

// Approximate because it excludes subresource alignment or multisampling.
quint32 QRhiImplementation::approxByteSizeForTexture(QRhiTexture::Format format, const QSize &baseSize, int depth,
                                                     int mipCount, int layerCount)
{
    quint32 approxSize = 0;
    for (int level = 0; level < mipCount; ++level) {
        quint32 byteSize = 0;
        const QSize size(qFloor(qreal(qMax(1, baseSize.width() >> level))),
                         qFloor(qreal(qMax(1, baseSize.height() >> level))));
        textureFormatInfo(format, size, nullptr, &byteSize, nullptr);
        approxSize += byteSize;
    }
    approxSize *= depth; // 3D texture depth or 1 otherwise
    approxSize *= uint(layerCount); // 6 for cubemaps or 1 otherwise
    return approxSize;
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
        const int binding = it->data()->binding;
        if (binding >= CHECKED_BINDINGS_COUNT)
            continue;
        if (binding < 0) {
            qWarning("Invalid binding number %d", binding);
            bindingsOk = false;
            continue;
        }
        switch (it->data()->type) {
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
        case QRhiShaderResourceBinding::ImageLoad:
            Q_FALLTHROUGH();
        case QRhiShaderResourceBinding::ImageStore:
            Q_FALLTHROUGH();
        case QRhiShaderResourceBinding::ImageLoadStore:
            if (!bindingSeen[binding]) {
                bindingSeen[binding] = true;
            } else {
                qWarning("Image duplicates an existing binding number %d", binding);
                bindingsOk = false;
            }
            break;
        case QRhiShaderResourceBinding::BufferLoad:
            Q_FALLTHROUGH();
        case QRhiShaderResourceBinding::BufferStore:
            Q_FALLTHROUGH();
        case QRhiShaderResourceBinding::BufferLoadStore:
            if (!bindingSeen[binding]) {
                bindingSeen[binding] = true;
            } else {
                qWarning("Buffer duplicates an existing binding number %d", binding);
                bindingsOk = false;
            }
            break;
        default:
            qWarning("Unknown binding type %d", int(it->data()->type));
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

    qDeleteAll(d->pendingDeleteResources);
    d->pendingDeleteResources.clear();

    runCleanup();

    d->destroy();
    delete d;
}

/*!
    \return a new QRhi instance with a backend for the graphics API specified by \a impl.

    \a params must point to an instance of one of the backend-specific
    subclasses of QRhiInitParams, such as, QRhiVulkanInitParams,
    QRhiMetalInitParams, QRhiD3D11InitParams, QRhiGles2InitParams. See these
    classes for examples on creating a QRhi.

    \a flags is optional. It is used to enable profile and debug related
    features that are potentially expensive and should only be used during
    development.
 */
QRhi *QRhi::create(Implementation impl, QRhiInitParams *params, Flags flags, QRhiNativeHandles *importDevice)
{
    QScopedPointer<QRhi> r(new QRhi);

    switch (impl) {
    case Null:
        r->d = new QRhiNull(static_cast<QRhiNullInitParams *>(params));
        break;
    case Vulkan:
#if QT_CONFIG(vulkan)
        r->d = new QRhiVulkan(static_cast<QRhiVulkanInitParams *>(params),
                              static_cast<QRhiVulkanNativeHandles *>(importDevice));
        break;
#else
        Q_UNUSED(importDevice);
        qWarning("This build of Qt has no Vulkan support");
        break;
#endif
    case OpenGLES2:
#ifndef QT_NO_OPENGL
        r->d = new QRhiGles2(static_cast<QRhiGles2InitParams *>(params),
                             static_cast<QRhiGles2NativeHandles *>(importDevice));
        break;
#else
        qWarning("This build of Qt has no OpenGL support");
        break;
#endif
    case D3D11:
#ifdef Q_OS_WIN
        r->d = new QRhiD3D11(static_cast<QRhiD3D11InitParams *>(params),
                             static_cast<QRhiD3D11NativeHandles *>(importDevice));
        break;
#else
        qWarning("This platform has no Direct3D 11 support");
        break;
#endif
    case Metal:
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
        r->d = new QRhiMetal(static_cast<QRhiMetalInitParams *>(params),
                             static_cast<QRhiMetalNativeHandles *>(importDevice));
        break;
#else
        qWarning("This platform has no Metal support");
        break;
#endif
    default:
        break;
    }

    if (r->d) {
        r->d->q = r.data();

        if (flags.testFlag(EnableProfiling)) {
            QRhiProfilerPrivate *profD = QRhiProfilerPrivate::get(&r->d->profiler);
            profD->rhiDWhenEnabled = r->d;
            const_cast<QLoggingCategory &>(QRHI_LOG_INFO()).setEnabled(QtDebugMsg, true);
        }

        // Play nice with QSG_INFO since that is still the most commonly used
        // way to get graphics info printed from Qt Quick apps, and the Quick
        // scenegraph is our primary user.
        if (qEnvironmentVariableIsSet("QSG_INFO"))
            const_cast<QLoggingCategory &>(QRHI_LOG_INFO()).setEnabled(QtDebugMsg, true);

        r->d->debugMarkers = flags.testFlag(EnableDebugMarkers);

        if (r->d->create(flags)) {
            r->d->implType = impl;
            r->d->implThread = QThread::currentThread();
            return r.take();
        }
    }

    return nullptr;
}

/*!
    \return the backend type for this QRhi.
 */
QRhi::Implementation QRhi::backend() const
{
    return d->implType;
}

/*!
    \return the backend type as string for this QRhi.
 */
const char *QRhi::backendName() const
{
    switch (d->implType) {
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
    default:
        return "Unknown";
    }
}

/*!
    \enum QRhiDriverInfo::DeviceType
    Specifies the graphics device's type, when the information is available. In
    practice this is only applicable with Vulkan and Metal. With others the
    value will always be UnknownDevice.

    \value UnknownDevice
    \value IntegratedDevice
    \value DiscreteDevice
    \value ExternalDevice
    \value VirtualDevice
    \value CpuDevice
*/

/*!
    \struct QRhiDriverInfo
    \internal
    \inmodule QtGui
    \since 6.1

    \brief Describes the physical device, adapter, or graphics API
    implementation that is used by an initialized QRhi.

    Graphics APIs offer different levels and kinds of information. The only
    value that is available across all APIs is the deviceName, which is a
    freetext description of the physical device, adapter, or is a combination
    of the strings reported for \c{GL_VENDOR} + \c{GL_RENDERER} +
    \c{GL_VERSION}. The deviceId is always 0 for OpenGL. vendorId is always 0
    for OpenGL and Metal. deviceType is always UnknownDevice for OpenGL and
    Direct 3D.
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
    default:
        return "";
    }
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
    \return the thread on which the QRhi was \l{QRhi::create()}{initialized}.
 */
QThread *QRhi::thread() const
{
    return d->implThread;
}

/*!
    Registers a \a callback that is invoked either when the QRhi is destroyed,
    or when runCleanup() is called.

    The callback will run with the graphics resource still available, so this
    provides an opportunity for the application to cleanly release QRhiResource
    instances belonging to the QRhi. This is particularly useful for managing
    the lifetime of resources stored in \c cache type of objects, where the
    cache holds QRhiResources or objects containing QRhiResources.

    \sa runCleanup(), ~QRhi()
 */
void QRhi::addCleanupCallback(const CleanupCallback &callback)
{
    d->addCleanupCallback(callback);
}

/*!
    Invokes all registered cleanup functions. The list of cleanup callbacks it
    then cleared. Normally destroying the QRhi does this automatically, but
    sometimes it can be useful to trigger cleanup in order to release all
    cached, non-essential resources.

    \sa addCleanupCallback()
 */
void QRhi::runCleanup()
{
    for (const CleanupCallback &f : qAsConst(d->cleanupCallbacks))
        f(this);

    d->cleanupCallbacks.clear();
}

/*!
    \class QRhiResourceUpdateBatch
    \internal
    \inmodule QtGui
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

    \badcode
    void init()
    {
        ...
        initialUpdates = rhi->nextResourceUpdateBatch();
        initialUpdates->uploadStaticBuffer(vbuf, vertexData);
        initialUpdates->uploadStaticBuffer(ibuf, indexData);
        ...
    }

    void render()
    {
        ...
        QRhiResourceUpdateBatch *resUpdates = rhi->nextResourceUpdateBatch();
        if (initialUpdates) {
            resUpdates->merge(initialUpdates);
            initialUpdates->release();
            initialUpdates = nullptr;
        }
        resUpdates->updateDynamicBuffer(...);
        ...
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
    \a data can safely be destroyed or changed once this function returns.

    \note If host writes are involved, which is the case with
    updateDynamicBuffer() typically as such buffers are backed by host visible
    memory with most backends, they may accumulate within a frame. Thus pass 1
    reading a region changed by a batch passed to pass 2 may see the changes
    specified in pass 2's update batch.

    \note QRhi transparently manages double buffering in order to prevent
    stalling the graphics pipeline. The fact that a QRhiBuffer may have
    multiple native underneath can be safely ignored when using the QRhi and
    QRhiResourceUpdateBatch.
 */
void QRhiResourceUpdateBatch::updateDynamicBuffer(QRhiBuffer *buf, int offset, int size, const void *data)
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
    Enqueues updating a region of a QRhiBuffer \a buf created with the type
    QRhiBuffer::Immutable or QRhiBuffer::Static.

    The region is specified \a offset and \a size. The actual bytes to write
    are specified by \a data which must have at least \a size bytes available.
    \a data can safely be destroyed or changed once this function returns.
 */
void QRhiResourceUpdateBatch::uploadStaticBuffer(QRhiBuffer *buf, int offset, int size, const void *data)
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
    Enqueues reading back a region of the QRhiBuffer \a buf. The size of the
    region is specified by \a size in bytes, \a offset is the offset in bytes
    to start reading from.

    A readback is asynchronous. \a result contains a callback that is invoked
    when the operation has completed. The data is provided in
    QRhiBufferReadbackResult::data. Upon successful completion that QByteArray
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
void QRhiResourceUpdateBatch::readBackBuffer(QRhiBuffer *buf, int offset, int size, QRhiBufferReadbackResult *result)
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

   \badcode
      beginFrame(sc);
      beginPass
      ...
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
      u = nextResourceUpdateBatch();
      QRhiReadbackDescription rb; // no texture -> uses the current backbuffer of sc
      u->readBackTexture(rb, rbResult);
      endPass(u);
      endFrame(sc);
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
   or 3D slice) at a time. The level and layer are specified by the respective
   fields in \a rb.

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
   QRhiResourceUpdateBatch::destroy() on it.

   \note Can be called outside beginFrame() - endFrame() as well since a batch
   instance just collects data on its own, it does not perform any operations.

   \warning The maximum number of batches is 64. When this limit is reached,
   the function will return null until a batch is returned to the pool.
 */
QRhiResourceUpdateBatch *QRhi::nextResourceUpdateBatch()
{
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
        const int oldSize = d->resUpdPool.count();
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

    activeBufferOpCount = 0;
    activeTextureOpCount = 0;

    const quint64 mask = 1ULL << quint64(poolIndex);
    rhi->resUpdPoolMap &= ~mask;
    poolIndex = -1;

    textureOps.clear();
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
    return activeBufferOpCount < BUFFER_OPS_STATIC_ALLOC - 16
            && activeTextureOpCount < TEXTURE_OPS_STATIC_ALLOC - 16;
}

void QRhiResourceUpdateBatchPrivate::trimOpLists()
{
    Q_ASSERT(poolIndex == -1); // must not be in use

    activeBufferOpCount = 0;
    bufferOps.clear();

    activeTextureOpCount = 0;
    textureOps.clear();
}

/*!
    Sometimes committing resource updates is necessary without starting a
    render pass. Not often needed, updates should typically be passed to
    beginPass (or endPass, in case of readbacks) instead.

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

    \badcode
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
    vertex bindings can be specified simply like the following (using C++11
    initializer syntax), assuming vbuf is the QRhiBuffer with all the
    interleaved position+color data:

    \badcode
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
    Records a non-indexed draw.

    The number of vertices is specified in \a vertexCount. For instanced
    drawing set \a instanceCount to a value other than 1. \a firstVertex is the
    index of the first vertex to draw. When drawing multiple instances, the
    first instance ID is specified by \a firstInstance.

    \note \a firstInstance may not be supported, and is ignored when the
    QRhi::BaseInstance feature is reported as not supported. The first ID is
    always 0 in that case.

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

    For instanced drawing set \a instanceCount to a value other than 1. When
    drawing multiple instances, the first instance ID is specified by \a
    firstInstance.

    \note \a firstInstance may not be supported, and is ignored when the
    QRhi::BaseInstance feature is reported as not supported. The first ID is
    always 0 in that case.

    \a vertexOffset (also called \c{base vertex}) is a signed value that is
    added to the element index before indexing into the vertex buffer. Support
    for this is not always available, and the value is ignored when the feature
    QRhi::BaseVertex is reported as unsupported.

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
    Records a named debug group on the command buffer. This is shown in
    graphics debugging tools such as \l{https://renderdoc.org/}{RenderDoc} and
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

    With Vulkan or Metal one can query the native command buffer or encoder
    objects via nativeHandles() and enqueue commands to them. With OpenGL or
    Direct3D 11 the (device) context can be retrieved from
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
int QRhi::mipLevelsForSize(const QSize &size) const
{
    return qFloor(std::log2(qMax(size.width(), size.height()))) + 1;
}

/*!
    \return the texture image size for a given \a mipLevel, calculated based on
    the level 0 size given in \a baseLevelSize.
 */
QSize QRhi::sizeForMipLevel(int mipLevel, const QSize &baseLevelSize) const
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
    QRhiGles2NativeHandles, QRhiMetalNativeHandles as appropriate.

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
    \return the associated QRhiProfiler instance.

    An instance is always available for each QRhi, but it is not very useful
    without EnableProfiling because no data is collected without setting the
    flag upon creation.
  */
QRhiProfiler *QRhi::profiler()
{
    return &d->profiler;
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
    \return a binary \a data blob with data collected from the
    QRhiGraphicsPipeline and QRhiComputePipeline successfully created during
    the lifetime of this QRhi.

    By saving and then, in subsequent runs of the same application, reloading
    the cache data, pipeline and shader creation times can potentially be
    accelerated.

    When the PipelineCacheDataLoadSave is reported as unsupported, the returned
    QByteArray is empty.

    When the EnablePipelineCacheDataSave flag was not specified when calling
    create(), the returned QByteArray may be empty, even when the
    PipelineCacheDataLoadSave feature is supported.

    When the returned data is non-empty, it is always specific to the QRhi
    backend, the graphics device, and the driver implementation in use. QRhi
    takes care of adding the appropriate header and safeguards that ensure that
    the data can always be passed safely to setPipelineCacheData().

    \note Calling releaseCachedResources() may, depending on the backend, clear
    the pipeline data collected. A subsequent call to this function may then
    not return any data.

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

    The blob returned by pipelineCacheData() is always specific to a QRhi
    backend, a graphics device, and a given version of the graphics driver.
    QRhi takes care of adding the appropriate header and safeguards that ensure
    that the data can always be passed safely to this function. If there is a
    mismatch, e.g. because the driver has been upgraded to a newer version, or
    because the data was generated from a different QRhi backend, a warning is
    printed and \a data is safely ignored.

    With Vulkan, this maps directly to VkPipelineCache. Calling this function
    creates a new Vulkan pipeline cache object, with its initial data sourced
    from \a data. The pipeline cache object is then used by all subsequently
    created QRhiGraphicsPipeline and QRhiComputePipeline objects, thus
    accelerating, potentially, the pipeline creation.

    \note QRhi cannot give any guarantees that \a data has an effect on the
    pipeline and shader creation performance. With APIs like Vulkan, it is up
    to the driver to decide if \a data is used for some purpose, or if it is
    ignored.

    \sa pipelineCacheData(), isFeatureSupported()
 */
void QRhi::setPipelineCacheData(const QByteArray &data)
{
    d->setPipelineCacheData(data);
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
                            int size)
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
    \return a new texture with the specified \a format, \a pixelSize, \a
    sampleCount, and \a flags.

    \note \a format specifies the requested internal and external format,
    meaning the data to be uploaded to the texture will need to be in a
    compatible format, while the native texture may (but is not guaranteed to,
    in case of OpenGL at least) use this format internally.

    \sa QRhiResource::destroy()
 */
QRhiTexture *QRhi::newTexture(QRhiTexture::Format format,
                              const QSize &pixelSize,
                              int sampleCount,
                              QRhiTexture::Flags flags)
{
    return d->createTexture(format, pixelSize, 1, sampleCount, flags);
}

/*!
    \return a new texture with the specified \a format, \a width, \a height, \a
    depth, \a sampleCount, and \a flags.

    This overload is suitable for 3D textures because it allows specifying \a
    depth. A 3D texture must have QRhiTexture::ThreeDimensional set in \a
    flags, but using this overload that can be omitted because the flag is set
    implicitly whenever \a depth is greater than 0. For 2D and cube textures \a
    depth should be set to 0.

    \overload
 */
QRhiTexture *QRhi::newTexture(QRhiTexture::Format format,
                              int width, int height, int depth,
                              int sampleCount,
                              QRhiTexture::Flags flags)
{
    if (depth > 0)
        flags |= QRhiTexture::ThreeDimensional;

    return d->createTexture(format, QSize(width, height), depth, sampleCount, flags);
}

/*!
    \return a new sampler with the specified magnification filter \a magFilter,
    minification filter \a minFilter, mipmapping mode \a mipmapMode, and the
    addressing (wrap) modes \a addressU, \a addressV, and \a addressW.

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

    Offscreen frames do not let the CPU - potentially - generate another frame
    while the GPU is still processing the previous one. This has the side
    effect that if readbacks are scheduled, the results are guaranteed to be
    available once endOffscreenFrame() returns. That is not the case with
    frames targeting a swapchain.

    The skeleton of rendering a frame without a swapchain and then reading the
    frame contents back could look like the following:

    \badcode
          QRhiReadbackResult rbResult;
          QRhiCommandBuffer *cb;
          beginOffscreenFrame(&cb);
          beginPass
          ...
          u = nextResourceUpdateBatch();
          u->readBackTexture(rb, &rbResult);
          endPass(u);
          endOffscreenFrame();
          // image data available in rbResult
   \endcode

   \sa endOffscreenFrame(), beginFrame()
 */
QRhi::FrameOpResult QRhi::beginOffscreenFrame(QRhiCommandBuffer **cb, BeginFrameFlags flags)
{
    if (d->inFrame)
        qWarning("Attempted to call beginOffscreenFrame() within a still active frame; ignored");

    QRhi::FrameOpResult r = !d->inFrame ? d->beginOffscreenFrame(cb, flags) : FrameOpSuccess;
    if (r == FrameOpSuccess)
        d->inFrame = true;

    return r;
}

/*!
    Ends and waits for the offscreen frame.

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

static QBasicAtomicInteger<QRhiGlobalObjectIdGenerator::Type> counter = Q_BASIC_ATOMIC_INITIALIZER(0);

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
    if (stages.testFlag(QRhiShaderResourceBinding::FragmentStage))
        return QRhiPassResourceTracker::BufFragmentStage;
    if (stages.testFlag(QRhiShaderResourceBinding::ComputeStage))
        return QRhiPassResourceTracker::BufComputeStage;

    Q_UNREACHABLE();
    return QRhiPassResourceTracker::BufVertexStage;
}

QRhiPassResourceTracker::TextureStage QRhiPassResourceTracker::toPassTrackerTextureStage(QRhiShaderResourceBinding::StageFlags stages)
{
    // pick the earlier stage (as this is going to be dstAccessMask)
    if (stages.testFlag(QRhiShaderResourceBinding::VertexStage))
        return QRhiPassResourceTracker::TexVertexStage;
    if (stages.testFlag(QRhiShaderResourceBinding::FragmentStage))
        return QRhiPassResourceTracker::TexFragmentStage;
    if (stages.testFlag(QRhiShaderResourceBinding::ComputeStage))
        return QRhiPassResourceTracker::TexComputeStage;

    Q_UNREACHABLE();
    return QRhiPassResourceTracker::TexVertexStage;
}

QT_END_NAMESPACE
