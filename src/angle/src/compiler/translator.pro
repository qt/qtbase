CONFIG += static
include(../config.pri)

INCLUDEPATH += \
    $$ANGLE_DIR/src \
    $$ANGLE_DIR/include

DEFINES += _SECURE_SCL=0 _LIB ANGLE_TRANSLATOR_IMPLEMENTATION ANGLE_TRANSLATOR_STATIC ANGLE_ENABLE_HLSL

FLEX_SOURCES = $$ANGLE_DIR/src/compiler/translator/glslang.l
BISON_SOURCES = $$ANGLE_DIR/src/compiler/translator/glslang.y

HEADERS += \
    $$ANGLE_DIR/include/GLSLANG/ResourceLimits.h \
    $$ANGLE_DIR/include/GLSLANG/ShaderLang.h \
    $$ANGLE_DIR/include/GLSLANG/ShaderVars.h \
    $$ANGLE_DIR/src/common/angleutils.h \
    $$ANGLE_DIR/src/common/blocklayout.h \
    $$ANGLE_DIR/src/common/debug.h \
    $$ANGLE_DIR/src/common/platform.h \
    $$ANGLE_DIR/src/common/tls.h \
    $$ANGLE_DIR/src/common/utilities.h \
    $$ANGLE_DIR/src/compiler/translator/ArrayReturnValueToOutParameter.h \
    $$ANGLE_DIR/src/compiler/translator/ASTMetadataHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/blocklayout.h \
    $$ANGLE_DIR/src/compiler/translator/blocklayoutHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/BaseTypes.h \
    $$ANGLE_DIR/src/compiler/translator/BuiltInFunctionEmulator.h \
    $$ANGLE_DIR/src/compiler/translator/BuiltInFunctionEmulatorGLSL.h \
    $$ANGLE_DIR/src/compiler/translator/BuiltInFunctionEmulatorHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/Cache.h \
    $$ANGLE_DIR/src/compiler/translator/CallDAG.h \
    $$ANGLE_DIR/src/compiler/translator/Common.h \
    $$ANGLE_DIR/src/compiler/translator/Compiler.h \
    $$ANGLE_DIR/src/compiler/translator/ConstantUnion.h \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraphBuilder.h \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraph.h \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraphOutput.h \
    $$ANGLE_DIR/src/compiler/translator/Diagnostics.h \
    $$ANGLE_DIR/src/compiler/translator/DirectiveHandler.h \
    $$ANGLE_DIR/src/compiler/translator/ExtensionBehavior.h \
    $$ANGLE_DIR/src/compiler/translator/EmulatePrecision.h \
    $$ANGLE_DIR/src/compiler/translator/FlagStd140Structs.h \
    $$ANGLE_DIR/src/compiler/translator/ForLoopUnroll.h \
    $$ANGLE_DIR/src/compiler/translator/HashNames.h \
    $$ANGLE_DIR/src/compiler/translator/InfoSink.h \
    $$ANGLE_DIR/src/compiler/translator/InitializeDll.h \
    $$ANGLE_DIR/src/compiler/translator/Initialize.h \
    $$ANGLE_DIR/src/compiler/translator/InitializeParseContext.h \
    $$ANGLE_DIR/src/compiler/translator/InitializeVariables.h \
    $$ANGLE_DIR/src/compiler/translator/intermediate.h \
    $$ANGLE_DIR/src/compiler/translator/IntermNode.h \
    $$ANGLE_DIR/src/compiler/translator/LoopInfo.h \
    $$ANGLE_DIR/src/compiler/translator/MMap.h \
    $$ANGLE_DIR/src/compiler/translator/NodeSearch.h \
    $$ANGLE_DIR/src/compiler/translator/osinclude.h \
    $$ANGLE_DIR/src/compiler/translator/Operator.h \
    $$ANGLE_DIR/src/compiler/translator/OutputESSL.h \
    $$ANGLE_DIR/src/compiler/translator/OutputGLSLBase.h \
    $$ANGLE_DIR/src/compiler/translator/OutputGLSL.h \
    $$ANGLE_DIR/src/compiler/translator/OutputHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/ParseContext.h \
    $$ANGLE_DIR/src/compiler/translator/PoolAlloc.h \
    $$ANGLE_DIR/src/compiler/translator/PruneEmptyDeclarations.h \
    $$ANGLE_DIR/src/compiler/translator/Pragma.h \
    $$ANGLE_DIR/src/compiler/translator/RegenerateStructNames.h \
    $$ANGLE_DIR/src/compiler/translator/RemovePow.h \
    $$ANGLE_DIR/src/compiler/translator/RemoveDynamicIndexing.h \
    $$ANGLE_DIR/src/compiler/translator/RemoveSwitchFallThrough.h \
    $$ANGLE_DIR/src/compiler/translator/RenameFunction.h \
    $$ANGLE_DIR/src/compiler/translator/RewriteDoWhile.h \
    $$ANGLE_DIR/src/compiler/translator/RewriteElseBlocks.h \
    $$ANGLE_DIR/src/compiler/translator/SeparateArrayInitialization.h \
    $$ANGLE_DIR/src/compiler/translator/SeparateDeclarations.h \
    $$ANGLE_DIR/src/compiler/translator/ScalarizeVecAndMatConstructorArgs.h \
    $$ANGLE_DIR/src/compiler/translator/SearchSymbol.h \
    $$ANGLE_DIR/src/compiler/translator/ShHandle.h \
    $$ANGLE_DIR/src/compiler/translator/SeparateExpressionsReturningArrays.h \
    $$ANGLE_DIR/src/compiler/translator/StructureHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/SymbolTable.h \
    $$ANGLE_DIR/src/compiler/translator/timing/RestrictFragmentShaderTiming.h \
    $$ANGLE_DIR/src/compiler/translator/timing/RestrictVertexShaderTiming.h \
    $$ANGLE_DIR/src/compiler/translator/TranslatorESSL.h \
    $$ANGLE_DIR/src/compiler/translator/TranslatorGLSL.h \
    $$ANGLE_DIR/src/compiler/translator/TranslatorHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/Types.h \
    $$ANGLE_DIR/src/compiler/translator/UnfoldShortCircuitAST.h \
    $$ANGLE_DIR/src/compiler/translator/UnfoldShortCircuitToIf.h \
    $$ANGLE_DIR/src/compiler/translator/UniformHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/util.h \
    $$ANGLE_DIR/src/compiler/translator/UtilsHLSL.h \
    $$ANGLE_DIR/src/compiler/translator/ValidateGlobalInitializer.h \
    $$ANGLE_DIR/src/compiler/translator/ValidateLimitations.h \
    $$ANGLE_DIR/src/compiler/translator/ValidateOutputs.h \
    $$ANGLE_DIR/src/compiler/translator/ValidateSwitch.h \
    $$ANGLE_DIR/src/compiler/translator/VariableInfo.h \
    $$ANGLE_DIR/src/compiler/translator/VariablePacker.h \
    $$ANGLE_DIR/src/compiler/translator/VersionGLSL.h \
    $$ANGLE_DIR/src/third_party/compiler/ArrayBoundsClamper.h


