TEMPLATE = lib
CONFIG += static
TARGET = $$qtLibraryTarget(translator_hlsl)

include(../config.pri)

# Mingw 4.7 chokes on implicit move semantics, so disable C++11 here
mingw: CONFIG -= c++11

INCLUDEPATH +=  $$ANGLE_DIR/src \
                $$ANGLE_DIR/include

DEFINES += COMPILER_IMPLEMENTATION

HEADERS += \
    $$ANGLE_DIR/src/compiler/DetectDiscontinuity.h \
    $$ANGLE_DIR/src/compiler/OutputHLSL.h \
    $$ANGLE_DIR/src/compiler/SearchSymbol.h \
    $$ANGLE_DIR/src/compiler/TranslatorHLSL.h \
    $$ANGLE_DIR/src/compiler/UnfoldShortCircuit.h \
    $$ANGLE_DIR/src/compiler/Uniform.h

SOURCES += \
    $$ANGLE_DIR/src/compiler/CodeGenHLSL.cpp \
    $$ANGLE_DIR/src/compiler/DetectDiscontinuity.cpp \
    $$ANGLE_DIR/src/compiler/OutputHLSL.cpp \
    $$ANGLE_DIR/src/compiler/SearchSymbol.cpp \
    $$ANGLE_DIR/src/compiler/TranslatorHLSL.cpp \
    $$ANGLE_DIR/src/compiler/UnfoldShortCircuit.cpp \
    $$ANGLE_DIR/src/compiler/Uniform.cpp
