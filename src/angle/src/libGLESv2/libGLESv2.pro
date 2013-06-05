TEMPLATE = lib
TARGET = $$qtLibraryTarget(libGLESv2)
CONFIG += simd

include(../common/common.pri)

INCLUDEPATH += $$OUT_PWD/.. $$ANGLE_DIR/src/libGLESv2

# Remember to adapt tools/configure/configureapp.cpp if the Direct X version changes.
angle_d3d11 {
    LIBS += -ldxgi -ld3d11
} else {
    LIBS += -ld3d9
}
LIBS += -ldxguid -ld3dcompiler
STATICLIBS = translator_common translator_hlsl preprocessor

for(libname, STATICLIBS) {
    # Appends 'd' to the library for debug builds and builds up the fully
    # qualified path to pass to the linker.
    staticlib = $$QT_BUILD_TREE/lib/$${QMAKE_PREFIX_STATICLIB}$$qtLibraryTarget($$libname).$${QMAKE_EXTENSION_STATICLIB}
    LIBS += $$staticlib
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
    $$ANGLE_DIR/src/libGLESv2/renderer/Renderer.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/TextureStorage.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/VertexBuffer.cpp \
    $$ANGLE_DIR/src/libGLESv2/renderer/VertexDataManager.cpp

SSE2_SOURCES += $$ANGLE_DIR/src/libGLESv2/renderer/ImageSSE2.cpp

angle_d3d11 {
    HEADERS += \
        $$ANGLE_DIR/src/libGLESv2/renderer/BufferStorage11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/Fence11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/Image11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/IndexBuffer11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/InputLayoutCache.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/Query11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/Renderer11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/renderer11_utils.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/RenderTarget11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/RenderStateCache.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/ShaderExecutable11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/SwapChain11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/TextureStorage11.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/VertexBuffer11.h

    SOURCES += \
        $$ANGLE_DIR/src/libGLESv2/renderer/BufferStorage11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/Fence11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/Image11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/IndexBuffer11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/InputLayoutCache.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/Query11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/Renderer11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/renderer11_utils.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/RenderTarget11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/RenderStateCache.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/ShaderExecutable11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/SwapChain11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/TextureStorage11.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/VertexBuffer11.cpp
} else {
    HEADERS += \
        $$ANGLE_DIR/src/libGLESv2/renderer/Blit.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/BufferStorage9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/Fence9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/Image9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/IndexBuffer9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/Query9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/Renderer9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/renderer9_utils.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/RenderTarget9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/ShaderExecutable9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/SwapChain9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/TextureStorage9.h \
        $$ANGLE_DIR/src/libGLESv2/renderer/VertexBuffer9.h

    SOURCES += \
        $$ANGLE_DIR/src/libGLESv2/renderer/Blit.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/BufferStorage9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/Fence9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/Image9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/IndexBuffer9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/Query9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/Renderer9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/renderer9_utils.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/RenderTarget9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/ShaderExecutable9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/SwapChain9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/TextureStorage9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/VertexBuffer9.cpp \
        $$ANGLE_DIR/src/libGLESv2/renderer/VertexDeclarationCache.cpp
}

!static {
    DEF_FILE = $$ANGLE_DIR/src/libGLESv2/$${TARGET}.def
    win32-g++*:equals(QT_ARCH, i386): DEF_FILE = $$ANGLE_DIR/src/libGLESv2/$${TARGET}_mingw32.def
}

float_converter.target = float_converter
float_converter.commands = python $$ANGLE_DIR/src/libGLESv2/Float16ToFloat32.py \
                > $$ANGLE_DIR/src/libGLESv2/Float16ToFloat32.cpp
QMAKE_EXTRA_TARGETS += float_converter

# Generate the shader header files.
PS_INPUT = $$ANGLE_DIR/src/libGLESv2/renderer/shaders/Blit.ps
VS_INPUT = $$ANGLE_DIR/src/libGLESv2/renderer/shaders/Blit.vs
PASSTHROUGH_INPUT = $$ANGLE_DIR/src/libGLESv2/renderer/shaders/Passthrough11.hlsl
CLEAR_INPUT = $$ANGLE_DIR/src/libGLESv2/renderer/shaders/Clear11.hlsl
PIXEL_SHADERS = passthroughps luminanceps componentmaskps
PIXEL_SHADERS_PASSTHROUGH = PassthroughRGBA PassthroughRGB \
                            PassthroughLum PassthroughLumAlpha
VERTEX_SHADERS = standardvs flipyvs
VERTEX_SHADERS_PASSTHROUGH = Passthrough
CLEAR_SHADERS = Clear
SHADER_DIR = $$OUT_PWD/renderer/shaders/compiled

for (ps, PIXEL_SHADERS) {
    fxc_ps_$${ps}.commands = $$FXC /nologo /E $$ps /T ps_2_0 /Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    fxc_ps_$${ps}.output = $$SHADER_DIR/$${ps}.h
    fxc_ps_$${ps}.input = PS_INPUT
    fxc_ps_$${ps}.dependency_type = TYPE_C
    fxc_ps_$${ps}.variable_out = HEADERS
    fxc_ps_$${ps}.CONFIG += target_predeps
    QMAKE_EXTRA_COMPILERS += fxc_ps_$${ps}
}
for (ps, PIXEL_SHADERS_PASSTHROUGH) {
    fxc_ps_$${ps}.commands = $$FXC /nologo /E PS_$$ps /T ps_4_0 /Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    fxc_ps_$${ps}.output = $$SHADER_DIR/$${ps}11ps.h
    fxc_ps_$${ps}.input = PASSTHROUGH_INPUT
    fxc_ps_$${ps}.dependency_type = TYPE_C
    fxc_ps_$${ps}.variable_out = HEADERS
    fxc_ps_$${ps}.CONFIG += target_predeps
    QMAKE_EXTRA_COMPILERS += fxc_ps_$${ps}
}
for (ps, CLEAR_SHADERS) {
    fxc_ps_$${ps}.commands = $$FXC /nologo /E PS_$$ps /T ps_4_0 /Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    fxc_ps_$${ps}.output = $$SHADER_DIR/$${ps}11ps.h
    fxc_ps_$${ps}.input = CLEAR_INPUT
    fxc_ps_$${ps}.dependency_type = TYPE_C
    fxc_ps_$${ps}.variable_out = HEADERS
    fxc_ps_$${ps}.CONFIG += target_predeps
    QMAKE_EXTRA_COMPILERS += fxc_ps_$${ps}
}
for (vs, VERTEX_SHADERS) {
    fxc_vs_$${vs}.commands = $$FXC /nologo /E $$vs /T vs_2_0 /Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    fxc_vs_$${vs}.output = $$SHADER_DIR/$${vs}.h
    fxc_vs_$${vs}.input = VS_INPUT
    fxc_vs_$${vs}.dependency_type = TYPE_C
    fxc_vs_$${vs}.variable_out = HEADERS
    fxc_vs_$${vs}.CONFIG += target_predeps
    QMAKE_EXTRA_COMPILERS += fxc_vs_$${vs}
}
for (vs, VERTEX_SHADERS_PASSTHROUGH) {
    fxc_vs_$${vs}.commands = $$FXC /nologo /E VS_$$vs /T vs_4_0 /Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    fxc_vs_$${vs}.output = $$SHADER_DIR/$${vs}11vs.h
    fxc_vs_$${vs}.input = PASSTHROUGH_INPUT
    fxc_vs_$${vs}.dependency_type = TYPE_C
    fxc_vs_$${vs}.variable_out = HEADERS
    fxc_vs_$${vs}.CONFIG += target_predeps
    QMAKE_EXTRA_COMPILERS += fxc_vs_$${vs}
}
for (vs, CLEAR_SHADERS) {
    fxc_vs_$${vs}.commands = $$FXC /nologo /E VS_$$vs /T vs_4_0 /Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    fxc_vs_$${vs}.output = $$SHADER_DIR/$${vs}11vs.h
    fxc_vs_$${vs}.input = CLEAR_INPUT
    fxc_vs_$${vs}.dependency_type = TYPE_C
    fxc_vs_$${vs}.variable_out = HEADERS
    fxc_vs_$${vs}.CONFIG += target_predeps
    QMAKE_EXTRA_COMPILERS += fxc_vs_$${vs}
}

load(qt_installs)

khr_headers.files = $$ANGLE_DIR/include/KHR/khrplatform.h
khr_headers.path = $$[QT_INSTALL_HEADERS]/QtANGLE/KHR
gles2_headers.files = \
    $$ANGLE_DIR/include/GLES2/gl2.h \
    $$ANGLE_DIR/include/GLES2/gl2ext.h \
    $$ANGLE_DIR/include/GLES2/gl2platform.h
gles2_headers.path = $$[QT_INSTALL_HEADERS]/QtANGLE/GLES2
INSTALLS += khr_headers gles2_headers


