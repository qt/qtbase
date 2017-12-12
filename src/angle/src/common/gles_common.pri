CONFIG += simd no_batch object_parallel_to_source
include(common.pri)

INCLUDEPATH += \
    $$OUT_PWD/.. \
    $$ANGLE_DIR \
    $$ANGLE_DIR/src/libANGLE

# Remember to adapt src/gui/configure.* if the Direct X version changes.
!winrt: \
    QMAKE_USE_PRIVATE += d3d9
winrt: \
    QMAKE_USE_PRIVATE += d3dcompiler d3d11 dxgi

QMAKE_USE_PRIVATE += dxguid

STATICLIBS = translator preprocessor
for(libname, STATICLIBS) {
    # Appends 'd' to the library for debug builds and builds up the fully
    # qualified path to pass to the linker.
    staticlib = $$QT_BUILD_TREE/lib/$${QMAKE_PREFIX_STATICLIB}$$qtLibraryTarget($$libname).$${QMAKE_EXTENSION_STATICLIB}
    LIBS_PRIVATE += $$staticlib
    PRE_TARGETDEPS += $$staticlib
}

DEFINES += LIBANGLE_IMPLEMENTATION LIBGLESV2_IMPLEMENTATION GL_APICALL= GL_GLEXT_PROTOTYPES= EGLAPI=
!winrt: DEFINES += ANGLE_ENABLE_D3D9 ANGLE_SKIP_DXGI_1_2_CHECK

QT_FOR_CONFIG += gui-private
include($$OUT_PWD/../../../gui/qtgui-config.pri)

qtConfig(angle_d3d11_qdtd): DEFINES += ANGLE_D3D11_QDTD_AVAILABLE

HEADERS += \
    $$ANGLE_DIR/src/common/mathutil.h \
    $$ANGLE_DIR/src/common/utilities.h \
    $$ANGLE_DIR/src/common/version.h \
    $$ANGLE_DIR/src/common/MemoryBuffer.h \
    $$ANGLE_DIR/src/common/angleutils.h \
    $$ANGLE_DIR/src/common/debug.h \
    $$ANGLE_DIR/src/common/event_tracer.h \
    $$ANGLE_DIR/src/libANGLE/angletypes.h \
    $$ANGLE_DIR/src/libANGLE/AttributeMap.h \
    $$ANGLE_DIR/src/libANGLE/BinaryStream.h \
    $$ANGLE_DIR/src/libANGLE/Buffer.h \
    $$ANGLE_DIR/src/libANGLE/Caps.h \
    $$ANGLE_DIR/src/libANGLE/Compiler.h \
    $$ANGLE_DIR/src/libANGLE/Config.h \
    $$ANGLE_DIR/src/libANGLE/Constants.h \
    $$ANGLE_DIR/src/libANGLE/Context.h \
    $$ANGLE_DIR/src/libANGLE/Debug.h \
    $$ANGLE_DIR/src/libANGLE/Device.h \
    $$ANGLE_DIR/src/libANGLE/Display.h \
    $$ANGLE_DIR/src/libANGLE/Error.h \
    $$ANGLE_DIR/src/libANGLE/features.h \
    $$ANGLE_DIR/src/libANGLE/Fence.h \
    $$ANGLE_DIR/src/libANGLE/formatutils.h \
    $$ANGLE_DIR/src/libANGLE/Framebuffer.h \
    $$ANGLE_DIR/src/libANGLE/FramebufferAttachment.h \
    $$ANGLE_DIR/src/libANGLE/HandleAllocator.h \
    $$ANGLE_DIR/src/libANGLE/ImageIndex.h \
    $$ANGLE_DIR/src/libANGLE/IndexRangeCache.h \
    $$ANGLE_DIR/src/libANGLE/Program.h \
    $$ANGLE_DIR/src/libANGLE/Query.h \
    $$ANGLE_DIR/src/libANGLE/queryconversions.h \
    $$ANGLE_DIR/src/libANGLE/RefCountObject.h \
    $$ANGLE_DIR/src/libANGLE/Renderbuffer.h \
    $$ANGLE_DIR/src/libANGLE/ResourceManager.h \
    $$ANGLE_DIR/src/libANGLE/Sampler.h \
    $$ANGLE_DIR/src/libANGLE/Shader.h \
    $$ANGLE_DIR/src/libANGLE/State.h \
    $$ANGLE_DIR/src/libANGLE/Surface.h \
    $$ANGLE_DIR/src/libANGLE/Texture.h \
    $$ANGLE_DIR/src/libANGLE/TransformFeedback.h \
    $$ANGLE_DIR/src/libANGLE/Uniform.h \
    $$ANGLE_DIR/src/libANGLE/validationEGL.h \
    $$ANGLE_DIR/src/libANGLE/validationES.h \
    $$ANGLE_DIR/src/libANGLE/validationES2.h \
    $$ANGLE_DIR/src/libANGLE/validationES3.h \
    $$ANGLE_DIR/src/libANGLE/VertexArray.h \
    $$ANGLE_DIR/src/libANGLE/VertexAttribute.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/BufferD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/CompilerD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/DeviceD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/DisplayD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/DynamicHLSL.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/EGLImageD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/formatutilsD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/FramebufferD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/HLSLCompiler.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/ImageD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/IndexBuffer.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/IndexDataManager.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/ProgramD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/RenderbufferD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/RendererD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/RenderTargetD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/ShaderD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/ShaderExecutableD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/SurfaceD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/SwapChainD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/TextureD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/TextureStorage.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/VertexBuffer.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/VertexDataManager.h \
    $$ANGLE_DIR/src/libANGLE/renderer/BufferImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/CompilerImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/DeviceImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/DisplayImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/FenceNVImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/FramebufferImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/ProgramImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/QueryImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/RenderbufferImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/ShaderImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/SurfaceImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/TextureImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/TransformFeedbackImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/VertexArrayImpl.h \
    $$ANGLE_DIR/src/libANGLE/ResourceManager.h \
    $$ANGLE_DIR/src/libANGLE/Sampler.h \
    $$ANGLE_DIR/src/libANGLE/Shader.h \
    $$ANGLE_DIR/src/libANGLE/State.h \
    $$ANGLE_DIR/src/libANGLE/Texture.h \
    $$ANGLE_DIR/src/libANGLE/TransformFeedback.h \
    $$ANGLE_DIR/src/libANGLE/Uniform.h \
    $$ANGLE_DIR/src/libANGLE/validationES2.h \
    $$ANGLE_DIR/src/libANGLE/validationES3.h \
    $$ANGLE_DIR/src/libANGLE/validationES.h \
    $$ANGLE_DIR/src/libANGLE/VertexArray.h \
    $$ANGLE_DIR/src/libANGLE/VertexAttribute.h \
    $$ANGLE_DIR/src/libGLESv2/entry_points_egl.h \
    $$ANGLE_DIR/src/libGLESv2/entry_points_egl_ext.h \
    $$ANGLE_DIR/src/libGLESv2/entry_points_gles_2_0_ext.h \
    $$ANGLE_DIR/src/libGLESv2/global_state.h \
    $$ANGLE_DIR/src/libGLESv2/resource.h

