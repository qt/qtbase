CONFIG += simd installed
include(../common/common.pri)

INCLUDEPATH += $$OUT_PWD/.. $$ANGLE_DIR/src/libGLESv2

# Remember to adapt tools/configure/configureapp.cpp if the Direct X version changes.
angle_d3d11: \
    LIBS_PRIVATE += -ldxgi -ld3d11
!winrt: \
    LIBS_PRIVATE += -ld3d9

LIBS_PRIVATE += -ldxguid

STATICLIBS = translator preprocessor
for(libname, STATICLIBS) {
    # Appends 'd' to the library for debug builds and builds up the fully
    # qualified path to pass to the linker.
    staticlib = $$QT_BUILD_TREE/lib/$${QMAKE_PREFIX_STATICLIB}$$qtLibraryTarget($$libname).$${QMAKE_EXTENSION_STATICLIB}
    LIBS_PRIVATE += $$staticlib
    PRE_TARGETDEPS += $$staticlib
}

HEADERS += \
    $$ANGLE_DIR/src/third_party/murmurhash/MurmurHash3.h \
    $$ANGLE_DIR/src/libGLESv2/BinaryStream.h \
    $$ANGLE_DIR/src/libGLESv2/Buffer.h \
    $$ANGLE_DIR/src/libGLESv2/Context.h \
    $$ANGLE_DIR/src/libGLESv2/Fence.h \
    $$ANGLE_DIR/src/libGLESv2/Framebuffer.h \
    $$ANGLE_DIR/src/libGLESv2/HandleAllocator.h \
    $$ANGLE_DIR/src/libGLESv2/main.h \
    $$ANGLE_DIR/src/libGLESv2/mathutil.h \
    $$ANGLE_DIR/src/libGLESv2/Program.h \
    $$ANGLE_DIR/src/libGLESv2/ProgramBinary.h \
    $$ANGLE_DIR/src/libGLESv2/Query.h \
    $$ANGLE_DIR/src/libGLESv2/Renderbuffer.h \
    $$ANGLE_DIR/src/libGLESv2/resource.h \
    $$ANGLE_DIR/src/libGLESv2/ResourceManager.h \
    $$ANGLE_DIR/src/libGLESv2/Shader.h \
    $$ANGLE_DIR/src/libGLESv2/Texture.h \
    $$ANGLE_DIR/src/libGLESv2/Uniform.h \
    $$ANGLE_DIR/src/libGLESv2/utilities.h \
    $$ANGLE_DIR/src/libGLESv2/vertexconversion.h \
    $$ANGLE_DIR/src/libGLESv2/precompiled.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/BufferStorage.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/Image.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/IndexBuffer.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/IndexDataManager.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/IndexCacheRange.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/Renderer.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/RenderTarget.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/ShaderExecutable.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/SwapChain.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/TextureStorage.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/VertexBuffer.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/VertexDataManager.h \
    $$ANGLE_DIR/src/libGLESv2/renderer/VertexDeclarationCache.h

SOURCES += \
    $$ANGLE_DIR/src/third_party/murmurhash/MurmurHash3.cpp \
    $$ANGLE_DIR/src/libGLESv2/Buffer.cpp \
    $$ANGLE_DIR/src/libGLESv2/Context.cpp \
    $$ANGLE_DIR/src/libGLESv2/Fence.cpp \
    $$ANGLE_DIR/src/libGLESv2/Float16ToFloat32.cpp \
    $$ANGLE_DIR/src/libGLESv2/Framebuffer.cpp \
    $$ANGLE_DIR/src/libGLESv2/HandleAllocator.cpp \
    $$ANGLE_DIR/src/libGLESv2/libGLESv2.cpp \
    $$ANGLE_DIR/src/libGLESv2/main.cpp \
    $$ANGLE_DIR/src/libGLESv2/Program.cpp \
    $$ANGLE_DIR/src/libGLESv2/ProgramBinary.cpp \
    $$ANGLE_DIR/src/libGLESv2/Query.cpp \
    $$ANGLE_DIR/src/libGLESv2/Renderbuffer.cpp \
    $$ANGLE_DIR/src/libGLESv2/ResourceManager.cpp \
    $$ANGLE_DIR/src/libGLESv2/Shader.cpp \
    $$ANGLE_DIR/src/libGLESv2/Texture.cpp \
    $$ANGLE_DIR/src/libGLESv2/Uniform.cpp \
    $$ANGLE_DIR/src/libGLESv2/utilities.cpp \
    $$ANGLE_DIR/src/libGLESv2/precompiled.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/BufferStorage.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/Image.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/IndexBuffer.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/IndexDataManager.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/IndexRangeCache.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/Renderer.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/TextureStorage.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/VertexBuffer.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/VertexDataManager.cpp

