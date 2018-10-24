linux:!static {
    precompile_header {
        # we'll get an error if we just use SOURCES +=
        no_pch_assembler.commands = $$QMAKE_CC -c $(CFLAGS) $(INCPATH) ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
        no_pch_assembler.dependency_type = TYPE_C
        no_pch_assembler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
        no_pch_assembler.input = NO_PCH_ASM
        no_pch_assembler.name = compiling[no_pch] ${QMAKE_FILE_IN}
        silent: no_pch_assembler.commands = @echo compiling[no_pch] ${QMAKE_FILE_IN} && $$no_pch_assembler.commands
        CMAKE_ANGLE_GLES2_IMPLIB_RELEASE = libGLESv2.$${QMAKE_EXTENSION_STATICLIB}
        HOST_BINS = $$[QT_HOST_BINS]
        CMAKE_HOST_DATA_DIR = $$[QT_HOST_DATA/src]/
        TR_EXCLUDE += ../3rdparty/*

        QMAKE_EXTRA_COMPILERS += no_pch_assembler
        NO_PCH_ASM += global/minimum-linux.S
    } else {
        SOURCES += global/minimum-linux.S
    }
    HEADERS += global/minimum-linux_p.h
}