SOURCES += \
    $$ANGLE_DIR/src/common/mathutil.cpp \
    $$ANGLE_DIR/src/common/utilities.cpp \
    $$ANGLE_DIR/src/common/MemoryBuffer.cpp \
    $$ANGLE_DIR/src/common/angleutils.cpp \
    $$ANGLE_DIR/src/common/debug.cpp \
    $$ANGLE_DIR/src/common/event_tracer.cpp \
    $$ANGLE_DIR/src/common/Float16ToFloat32.cpp \
    $$ANGLE_DIR/src/common/string_utils.cpp \
    $$ANGLE_DIR/src/common/uniform_type_info_autogen.cpp \
    $$ANGLE_DIR/src/common/third_party/smhasher/src/PMurHash.cpp \
    $$ANGLE_DIR/src/common/third_party/base/anglebase/sha1.cc \
    $$ANGLE_DIR/src/image_util/copyimage.cpp \
    $$ANGLE_DIR/src/image_util/imageformats.cpp \
    $$ANGLE_DIR/src/image_util/loadimage.cpp \
    $$ANGLE_DIR/src/image_util/loadimage_etc.cpp \
    $$ANGLE_DIR/src/libANGLE/angletypes.cpp \
    $$ANGLE_DIR/src/libANGLE/AttributeMap.cpp \
    $$ANGLE_DIR/src/libANGLE/Buffer.cpp \
    $$ANGLE_DIR/src/libANGLE/Caps.cpp \
    $$ANGLE_DIR/src/libANGLE/Compiler.cpp \
    $$ANGLE_DIR/src/libANGLE/Config.cpp \
    $$ANGLE_DIR/src/libANGLE/Context.cpp \
    $$ANGLE_DIR/src/libANGLE/ContextState.cpp \
    $$ANGLE_DIR/src/libANGLE/Debug.cpp \
    $$ANGLE_DIR/src/libANGLE/Device.cpp \
    $$ANGLE_DIR/src/libANGLE/Display.cpp \
    $$ANGLE_DIR/src/libANGLE/Error.cpp \
    $$ANGLE_DIR/src/libANGLE/es3_copy_conversion_table_autogen.cpp \
    $$ANGLE_DIR/src/libANGLE/Fence.cpp \
    $$ANGLE_DIR/src/libANGLE/formatutils.cpp \
    $$ANGLE_DIR/src/libANGLE/format_map_autogen.cpp \
    $$ANGLE_DIR/src/libANGLE/Framebuffer.cpp \
    $$ANGLE_DIR/src/libANGLE/FramebufferAttachment.cpp \
    $$ANGLE_DIR/src/libANGLE/HandleAllocator.cpp \
    $$ANGLE_DIR/src/libANGLE/HandleRangeAllocator.cpp \
    $$ANGLE_DIR/src/libANGLE/Image.cpp \
    $$ANGLE_DIR/src/libANGLE/ImageIndex.cpp \
    $$ANGLE_DIR/src/libANGLE/IndexRangeCache.cpp \
    $$ANGLE_DIR/src/libANGLE/LoggingAnnotator.cpp \
    $$ANGLE_DIR/src/libANGLE/MemoryProgramCache.cpp \
    $$ANGLE_DIR/src/libANGLE/PackedGLEnums_autogen.cpp \
    $$ANGLE_DIR/src/libANGLE/params.cpp \
    $$ANGLE_DIR/src/libANGLE/Path.cpp \
    $$ANGLE_DIR/src/libANGLE/Platform.cpp \
    $$ANGLE_DIR/src/libANGLE/Program.cpp \
    $$ANGLE_DIR/src/libANGLE/ProgramLinkedResources.cpp \
    $$ANGLE_DIR/src/libANGLE/ProgramPipeline.cpp \
    $$ANGLE_DIR/src/libANGLE/Query.cpp \
    $$ANGLE_DIR/src/libANGLE/queryconversions.cpp \
    $$ANGLE_DIR/src/libANGLE/queryutils.cpp \
    $$ANGLE_DIR/src/libANGLE/Renderbuffer.cpp \
    $$ANGLE_DIR/src/libANGLE/ResourceManager.cpp \
    $$ANGLE_DIR/src/libANGLE/Sampler.cpp \
    $$ANGLE_DIR/src/libANGLE/Shader.cpp \
    $$ANGLE_DIR/src/libANGLE/State.cpp \
    $$ANGLE_DIR/src/libANGLE/Stream.cpp \
    $$ANGLE_DIR/src/libANGLE/Surface.cpp \
    $$ANGLE_DIR/src/libANGLE/Texture.cpp \
    $$ANGLE_DIR/src/libANGLE/Thread.cpp \
    $$ANGLE_DIR/src/libANGLE/TransformFeedback.cpp \
    $$ANGLE_DIR/src/libANGLE/Uniform.cpp \
    $$ANGLE_DIR/src/libANGLE/validationEGL.cpp \
    $$ANGLE_DIR/src/libANGLE/validationES.cpp \
    $$ANGLE_DIR/src/libANGLE/validationES2.cpp \
    $$ANGLE_DIR/src/libANGLE/validationES3.cpp \
    $$ANGLE_DIR/src/libANGLE/validationES31.cpp \
    $$ANGLE_DIR/src/libANGLE/VaryingPacking.cpp \
    $$ANGLE_DIR/src/libANGLE/VertexArray.cpp \
    $$ANGLE_DIR/src/libANGLE/VertexAttribute.cpp \
    $$ANGLE_DIR/src/libANGLE/WorkerThread.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/ContextImpl.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/DeviceImpl.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/DisplayImpl.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/driver_utils.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/Format_table_autogen.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/load_functions_table_autogen.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/renderer_utils.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/SurfaceImpl.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/TextureImpl.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/BufferD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/CompilerD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/DeviceD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/DisplayD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/DynamicHLSL.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/EGLImageD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/FramebufferD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/HLSLCompiler.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/ImageD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/IndexBuffer.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/IndexDataManager.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/NativeWindowD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/ProgramD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/RenderbufferD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/RendererD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/RenderTargetD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/ShaderD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/ShaderExecutableD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/SurfaceD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/SwapChainD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/TextureD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/VertexBuffer.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/VertexDataManager.cpp \
    $$ANGLE_DIR/src/libGLESv2/entry_points_egl.cpp \
    $$ANGLE_DIR/src/libGLESv2/entry_points_egl_ext.cpp \
    $$ANGLE_DIR/src/libGLESv2/entry_points_gles_2_0_autogen.cpp \
    $$ANGLE_DIR/src/libGLESv2/proc_table_autogen.cpp \
    $$ANGLE_DIR/src/libGLESv2/entry_points_gles_2_0_ext.cpp \
    $$ANGLE_DIR/src/libGLESv2/entry_points_gles_3_0_autogen.cpp \
    $$ANGLE_DIR/src/libGLESv2/entry_points_gles_3_1_autogen.cpp \
    $$ANGLE_DIR/src/libGLESv2/global_state.cpp \
    $$ANGLE_DIR/src/libGLESv2/libGLESv2.cpp