SOURCES += \
    $$ANGLE_DIR/src/common/debug.cpp \
    $$ANGLE_DIR/src/common/tls.cpp \
    $$ANGLE_DIR/src/compiler/translator/ArrayReturnValueToOutParameter.cpp \
    $$ANGLE_DIR/src/compiler/translator/ASTMetadataHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/blocklayout.cpp \
    $$ANGLE_DIR/src/compiler/translator/blocklayoutHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/BuiltInFunctionEmulator.cpp \
    $$ANGLE_DIR/src/compiler/translator/BuiltInFunctionEmulatorGLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/BuiltInFunctionEmulatorHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/Cache.cpp \
    $$ANGLE_DIR/src/compiler/translator/CallDAG.cpp \
    $$ANGLE_DIR/src/compiler/translator/CodeGen.cpp \
    $$ANGLE_DIR/src/compiler/translator/Compiler.cpp \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraph.cpp \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraphBuilder.cpp \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraphOutput.cpp \
    $$ANGLE_DIR/src/compiler/translator/depgraph/DependencyGraphTraverse.cpp \
    $$ANGLE_DIR/src/compiler/translator/Diagnostics.cpp \
    $$ANGLE_DIR/src/compiler/translator/DirectiveHandler.cpp \
    $$ANGLE_DIR/src/compiler/translator/EmulatePrecision.cpp \
    $$ANGLE_DIR/src/compiler/translator/FlagStd140Structs.cpp \
    $$ANGLE_DIR/src/compiler/translator/ForLoopUnroll.cpp \
    $$ANGLE_DIR/src/compiler/translator/InfoSink.cpp \
    $$ANGLE_DIR/src/compiler/translator/Initialize.cpp \
    $$ANGLE_DIR/src/compiler/translator/InitializeDll.cpp \
    $$ANGLE_DIR/src/compiler/translator/InitializeParseContext.cpp \
    $$ANGLE_DIR/src/compiler/translator/InitializeVariables.cpp \
    $$ANGLE_DIR/src/compiler/translator/Intermediate.cpp \
    $$ANGLE_DIR/src/compiler/translator/IntermNode.cpp \
    $$ANGLE_DIR/src/compiler/translator/intermOut.cpp \
    $$ANGLE_DIR/src/compiler/translator/IntermTraverse.cpp \
    $$ANGLE_DIR/src/compiler/translator/LoopInfo.cpp \
    $$ANGLE_DIR/src/compiler/translator/Operator.cpp \
    $$ANGLE_DIR/src/compiler/translator/OutputESSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/OutputGLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/OutputGLSLBase.cpp \
    $$ANGLE_DIR/src/compiler/translator/OutputHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/ParseContext.cpp \
    $$ANGLE_DIR/src/compiler/translator/PoolAlloc.cpp \
    $$ANGLE_DIR/src/compiler/translator/PruneEmptyDeclarations.cpp \
    $$ANGLE_DIR/src/compiler/translator/RegenerateStructNames.cpp \
    $$ANGLE_DIR/src/compiler/translator/RemovePow.cpp \
    $$ANGLE_DIR/src/compiler/translator/RemoveDynamicIndexing.cpp \
    $$ANGLE_DIR/src/compiler/translator/RemoveSwitchFallThrough.cpp \
    $$ANGLE_DIR/src/compiler/translator/RewriteDoWhile.cpp \
    $$ANGLE_DIR/src/compiler/translator/RewriteElseBlocks.cpp \
    $$ANGLE_DIR/src/compiler/translator/SeparateArrayInitialization.cpp \
    $$ANGLE_DIR/src/compiler/translator/SeparateDeclarations.cpp \
    $$ANGLE_DIR/src/compiler/translator/ScalarizeVecAndMatConstructorArgs.cpp \
    $$ANGLE_DIR/src/compiler/translator/SearchSymbol.cpp \
    $$ANGLE_DIR/src/compiler/translator/ShaderLang.cpp \
    $$ANGLE_DIR/src/compiler/translator/ShaderVars.cpp \
    $$ANGLE_DIR/src/compiler/translator/SeparateExpressionsReturningArrays.cpp \
    $$ANGLE_DIR/src/compiler/translator/StructureHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/SymbolTable.cpp \
    $$ANGLE_DIR/src/compiler/translator/timing/RestrictFragmentShaderTiming.cpp \
    $$ANGLE_DIR/src/compiler/translator/timing/RestrictVertexShaderTiming.cpp \
    $$ANGLE_DIR/src/compiler/translator/TranslatorESSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/TranslatorGLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/TranslatorHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/Types.cpp \
    $$ANGLE_DIR/src/compiler/translator/UnfoldShortCircuitToIf.cpp \
    $$ANGLE_DIR/src/compiler/translator/UnfoldShortCircuitAST.cpp \
    $$ANGLE_DIR/src/compiler/translator/UniformHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/UtilsHLSL.cpp \
    $$ANGLE_DIR/src/compiler/translator/util.cpp \
    $$ANGLE_DIR/src/compiler/translator/ValidateGlobalInitializer.cpp \
    $$ANGLE_DIR/src/compiler/translator/ValidateLimitations.cpp \
    $$ANGLE_DIR/src/compiler/translator/ValidateOutputs.cpp \
    $$ANGLE_DIR/src/compiler/translator/ValidateSwitch.cpp \
    $$ANGLE_DIR/src/compiler/translator/VariableInfo.cpp \
    $$ANGLE_DIR/src/compiler/translator/VariablePacker.cpp \
    $$ANGLE_DIR/src/compiler/translator/VersionGLSL.cpp \
    $$ANGLE_DIR/src/third_party/compiler/ArrayBoundsClamper.cpp


