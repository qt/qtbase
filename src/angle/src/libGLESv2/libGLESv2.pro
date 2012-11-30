TEMPLATE = lib
TARGET = $$qtLibraryTarget(libGLESv2)
DEPENDPATH += . shaders
CONFIG += simd

include(../common/common.pri)

INCLUDEPATH += $$OUT_PWD/..

# Remember to adapt tools/configure/configureapp.cpp if the Direct X version changes.
LIBS += -ld3d9 -ld3dcompiler
STATICLIBS = translator_common translator_hlsl preprocessor

for(libname, STATICLIBS) {
    # Appends 'd' to the library for debug builds and builds up the fully
    # qualified path to pass to the linker.
    staticlib = $$QT_BUILD_TREE/lib/$${QMAKE_PREFIX_STATICLIB}$$qtLibraryTarget($$libname).$${QMAKE_EXTENSION_STATICLIB}
    LIBS += $$staticlib
    PRE_TARGETDEPS += $$staticlib
}

HEADERS += \
    $$ANGLE_DIR/src/libGLESv2/BinaryStream.h \
    $$ANGLE_DIR/src/libGLESv2/Blit.h \
    $$ANGLE_DIR/src/libGLESv2/Buffer.h \
    $$ANGLE_DIR/src/libGLESv2/Context.h \
    $$ANGLE_DIR/src/libGLESv2/D3DConstantTable.h \
    $$ANGLE_DIR/src/libGLESv2/Fence.h \
    $$ANGLE_DIR/src/libGLESv2/Framebuffer.h \
    $$ANGLE_DIR/src/libGLESv2/HandleAllocator.h \
    $$ANGLE_DIR/src/libGLESv2/IndexDataManager.h \
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
    $$ANGLE_DIR/src/libGLESv2/utilities.h \
    $$ANGLE_DIR/src/libGLESv2/vertexconversion.h \
    $$ANGLE_DIR/src/libGLESv2/VertexDataManager.h

SOURCES += \
    $$ANGLE_DIR/src/libGLESv2/Blit.cpp \
    $$ANGLE_DIR/src/libGLESv2/Buffer.cpp \
    $$ANGLE_DIR/src/libGLESv2/Context.cpp \
    $$ANGLE_DIR/src/libGLESv2/D3DConstantTable.cpp \
    $$ANGLE_DIR/src/libGLESv2/Fence.cpp \
    $$ANGLE_DIR/src/libGLESv2/Framebuffer.cpp \
    $$ANGLE_DIR/src/libGLESv2/Float16ToFloat32.cpp \
    $$ANGLE_DIR/src/libGLESv2/HandleAllocator.cpp \
    $$ANGLE_DIR/src/libGLESv2/IndexDataManager.cpp \
    $$ANGLE_DIR/src/libGLESv2/libGLESv2.cpp \
    $$ANGLE_DIR/src/libGLESv2/main.cpp \
    $$ANGLE_DIR/src/libGLESv2/Program.cpp \
    $$ANGLE_DIR/src/libGLESv2/ProgramBinary.cpp \
    $$ANGLE_DIR/src/libGLESv2/Query.cpp \
    $$ANGLE_DIR/src/libGLESv2/Renderbuffer.cpp \
    $$ANGLE_DIR/src/libGLESv2/ResourceManager.cpp \
    $$ANGLE_DIR/src/libGLESv2/Shader.cpp \
    $$ANGLE_DIR/src/libGLESv2/Texture.cpp \
    $$ANGLE_DIR/src/libGLESv2/utilities.cpp \
    $$ANGLE_DIR/src/libGLESv2/VertexDataManager.cpp

SSE2_SOURCES += $$ANGLE_DIR/src/libGLESv2/TextureSSE2.cpp

msvc:DEF_FILE = $$ANGLE_DIR/src/libGLESv2/$${TARGET}.def

float_converter.target = float_converter
float_converter.commands = python $$ANGLE_DIR/src/libGLESv2/Float16ToFloat32.py \
                > $$ANGLE_DIR/src/libGLESv2/Float16ToFloat32.cpp
QMAKE_EXTRA_TARGETS += float_converter

# Generate the shader header files.
PS_INPUT = $$ANGLE_DIR/src/libGLESv2/shaders/Blit.ps
VS_INPUT = $$ANGLE_DIR/src/libGLESv2/shaders/Blit.vs
PIXEL_SHADERS = passthroughps luminanceps componentmaskps
VERTEX_SHADERS = standardvs flipyvs
SHADER_DIR = $$OUT_PWD/shaders

for (ps, PIXEL_SHADERS) {
    fxc_$${ps}.commands = $$FXC /nologo /E $$ps /T ps_2_0 /Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    fxc_$${ps}.output = $$SHADER_DIR/$${ps}.h
    fxc_$${ps}.input = PS_INPUT
    fxc_$${ps}.dependency_type = TYPE_C
    fxc_$${ps}.variable_out = HEADERS
    QMAKE_EXTRA_COMPILERS += fxc_$${ps}
}
for (vs, VERTEX_SHADERS) {
    fxc_$${vs}.commands = $$FXC /nologo /E $$vs /T vs_2_0 /Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
    fxc_$${vs}.output = $$SHADER_DIR/$${vs}.h
    fxc_$${vs}.input = VS_INPUT
    fxc_$${vs}.dependency_type = TYPE_C
    fxc_$${vs}.variable_out = HEADERS
    QMAKE_EXTRA_COMPILERS += fxc_$${vs}
}

load(qt_installs)

khr_headers.files = $$ANGLE_DIR/include/KHR/khrplatform.h
khr_headers.path = $$[QT_INSTALL_HEADERS]/KHR
gles2_headers.files = \
    $$ANGLE_DIR/include/GLES2/gl2.h \
    $$ANGLE_DIR/include/GLES2/gl2ext.h \
    $$ANGLE_DIR/include/GLES2/gl2platform.h
gles2_headers.path = $$[QT_INSTALL_HEADERS]/GLES2
INSTALLS += khr_headers gles2_headers