angle_d3d11 {
    HEADERS += \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Blit11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Buffer11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Clear11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/DebugAnnotator11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/dxgi_support_table.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Fence11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Framebuffer11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/formatutils11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Image11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/IndexBuffer11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/InputLayoutCache.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/NativeWindow11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/PixelTransfer11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Query11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Renderer11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/renderer11_utils.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/RenderTarget11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/RenderStateCache.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/ShaderExecutable11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/StateManager11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/SwapChain11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/TextureStorage11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Trim11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/texture_format_table.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/VertexBuffer11.h

    SOURCES += \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Blit11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Buffer11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Clear11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Context11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/DebugAnnotator11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/dxgi_format_map_autogen.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/dxgi_support_table.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Fence11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Framebuffer11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/formatutils11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Image11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/IndexBuffer11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/InputLayoutCache.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/PixelTransfer11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/ProgramPipeline11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Query11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Renderer11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/renderer11_utils.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/RenderTarget11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/RenderStateCache.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/ResourceManager11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/ShaderExecutable11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/StateManager11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/StreamProducerNV12.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/SwapChain11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/TextureStorage11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/TransformFeedback11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Trim11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/texture_format_table.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/texture_format_table_autogen.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/VertexArray11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/VertexBuffer11.cpp
}

!winrt {
    HEADERS += \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/Blit9.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/Buffer9.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/DebugAnnotator9.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/Fence9.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/Framebuffer9.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/formatutils9.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/Image9.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/IndexBuffer9.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/Query9.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/Renderer9.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/renderer9_utils.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/RenderTarget9.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/ShaderExecutable9.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/StateManager9.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/SwapChain9.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/TextureStorage9.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/VertexBuffer9.h \
        $$ANGLE_DIR/src/third_party/systeminfo/SystemInfo.h

    SOURCES += \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/Blit9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/Buffer9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/Context9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/DebugAnnotator9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/Fence9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/Framebuffer9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/formatutils9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/Image9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/IndexBuffer9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/NativeWindow9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/Query9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/Renderer9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/renderer9_utils.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/RenderTarget9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/ShaderExecutable9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/StateManager9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/SwapChain9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/TextureStorage9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/VertexBuffer9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/VertexDeclarationCache.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/win32/NativeWindow11Win32.cpp \
        $$ANGLE_DIR/src/third_party/systeminfo/SystemInfo.cpp
} else {
    HEADERS += \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/winrt/CoreWindowNativeWindow.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/winrt/InspectableNativeWindow.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/winrt/SwapChainPanelNativeWindow.h

    SOURCES += \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/winrt/CoreWindowNativeWindow.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/winrt/InspectableNativeWindow.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/winrt/NativeWindow11WinRT.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/winrt/SwapChainPanelNativeWindow.cpp
}

