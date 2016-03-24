CONFIG += simd installed no_batch
include(../common/common.pri)
DEF_FILE_TARGET=$${TARGET}
TARGET=$$qtLibraryTarget($${LIBGLESV2_NAME})

INCLUDEPATH += $$OUT_PWD/.. $$ANGLE_DIR/src/libANGLE

# Remember to adapt tools/configure/configureapp.cpp if the Direct X version changes.
!winrt: \
    LIBS_PRIVATE += -ld3d9
winrt: \
    LIBS_PRIVATE += -ld3dcompiler -ldxgi -ld3d11

LIBS_PRIVATE += -ldxguid

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

HEADERS += \
    $$ANGLE_DIR/src/common/mathutil.h \
    $$ANGLE_DIR/src/common/blocklayout.h \
    $$ANGLE_DIR/src/common/NativeWindow.h \
    $$ANGLE_DIR/src/common/shadervars.h \
    $$ANGLE_DIR/src/common/utilities.h \
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
    $$ANGLE_DIR/src/libANGLE/Data.h \
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
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/copyimage.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/DeviceD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/DisplayD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/DynamicHLSL.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/EGLImageD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/formatutilsD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/FramebufferD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/generatemip.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/HLSLCompiler.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/ImageD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/imageformats.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/IndexBuffer.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/IndexDataManager.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/loadimage.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/loadimage_etc.h \
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
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/TransformFeedbackD3D.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/VaryingPacking.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/VertexBuffer.h \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/VertexDataManager.h \
    $$ANGLE_DIR/src/libANGLE/renderer/BufferImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/CompilerImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/DeviceImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/DisplayImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/FenceNVImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/FenceSyncImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/FramebufferImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/Image.h \
    $$ANGLE_DIR/src/libANGLE/renderer/ImplFactory.h \
    $$ANGLE_DIR/src/libANGLE/renderer/ProgramImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/QueryImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/RenderbufferImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/Renderer.h \
    $$ANGLE_DIR/src/libANGLE/renderer/ShaderImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/SurfaceImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/TextureImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/TransformFeedbackImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/VertexArrayImpl.h \
    $$ANGLE_DIR/src/libANGLE/renderer/Workarounds.h \
    $$ANGLE_DIR/src/libANGLE/resource.h \
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
    $$ANGLE_DIR/src/libANGLE/vertexconversion.h \
    $$ANGLE_DIR/src/libGLESv2/entry_points_egl.h \
    $$ANGLE_DIR/src/libGLESv2/entry_points_egl_ext.h \
    $$ANGLE_DIR/src/libGLESv2/entry_points_gles_2_0.h \
    $$ANGLE_DIR/src/libGLESv2/entry_points_gles_2_0_ext.h \
    $$ANGLE_DIR/src/libGLESv2/entry_points_gles_3_0.h \
    $$ANGLE_DIR/src/libGLESv2/entry_points_gles_3_0_ext.h \
    $$ANGLE_DIR/src/libGLESv2/global_state.h \
    $$ANGLE_DIR/src/libGLESv2/resource.h \
    $$ANGLE_DIR/src/third_party/murmurhash/MurmurHash3.h