SSE2_SOURCES += $$ANGLE_DIR/src/libGLESv2/renderer/ImageSSE2.cpp

angle_d3d11 {
    HEADERS += \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/BufferStorage11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/Fence11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/Image11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/IndexBuffer11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/InputLayoutCache.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/Query11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/Renderer11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/renderer11_utils.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/RenderTarget11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/RenderStateCache.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/ShaderExecutable11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/SwapChain11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/TextureStorage11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/VertexBuffer11.h

    SOURCES += \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/BufferStorage11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/Fence11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/Image11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/IndexBuffer11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/InputLayoutCache.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/Query11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/Renderer11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/renderer11_utils.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/RenderTarget11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/RenderStateCache.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/ShaderExecutable11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/SwapChain11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/TextureStorage11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/VertexBuffer11.cpp
}

!winrt {
    HEADERS += \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/Blit.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/BufferStorage9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/Fence9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/Image9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/IndexBuffer9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/Query9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/Renderer9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/renderer9_utils.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/RenderTarget9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/ShaderExecutable9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/SwapChain9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/TextureStorage9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/VertexBuffer9.h

    SOURCES += \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/Blit.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/BufferStorage9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/Fence9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/Image9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/IndexBuffer9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/Query9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/Renderer9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/renderer9_utils.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/RenderTarget9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/ShaderExecutable9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/SwapChain9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/TextureStorage9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/VertexBuffer9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/VertexDeclarationCache.cpp
}

!static {
    DEF_FILE = $$ANGLE_DIR/src/libGLESv2/$${TARGET}.def
    mingw:equals(QT_ARCH, i386): DEF_FILE = $$ANGLE_DIR/src/libGLESv2/$${TARGET}_mingw32.def
}

float_converter.target = float_converter
float_converter.commands = python $$ANGLE_DIR/src/libGLESv2/Float16ToFloat32.py \
                > $$ANGLE_DIR/src/libGLESv2/Float16ToFloat32.cpp
QMAKE_EXTRA_TARGETS += float_converter

# Generate the shader header files.
PS_BLIT_INPUT = $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/shaders/Blit.ps
VS_BLIT_INPUT = $$ANGLE_DIR/src/libGLESv2/renderer/d3d9/shaders/Blit.vs
PASSTHROUGH_INPUT = $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/shaders/Passthrough11.hlsl
CLEAR_INPUT = $$ANGLE_DIR/src/libGLESv2/renderer/d3d11/shaders/Clear11.hlsl
PIXEL_SHADERS_BLIT = passthroughps luminanceps componentmaskps
PIXEL_SHADERS_PASSTHROUGH = PassthroughRGBA PassthroughRGB \
                            PassthroughLum PassthroughLumAlpha
PIXEL_SHADERS_CLEAR = ClearSingle ClearMultiple
VERTEX_SHADERS_BLIT = standardvs flipyvs
VERTEX_SHADERS_PASSTHROUGH = Passthrough
VERTEX_SHADERS_CLEAR = Clear
SHADER_DIR_9 = $$OUT_PWD/renderer/d3d9/shaders/compiled
SHADER_DIR_11 = $$OUT_PWD/renderer/d3d11/shaders/compiled