#load_functions.target = load_functions
#load_functions.commands = python $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/gen_load_functions_table.py \
#                > $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/gen_load_functions_table.cpp
#QMAKE_EXTRA_TARGETS += load_functions

# HLSL shaders
BLITVS = $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/shaders/Blit.vs
standardvs.input = BLITVS
standardvs.type = vs_2_0
standardvs.output = standardvs.h

BLITPS = $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/shaders/Blit.ps
passthroughps.input = BLITPS
passthroughps.type = ps_2_0
passthroughps.output = passthroughps.h
luminanceps.input = BLITPS
luminanceps.type = ps_2_0
luminanceps.output = luminanceps.h
luminancepremultps.input = BLITPS
luminancepremultps.type = ps_2_0
luminancepremultps.output = luminancepremultps.h
luminanceunmultps.input = BLITPS
luminanceunmultps.type = ps_2_0
luminanceunmultps.output = luminanceunmultps.h
componentmaskps.input = BLITPS
componentmaskps.type = ps_2_0
componentmaskps.output = componentmaskps.h
componentmaskpremultps.input = BLITPS
componentmaskpremultps.type = ps_2_0
componentmaskpremultps.output = componentmaskpremultps.h
componentmaskunmultps.input = BLITPS
componentmaskunmultps.type = ps_2_0
componentmaskunmultps.output = componentmaskunmultps.h

PASSTHROUGH2D = $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/shaders/Passthrough2D11.hlsl
VS_Passthrough2D.input = PASSTHROUGH2D
VS_Passthrough2D.type = vs_4_0_level_9_3
VS_Passthrough2D.output = passthrough2d11vs.h
PS_PassthroughRGBA2D.input = PASSTHROUGH2D
PS_PassthroughRGBA2D.type = ps_4_0_level_9_3
PS_PassthroughRGBA2D.output = passthroughrgba2d11ps.h
PS_PassthroughRGB2D.input = PASSTHROUGH2D
PS_PassthroughRGB2D.type = ps_4_0_level_9_3
PS_PassthroughRGB2D.output = passthroughrgb2d11ps.h
PS_PassthroughRG2D.input = PASSTHROUGH2D
PS_PassthroughRG2D.type = ps_4_0_level_9_3
PS_PassthroughRG2D.output = passthroughrg2d11ps.h
PS_PassthroughR2D.input = PASSTHROUGH2D
PS_PassthroughR2D.type = ps_4_0_level_9_3
PS_PassthroughR2D.output = passthroughr2d11ps.h
PS_PassthroughLum2D.input = PASSTHROUGH2D
PS_PassthroughLum2D.type = ps_4_0_level_9_3
PS_PassthroughLum2D.output = passthroughlum2d11ps.h
PS_PassthroughLumAlpha2D.input = PASSTHROUGH2D
PS_PassthroughLumAlpha2D.type = ps_4_0_level_9_3
PS_PassthroughLumAlpha2D.output = passthroughlumalpha2d11ps.h
PS_PassthroughDepth2D.input = PASSTHROUGH2D
PS_PassthroughDepth2D.type = ps_4_0
PS_PassthroughDepth2D.output = passthroughdepth2d11ps.h
PS_PassthroughRGBA2DUI.input = PASSTHROUGH2D
PS_PassthroughRGBA2DUI.type = ps_4_0
PS_PassthroughRGBA2DUI.output = passthroughrgba2dui11ps.h
PS_PassthroughRGBA2DI.input = PASSTHROUGH2D
PS_PassthroughRGBA2DI.type = ps_4_0
PS_PassthroughRGBA2DI.output = passthroughrgba2di11ps.h
PS_PassthroughRGB2DUI.input = PASSTHROUGH2D
PS_PassthroughRGB2DUI.type = ps_4_0
PS_PassthroughRGB2DUI.output = passthroughrgb2dui11ps.h
PS_PassthroughRGB2DI.input = PASSTHROUGH2D
PS_PassthroughRGB2DI.type = ps_4_0
PS_PassthroughRGB2DI.output = passthroughrgb2di11ps.h
PS_PassthroughRG2DUI.input = PASSTHROUGH2D
PS_PassthroughRG2DUI.type = ps_4_0
PS_PassthroughRG2DUI.output = passthroughrg2dui11ps.h
PS_PassthroughRG2DI.input = PASSTHROUGH2D
PS_PassthroughRG2DI.type = ps_4_0
PS_PassthroughRG2DI.output = passthroughrg2di11ps.h
PS_PassthroughR2DUI.input = PASSTHROUGH2D
PS_PassthroughR2DUI.type = ps_4_0
PS_PassthroughR2DUI.output = passthroughr2dui11ps.h
PS_PassthroughR2DI.input = PASSTHROUGH2D
PS_PassthroughR2DI.type = ps_4_0
PS_PassthroughR2DI.output = passthroughr2di11ps.h
PS_PassthroughA2D.input = PASSTHROUGH2D
PS_PassthroughA2D.type = ps_4_0_level_9_3
PS_PassthroughA2D.output = passthrougha2d11ps.h
PS_PassthroughRGBA2DMS.input = PASSTHROUGH2D
PS_PassthroughRGBA2DMS.type = ps_4_1
PS_PassthroughRGBA2DMS.output = passthroughrgba2dms11ps.h

