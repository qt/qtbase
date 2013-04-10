TEMPLATE = lib
CONFIG += static
TARGET = $$qtLibraryTarget(translator_common)

include(../config.pri)

INCLUDEPATH += \
    $$ANGLE_DIR/src \
    $$ANGLE_DIR/include

DEFINES += _SECURE_SCL=0 _LIB COMPILER_IMPLEMENTATION

FLEX_SOURCES = $$ANGLE_DIR/src/compiler/glslang.l
BISON_SOURCES = $$ANGLE_DIR/src/compiler/glslang.y

HEADERS += \
    $$ANGLE_DIR/src/compiler/BaseTypes.h \
    $$ANGLE_DIR/src/compiler/BuiltInFunctionEmulator.h \
    $$ANGLE_DIR/src/compiler/Common.h \
    $$ANGLE_DIR/src/compiler/ConstantUnion.h \
    $$ANGLE_DIR/src/compiler/debug.h \
    $$ANGLE_DIR/src/compiler/DetectRecursion.h \
    $$ANGLE_DIR/src/compiler/Diagnostics.h \
    $$ANGLE_DIR/src/compiler/DirectiveHandler.h \
    $$ANGLE_DIR/src/compiler/ForLoopUnroll.h \
    $$ANGLE_DIR/src/compiler/InfoSink.h \
    $$ANGLE_DIR/src/compiler/Initialize.h \
    $$ANGLE_DIR/src/compiler/InitializeDll.h \
    $$ANGLE_DIR/src/compiler/InitializeGlobals.h \
    $$ANGLE_DIR/src/compiler/InitializeParseContext.h \
    $$ANGLE_DIR/src/compiler/intermediate.h \
    $$ANGLE_DIR/src/compiler/localintermediate.h \
    $$ANGLE_DIR/src/compiler/MapLongVariableNames.h \
    $$ANGLE_DIR/src/compiler/MMap.h \
    $$ANGLE_DIR/src/compiler/osinclude.h \
    $$ANGLE_DIR/src/compiler/ParseHelper.h \
    $$ANGLE_DIR/src/compiler/PoolAlloc.h \
    $$ANGLE_DIR/src/compiler/QualifierAlive.h \
    $$ANGLE_DIR/src/compiler/RemoveTree.h \
    $$ANGLE_DIR/src/compiler/RenameFunction.h \
    $$ANGLE_DIR/include/GLSLANG/ResourceLimits.h \
    $$ANGLE_DIR/include/GLSLANG/ShaderLang.h \
    $$ANGLE_DIR/src/compiler/ShHandle.h \
    $$ANGLE_DIR/src/compiler/SymbolTable.h \
    $$ANGLE_DIR/src/compiler/Types.h \
    $$ANGLE_DIR/src/compiler/UnfoldShortCircuit.h \
    $$ANGLE_DIR/src/compiler/util.h \
    $$ANGLE_DIR/src/compiler/ValidateLimitations.h \
    $$ANGLE_DIR/src/compiler/VariableInfo.h \
    $$ANGLE_DIR/src/compiler/VariablePacker.h \
    $$ANGLE_DIR/src/compiler/timing/RestrictFragmentShaderTiming.h \
    $$ANGLE_DIR/src/compiler/timing/RestrictVertexShaderTiming.h \
    $$ANGLE_DIR/src/compiler/depgraph/DependencyGraph.h \
    $$ANGLE_DIR/src/compiler/depgraph/DependencyGraphBuilder.h \
    $$ANGLE_DIR/src/compiler/depgraph/DependencyGraphOutput.h \
    $$ANGLE_DIR/src/third_party/compiler/ArrayBoundsClamper.h

