TEMPLATE = lib
CONFIG += static
TARGET = $$qtLibraryTarget(translator_hlsl)

include(../config.pri)

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