CLEAR = $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/shaders/Clear11.hlsl
VS_Clear_FL9.input = CLEAR
VS_Clear_FL9.type = vs_4_0_level_9_3
VS_Clear_FL9.output = clear11_fl9vs.h
PS_ClearFloat_FL9.input = CLEAR
PS_ClearFloat_FL9.type = ps_4_0_level_9_3
PS_ClearFloat_FL9.output = clearfloat11_fl9ps.h
VS_Clear.input = CLEAR
VS_Clear.type = vs_4_0
VS_Clear.output = clear11vs.h
VS_Multiview_Clear.input = CLEAR
VS_Multiview_Clear.type = vs_4_0
VS_Multiview_Clear.output = clear11multiviewvs.h
GS_Multiview_Clear.input = CLEAR
GS_Multiview_Clear.type = gs_4_0
GS_Multiview_Clear.output = clear11multiviewgs.h
PS_ClearDepth.input = CLEAR
PS_ClearDepth.type = ps_4_0
PS_ClearDepth.output = cleardepth11ps.h
PS_ClearFloat1.input = CLEAR
PS_ClearFloat1.type = ps_4_0
PS_ClearFloat1.output = clearfloat11ps1.h
PS_ClearFloat2.input = CLEAR
PS_ClearFloat2.type = ps_4_0
PS_ClearFloat2.output = clearfloat11ps2.h
PS_ClearFloat3.input = CLEAR
PS_ClearFloat3.type = ps_4_0
PS_ClearFloat3.output = clearfloat11ps3.h
PS_ClearFloat4.input = CLEAR
PS_ClearFloat4.type = ps_4_0
PS_ClearFloat4.output = clearfloat11ps4.h
PS_ClearFloat5.input = CLEAR
PS_ClearFloat5.type = ps_4_0
PS_ClearFloat5.output = clearfloat11ps5.h
PS_ClearFloat6.input = CLEAR
PS_ClearFloat6.type = ps_4_0
PS_ClearFloat6.output = clearfloat11ps6.h
PS_ClearFloat7.input = CLEAR
PS_ClearFloat7.type = ps_4_0
PS_ClearFloat7.output = clearfloat11ps7.h
PS_ClearFloat8.input = CLEAR
PS_ClearFloat8.type = ps_4_0
PS_ClearFloat8.output = clearfloat11ps8.h
PS_ClearUint1.input = CLEAR
PS_ClearUint1.type = ps_4_0
PS_ClearUint1.output = clearuint11ps1.h
PS_ClearUint2.input = CLEAR
PS_ClearUint2.type = ps_4_0
PS_ClearUint2.output = clearuint11ps2.h
PS_ClearUint3.input = CLEAR
PS_ClearUint3.type = ps_4_0
PS_ClearUint3.output = clearuint11ps3.h
PS_ClearUint4.input = CLEAR
PS_ClearUint4.type = ps_4_0
PS_ClearUint4.output = clearuint11ps4.h
PS_ClearUint5.input = CLEAR
PS_ClearUint5.type = ps_4_0
PS_ClearUint5.output = clearuint11ps5.h
PS_ClearUint6.input = CLEAR
PS_ClearUint6.type = ps_4_0
PS_ClearUint6.output = clearuint11ps6.h
PS_ClearUint7.input = CLEAR
PS_ClearUint7.type = ps_4_0
PS_ClearUint7.output = clearuint11ps7.h
PS_ClearUint8.input = CLEAR
PS_ClearUint8.type = ps_4_0
PS_ClearUint8.output = clearuint11ps8.h
PS_ClearSint1.input = CLEAR
PS_ClearSint1.type = ps_4_0
PS_ClearSint1.output = clearsint11ps1.h
PS_ClearSint2.input = CLEAR
PS_ClearSint2.type = ps_4_0
PS_ClearSint2.output = clearsint11ps2.h
PS_ClearSint3.input = CLEAR
PS_ClearSint3.type = ps_4_0
PS_ClearSint3.output = clearsint11ps3.h
PS_ClearSint4.input = CLEAR
PS_ClearSint4.type = ps_4_0
PS_ClearSint4.output = clearsint11ps4.h
PS_ClearSint5.input = CLEAR
PS_ClearSint5.type = ps_4_0
PS_ClearSint5.output = clearsint11ps5.h
PS_ClearSint6.input = CLEAR
PS_ClearSint6.type = ps_4_0
PS_ClearSint6.output = clearsint11ps6.h
PS_ClearSint7.input = CLEAR
PS_ClearSint7.type = ps_4_0
PS_ClearSint7.output = clearsint11ps7.h
PS_ClearSint8.input = CLEAR
PS_ClearSint8.type = ps_4_0
PS_ClearSint8.output = clearsint11ps8.h