for (ps, PIXEL_SHADERS_BLIT) {
    fxc_ps_$${ps}.commands = $$FXC /nologo /E $$ps /T ps_2_0 /Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    fxc_ps_$${ps}.output = $$SHADER_DIR_9/$${ps}.h
    fxc_ps_$${ps}.input = PS_BLIT_INPUT
    fxc_ps_$${ps}.dependency_type = TYPE_C
    fxc_ps_$${ps}.variable_out = HEADERS
    fxc_ps_$${ps}.CONFIG += target_predeps
    !winrt: QMAKE_EXTRA_COMPILERS += fxc_ps_$${ps}
}
for (ps, PIXEL_SHADERS_PASSTHROUGH) {
    fxc_ps_$${ps}.commands = $$FXC /nologo /E PS_$$ps /T ps_4_0_level_9_1 /Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    fxc_ps_$${ps}.output = $$SHADER_DIR_11/$${ps}11ps.h
    fxc_ps_$${ps}.input = PASSTHROUGH_INPUT
    fxc_ps_$${ps}.dependency_type = TYPE_C
    fxc_ps_$${ps}.variable_out = HEADERS
    fxc_ps_$${ps}.CONFIG += target_predeps
    angle_d3d11: QMAKE_EXTRA_COMPILERS += fxc_ps_$${ps}
}
for (ps, PIXEL_SHADERS_CLEAR) {
    fxc_ps_$${ps}.commands = $$FXC /nologo /E PS_$$ps /T ps_4_0_level_9_1 /Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    fxc_ps_$${ps}.output = $$SHADER_DIR_11/$${ps}11ps.h
    fxc_ps_$${ps}.input = CLEAR_INPUT
    fxc_ps_$${ps}.dependency_type = TYPE_C
    fxc_ps_$${ps}.variable_out = HEADERS
    fxc_ps_$${ps}.CONFIG += target_predeps
    angle_d3d11: QMAKE_EXTRA_COMPILERS += fxc_ps_$${ps}
}
for (vs, VERTEX_SHADERS_BLIT) {
    fxc_vs_$${vs}.commands = $$FXC /nologo /E $$vs /T vs_2_0 /Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    fxc_vs_$${vs}.output = $$SHADER_DIR_9/$${vs}.h
    fxc_vs_$${vs}.input = VS_BLIT_INPUT
    fxc_vs_$${vs}.dependency_type = TYPE_C
    fxc_vs_$${vs}.variable_out = HEADERS
    fxc_vs_$${vs}.CONFIG += target_predeps
    !winrt: QMAKE_EXTRA_COMPILERS += fxc_vs_$${vs}
}
for (vs, VERTEX_SHADERS_PASSTHROUGH) {
    fxc_vs_$${vs}.commands = $$FXC /nologo /E VS_$$vs /T vs_4_0_level_9_1 /Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    fxc_vs_$${vs}.output = $$SHADER_DIR_11/$${vs}11vs.h
    fxc_vs_$${vs}.input = PASSTHROUGH_INPUT
    fxc_vs_$${vs}.dependency_type = TYPE_C
    fxc_vs_$${vs}.variable_out = HEADERS
    fxc_vs_$${vs}.CONFIG += target_predeps
    angle_d3d11: QMAKE_EXTRA_COMPILERS += fxc_vs_$${vs}
}
for (vs, VERTEX_SHADERS_CLEAR) {
    fxc_vs_$${vs}.commands = $$FXC /nologo /E VS_$$vs /T vs_4_0_level_9_1 /Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    fxc_vs_$${vs}.output = $$SHADER_DIR_11/$${vs}11vs.h
    fxc_vs_$${vs}.input = CLEAR_INPUT
    fxc_vs_$${vs}.dependency_type = TYPE_C
    fxc_vs_$${vs}.variable_out = HEADERS
    fxc_vs_$${vs}.CONFIG += target_predeps
    angle_d3d11: QMAKE_EXTRA_COMPILERS += fxc_vs_$${vs}
}

khr_headers.files = $$ANGLE_DIR/include/KHR/khrplatform.h
khr_headers.path = $$[QT_INSTALL_HEADERS]/QtANGLE/KHR
gles2_headers.files = \
    $$ANGLE_DIR/include/GLES2/gl2.h \
    $$ANGLE_DIR/include/GLES2/gl2ext.h \
    $$ANGLE_DIR/include/GLES2/gl2platform.h
gles2_headers.path = $$[QT_INSTALL_HEADERS]/QtANGLE/GLES2
INSTALLS += khr_headers gles2_headers
