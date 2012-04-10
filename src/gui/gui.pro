load(qt_module)

TARGET     = QtGui
QT = core-private
MODULE_CONFIG = opengl

DEFINES   += QT_BUILD_GUI_LIB QT_NO_USING_NAMESPACE

load(qt_module_config)

# Code coverage with TestCocoon
# The following is required as extra compilers use $$QMAKE_CXX instead of $(CXX).
# Without this, testcocoon.prf is read only after $$QMAKE_CXX is used by the
# extra compilers.
testcocoon {
    load(testcocoon)
}

QMAKE_DOCS = $$PWD/doc/qtgui.qdocconf
QMAKE_DOCS_INDEX = ../../doc

include(accessible/accessible.pri)
include(kernel/kernel.pri)
include(image/image.pri)
include(text/text.pri)
include(painting/painting.pri)
include(util/util.pri)
include(math3d/math3d.pri)
include(opengl/opengl.pri)
include(animation/animation.pri)

QMAKE_LIBS += $$QMAKE_LIBS_GUI

win32:!contains(QT_CONFIG, directwrite) {
    DEFINES += QT_NO_DIRECTWRITE
}

*-g++*|linux-icc*|*-clang|*-qcc* {
        sse2 {
            sse2_compiler.commands = $$QMAKE_CXX -c $(CXXFLAGS)
            sse2_compiler.commands += $$QMAKE_CFLAGS_SSE2
            sse2_compiler.commands += $(INCPATH) ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
            sse2_compiler.dependency_type = TYPE_C
            sse2_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
            sse2_compiler.input = SSE2_SOURCES
            sse2_compiler.variable_out = OBJECTS
            sse2_compiler.name = compiling[sse2] ${QMAKE_FILE_IN}
            silent:sse2_compiler.commands = @echo compiling[sse2] ${QMAKE_FILE_IN} && $$sse2_compiler.commands
            QMAKE_EXTRA_COMPILERS += sse2_compiler
        }
        ssse3 {
            ssse3_compiler.commands = $$QMAKE_CXX -c $(CXXFLAGS)
            ssse3_compiler.commands += $$QMAKE_CFLAGS_SSSE3
            ssse3_compiler.commands += $(INCPATH) ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
            ssse3_compiler.dependency_type = TYPE_C
            ssse3_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
            ssse3_compiler.input = SSSE3_SOURCES
            ssse3_compiler.variable_out = OBJECTS
            ssse3_compiler.name = compiling[ssse3] ${QMAKE_FILE_IN}
            silent:ssse3_compiler.commands = @echo compiling[ssse3] ${QMAKE_FILE_IN} && $$ssse3_compiler.commands
            QMAKE_EXTRA_COMPILERS += ssse3_compiler
        }
        avx {
            avx_compiler.commands = $$QMAKE_CXX -c $(CXXFLAGS)
            avx_compiler.commands += $$QMAKE_CFLAGS_AVX
            avx_compiler.commands += $(INCPATH) ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
            avx_compiler.dependency_type = TYPE_C
            avx_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
            avx_compiler.input = AVX_SOURCES
            avx_compiler.variable_out = OBJECTS
            avx_compiler.name = compiling[avx] ${QMAKE_FILE_IN}
            silent:avx_compiler.commands = @echo compiling[avx] ${QMAKE_FILE_IN} && $$avx_compiler.commands
            QMAKE_EXTRA_COMPILERS += avx_compiler
        }
        neon {
            HEADERS += $$NEON_HEADERS

            DRAWHELPER_NEON_ASM_FILES = $$NEON_ASM

            neon_compiler.commands = $$QMAKE_CXX -c $(CXXFLAGS)
            neon_compiler.commands += $$QMAKE_CFLAGS_NEON
            neon_compiler.commands += $(INCPATH) ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
            neon_compiler.dependency_type = TYPE_C
            neon_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
            neon_compiler.input = DRAWHELPER_NEON_ASM_FILES NEON_SOURCES
            neon_compiler.variable_out = OBJECTS
            neon_compiler.name = compiling[neon] ${QMAKE_FILE_IN}
            silent:neon_compiler.commands = @echo compiling[neon] ${QMAKE_FILE_IN} && $$neon_compiler.commands
            QMAKE_EXTRA_COMPILERS += neon_compiler
        }
        iwmmxt {
            iwmmxt_compiler.commands = $$QMAKE_CXX -c $(CXXFLAGS)
            iwmmxt_compiler.commands += $$QMAKE_CFLAGS_IWMMXT
            iwmmxt_compiler.commands += $(INCPATH) ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
            iwmmxt_compiler.dependency_type = TYPE_C
            iwmmxt_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
            iwmmxt_compiler.input = IWMMXT_SOURCES
            iwmmxt_compiler.variable_out = OBJECTS
            iwmmxt_compiler.name = compiling[iwmmxt] ${QMAKE_FILE_IN}
            silent:iwmmxt_compiler.commands = @echo compiling[iwmmxt] ${QMAKE_FILE_IN} && $$iwmmxt_compiler.commands
            QMAKE_EXTRA_COMPILERS += iwmmxt_compiler
        }
        mips_dsp {
            HEADERS += $$MIPS_DSP_HEADERS

            DRAWHELPER_MIPS_DSP_ASM_FILES = $$MIPS_DSP_ASM
            mips_dsp_compiler.commands = $$QMAKE_CXX -c
            mips_dsp_compiler.commands += $(CXXFLAGS) $(INCPATH) ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
            mips_dsp_compiler.dependency_type = TYPE_C
            mips_dsp_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
            mips_dsp_compiler.input = DRAWHELPER_MIPS_DSP_ASM_FILES MIPS_DSP_SOURCES
            mips_dsp_compiler.variable_out = OBJECTS
            mips_dsp_compiler.name = compiling[mips_dsp] ${QMAKE_FILE_IN}
            silent:mips_dsp_compiler.commands = @echo compiling[mips_dsp] ${QMAKE_FILE_IN} && $$mips_dsp_compiler.commands
            QMAKE_EXTRA_COMPILERS += mips_dsp_compiler
        }
        mips_dspr2 {
            HEADERS += $$MIPS_DSP_HEADERS

            DRAWHELPER_MIPS_DSPR2_ASM_FILES += $$MIPS_DSPR2_ASM
            mips_dspr2_compiler.commands = $$QMAKE_CXX -c
            mips_dspr2_compiler.commands += $(CXXFLAGS) $(INCPATH) ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
            mips_dspr2_compiler.dependency_type = TYPE_C
            mips_dspr2_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
            mips_dspr2_compiler.input = DRAWHELPER_MIPS_DSPR2_ASM_FILES
            mips_dspr2_compiler.variable_out = OBJECTS
            mips_dspr2_compiler.name = compiling[mips_dspr2] ${QMAKE_FILE_IN}
            silent:mips_dspr2_compiler.commands = @echo compiling[mips_dspr2] ${QMAKE_FILE_IN} && $$mips_dspr2_compiler.commands
            QMAKE_EXTRA_COMPILERS += mips_dspr2_compiler
        }
} else:win32-msvc* {
        sse2 {
            sse2_compiler.commands = $$QMAKE_CXX -c $(CXXFLAGS)
            sse2_compiler.commands += $$QMAKE_CFLAGS_SSE2
            sse2_compiler.commands += $(INCPATH) ${QMAKE_FILE_IN} -Fo${QMAKE_FILE_OUT}
            sse2_compiler.dependency_type = TYPE_C
            sse2_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
            sse2_compiler.input = SSE2_SOURCES
            sse2_compiler.variable_out = OBJECTS
            sse2_compiler.name = compiling[sse2] ${QMAKE_FILE_IN}
            silent:sse2_compiler.commands = @echo compiling[sse2] ${QMAKE_FILE_IN} && $$sse2_compiler.commands
            QMAKE_EXTRA_COMPILERS += sse2_compiler
        }
        ssse3 {
            ssse3_compiler.commands = $$QMAKE_CXX -c $(CXXFLAGS)
            ssse3_compiler.commands += $$QMAKE_CFLAGS_SSSE3
            ssse3_compiler.commands += $(INCPATH) ${QMAKE_FILE_IN} -Fo${QMAKE_FILE_OUT}
            ssse3_compiler.dependency_type = TYPE_C
            ssse3_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
            ssse3_compiler.input = SSSE3_SOURCES
            ssse3_compiler.variable_out = OBJECTS
            ssse3_compiler.name = compiling[ssse3] ${QMAKE_FILE_IN}
            silent:ssse3_compiler.commands = @echo compiling[ssse3] ${QMAKE_FILE_IN} && $$ssse3_compiler.commands
            QMAKE_EXTRA_COMPILERS += ssse3_compiler
        }
        avx {
            avx_compiler.commands = $$QMAKE_CXX -c $(CXXFLAGS) -D_M_AVX
            avx_compiler.commands += $$QMAKE_CFLAGS_AVX
            avx_compiler.commands += $(INCPATH) ${QMAKE_FILE_IN} -Fo${QMAKE_FILE_OUT}
            avx_compiler.dependency_type = TYPE_C
            avx_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
            avx_compiler.input = AVX_SOURCES
            avx_compiler.variable_out = OBJECTS
            avx_compiler.name = compiling[avx] ${QMAKE_FILE_IN}
            silent:avx_compiler.commands = @echo compiling[avx] ${QMAKE_FILE_IN} && $$avx_compiler.commands
            QMAKE_EXTRA_COMPILERS += avx_compiler
        }
} else:false {
    # This allows an IDE like Creator to know that these files are part of the sources
    SOURCES += $$SSE2_SOURCES $$SSSE3_SOURCES \
                $$AVX_SOURCES \
                $$NEON_SOURCES $$NEON_ASM \
                $$IWMMXT_SOURCES \
                $$MIPS_DSP_SOURCES $$MIPS_DSP_ASM $$MIPS_DSPR2_ASM
}