SOURCES += \
    $$ANGLE_DIR/src/common/mathutil.cpp \
    $$ANGLE_DIR/src/common/utilities.cpp \
    $$ANGLE_DIR/src/common/MemoryBuffer.cpp \
    $$ANGLE_DIR/src/common/angleutils.cpp \
    $$ANGLE_DIR/src/common/debug.cpp \
    $$ANGLE_DIR/src/common/event_tracer.cpp \
    $$ANGLE_DIR/src/common/Float16ToFloat32.cpp \
    $$ANGLE_DIR/src/third_party/murmurhash/MurmurHash3.cpp \
    $$ANGLE_DIR/src/libANGLE/angletypes.cpp \
    $$ANGLE_DIR/src/libANGLE/AttributeMap.cpp \
    $$ANGLE_DIR/src/libANGLE/Buffer.cpp \
    $$ANGLE_DIR/src/libANGLE/Caps.cpp \
    $$ANGLE_DIR/src/libANGLE/Compiler.cpp \
    $$ANGLE_DIR/src/libANGLE/Config.cpp \
    $$ANGLE_DIR/src/libANGLE/Context.cpp \
    $$ANGLE_DIR/src/libANGLE/Data.cpp \
    $$ANGLE_DIR/src/libANGLE/Device.cpp \
    $$ANGLE_DIR/src/libANGLE/Display.cpp \
    $$ANGLE_DIR/src/libANGLE/Error.cpp \
    $$ANGLE_DIR/src/libANGLE/Fence.cpp \
    $$ANGLE_DIR/src/libANGLE/formatutils.cpp \
    $$ANGLE_DIR/src/libANGLE/Framebuffer.cpp \
    $$ANGLE_DIR/src/libANGLE/FramebufferAttachment.cpp \
    $$ANGLE_DIR/src/libANGLE/HandleAllocator.cpp \
    $$ANGLE_DIR/src/libANGLE/Image.cpp \
    $$ANGLE_DIR/src/libANGLE/ImageIndex.cpp \
    $$ANGLE_DIR/src/libANGLE/IndexRangeCache.cpp \
    $$ANGLE_DIR/src/libANGLE/Platform.cpp \
    $$ANGLE_DIR/src/libANGLE/Program.cpp \
    $$ANGLE_DIR/src/libANGLE/Query.cpp \
    $$ANGLE_DIR/src/libANGLE/queryconversions.cpp \
    $$ANGLE_DIR/src/libANGLE/Renderbuffer.cpp \
    $$ANGLE_DIR/src/libANGLE/ResourceManager.cpp \
    $$ANGLE_DIR/src/libANGLE/Sampler.cpp \
    $$ANGLE_DIR/src/libANGLE/Shader.cpp \
    $$ANGLE_DIR/src/libANGLE/State.cpp \
    $$ANGLE_DIR/src/libANGLE/Surface.cpp \
    $$ANGLE_DIR/src/libANGLE/Texture.cpp \
    $$ANGLE_DIR/src/libANGLE/TransformFeedback.cpp \
    $$ANGLE_DIR/src/libANGLE/Uniform.cpp \
    $$ANGLE_DIR/src/libANGLE/validationEGL.cpp \
    $$ANGLE_DIR/src/libANGLE/validationES.cpp \
    $$ANGLE_DIR/src/libANGLE/validationES2.cpp \
    $$ANGLE_DIR/src/libANGLE/validationES3.cpp \
    $$ANGLE_DIR/src/libANGLE/VertexArray.cpp \
    $$ANGLE_DIR/src/libANGLE/VertexAttribute.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/DeviceImpl.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/DisplayImpl.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/Renderer.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/SurfaceImpl.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/BufferD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/CompilerD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/copyimage.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/DeviceD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/DisplayD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/DynamicHLSL.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/EGLImageD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/formatutilsD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/FramebufferD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/HLSLCompiler.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/ImageD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/IndexBuffer.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/IndexDataManager.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/loadimage.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/loadimage_etc.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/ProgramD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/RenderbufferD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/RendererD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/RenderTargetD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/ShaderD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/ShaderExecutableD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/SurfaceD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/TextureD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/TransformFeedbackD3D.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/VaryingPacking.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/VertexBuffer.cpp \
    $$ANGLE_DIR/src/libANGLE/renderer/d3d/VertexDataManager.cpp \
    $$ANGLE_DIR/src/libGLESv2/entry_points_egl.cpp \
    $$ANGLE_DIR/src/libGLESv2/entry_points_egl_ext.cpp \
    $$ANGLE_DIR/src/libGLESv2/entry_points_gles_2_0.cpp \
    $$ANGLE_DIR/src/libGLESv2/entry_points_gles_2_0_ext.cpp \
    $$ANGLE_DIR/src/libGLESv2/entry_points_gles_3_0.cpp \
    $$ANGLE_DIR/src/libGLESv2/entry_points_gles_3_0_ext.cpp \
    $$ANGLE_DIR/src/libGLESv2/global_state.cpp \
    $$ANGLE_DIR/src/libGLESv2/libGLESv2.cpp

SSE2_SOURCES += $$ANGLE_DIR/src/libANGLE/renderer/d3d/loadimageSSE2.cpp

DEBUG_SOURCE = $$ANGLE_DIR/src/libANGLE/Debug.cpp
debug_copy.input = DEBUG_SOURCE
debug_copy.output = $$ANGLE_DIR/src/libANGLE/Debug2.cpp
debug_copy.commands = $$QMAKE_COPY \"${QMAKE_FILE_IN}\" \"${QMAKE_FILE_OUT}\"
debug_copy.variable_out = GENERATED_SOURCES
debug_copy.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += debug_copy

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
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/internal_format_initializer_table.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/load_functions_table.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/PixelTransfer11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Query11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Renderer11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/renderer11_utils.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/RenderTarget11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/RenderStateCache.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/ShaderExecutable11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/StateManager11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/SwapChain11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/swizzle_format_info.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/TextureStorage11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Trim11.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/texture_format_table.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/VertexBuffer11.h

    SOURCES += \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Blit11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Buffer11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Clear11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/DebugAnnotator11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/dxgi_support_table.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Fence11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Framebuffer11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/formatutils11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Image11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/IndexBuffer11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/InputLayoutCache.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/internal_format_initializer_table.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/load_functions_table_autogen.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/PixelTransfer11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Query11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Renderer11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/renderer11_utils.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/RenderTarget11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/RenderStateCache.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/ShaderExecutable11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/StateManager11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/SwapChain11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/swizzle_format_info_autogen.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/TextureStorage11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/Trim11.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/texture_format_table_autogen.cpp \
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
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/DebugAnnotator9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/Fence9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/Framebuffer9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/formatutils9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/Image9.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/IndexBuffer9.cpp \
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
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/win32/NativeWindow.cpp \
        $$ANGLE_DIR/src/third_party/systeminfo/SystemInfo.cpp
} else {
    HEADERS += \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/winrt/CoreWindowNativeWindow.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/winrt/InspectableNativeWindow.h \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/winrt/SwapChainPanelNativeWindow.h

    SOURCES += \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/winrt/CoreWindowNativeWindow.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/winrt/InspectableNativeWindow.cpp \
        $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/winrt/SwapChainPanelNativeWindow.cpp
}