SOURCES += \
    $$ANGLE_DIR/src/compiler/BuiltInFunctionEmulator.cpp \
    $$ANGLE_DIR/src/compiler/Compiler.cpp \
    $$ANGLE_DIR/src/compiler/debug.cpp \
    $$ANGLE_DIR/src/compiler/DetectRecursion.cpp \
    $$ANGLE_DIR/src/compiler/Diagnostics.cpp \
    $$ANGLE_DIR/src/compiler/DirectiveHandler.cpp \
    $$ANGLE_DIR/src/compiler/ForLoopUnroll.cpp \
    $$ANGLE_DIR/src/compiler/InfoSink.cpp \
    $$ANGLE_DIR/src/compiler/Initialize.cpp \
    $$ANGLE_DIR/src/compiler/InitializeDll.cpp \
    $$ANGLE_DIR/src/compiler/InitializeParseContext.cpp \
    $$ANGLE_DIR/src/compiler/Intermediate.cpp \
    $$ANGLE_DIR/src/compiler/intermOut.cpp \
    $$ANGLE_DIR/src/compiler/IntermTraverse.cpp \
    $$ANGLE_DIR/src/compiler/MapLongVariableNames.cpp \
    $$ANGLE_DIR/src/compiler/ossource_win.cpp \
    $$ANGLE_DIR/src/compiler/parseConst.cpp \
    $$ANGLE_DIR/src/compiler/ParseHelper.cpp \
    $$ANGLE_DIR/src/compiler/PoolAlloc.cpp \
    $$ANGLE_DIR/src/compiler/QualifierAlive.cpp \
    $$ANGLE_DIR/src/compiler/RemoveTree.cpp \
    $$ANGLE_DIR/src/compiler/ShaderLang.cpp \
    $$ANGLE_DIR/src/compiler/SymbolTable.cpp \
    $$ANGLE_DIR/src/compiler/util.cpp \
    $$ANGLE_DIR/src/compiler/ValidateLimitations.cpp \
    $$ANGLE_DIR/src/compiler/VariableInfo.cpp \
    $$ANGLE_DIR/src/compiler/VariablePacker.cpp \
    $$ANGLE_DIR/src/compiler/depgraph/DependencyGraph.cpp \
    $$ANGLE_DIR/src/compiler/depgraph/DependencyGraphBuilder.cpp \
    $$ANGLE_DIR/src/compiler/depgraph/DependencyGraphOutput.cpp \
    $$ANGLE_DIR/src/compiler/depgraph/DependencyGraphTraverse.cpp \
    $$ANGLE_DIR/src/compiler/timing/RestrictFragmentShaderTiming.cpp \
    $$ANGLE_DIR/src/compiler/timing/RestrictVertexShaderTiming.cpp \
    $$ANGLE_DIR/src/third_party/compiler/ArrayBoundsClamper.cpp

# NOTE: 'win_flex' and 'bison' can be found in qt5/gnuwin32/bin
flex.commands = $$addGnuPath(win_flex) --noline --nounistd --outfile=${QMAKE_FILE_BASE}_lex.cpp ${QMAKE_FILE_NAME}
flex.output = ${QMAKE_FILE_BASE}_lex.cpp
flex.input = FLEX_SOURCES
flex.dependency_type = TYPE_C
flex.variable_out = GENERATED_SOURCES
QMAKE_EXTRA_COMPILERS += flex

bison.commands = $$addGnuPath(bison) --no-lines --skeleton=yacc.c --defines=${QMAKE_FILE_BASE}_tab.h \
                --output=${QMAKE_FILE_BASE}_tab.cpp ${QMAKE_FILE_NAME}
bison.output = ${QMAKE_FILE_BASE}_tab.h
bison.input = BISON_SOURCES
bison.dependency_type = TYPE_C
bison.variable_out = GENERATED_SOURCES
QMAKE_EXTRA_COMPILERS += bison

# This is a dummy compiler to work around the fact that an extra compiler can only
# have one output file even if the command generates two.
MAKEFILE_NOOP_COMMAND = @echo -n
msvc: MAKEFILE_NOOP_COMMAND = @echo >NUL
bison_impl.output = ${QMAKE_FILE_BASE}_tab.cpp
bison_impl.input = BISON_SOURCES
bison_impl.commands = $$MAKEFILE_NOOP_COMMAND
bison_impl.depends = ${QMAKE_FILE_BASE}_tab.h
bison_impl.output = ${QMAKE_FILE_BASE}_tab.cpp
bison_impl.variable_out = GENERATED_SOURCES
QMAKE_EXTRA_COMPILERS += bison_impl