PASSTHROUGH3D = $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/shaders/Passthrough3D11.hlsl
VS_Passthrough3D.input = PASSTHROUGH3D
VS_Passthrough3D.type = vs_4_0
VS_Passthrough3D.output = passthrough3d11vs.h
GS_Passthrough3D.input = PASSTHROUGH3D
GS_Passthrough3D.type = gs_4_0
GS_Passthrough3D.output = passthrough3d11gs.h
PS_PassthroughRGBA3D.input = PASSTHROUGH3D
PS_PassthroughRGBA3D.type = ps_4_0
PS_PassthroughRGBA3D.output = passthroughrgba3d11ps.h
PS_PassthroughRGBA3DUI.input = PASSTHROUGH3D
PS_PassthroughRGBA3DUI.type = ps_4_0
PS_PassthroughRGBA3DUI.output = passthroughrgba3dui11ps.h
PS_PassthroughRGBA3DI.input = PASSTHROUGH3D
PS_PassthroughRGBA3DI.type = ps_4_0
PS_PassthroughRGBA3DI.output = passthroughrgba3di11ps.h
PS_PassthroughRGB3D.input = PASSTHROUGH3D
PS_PassthroughRGB3D.type = ps_4_0
PS_PassthroughRGB3D.output = passthroughrgb3d11ps.h
PS_PassthroughRGB3DUI.input = PASSTHROUGH3D
PS_PassthroughRGB3DUI.type = ps_4_0
PS_PassthroughRGB3DUI.output = passthroughrgb3dui11ps.h
PS_PassthroughRGB3DI.input = PASSTHROUGH3D
PS_PassthroughRGB3DI.type = ps_4_0
PS_PassthroughRGB3DI.output = passthroughrgb3di11ps.h
PS_PassthroughRG3D.input = PASSTHROUGH3D
PS_PassthroughRG3D.type = ps_4_0
PS_PassthroughRG3D.output = passthroughrg3d11ps.h
PS_PassthroughRG3DUI.input = PASSTHROUGH3D
PS_PassthroughRG3DUI.type = ps_4_0
PS_PassthroughRG3DUI.output = passthroughrg3dui11ps.h
PS_PassthroughRG3DI.input = PASSTHROUGH3D
PS_PassthroughRG3DI.type = ps_4_0
PS_PassthroughRG3DI.output = passthroughrg3di11ps.h
PS_PassthroughR3D.input = PASSTHROUGH3D
PS_PassthroughR3D.type = ps_4_0
PS_PassthroughR3D.output = passthroughr3d11ps.h
PS_PassthroughR3DUI.input = PASSTHROUGH3D
PS_PassthroughR3DUI.type = ps_4_0
PS_PassthroughR3DUI.output = passthroughr3dui11ps.h
PS_PassthroughR3DI.input = PASSTHROUGH3D
PS_PassthroughR3DI.type = ps_4_0
PS_PassthroughR3DI.output = passthroughr3di11ps.h
PS_PassthroughLum3D.input = PASSTHROUGH3D
PS_PassthroughLum3D.type = ps_4_0
PS_PassthroughLum3D.output = passthroughlum3d11ps.h
PS_PassthroughLumAlpha3D.input = PASSTHROUGH3D
PS_PassthroughLumAlpha3D.type = ps_4_0
PS_PassthroughLumAlpha3D.output = passthroughlumalpha3d11ps.h

SWIZZLE = $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/shaders/Swizzle11.hlsl
PS_SwizzleF2D.input = SWIZZLE
PS_SwizzleF2D.type = ps_4_0
PS_SwizzleF2D.output = swizzlef2dps.h
PS_SwizzleI2D.input = SWIZZLE
PS_SwizzleI2D.type = ps_4_0
PS_SwizzleI2D.output = swizzlei2dps.h
PS_SwizzleUI2D.input = SWIZZLE
PS_SwizzleUI2D.type = ps_4_0
PS_SwizzleUI2D.output = swizzleui2dps.h
PS_SwizzleF3D.input = SWIZZLE
PS_SwizzleF3D.type = ps_4_0
PS_SwizzleF3D.output = swizzlef3dps.h
PS_SwizzleI3D.input = SWIZZLE
PS_SwizzleI3D.type = ps_4_0
PS_SwizzleI3D.output = swizzlei3dps.h
PS_SwizzleUI3D.input = SWIZZLE
PS_SwizzleUI3D.type = ps_4_0
PS_SwizzleUI3D.output = swizzleui3dps.h
PS_SwizzleF2DArray.input = SWIZZLE
PS_SwizzleF2DArray.type = ps_4_0
PS_SwizzleF2DArray.output = swizzlef2darrayps.h
PS_SwizzleI2DArray.input = SWIZZLE
PS_SwizzleI2DArray.type = ps_4_0
PS_SwizzleI2DArray.output = swizzlei2darrayps.h
PS_SwizzleUI2DArray.input = SWIZZLE
PS_SwizzleUI2DArray.type = ps_4_0
PS_SwizzleUI2DArray.output = swizzleui2darrayps.h