# NOTE: 'flex' and 'bison' can be found in qt5/gnuwin32/bin
flex.commands = $$addGnuPath(flex) --noline --nounistd --outfile=${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
flex.output = $${BUILDSUBDIR}${QMAKE_FILE_BASE}_lex.cpp
flex.input = FLEX_SOURCES
flex.dependency_type = TYPE_C
flex.variable_out = GENERATED_SOURCES
QMAKE_EXTRA_COMPILERS += flex

defineReplace(myDirName) { return($$dirname(1)) }
bison.commands = $$addGnuPath(bison) --no-lines --skeleton=yacc.c --defines=${QMAKE_FILE_OUT} \
                --output=${QMAKE_FUNC_FILE_OUT_myDirName}$$QMAKE_DIR_SEP${QMAKE_FILE_OUT_BASE}.cpp \
                ${QMAKE_FILE_NAME}
bison.output = $${BUILDSUBDIR}${QMAKE_FILE_BASE}_tab.h
bison.input = BISON_SOURCES
bison.dependency_type = TYPE_C
bison.variable_out = GENERATED_SOURCES
QMAKE_EXTRA_COMPILERS += bison

# This is a dummy compiler to work around the fact that an extra compiler can only
# have one output file even if the command generates two.
MAKEFILE_NOOP_COMMAND = @echo -n
msvc: MAKEFILE_NOOP_COMMAND = @echo >NUL
bison_impl.output = $${BUILDSUBDIR}${QMAKE_FILE_BASE}_tab.cpp
bison_impl.input = BISON_SOURCES
bison_impl.commands = $$MAKEFILE_NOOP_COMMAND
bison_impl.depends = $${BUILDSUBDIR}${QMAKE_FILE_BASE}_tab.h
bison_impl.output = $${BUILDSUBDIR}${QMAKE_FILE_BASE}_tab.cpp
bison_impl.variable_out = GENERATED_SOURCES
QMAKE_EXTRA_COMPILERS += bison_impl
