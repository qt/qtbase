include(../common/gles_common.pri)

TARGET = $$qtLibraryTarget($${LIBQTANGLE_NAME})

DEF_FILE_TARGET = $${TARGET}

!build_pass {
    # Merge libGLESv2 and libEGL .def files located under $$ANGLE_DIR into QtANGLE$${SUFFIX}.def
    DEF_FILES = \
        libGLESv2/libGLESv2 \
        libEGL/libEGL

    SUFFIX =
    for (DEBUG_RELEASE, $$list(0 1)) {
        DEF_MERGED = \
            "LIBRARY $${LIBQTANGLE_NAME}$$SUFFIX" \
            EXPORTS
        mingw: SUFFIX = $${SUFFIX}_mingw32
        PASS = 0
        MAX_ORDINAL = 0

        for (DEF_FILE, DEF_FILES) {
            DEF_FILE_PATH = $$ANGLE_DIR/src/$$DEF_FILE$${SUFFIX}.def
            DEF_SRC = $$cat($$DEF_FILE_PATH, lines)
            DEF_MERGED += \
                ";" \
                ";   Generated from:" \
                ";   $$DEF_FILE_PATH"

            for (line, DEF_SRC) {
                !contains(line, "(LIBRARY.*|EXPORTS)") {
                    LINESPLIT = $$split(line, @)
                    !count(LINESPLIT, 1) {
                        equals(PASS, 1) {
                            # In the second .def file we must allocate new ordinals in order
                            # to not clash with the ordinals from the first file. We then start off
                            # from MAX_ORDINAL + 1 and increase sequentially
                            MAX_ORDINAL = $$num_add($$MAX_ORDINAL, 1)
                            line = $$section(line, @, 0, -2)@$$MAX_ORDINAL
                        } else {
                            ORDINAL = $$last(LINESPLIT)
                            greaterThan(ORDINAL, $$MAX_ORDINAL): \
                                MAX_ORDINAL = $$ORDINAL
                        }
                    }
                    DEF_MERGED += $$line
                }
            }
            PASS = 1
        }
        write_file($${LIBQTANGLE_NAME}$${SUFFIX}.def, DEF_MERGED)|error()
        SUFFIX = "d"
    }
}

SOURCES += $$ANGLE_DIR/src/libEGL/libEGL.cpp

!static {
    DEF_FILE = $$PWD/$${DEF_FILE_TARGET}.def
    mingw: equals(QT_ARCH, i386): DEF_FILE = $$PWD/$${DEF_FILE_TARGET}_mingw32.def
} else {
    DEFINES += DllMain=DllMain_ANGLE # prevent symbol from conflicting with the user's DllMain
}

egl_headers.files = \
    $$ANGLE_DIR/include/EGL/egl.h \
    $$ANGLE_DIR/include/EGL/eglext.h \
    $$ANGLE_DIR/include/EGL/eglplatform.h
egl_headers.path = $$[QT_INSTALL_HEADERS]/QtANGLE/EGL
INSTALLS += egl_headers