BUFFERTOTEXTURE = $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/shaders/BufferToTexture11.hlsl
VS_BufferToTexture.input = BUFFERTOTEXTURE
VS_BufferToTexture.type = vs_4_0
VS_BufferToTexture.output = buffertotexture11_vs.h
GS_BufferToTexture.input = BUFFERTOTEXTURE
GS_BufferToTexture.type = gs_4_0
GS_BufferToTexture.output = buffertotexture11_gs.h
PS_BufferToTexture_4F.input = BUFFERTOTEXTURE
PS_BufferToTexture_4F.type = ps_4_0
PS_BufferToTexture_4F.output = buffertotexture11_ps_4f.h
PS_BufferToTexture_4I.input = BUFFERTOTEXTURE
PS_BufferToTexture_4I.type = ps_4_0
PS_BufferToTexture_4I.output = buffertotexture11_ps_4i.h
PS_BufferToTexture_4UI.input = BUFFERTOTEXTURE
PS_BufferToTexture_4UI.type = ps_4_0
PS_BufferToTexture_4UI.output = buffertotexture11_ps_4ui.h

MULTIPLYALPHA = $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/shaders/MultiplyAlpha.hlsl
PS_FtoF_PM_RGBA.input = MULTIPLYALPHA
PS_FtoF_PM_RGBA.type = ps_4_0
PS_FtoF_PM_RGBA.output = multiplyalpha_ftof_pm_rgba_ps.h
PS_FtoF_UM_RGBA.input = MULTIPLYALPHA
PS_FtoF_UM_RGBA.type = ps_4_0
PS_FtoF_UM_RGBA.output = multiplyalpha_ftof_um_rgba_ps.h
PS_FtoF_PM_RGB.input = MULTIPLYALPHA
PS_FtoF_PM_RGB.type = ps_4_0
PS_FtoF_PM_RGB.output = multiplyalpha_ftof_pm_rgb_ps.h
PS_FtoF_UM_RGB.input = MULTIPLYALPHA
PS_FtoF_UM_RGB.type = ps_4_0
PS_FtoF_UM_RGB.output = multiplyalpha_ftof_um_rgb_ps.h
PS_FtoU_PT_RGBA.input = MULTIPLYALPHA
PS_FtoU_PT_RGBA.type = ps_4_0
PS_FtoU_PT_RGBA.output = multiplyalpha_ftou_pt_rgba_ps.h
PS_FtoU_PM_RGBA.input = MULTIPLYALPHA
PS_FtoU_PM_RGBA.type = ps_4_0
PS_FtoU_PM_RGBA.output = multiplyalpha_ftou_pm_rgba_ps.h
PS_FtoU_UM_RGBA.input = MULTIPLYALPHA
PS_FtoU_UM_RGBA.type = ps_4_0
PS_FtoU_UM_RGBA.output = multiplyalpha_ftou_um_rgba_ps.h
PS_FtoU_PT_RGB.input = MULTIPLYALPHA
PS_FtoU_PT_RGB.type = ps_4_0
PS_FtoU_PT_RGB.output = multiplyalpha_ftou_pt_rgb_ps.h
PS_FtoU_PM_RGB.input = MULTIPLYALPHA
PS_FtoU_PM_RGB.type = ps_4_0
PS_FtoU_PM_RGB.output = multiplyalpha_ftou_pm_rgb_ps.h
PS_FtoU_UM_RGB.input = MULTIPLYALPHA
PS_FtoU_UM_RGB.type = ps_4_0
PS_FtoU_UM_RGB.output = multiplyalpha_ftou_um_rgb_ps.h
PS_FtoF_PM_LUMA.input = MULTIPLYALPHA
PS_FtoF_PM_LUMA.type = ps_4_0
PS_FtoF_PM_LUMA.output = multiplyalpha_ftof_pm_luma_ps.h
PS_FtoF_UM_LUMA.input = MULTIPLYALPHA
PS_FtoF_UM_LUMA.type = ps_4_0
PS_FtoF_UM_LUMA.output = multiplyalpha_ftof_um_luma_ps.h
PS_FtoF_PM_LUMAALPHA.input = MULTIPLYALPHA
PS_FtoF_PM_LUMAALPHA.type = ps_4_0
PS_FtoF_PM_LUMAALPHA.output = multiplyalpha_ftof_pm_lumaalpha_ps.h
PS_FtoF_UM_LUMAALPHA.input = MULTIPLYALPHA
PS_FtoF_UM_LUMAALPHA.type = ps_4_0
PS_FtoF_UM_LUMAALPHA.output = multiplyalpha_ftof_um_lumaalpha_ps.h

RESOLVEDEPTHSTENCIL = \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/shaders/ResolveDepthStencil.hlsl
VS_ResolveDepthStencil.input = RESOLVEDEPTHSTENCIL
VS_ResolveDepthStencil.type = vs_4_1
VS_ResolveDepthStencil.output = resolvedepthstencil11_vs.h
PS_ResolveDepth.input = RESOLVEDEPTHSTENCIL
PS_ResolveDepth.type = ps_4_1
PS_ResolveDepth.output = resolvedepth11_ps.h
PS_ResolveDepthStencil.input = RESOLVEDEPTHSTENCIL
PS_ResolveDepthStencil.type = ps_4_1
PS_ResolveDepthStencil.output = resolvedepthstencil11_ps.h
PS_ResolveStencil.input = RESOLVEDEPTHSTENCIL
PS_ResolveStencil.type = ps_4_1
PS_ResolveStencil.output = resolvestencil11_ps.h