!static {
    DEF_FILE = $$ANGLE_DIR/src/libGLESv2/$${DEF_FILE_TARGET}.def
    mingw:equals(QT_ARCH, i386): DEF_FILE = $$ANGLE_DIR/src/libGLESv2/$${DEF_FILE_TARGET}_mingw32.def
} else {
    DEFINES += DllMain=DllMain_ANGLE # prevent symbol from conflicting with the user's DllMain
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
flipyvs.input = BLITVS
flipyvs.type = vs_2_0
flipyvs.output = flipyvs.h

BLITPS = $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d9/shaders/Blit.ps
passthroughps.input = BLITPS
passthroughps.type = ps_2_0
passthroughps.output = passthroughps.h
luminanceps.input = BLITPS
luminanceps.type = ps_2_0
luminanceps.output = luminanceps.h
componentmaskps.input = BLITPS
componentmaskps.type = ps_2_0
componentmaskps.output = componentmaskps.h

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

CLEAR = $$ANGLE_DIR/src/libANGLE/renderer/d3d/d3d11/shaders/Clear11.hlsl
VS_ClearFloat.input = CLEAR
VS_ClearFloat.type = vs_4_0_level_9_3
VS_ClearFloat.output = clearfloat11vs.h
PS_ClearFloat_FL9.input = CLEAR
PS_ClearFloat_FL9.type = ps_4_0_level_9_3
PS_ClearFloat_FL9.output = clearfloat11_fl9ps.h
PS_ClearFloat.input = CLEAR
PS_ClearFloat.type = ps_4_0
PS_ClearFloat.output = clearfloat11ps.h
VS_ClearUint.input = CLEAR
VS_ClearUint.type = vs_4_0
VS_ClearUint.output = clearuint11vs.h
PS_ClearUint.input = CLEAR
PS_ClearUint.type = ps_4_0
PS_ClearUint.output = clearuint11ps.h
VS_ClearSint.input = CLEAR
VS_ClearSint.type = vs_4_0
VS_ClearSint.output = clearsint11vs.h
PS_ClearSint.input = CLEAR
PS_ClearSint.type = ps_4_0
PS_ClearSint.output = clearsint11ps.h

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

# D3D11
angle_d3d11: SHADERS = VS_Passthrough2D \
    PS_PassthroughRGB2D PS_PassthroughRGB2DUI PS_PassthroughRGB2DI \
    PS_PassthroughRGBA2D PS_PassthroughRGBA2DUI PS_PassthroughRGBA2DI \
    PS_PassthroughRG2D PS_PassthroughRG2DUI PS_PassthroughRG2DI \
    PS_PassthroughR2D PS_PassthroughR2DUI PS_PassthroughR2DI \
    PS_PassthroughLum2D PS_PassthroughLumAlpha2D PS_PassthroughDepth2D \
    VS_ClearFloat VS_ClearUint VS_ClearSint \
    PS_ClearFloat PS_ClearFloat_FL9 PS_ClearUint PS_ClearSint \
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
    PS_BufferToTexture_4F PS_BufferToTexture_4I PS_BufferToTexture_4UI

# D3D9
!winrt: SHADERS += standardvs flipyvs passthroughps luminanceps componentmaskps

# Generate headers
for (SHADER, SHADERS) {
    INPUT = $$eval($${SHADER}.input)
    OUT_DIR = $$OUT_PWD/libANGLE/$$relative_path($$dirname($$INPUT), $$ANGLE_DIR/src/libANGLE)/compiled
    fxc_$${SHADER}.commands = $$FXC /nologo /E $${SHADER} /T $$eval($${SHADER}.type) /Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
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
    $$ANGLE_DIR/include/GLES2/gl2platform.h
gles2_headers.path = $$[QT_INSTALL_HEADERS]/QtANGLE/GLES2
gles3_headers.files = \
    $$ANGLE_DIR/include/GLES3/gl3.h \
    $$ANGLE_DIR/include/GLES3/gl3ext.h \
    $$ANGLE_DIR/include/GLES3/gl3platform.h
gles3_headers.path = $$[QT_INSTALL_HEADERS]/QtANGLE/GLES3
INSTALLS += khr_headers gles2_headers
angle_d3d11: INSTALLS += gles3_headers