# D3D11
angle_d3d11: SHADERS = VS_Passthrough2D \
    PS_PassthroughRGB2D PS_PassthroughRGB2DUI PS_PassthroughRGB2DI \
    PS_PassthroughRGBA2D PS_PassthroughRGBA2DUI PS_PassthroughRGBA2DI \
    PS_PassthroughRG2D PS_PassthroughRG2DUI PS_PassthroughRG2DI \
    PS_PassthroughR2D PS_PassthroughR2DUI PS_PassthroughR2DI \
    PS_PassthroughA2D \
    PS_PassthroughLum2D PS_PassthroughLumAlpha2D PS_PassthroughDepth2D \
    VS_Clear_FL9 PS_ClearFloat_FL9 VS_Clear VS_Multiview_Clear \
    GS_Multiview_Clear PS_ClearDepth \
    PS_ClearFloat1 PS_ClearFloat2 PS_ClearFloat3 PS_ClearFloat4 PS_ClearFloat5 \
    PS_ClearFloat6 PS_ClearFloat7 PS_ClearFloat8 \
    PS_ClearUint1 PS_ClearUint2 PS_ClearUint3 PS_ClearUint4 \
    PS_ClearUint5 PS_ClearUint6 PS_ClearUint7 PS_ClearUint8 \
    PS_ClearSint1 PS_ClearSint2 PS_ClearSint3 PS_ClearSint4 \
    PS_ClearSint5 PS_ClearSint6 PS_ClearSint7 PS_ClearSint8 \
    VS_Passthrough3D GS_Passthrough3D \
    PS_PassthroughRGBA3D PS_PassthroughRGBA3DUI PS_PassthroughRGBA3DI \
    PS_PassthroughRGB3D PS_PassthroughRGB3DUI PS_PassthroughRGB3DI \
    PS_PassthroughRG3D PS_PassthroughRG3DUI PS_PassthroughRG3DI \
    PS_PassthroughR3D PS_PassthroughR3DUI PS_PassthroughR3DI \
    PS_PassthroughLum3D PS_PassthroughLumAlpha3D \
    PS_SwizzleF2D PS_SwizzleI2D PS_SwizzleUI2D \
    PS_SwizzleF3D PS_SwizzleI3D PS_SwizzleUI3D \
    PS_SwizzleF2DArray PS_SwizzleI2DArray PS_SwizzleUI2DArray \
    VS_BufferToTexture GS_BufferToTexture \
    PS_BufferToTexture_4F PS_BufferToTexture_4I PS_BufferToTexture_4UI \
    PS_FtoF_PM_RGBA PS_FtoF_UM_RGBA PS_FtoF_PM_RGB PS_FtoF_UM_RGB \
    PS_FtoU_PT_RGBA PS_FtoU_PM_RGBA PS_FtoU_UM_RGBA \
    PS_FtoU_PT_RGB PS_FtoU_PM_RGB PS_FtoU_UM_RGB \
    PS_FtoF_PM_LUMA PS_FtoF_UM_LUMA \
    PS_FtoF_PM_LUMAALPHA PS_FtoF_UM_LUMAALPHA \
    VS_ResolveDepthStencil PS_ResolveDepth PS_ResolveDepthStencil PS_ResolveStencil

# This shader causes an internal compiler error in mingw73. Re-enable it, when
# our mingw version can handle it.
!mingw: angle_d3d11: SHADERS += PS_PassthroughRGBA2DMS

# D3D9
!winrt: SHADERS += standardvs passthroughps \
    luminanceps luminancepremultps luminanceunmultps \
    componentmaskps componentmaskpremultps componentmaskunmultps

# Generate headers
for (SHADER, SHADERS) {
    INPUT = $$eval($${SHADER}.input)
    OUT_DIR = $$OUT_PWD/libANGLE/$$relative_path($$dirname($$INPUT), $$ANGLE_DIR/src/libANGLE)/compiled
    fxc_$${SHADER}.commands = $$FXC -nologo -E $${SHADER} -T $$eval($${SHADER}.type) -Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    fxc_$${SHADER}.output = $$OUT_DIR/$$eval($${SHADER}.output)
    fxc_$${SHADER}.input = $$INPUT
    fxc_$${SHADER}.dependency_type = TYPE_C
    fxc_$${SHADER}.variable_out = HEADERS
    fxc_$${SHADER}.CONFIG += target_predeps
    QMAKE_EXTRA_COMPILERS += fxc_$${SHADER}
}

khr_headers.files = $$ANGLE_DIR/include/KHR/khrplatform.h
khr_headers.path = $$[QT_INSTALL_HEADERS]/QtANGLE/KHR
gles2_headers.files = \
    $$ANGLE_DIR/include/GLES2/gl2.h \
    $$ANGLE_DIR/include/GLES2/gl2ext.h \
    $$ANGLE_DIR/include/GLES2/gl2ext_angle.h \
    $$ANGLE_DIR/include/GLES2/gl2platform.h
gles2_headers.path = $$[QT_INSTALL_HEADERS]/QtANGLE/GLES2
gles3_headers.files = \
    $$ANGLE_DIR/include/GLES3/gl3.h \
    $$ANGLE_DIR/include/GLES3/gl3platform.h
gles3_headers.path = $$[QT_INSTALL_HEADERS]/QtANGLE/GLES3
INSTALLS += khr_headers gles2_headers
angle_d3d11: INSTALLS += gles3_headers
